#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include "database.h"
#include "filesystem.h"

int to_stdout;
	
// initialization
// 	needs to be run before using any other functions
int init(int force);

// load config from CONFIG_FILE
//	*MUST BE DONE BEFORE INIT*
void loadConfig();

// put file to the filesystem
void putFile(char *filename);

// get file from the filesystem
void getFile(char *filename, char *outfile);