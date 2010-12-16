
#define BUFFER_SIZE 1048576 	 	// 1Mb 1048576
#define CHUNK_SIZE 4*BUFFER_SIZE 	// We use 4MB as our chunk size

int DISK_TOTAL;
char DISK_PATH[50];
int DISK_RAID;
char **DISK_ARRAY;

void raid5_putFile(char *filename);

void raid5_getFile(bf_file** file, char *outfile);
