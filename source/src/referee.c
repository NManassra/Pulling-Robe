#include "referee.h"           
#include "utils.h"             
#include "ipc_manager.h"       
#include "signal_handler.h"    
#include "opengl.h"            
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>            
#include <sys/wait.h>          
#include <signal.h>            
#include <time.h>              
#include <string.h>
#include <fcntl.h>             
#include <sys/stat.h>         

#define PLAYERS_PER_TEAM 4
#define TOTAL_PLAYERS (PLAYERS_PER_TEAM * 2)  

pid_t player_pids[TOTAL_PLAYERS];     // store process IDs of player processes
int pipes[TOTAL_PLAYERS][2];          // pipes for IPC: one pipe per player
int scores[2] = {0, 0};               // score for team 1 and team 2
int consecutive_wins[2] = {0, 0};     // track consecutive round wins
time_t game_start_time;              // time when game started

// handler for alarm signal , end game after timeout
void alarm_handler(int sig) {
    fflush(stdout);
    printf("\n[Referee] Maximum game duration reached!\n");

    // terminate all player processes
    for (int i = 0; i < TOTAL_PLAYERS; i++)
        kill(player_pids[i], SIGTERM);
    exit(0);
}

// function to cleanly terminate game
void terminate_game(int fifo_fd) {
    // kill all player processes and wait for them to exit
    for (int i = 0; i < TOTAL_PLAYERS; i++) {
        kill(player_pids[i], SIGTERM);
        wait(NULL);
    }

    // close and delete the FIFO used for visualizer communication
    close(fifo_fd);
    unlink("/tmp/rope_game_fifo");

    fflush(stdout);
    printf("\n[Referee] Game Over! Final Score - Team 1: %d, Team 2: %d\n", scores[0], scores[1]);
    exit(0);
}

