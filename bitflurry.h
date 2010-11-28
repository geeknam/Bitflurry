#include <stdio.h>
#include <stdlib.h>

#include "database.h"

#define DISK_TOTAL 3

void init();
void putFile(char *filename, int length);
void getFile(char *filename);