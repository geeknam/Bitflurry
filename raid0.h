	
#define BUFFER_SIZE 1048576 	 	// 1Mb 1048576
#define CHUNK_SIZE 4*BUFFER_SIZE 	// We use 4MB as our chunk size

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

#include <sqlite3.h>

int DISK_TOTAL;
char DISK_PATH[50];
int DISK_RAID;
char **DISK_ARRAY;

void raid0_putFile(char *filename);
void raid0_getFile(bf_file** file, char *outfile);
