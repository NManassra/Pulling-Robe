#include "player.h"
#include "utils.h"
#include "signal_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

// flags that change when the player receives certain signals
volatile sig_atomic_t ready = 0;         //  1 when player is told to get ready
volatile sig_atomic_t start_pulling = 0; // 1 when player is told to start pulling
volatile sig_atomic_t terminate = 0;     //  1 when player should stop and exit
volatile sig_atomic_t reset = 0;         //  1 when player should reset (restart)


// functions run when signals are received
void sigusr1_handler(int sig) { ready = 1; }             // signal to get ready
void sigusr2_handler(int sig) { start_pulling = 1; }     //  start pulling
void sigterm_handler(int sig) { terminate = 1; }         //  terminate the player
void sigint_handler(int sig) { reset = 1; }              // reset energy and pulling

//  the main function for each player process
void run_player(int player_id, int pipe_fd, const char* config_path) {
    // set a random seed so each player get different values
    srand(time(NULL) ^ (player_id << 8));

    // load game configuration from the file
    read_config(config_path);

    // each team has 4 players.
    // this calculates which team and player number this is.
    int team = (player_id / 4) + 1;
    int player_number = player_id % 4;

    // set up what each signal should do
    setup_signal_handler(SIGUSR1, sigusr1_handler);
    setup_signal_handler(SIGUSR2, sigusr2_handler);
    setup_signal_handler(SIGTERM, sigterm_handler);
    setup_signal_handler(SIGINT, sigint_handler);

    // assign random energy and decrease rate for this player
    int energy = rand_range(ENERGY_MIN, ENERGY_MAX);
    int tempEnergy = energy; // save a copy to restore later if needed
    int decrease_rate = rand_range(DECREASE_MIN, DECREASE_MAX);

    // show that the player has been initialized
    fflush(stdout); // Make sure output is printed immediately
    printf("[Team %d][Player %d] initialized with energy %d, decrease rate %d\n", team, player_number, energy, decrease_rate);
    fflush(stdout);

    // main game loop - runs until terminate flag is set
    while (!terminate) {
        ready = start_pulling = 0; // Reset flags

        // wait until ready signal is received
        while (!ready && !terminate) pause();
        if (terminate) break;

        printf("[Team %d][Player %d] ready!\n", team, player_number);
        fflush(stdout);

        // wait until start signal is received
        while (!start_pulling && !terminate) pause();
        if (terminate) break;

        printf("[Team %d][Player %d] started pulling!\n", team, player_number);
        fflush(stdout);

        // pulling phase - runs until terminate or reset is triggered
        while (!terminate && start_pulling) {
            // If reset signal received
            if (reset) {
                reset = 0;
                printf("[Team %d][Player %d] stopped pulling and resetting!\n", team, player_number);
                fflush(stdout);
                energy = tempEnergy; // Restore energy
                break; // Stop pulling
            }

            // simulate a chance that the player might fall (10% chance)
            if (rand_range(1, 100) <= 10) {
                printf("[Team %d][Player %d] fell!\n", team, player_number);
                energy = 0; // player has no energy while fallen
                tempEnergy = rand_range(ENERGY_MIN / 2, ENERGY_MAX); // prepare new energy
                sleep(rand_range(REJOIN_MIN, REJOIN_MAX)); // wait before rejoining
                energy = tempEnergy;

                printf("[Team %d][Player %d] rejoined with energy %d\n", team, player_number, energy);
                fflush(stdout);
            }

            // send current energy to the referee through the pipe
            if (write(pipe_fd, &energy, sizeof(int)) == -1)
                perror("[Team %d][Player] write to pipe failed");

            // decrease the energy after pulling
            energy -= decrease_rate;
            tempEnergy = energy; // Save it again
            if (energy < 0) energy = 0; // Energy shouldn't be negative

            sleep(1); // Wait a bit before next pull
        }
    }

    // when done, close the pipe and print a message
    close(pipe_fd);

    printf("[Team %d][Player %d] terminated.\n", team, player_number);
    fflush(stdout);

    exit(EXIT_SUCCESS); // end the program
}
