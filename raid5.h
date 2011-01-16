#include "filesystem.h"

int DISK_TOTAL;
char DISK_PATH[50];
int DISK_RAID;
char **DISK_ARRAY;

void raid5_putFile(char *filename);

void raid5_reParity(int start_row, int end_row);

void raid5_getFile(bf_file* file, char *outfile);
