#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

// global variables that will be set from the config file
int ENERGY_MIN, ENERGY_MAX;
int DECREASE_MIN, DECREASE_MAX;
int REJOIN_MIN, REJOIN_MAX;
int WIN_THRESHOLD, MAX_SCORE, MAX_DURATION;

// external functions declared from other files
extern void run_referee(int argc, char **argv);
extern void run_player(int, int, const char*);

// function to generate a random number between min and max (inclusive)
int rand_range(int min, int max) {
    return rand() % (max - min + 1) + min;
}

// this function reads values from the config file and sets global variables
void read_config(const char* filename) {
    FILE* file = fopen(filename, "r"); // open the file for reading
    if (!file) {
        perror("could not open config file"); // print error if failed
        exit(EXIT_FAILURE);
    }

    char line[256];
    // read each line of the file
    while (fgets(line, sizeof(line), file)) {
        // skip empty lines or comment lines (starting with #)
        if (line[0] == '#' || strlen(line) < 2)
            continue;

        // check for specific config keys and extract values
        if (sscanf(line, "ENERGY_RANGE %d %d", &ENERGY_MIN, &ENERGY_MAX) == 2) continue;
        if (sscanf(line, "ENERGY_DECREASE_RATE %d %d", &DECREASE_MIN, &DECREASE_MAX) == 2) continue;
        if (sscanf(line, "REJOIN_TIME %d %d", &REJOIN_MIN, &REJOIN_MAX) == 2) continue;
        if (sscanf(line, "WIN_THRESHOLD %d", &WIN_THRESHOLD) == 1) continue;
        if (sscanf(line, "MAX_SCORE %d", &MAX_SCORE) == 1) continue;
        if (sscanf(line, "MAX_DURATION %d", &MAX_DURATION) == 1) continue;
    }

    fclose(file); // close the file after reading
}

// this is the main function where execution starts
int main(int argc, char* argv[]) {
    // check if the program is running as a player
    if (argc >= 2 && strcmp(argv[1], "player") == 0) {
        // player mode requires exactly 5 arguments
        if(argc != 5) {
            fprintf(stderr, "usage: %s player <player_id> <pipe_fd> <config_path>\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        // extract player id, pipe file descriptor, and config file path
        int player_id = atoi(argv[2]);
        int pipe_fd = atoi(argv[3]);
        char* config_path = argv[4];

        // start the player logic
        run_player(player_id, pipe_fd, config_path);
    } else {
        // if not player mode, it should run as referee and needs 2 arguments
        if(argc != 2) {
            fprintf(stderr, "usage: %s <config_file>\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        // start the referee logic
        run_referee(argc, argv);
    }

    return 0; // program finished successfully
}