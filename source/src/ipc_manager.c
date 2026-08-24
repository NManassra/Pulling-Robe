#include "ipc_manager.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// this func makes many pipes.
// each pip for communication between two processes (referee and players).
void create_pipes(int pipes[][2], int count) {
    for(int i = 0; i < count; i++) {
        //  create a pipe.
        // if it fails, show an error message and stop the program.
        if(pipe(pipes[i]) == -1) {
            perror("pipe creation failed");
            exit(EXIT_FAILURE);
        }
    }
}

// function closes pipes not needed.
// it prevents processes from accidentally reading from or writing to the wrong pipe.
// 'mode'  if the current process is the referee (0) or a player (1).
void close_unused_pipes(int pipes[][2], int count, int current_index, int mode) {
    // if the current process is the referee:
    if(mode == 0) {
        // referee only needs to read from the pipes,
        // so we close the write ends of all pipes.
        close(pipes[i][1]);
    } else {
        // if the current process is a player:
        for(int i = 0; i < count; i++) {
            if(i != current_index) {
                // if the pipe is not for this player,
                // we close both read and write ends of that pipe.
                close(pipes[i][0]);
                close(pipes[i][1]);
            } else {
                // if it is this player's own pipe,we close only the read end, because the player will only write to it.
                close(pipes[i][0]);
            }
        }
    }
}
