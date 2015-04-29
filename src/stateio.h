#ifndef STATE_IO_H
#define STATE_IO_H

#include <jansson.h>
kilobot** bot_loader(const char *filename, int *n_bots);
void save_bot_state_to_file(kilobot **bot_array, int array_size, const char *filename);
json_t *json_rep_all_bots(kilobot **bot_array, int array_size, int ticks);

#endif
