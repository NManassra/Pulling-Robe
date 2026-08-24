#include "signal_handler.h"
#include <stdio.h>
#include <stdlib.h>

// this function sets up what should happen when a specific signal is received.
// for example, when the program receives SIGINT, it can run a specific function ( handler).
void setup_signal_handler(int signum, void (*handler)(int)) {
    struct sigaction sa;               // define the behavior for the signal
    sa.sa_handler = handler;           //  run when the signal is received
    sigemptyset(&sa.sa_mask);          // block no other signals during the execution of the handler
    sa.sa_flags = 0;                   // no special flags used here

    // set the signal action using sigaction()
    // if it fails, print an error and exit the program
    if (sigaction(signum, &sa, NULL) == -1) {
        perror("sigaction error");
        exit(EXIT_FAILURE);
    }
}

// this function sends a signal to a process
// 'pid' is the ID of the process we want to send the signal to
// 'signum' is the signal number we want to send
void send_signal(pid_t pid, int signum) {
    // use kill() to send the signal
    // if it fails, print an error message
    if (kill(pid, signum) == -1) {
        perror("signal sending failed");
    }
}
