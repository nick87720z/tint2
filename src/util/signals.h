#ifndef SIGNALS_H
#define SIGNALS_H

typedef struct sigaction sigaction_t;

void init_signals();
void init_signals_postconfig();
void emit_self_restart(const char *reason);
int get_signal_pending();
void reset_signals();

void handle_sigchld_events();

extern int sigchild_pipe_valid;
extern int sigchild_pipe[2];

extern sigset_t select_sigset; // Signals for select() call, initialized by init_signals()

#endif
