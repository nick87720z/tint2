/*************************************************************************
*
* Copyright (C) 2009 Andreas.Fink (Andreas.Fink85@gmail.com)
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

#ifndef TIMER_H
#define TIMER_H

#include <glib.h>
#include <time.h>
#include <sys/time.h>
#include "bool.h"

extern bool timers_warnings;
extern bool debug_timers;

typedef void TimerCallback(void *arg);

typedef struct {
    char name_[64];
    bool enabled_;
    long long expiration_time_ms_;
    int period_ms_;
    TimerCallback *callback_;
    void *arg_;
    bool destroy_;
} Timer;

#define DEFAULT_TIMER {"", 0, 0, 0, 0, 0, false}

#define INIT_TIMER(t) init_timer(&t, #t)

void default_timers();
// Initialize the timer module.

void cleanup_timers();
// Destroy the timer module.

void init_timer(Timer *timer, const char *name);
// Initialize a timer. Caller keeps ownership.

void destroy_timer(Timer *timer);
// Destroy a timer. Does not free() the pointer.

void change_timer(Timer *timer, bool enabled, int delay_ms, int period_ms, TimerCallback *callback, void *arg);
// Modify a timer.

void stop_timer(Timer *timer);

struct timespec *get_duration_to_next_timer_expiration();
// Get the time duration to the next expiration time, or NULL if there is no active timer.
// Do not free the pointer; it is harmless to change its contents.

void handle_expired_timers();
// Trigger all expired timers, and reschedule them if they are periodic timers

// Time helper functions.

gint compare_timespecs(const struct timespec *t1, const struct timespec *t2);
// Returns  -1 if t1 <  t2,
//           0 if t1 == t2,
//           1 if t1 > t2

struct timespec add_msec_to_timespec(struct timespec ts, int msec);

double profiling_get_time();
// Returns the time difference in seconds between the current time and the last time this function was called.
// At the first call returns zero.

double get_time();
// Get current time in seconds, from an unspecified origin.

#endif // TIMER_H
