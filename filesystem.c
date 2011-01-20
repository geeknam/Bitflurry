#include "filesystem.h"

/* Get size in bytes of the specified file */
long fs_getFileSize(char *filename){
    struct stat statBuffer;

    if (stat(filename, &statBuffer) != 0)
    {
        printf("\n\nERROR: Unable to obtain attributes of file %s.", filename);
        return -1;
    }
    return (long) statBuffer.st_size;
}

// split file into chunk and put them in our storage arrays
void fs_putFile(char *filename, int raidLevel) {
	time_t start_time, end_time;
	start_time = time(NULL);
	
	switch (raidLevel) {
		case 0:
			raid0_putFile(filename);
			break;
		case 4:
			raid4_putFile(filename);
			break;
		case 5:
			raid5_putFile(filename);
			break;
		case 6:
			raid6_putFile(filename);
			break;
		default:
			printf("No RAID level specified. Cannot continue.");
	}
	
	end_time = time(NULL);
	printf("The operation took %s seconds.\n", end_time-start_time);
}


void fs_getFile(char *filename, int raidLevel, char *outfile) {
	time_t start_time, end_time;
	start_time = time(NULL);
	
	bf_file * file = db_getFile(filename);
	
	if (file == NULL) {
		printf("File not found.\n");
	} else {

		switch (raidLevel) {
			case 0:
				raid0_getFile(file, outfile);
				break;
			case 4:
				raid4_getFile(file, outfile);
				break;
			case 5:
				raid5_getFile(file, outfile);
				break;
			case 6:
				raid6_getFile(file, outfile);
				break;
			default:
				printf("No RAID level specified. Cannot continue.");
		}
	}

	end_time = time(NULL);
	if (!to_stdout) printf("The operation took %s seconds.\n", end_time-start_time);
}

void fs_fsck(int raidLevel) {
	time_t start_time, end_time;
	start_time = time(NULL);
	
	switch (raidLevel) {
		case 0:
			raid0_fsck();
			break;
		case 4:
			raid4_fsck();
			break;
		case 5:
			raid5_fsck();
			break;
		case 6:
			raid6_fsck();
			break;
		default:
			printf("No RAID level specified. Cannot continue.");
	}
	
	end_time = time(NULL);
	printf("The operation took %s seconds.\n", end_time-start_time);
}
