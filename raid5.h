#include "filesystem.h"

void raid5_putFile(char *filename);

void raid5_reParity(int start_row, int end_row);

void raid5_fsck();

void raid5_getFile(bf_file* file, char *outfile);
