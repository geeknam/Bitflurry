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
void fs_putFile(char *filename, int *raidLevel) {
	switch (*raidLevel) {
		case 0:
			raid0_putFile(filename);
			break;
		case 5:
			raid5_putFile(filename);
			break;
		default:
			printf("No RAID level specified. Cannot continue.");
	}
}


void fs_getFile(char *filename, int *raidLevel, char *outfile) {
	
	bf_file * file = db_getFile(filename);
	
	if (file == NULL) {
		printf("File not found.\n");
	} else {

		switch (*raidLevel) {
			case 0:
				raid0_getFile(&file, outfile);
				break;
			case 5:
				raid5_getFile(&file, outfile);
				break;
			default:
				printf("No RAID level specified. Cannot continue.");
		}
	}
}
