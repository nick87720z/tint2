/**************************************************************************
*
* Tint2 : Linux battery
*
* Copyright (C) 2015 Sebastian Reichel <sre@ring0.de>
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* or any later version as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

#ifdef __linux__

#include <stdlib.h>
#include <unistd.h>

#include "common.h"
#include "battery.h"
#include "uevent.h"

enum psy_type {
    PSY_UNKNOWN,
    PSY_BATTERY,
    PSY_MAINS,
};

struct psy_battery {
    /* generic properties */
    gchar *name;
    /* monotonic time, in microseconds */
    gint64 timestamp;
    /* sysfs files */
    gchar *path_present;
    gchar *path_level_now;
    gchar *path_level_full;
    gchar *path_rate_now;
    gchar *path_status;
    /* values */
    gboolean present;
    gint level_now;
    gint level_full;
    gint rate_now;
    gchar unit;
    ChargeState status;
};

struct psy_mains {
    gchar *name;        /* generic properties */
    gchar *path_online; /* sysfs files */
    gboolean online;    /* values */
};

static int file_get_contents( char *pathname, char **content)
// Returns number of loaded characters or -1 if error occured
// Actual error code is written to errno
{
    errno = 0;
    if (! pathname || !content)
        return -1;

    int result = -1;
    int fd = open( pathname, O_RDONLY);
    if (fd == -1)
        return result;

    int len = lseek( fd, 0, SEEK_END);
    if (len == -1)
        goto end;

    lseek( fd, 0, SEEK_SET);
    char *data = malloc( len + 1);
    if (! data)
        goto end;

    result = len;
    char *p = data;
    while (len)
    {
        int ret = read( fd, data, len);
        if (ret == 0)
            break;
        if (ret == -1)
        {
            if (errno == EINTR)
                continue;

            free( data);
            result = -1;
            goto end;
        }
        len -= ret;
        p += ret;
    }
    *content = data;
    result -= len;
    data[ result ] = '\0';

end:
    close( fd);
    return result;
}

static gboolean is_file_non_empty(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return FALSE;
    char buffer[1024];
    size_t count = fread(buffer, 1, sizeof(buffer), f);
    fclose(f);
    return count > 0;
}

static void uevent_battery_update()
{
    update_battery_tick(NULL);
}
static struct uevent_notify psy_change = {UEVENT_CHANGE, "power_supply", NULL, uevent_battery_update};

static void uevent_battery_plug()
{
    fprintf(stderr, "tint2: reinitialize batteries after HW change\n");
    reinit_battery();
}
static struct uevent_notify psy_plug = {UEVENT_ADD | UEVENT_REMOVE, "power_supply", NULL, uevent_battery_plug};

#define RETURN_ON_ERROR(err)                                                        \
    if (err) {                                                                           \
        fprintf(stderr, RED "tint2: %s:%d: errror" RESET "\n", __FILE__, __LINE__); \
        return FALSE;                                                               \
    }

static GList *batteries = NULL;
static GList *mains = NULL;

static guint8 level_to_percent(gint level_now, gint level_full)
{
    return 0.5 + ((level_now <= level_full ? level_now : level_full) * 100.0) / level_full;
}

static enum psy_type power_supply_get_type(const gchar *entryname)
{
    gchar *path_type = strdup_printf( NULL, "%s/sys/class/power_supply/%s/type", battery_sys_prefix, entryname);
    gchar *type;

    if (file_get_contents( path_type, &type) == -1) {
        fprintf(stderr, RED "tint2: %s:%d: read failed for %s" RESET "\n", __FILE__, __LINE__, path_type);
        free( path_type);
        return PSY_UNKNOWN;
    }
    free( path_type);

    if (!g_strcmp0(type, "Battery\n")) {
        free( type);
        return PSY_BATTERY;
    }

    if (!g_strcmp0(type, "Mains\n")) {
        free( type);
        return PSY_MAINS;
    }

    free( type);

    return PSY_UNKNOWN;
}

