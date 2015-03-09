#ifndef STATE_IO_H
#define STATE_IO_H

kilobot** bot_loader(const char *filename, int *n_bots);
void save_bot_state_to_file(kilobot **bot_array, int array_size, const char *filename);

#endif
