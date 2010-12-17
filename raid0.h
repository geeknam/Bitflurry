#include "filesystem.h"

int DISK_TOTAL;
char DISK_PATH[50];
int DISK_RAID;
char **DISK_ARRAY;

void raid0_putFile(char *filename);
void raid0_getFile(bf_file* file, char *outfile);