static gboolean init_linux_battery(struct psy_battery *bat)
{
    const gchar *entryname = bat->name;

    bat->path_present = strdup_printf( NULL, "%s/sys/class/power_supply/%s/present", battery_sys_prefix, entryname);
    if (!is_file_non_empty(bat->path_present)) {
        fprintf(stderr, RED "tint2: %s:%d: read failed for %s" RESET "\n", __FILE__, __LINE__, bat->path_present);
        return FALSE;
    }

    bat->path_level_now = strdup_printf( NULL, "%s/sys/class/power_supply/%s/energy_now", battery_sys_prefix, entryname);
    bat->path_level_full = strdup_printf( NULL, "%s/sys/class/power_supply/%s/energy_full", battery_sys_prefix, entryname);
    bat->path_rate_now = strdup_printf( NULL, "%s/sys/class/power_supply/%s/power_now", battery_sys_prefix, entryname);
    bat->unit = 'W';

    if (!is_file_non_empty(bat->path_level_now) ||
        !is_file_non_empty(bat->path_level_full))
    {
        free( bat->path_level_now);
        free( bat->path_level_full);
        free( bat->path_rate_now);

        bat->path_level_now = strdup_printf( NULL, "%s/sys/class/power_supply/%s/charge_now", battery_sys_prefix, entryname);
        bat->path_level_full = strdup_printf( NULL, "%s/sys/class/power_supply/%s/charge_full", battery_sys_prefix, entryname);
        bat->path_rate_now = strdup_printf( NULL, "%s/sys/class/power_supply/%s/current_now", battery_sys_prefix, entryname);
        bat->unit = 'A';

        if (!is_file_non_empty(bat->path_level_now)) {
            fprintf(stderr, RED "tint2: %s:%d: read failed for %s" RESET "\n", __FILE__, __LINE__, bat->path_level_now);
            return FALSE;
        }
        if (!is_file_non_empty(bat->path_level_full)) {
            fprintf(stderr, RED "tint2: %s:%d: read failed for %s" RESET "\n", __FILE__, __LINE__, bat->path_level_full);
            return FALSE;
        }
    }

    bat->path_status = strdup_printf( NULL, "%s/sys/class/power_supply/%s/status", battery_sys_prefix, entryname);
    if (!is_file_non_empty(bat->path_status)) {
        fprintf(stderr, RED "tint2: %s:%d: read failed for %s" RESET "\n", __FILE__, __LINE__, bat->path_status);
        return FALSE;
    }

    return TRUE;
}

static gboolean init_linux_mains(struct psy_mains *ac)
{
    const gchar *entryname = ac->name;
    ac->path_online = strdup_printf( NULL, "%s/sys/class/power_supply/%s/online", battery_sys_prefix, entryname);
    if (!is_file_non_empty(ac->path_online)) {
        fprintf(stderr, RED "tint2: %s:%d: read failed for %s" RESET "\n", __FILE__, __LINE__, ac->path_online);
        return FALSE;
    }

    return TRUE;
}

static void psy_battery_free(gpointer data)
{
    struct psy_battery *bat = data;
    free( bat->name);
    free( bat->path_status);
    free( bat->path_rate_now);
    free( bat->path_level_full);
    free( bat->path_level_now);
    free( bat->path_present);
    free( bat);
}

static void psy_mains_free(gpointer data)
{
    struct psy_mains *ac = data;
    free( ac->name);
    free( ac->path_online);
    free( ac);
}

void battery_os_free()
{
    uevent_unregister_notifier(&psy_change);
    uevent_unregister_notifier(&psy_plug);

    g_list_free_full(batteries, psy_battery_free);
    batteries = NULL;
    g_list_free_full(mains, psy_mains_free);
    mains = NULL;
}

static void add_battery(const char *entryname)
{
    struct psy_battery *bat = calloc( 1, sizeof(*bat));
    bat->name = strdup( entryname);

    if (init_linux_battery(bat)) {
        batteries = g_list_append(batteries, bat);
        fprintf(stderr, GREEN "Found battery \"%s\"" RESET "\n", bat->name);
    } else {
        psy_battery_free( bat);
        fprintf(stderr, RED "tint2: Failed to initialize battery \"%s\"" RESET "\n", entryname);
    }
}

static void add_mains(const char *entryname)
{
    struct psy_mains *ac = calloc( 1, sizeof(*ac));
    ac->name = strdup( entryname);

    if (init_linux_mains(ac)) {
        mains = g_list_append(mains, ac);
        fprintf(stderr, GREEN "Found mains \"%s\"" RESET "\n", ac->name);
    } else {
        psy_mains_free( ac);
        fprintf(stderr, RED "tint2: Failed to initialize mains \"%s\"" RESET "\n", entryname);
    }
}

