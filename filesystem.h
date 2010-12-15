#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

#include "database.h"

#define BUFFER_SIZE 1048576 	 	// 1Mb 1048576
#define CHUNK_SIZE 4*BUFFER_SIZE 	// We use 4MB as our chunk size

int DISK_TOTAL;
char DISK_PATH[50];
int DISK_RAID;
char **DISK_ARRAY;

long fs_getFileSize(char *filename);
void fs_putFile(char *filename);
void fs_getFile(char *filename, char *outfile);

#endif