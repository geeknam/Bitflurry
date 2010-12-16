#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdio.h>

#include "database.h"
#include "bitflurry.h"

#include "raid0.h"
#include "raid5.h"

long fs_getFileSize(char *filename);
void fs_putFile(char *filename, int *raidLevel);
void fs_getFile(char *filename, int *raidLevel, char *outfile);

#endif
