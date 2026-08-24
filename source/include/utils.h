#ifndef UTILS_H
#define UTILS_H

int rand_range(int min, int max);
void read_config(const char* filename);

extern int ENERGY_MIN, ENERGY_MAX;
extern int DECREASE_MIN, DECREASE_MAX;
extern int REJOIN_MIN, REJOIN_MAX;
extern int WIN_THRESHOLD, MAX_SCORE, MAX_DURATION;

#endif
