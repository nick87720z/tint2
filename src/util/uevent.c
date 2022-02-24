/**************************************************************************
*
* Linux Kernel uevent handler
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

#include "uevent.h"
int uevent_fd = -1;

#ifdef ENABLE_UEVENT

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <linux/types.h>
#include <linux/netlink.h>

#include "common.h"

static struct sockaddr_nl nls;
static GSList *notifiers = NULL;

#define has_prefix(str, end, prefix, prefixlen) (                                        \
    (end && (end - str) < prefixlen) || memcmp(str, prefix, prefixlen) != 0              \
    ? NULL : str + prefixlen                                                             \
)

#define HAS_CONST_PREFIX(str, end, prefix) has_prefix((str), end, prefix, sizeof(prefix) - 1)

static void uevent_free(struct uevent *ev)
{
    free(ev->path);
    free(ev->subsystem);
    g_slist_free_full(ev->params, free);
    free(ev);
}

static int uevent_new(struct uevent *ev, char *buffer, int size)
// Fills event structure at ev address.
// Returns 1 on success , 0 on error.
{
    gboolean first = TRUE;

    if (!size || !ev)
        return 0;

    /* must be null-terminated */
    buffer[size - 1] = '\0';

    const char *s   = buffer;
    const char *end = buffer + size;
    while (s < end) {
        if (first)
        {
            const char *p = strchr(s, '@');
            if (!p) {
                /* error: kernel events contain @, triggered by udev events, though */
                return 0;
            }
            // remaining part is device path - duplicate it
            p = strchr ((s=p+1), '\0') + 1;
            int l = p - s;
            ev->path = malloc (l);
            memcpy (ev->path, s, l);
            first = FALSE;
            s = p;
        }
        else
        {
            const char *val;
            if ((val = HAS_CONST_PREFIX(s, end, "ACTION=")) != NULL) {
                ev->action =
                    !strcmp(val, "add"   ) ? (s = val + sizeof("add"   ), UEVENT_ADD)
                :   !strcmp(val, "remove") ? (s = val + sizeof("remove"), UEVENT_REMOVE)
                :   !strcmp(val, "change") ? (s = val + sizeof("change"), UEVENT_CHANGE)
                :   /* default ? */          (s = strchr (val, '\0') + 1, UEVENT_UNKNOWN);
            } else if ((val = HAS_CONST_PREFIX(s, end, "SEQNUM=")) != NULL) {
                ev->sequence = atoi(val);
                s = strchr (s, '\0') + 1;
            } else if ((val = HAS_CONST_PREFIX(s, end, "SUBSYSTEM=")) != NULL) {
                char *p = strchr (val, '\0') + 1;
                int l = p - val;
                ev->subsystem = malloc (l);
                memcpy (ev->subsystem, val, l);
                s = p;
            } else {
                val = strchr(s, '=');
                if (val) {
                    // total allocation for both structure and its strings
                    // 3x less allocation headache
                    char *p = strchr (val, '\0');
                    int appendum = p - s + 1;
                    struct uevent_parameter *param = malloc(sizeof(*param) + appendum);
                    if (param)
                    {
                        param->key = (char *)(param + 1);
                        memcpy (param->key, s, appendum);
                        param->val = param->key + (val - s);
                        param->val[-1] = '\0';
                        ev->params = g_slist_prepend(ev->params, param);
                    }
                    s = p + 1;
                } else
                    s = strchr(s, '\0') + 1;
            }
        }
    }

    return 1;
}

void uevent_register_notifier(struct uevent_notify *nb)
{
    notifiers = g_slist_prepend(notifiers, nb);
}

void uevent_unregister_notifier(struct uevent_notify *nb)
{
    GSList *l = notifiers;

    while (l != NULL) {
        GSList *next = l->next;
        struct uevent_notify *lnb = l->data;

        if (memcmp(nb, lnb, sizeof(struct uevent_notify)) == 0)
            notifiers = g_slist_delete_link(notifiers, l);

        l = next;
    }
}

void uevent_handler()
{
    if (uevent_fd < 0)
        return;

    char buf[512];
    int len = recv(uevent_fd, buf, sizeof(buf), MSG_DONTWAIT);
    if (len < 0)
        return;

    struct uevent ev;

    if (uevent_new(&ev, buf, len)) {
        for (GSList *l = notifiers; l; l = l->next)
        {
            struct uevent_notify *nb = l->data;

            if (!(ev.action & nb->action) ||
                nb->subsystem && strcmp(ev.subsystem, nb->subsystem) != 0)

                continue;

            nb->cb(&ev, nb->userdata);
        }
    }
}

int uevent_init()
{
    /* Open hotplug event netlink socket */
    memset(&nls, 0, sizeof(struct sockaddr_nl));
    nls.nl_family = AF_NETLINK;
    nls.nl_pid = getpid();
    nls.nl_groups = -1;

    /* open socket */
    uevent_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (uevent_fd < 0) {
        fprintf(stderr, "tint2: Error: socket open failed\n");
        return -1;
    }

    /* Listen to netlink socket */
    if (bind(uevent_fd, (void *)&nls, sizeof(struct sockaddr_nl))) {
        fprintf(stderr, "tint2: Bind failed\n");
        return -1;
    }

    fprintf(stderr, "tint2: Kernel uevent interface initialized...\n");

    return uevent_fd;
}

void uevent_cleanup()
{
    if (uevent_fd >= 0)
        close(uevent_fd);
}

#endif
