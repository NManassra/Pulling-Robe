#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <signal.h>

void setup_signal_handler(int signum, void (*handler)(int));
void send_signal(pid_t pid, int signum);

#endif