gboolean battery_os_init()
{
    GDir *directory = 0;
    GError *error = NULL;
    const char *entryname;

    battery_os_free();

    gchar *dir_path = strdup_printf( NULL, "%s/sys/class/power_supply", battery_sys_prefix);
    directory = g_dir_open(dir_path, 0, &error);
    free( dir_path);
    g_error_free( error);
    RETURN_ON_ERROR( error);

    while ((entryname = g_dir_read_name(directory))) {
        fprintf(stderr, GREEN "tint2: Found power device %s" RESET "\n", entryname);
        enum psy_type type = power_supply_get_type(entryname);

        switch (type) {
        case PSY_BATTERY:
            add_battery(entryname);
            break;
        case PSY_MAINS:
            add_mains(entryname);
            break;
        default:
            break;
        }
    }

    g_dir_close(directory);

    uevent_register_notifier(&psy_change);
    uevent_register_notifier(&psy_plug);

    return batteries != NULL;
}

static gint estimate_rate_usage(struct psy_battery *bat, gint old_level_now, gint64 old_timestamp)
{
    gint64 diff_level = ABS(bat->level_now - old_level_now);
    gint64 diff_time = bat->timestamp - old_timestamp;

    /* µW = (µWh * 3600) / (µs / 1000000) */
    gint rate = diff_level * 3600 * 1000000 / MAX(1, diff_time);

    return rate;
}

static gboolean update_linux_battery(struct psy_battery *bat)
{
    gchar *data;

    gint64 old_timestamp = bat->timestamp;
    int old_level_now = bat->level_now;
    gint old_rate_now = bat->rate_now;

    /* reset values */
    bat->present = FALSE;
    bat->status = BATTERY_UNKNOWN;
    bat->level_now = 0;
    bat->level_full = 0;
    bat->rate_now = 0;
    bat->timestamp = g_get_monotonic_time();

    /* present */
    RETURN_ON_ERROR( file_get_contents( bat->path_present, &data) == -1);
    bat->present = (atoi(data) == 1);
    free( data);

    /* we are done, if battery is not present */
    if (!bat->present)
        return TRUE;

    /* status */
    bat->status = BATTERY_UNKNOWN;
    RETURN_ON_ERROR( file_get_contents( bat->path_status, &data) == -1);

    if (!g_strcmp0(data, "Charging\n")) {
        bat->status = BATTERY_CHARGING;
    } else if (!g_strcmp0(data, "Discharging\n")) {
        bat->status = BATTERY_DISCHARGING;
    } else if (!g_strcmp0(data, "Full\n")) {
        bat->status = BATTERY_FULL;
    }
    free( data);

    /* level now */
    RETURN_ON_ERROR( file_get_contents( bat->path_level_now, &data) == -1);
    bat->level_now = atoi(data);
    free( data);

    /* level full */
    RETURN_ON_ERROR( file_get_contents( bat->path_level_full, &data) == -1);
    bat->level_full = atoi(data);
    free( data);

    /* rate now */
    if (file_get_contents( bat->path_rate_now, &data) == -1)
    {
        if (errno != ENODEV)
            return FALSE;

        /* some hardware does not support reading current rate consumption */
        bat->rate_now = estimate_rate_usage(bat, old_level_now, old_timestamp);
        if (bat->rate_now == 0 && bat->status != BATTERY_FULL) {
            /* If the hardware updates the level slower than our sampling period,
             * we need to sample more rarely */
            bat->rate_now = old_rate_now;
            bat->timestamp = old_timestamp;
        }
    } else {
        bat->rate_now = atoi(data);
        free( data);
    }

    return TRUE;
}

static gboolean update_linux_mains(struct psy_mains *ac)
{
    gchar *data;
    ac->online = FALSE;

    /* online */
    RETURN_ON_ERROR( file_get_contents( ac->path_online, &data) == -1);
    ac->online = (atoi(data) == 1);
    free( data);

    return TRUE;
}

