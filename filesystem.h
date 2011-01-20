#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>

// These could be useful for RAID calculations
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>
#include <libgen.h>

#include "database.h"
#include "bitflurry.h"

#define BUFFER_SIZE 1048576 	 	// 1Mb 1048576
#define CHUNK_SIZE 4*BUFFER_SIZE 	// We use 4MB as our chunk size

#include "raid0.h"
#include "raid5.h"
#include "raid6.h"

int DISK_TOTAL;
char DISK_PATH[50];
int DISK_RAID;
char DISK_ARRAY[50][128];

long fs_getFileSize(char *filename);
void fs_putFile(char *filename, int raidLevel);
void fs_getFile(char *filename, int raidLevel, char *outfile);
void fs_fsck(int raidLevel);

#endif
