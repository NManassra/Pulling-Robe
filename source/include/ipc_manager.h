#ifndef IPC_MANAGER_H
#define IPC_MANAGER_H

void create_pipes(int pipes[][2], int count);
void close_unused_pipes(int pipes[][2], int count, int current_index, int mode);
#endif