int battery_os_update(BatteryState *state)
{
    GList *l;

    gint64 total_level_now = 0;
    gint64 total_level_full = 0;
    gint64 total_rate_now = 0;
    gint seconds = 0;

    gboolean charging = FALSE;
    gboolean discharging = FALSE;
    gboolean full = FALSE;
    gboolean ac_connected = FALSE;

    for (l = batteries; l != NULL; l = l->next) {
        struct psy_battery *bat = l->data;
        update_linux_battery(bat);

        total_level_now += bat->level_now;
        total_level_full += bat->level_full;
        total_rate_now += bat->rate_now;

        charging    |= (bat->status == BATTERY_CHARGING);
        discharging |= (bat->status == BATTERY_DISCHARGING);
        full        |= (bat->status == BATTERY_FULL);
    }

    for (l = mains; l != NULL; l = l->next) {
        struct psy_mains *ac = l->data;
        update_linux_mains(ac);
        ac_connected |= (ac->online);
    }

    /* build global state */
    if (charging && !discharging)
        state->state = BATTERY_CHARGING;
    else if (!charging && discharging)
        state->state = BATTERY_DISCHARGING;
    else if (!charging && !discharging && full)
        state->state = BATTERY_FULL;

    /* calculate seconds */
    if (total_rate_now > 0) {
        seconds = 3600 * (  state->state == BATTERY_CHARGING
                            ? total_level_full - total_level_now
                            : total_level_now )
                        / total_rate_now;
        seconds = MAX(0, seconds);
    }
    battery_state_set_time(state, seconds);

    /* calculate percentage */
    state->percentage = level_to_percent(total_level_now, total_level_full);

    /* AC state */
    state->ac_connected = ac_connected;

    if (state->state == BATTERY_UNKNOWN) {
        state->state =  !ac_connected                                   ? BATTERY_DISCHARGING
                        :total_rate_now == 0 && state->percentage >= 90 ? BATTERY_FULL
                        :                                               BATTERY_CHARGING;
    }

    return 0;
}

static gchar *level_human_readable(struct psy_battery *bat)
{
    gint now  = bat->level_now;
    gint full = bat->level_full;

    if (full >= 1000000)
        return strdup_printf(  NULL,
                               "%d.%d / %d.%d %ch",
                               now / 1000000,
                               (now % 1000000) / 100000,
                               full / 1000000,
                               (full % 1000000) / 100000,
                               bat->unit);
    else if (full >= 1000)
        return strdup_printf(  NULL,
                               "%d.%d / %d.%d m%ch",
                               now / 1000,
                               (now % 1000) / 100,
                               full / 1000,
                               (full % 1000) / 100,
                               bat->unit);
    else
        return strdup_printf( NULL, "%d / %d µ%ch", now, full, bat->unit);
}

static gchar *rate_human_readable(struct psy_battery *bat)
{
    gint rate = bat->rate_now;
    gchar unit = bat->unit;

    return  (rate >= 1000000) ? strdup_printf( NULL, "%d.%d %c", rate / 1000000, (rate % 1000000) / 100000, unit) :
            (rate >= 1000   ) ? strdup_printf( NULL, "%d.%d m%c", rate / 1000, (rate % 1000) / 100, unit) :
            (rate > 0)        ? strdup_printf( NULL, "%d µ%c", rate, unit) :
                                strdup_printf( NULL, "0 %c", unit);
}

char *battery_os_tooltip()
{
    GList *l;
    GString *tooltip = g_string_new("");
    gchar *result;

    for (l = batteries; l != NULL; l = l->next) {
        struct psy_battery *bat = l->data;

        if (tooltip->len)
            g_string_append_c(tooltip, '\n');

        g_string_append_printf(tooltip, "%s\n", bat->name);

        if (!bat->present) {
            g_string_append_printf(tooltip, "\tnot connected");
            continue;
        }

        gchar *rate = rate_human_readable(bat);
        gchar *level = level_human_readable(bat);
        gchar *state = (bat->status == BATTERY_UNKNOWN) ? "energy" : chargestate2str(bat->status);

        guint8 percentage = level_to_percent(bat->level_now, bat->level_full);

        g_string_append_printf(tooltip, "\t%s: %s (%u %%)\n\trate: %s", state, level, percentage, rate);

        free( rate);
        free( level);
    }

    for (l = mains; l != NULL; l = l->next) {
        struct psy_mains *ac = l->data;

        if (tooltip->len)
            g_string_append_c(tooltip, '\n');

        g_string_append_printf(tooltip, "%s\n", ac->name);
        g_string_append_printf(tooltip, ac->online ? "\tConnected" : "\tDisconnected");
    }

    result = tooltip->str;
    g_string_free(tooltip, FALSE);

    return result;
}

#endif
