#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

long getFileSize(const char *filename);

int putFile(const char *filename);
int getFile(const char *out_file, int total_drives, const char *files[]);

void createFileFromXOR(const char *out_file, int total_drives, const char *files[]);
void createFileFromXOR2(const char *out_file, int total_drives, unsigned int drive_lost, const char *files[]);