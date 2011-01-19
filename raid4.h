#include "filesystem.h"

void raid4_putFile(char *filename);
void raid4_getFile(bf_file* file, char *outfile);
void raid4_reParity(int start_row, int end_row);
void raid4_fsck();