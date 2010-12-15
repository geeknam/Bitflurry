#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "database.h"

#define DISK_TOTAL 3
#define DISK_PATH "disks/"
// our disks
//	*IMPORTANT* make sure the number of disk below equates to the DISK_TOTAL!!
char *dirs[] = {"chocolate", "strawberry", "vanilla"};

// initialization
// 	needs to be run before using any other functions
int init();

// put file to the filesystem
void putFile(char *filename, int length);

// get file from the filesystem
void getFile(char *filename);