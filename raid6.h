#include "filesystem.h"

void raid6_putFile(char *filename);
void raid6_getFile(bf_file* file, char *outfile);
void raid6_reParity(int start_row, int end_row);
void raid6_dp_reParity(int start_row, int end_row);
void raid6_fsck();
int raid6_fsck_sp();
int raid6_fsck_dp();