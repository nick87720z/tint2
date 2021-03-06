#include <errno.h>
#include <fcntl.h>
#include <glib.h>
#ifndef TINT2CONF
#ifdef HAVE_SN
#include <libsn/sn.h>
#endif
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common.h"
#include "panel.h"
#include "launcher.h"
#include "server.h"
#include "signals.h"

static sig_atomic_t signal_pending;

void signal_handler(int sig)
{
    // signal handler is light as it should be
    signal_pending = sig;
}

#ifdef BACKTRACE_ON_SIGNAL
void crash_handler(int sig)
{
    handle_crash(signal_name(sig));
    sigaction(sig, &(sigaction_t){.sa_handler = SIG_DFL}, NULL);
    raise(sig);
}
#endif

void reset_signals()
{
    for (int sig = 1; sig < 32; sig++) {
        signal(sig, SIG_DFL);
    }
    sigset_t signal_set;
    sigemptyset(&signal_set);
    sigprocmask(SIG_SETMASK, &signal_set, NULL);
}

#ifndef TINT2CONF
void init_signals()
{
    // Set signal handlers
    signal_pending = 0;

    reset_signals();

    sigaction(SIGCHLD, &(sigaction_t){.sa_handler = SIG_IGN}, NULL);

    sigaction_t sa = {.sa_handler = signal_handler, .sa_flags = SA_RESTART};
    sigaction(SIGUSR1,  &sa, NULL);
    sigaction(SIGUSR2,  &sa, NULL);
    sigaction(SIGINT,   &sa, NULL);
    sigaction(SIGTERM,  &sa, NULL);
    sigaction(SIGHUP,   &sa, NULL);

#ifdef BACKTRACE_ON_SIGNAL
    sigaction_t sa_crash = {.sa_handler = crash_handler};
    sigaction(SIGSEGV,  &sa_crash, NULL);
    sigaction(SIGFPE,   &sa_crash, NULL);
    sigaction(SIGPIPE,  &sa_crash, NULL);
    sigaction(SIGBUS,   &sa_crash, NULL);
    sigaction(SIGABRT,  &sa_crash, NULL);
    sigaction(SIGSYS,   &sa_crash, NULL);
#endif
}

int sigchild_pipe_valid = FALSE;
int sigchild_pipe[2];

static void sigchld_handler(int sig)
{
    if (!sigchild_pipe_valid)
        return;
    int savedErrno = errno;
    ssize_t unused = write(sigchild_pipe[1], "x", 1);
    (void)unused;
    errno = savedErrno;
}

void sigchld_handler_async()
{
    // Wait for all dead processes
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) != -1 && pid != 0) {
#ifdef HAVE_SN
        if (startup_notifications) {
            SnLauncherContext *ctx = (SnLauncherContext *)g_tree_lookup(server.pids, GINT_TO_POINTER(pid));
            if (ctx) {
                g_tree_remove(server.pids, GINT_TO_POINTER(pid));
                sn_launcher_context_complete(ctx);
                sn_launcher_context_unref(ctx);
            }
        }
#endif
        for (GList *l = panel_config.execp_list; l; l = l->next) {
            Execp *execp = l->data;
            if (g_tree_lookup(execp->backend->cmd_pids, GINT_TO_POINTER(pid)))
                execp_cmd_completed(execp, pid);
        }
    }
}

void handle_sigchld_events( fd_set *fds, int *fdn)
{
    if (sigchild_pipe_valid && fd_set_unset_fd( fds, fdn, sigchild_pipe[0]))
    {
        char buffer[1];
        while (read( sigchild_pipe[0], buffer, sizeof(buffer)) > 0)
            sigchld_handler_async();
    }
}

void init_signals_postconfig()
{
    gboolean need_sigchld = FALSE;
#ifdef HAVE_SN
    // Initialize startup-notification
    if (startup_notifications) {
        server.sn_display = sn_display_new(server.display, error_trap_push, error_trap_pop);
        server.pids = g_tree_new(cmp_ptr);
        need_sigchld = TRUE;
    }
#endif // HAVE_SN
    if (panel_config.execp_list)
        need_sigchld = TRUE;

    if (need_sigchld) {
        // Setup a handler for child termination
        if (pipe(sigchild_pipe) != 0) {
            fprintf(stderr, "tint2: Creating pipe failed.\n");
        } else {
            fcntl(sigchild_pipe[0], F_SETFL, O_NONBLOCK | fcntl(sigchild_pipe[0], F_GETFL));
            fcntl(sigchild_pipe[1], F_SETFL, O_NONBLOCK | fcntl(sigchild_pipe[1], F_GETFL));
            sigchild_pipe_valid = 1;
            if (sigaction(SIGCHLD, &(sigaction_t){.sa_handler = sigchld_handler, .sa_flags = SA_RESTART}, NULL))
                perror("sigaction");
        }
    }
}

void emit_self_restart(const char *reason)
{
    fprintf(stderr,
            YELLOW "%s %d: triggering tint2 restart, reason: %s" RESET "\n",
            __FILE__,
            __LINE__,
            reason);
    signal_pending = SIGUSR1;
}

int get_signal_pending()
{
    return signal_pending;
}
#endif