// main function that controls referee logic
void run_referee(int argc, char **argv) {
    srand(time(NULL));  // seed random number generator

    const char* config_file = argv[1];  // get config file path from arguments
    read_config(config_file);          // read config settings
    create_pipes(pipes, TOTAL_PLAYERS); // set up pipes for all players

    game_start_time = time(NULL);     // save start time
    alarm(MAX_DURATION + 25);         // set max duration for game
    setup_signal_handler(SIGALRM, alarm_handler);  // set up alarm signal handler

    // create FIFO pipe to communicate with OpenGL visualizer
    mkfifo("/tmp/rope_game_fifo", 0666);

    // Fork process to start the visualizer
    pid_t visualizer_pid = fork();
    if (visualizer_pid == 0) {
        setenv("LIBGL_ALWAYS_SOFTWARE", "1", 1);  // ensure OpenGL runs in software mode
        init_window(argc, argv);                 // initialize OpenGL window
        run_opengl_loop();                       // start OpenGL rendering loop
        exit(0);                                 // exit visualizer process
    }

    sleep(1);  // give visualizer time to start

    // open FIFO for writing game state updates to visualizer
    int fifo_fd = open("/tmp/rope_game_fifo", O_WRONLY);
    if (fifo_fd < 0) {
        perror("FIFO open failed (Referee)");
        exit(1);
    }

    // create player processes
    for (int i = 0; i < TOTAL_PLAYERS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // inside child (player) process
            char player_id[10], pipe_fd[10];
            sprintf(player_id, "%d", i);
            sprintf(pipe_fd, "%d", pipes[i][1]);  // use write end of pipe

            char config_path[256];
            realpath(config_file, config_path);  // get full path to config file

            // close all unused pipes for this player
            close_unused_pipes(pipes, TOTAL_PLAYERS, i, 1);

            // replace this process with player executable
            execl("./rope_pulling_game_sim", "./rope_pulling_game_sim", 
                  "player", player_id, pipe_fd, config_path, NULL);

            perror("execl failed");  // if exec fails, print error
            exit(EXIT_FAILURE);
        } else if (pid > 0) {
            player_pids[i] = pid;  // save child PID
        } else {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
    }

    // close pipes not needed by referee
    close_unused_pipes(pipes, TOTAL_PLAYERS, -1, 0);

    int round_number = 1;
    sleep(3);  // wait a bit before starting first round

    // start game loop: continues until a winner is determined
    while (1) {
        printf("\n[Referee] Round %d is starting!\n", round_number);
        fflush(stdout);

        // send SIGUSR1 to all players to signal round is starting
        for (int i = 0; i < TOTAL_PLAYERS; i++)
            send_signal(player_pids[i], SIGUSR1);

        sleep(2);  // give time to prepare

        // send SIGUSR2 to all players to start pulling
        for (int i = 0; i < TOTAL_PLAYERS; i++)
            send_signal(player_pids[i], SIGUSR2);

        int round_winner = -1;

        // keep checking efforts until there's a winner
        while (round_winner == -1) {
            int team_efforts[2] = {0, 0};  // reset team efforts

            // read effort from each player
            for (int i = 0; i < TOTAL_PLAYERS; i++) {
                int current_effort = 0;
                ssize_t n = read(pipes[i][0], &current_effort, sizeof(int));
                if (n <= 0) current_effort = 0;

                int team = (i < PLAYERS_PER_TEAM) ? 0 : 1;  // Team 0 or 1
                int position = (i % PLAYERS_PER_TEAM) + 1; // Position affects effort weight
                team_efforts[team] += current_effort * position;
            }

            // show team efforts
            printf("[Referee] Efforts -> Team 1: %d | Team 2: %d\n",
                   team_efforts[0], team_efforts[1]);
            fflush(stdout);

            // send current state to visualizer through FIFO
            char fifo_buf[128];
            sprintf(fifo_buf, "%d %d %d %d %d %d %d\n", round_number, team_efforts[0], team_efforts[1],
                    scores[0], scores[1], round_winner, 
                    (scores[0] >= MAX_SCORE || consecutive_wins[0] >= 2) ? 1 :
                    (scores[1] >= MAX_SCORE || consecutive_wins[1] >= 2) ? 2 : -1);
            write(fifo_fd, fifo_buf, strlen(fifo_buf));

            // check if a team won this round
            if (abs(team_efforts[0] - team_efforts[1]) >= WIN_THRESHOLD) {
                round_winner = (team_efforts[0] > team_efforts[1]) ? 0 : 1;
                scores[round_winner]++;
                consecutive_wins[round_winner]++;
                consecutive_wins[1 - round_winner] = 0;

                printf("[Referee] Team %d wins Round %d!\n",
                       round_winner + 1, round_number);
                fflush(stdout);
            }

            sleep(1);  // Pause before checking again
        }

        // check if a team won the game
        if (scores[round_winner] >= MAX_SCORE || consecutive_wins[round_winner] >= 2) {
            char fifo_buf[128];
            sprintf(fifo_buf, "%d %d %d %d %d %d %d\n", round_number, 0, 0,
                    scores[0], scores[1], round_winner, 
                    (scores[0] >= MAX_SCORE || consecutive_wins[0] >= 2) ? 1 :
                    (scores[1] >= MAX_SCORE || consecutive_wins[1] >= 2) ? 2 : -1);
            write(fifo_fd, fifo_buf, strlen(fifo_buf));

            // end the game
            terminate_game(fifo_fd);
        }

        // send final result of the round to visualizer
        char fifo_buf[128];
        sprintf(fifo_buf, "%d %d %d %d %d %d %d\n", round_number, 0, 0,
                scores[0], scores[1], round_winner, 
                (scores[0] >= MAX_SCORE || consecutive_wins[0] >= 2) ? 1 :
                (scores[1] >= MAX_SCORE || consecutive_wins[1] >= 2) ? 2 : -1);
        write(fifo_fd, fifo_buf, strlen(fifo_buf));

        // send SIGINT to players to end the round
        for (int i = 0; i < TOTAL_PLAYERS; i++)
            send_signal(player_pids[i], SIGINT);

        round_number++;  // move to next round
        sleep(5);        // wait before next round
    }
}