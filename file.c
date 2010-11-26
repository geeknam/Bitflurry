#include "file.h"
#include "galois.h"

#define BUFFER_SIZE 1048576 // 1Mb 1048576
#define CHUNK_SIZE 4*BUFFER_SIZE // We use 4MB as our chunk size

int main (int argc, char *argv[]) {
	// first argument: file to slice
	if (argv[1] == NULL) {
		printf("bitflurry [files component ONLY]\n");
		printf("(c)2010 FANI FYP Team / Republic Polytechnic\n\n");
		printf("Usage: file <command> <arguments>\n\n");
		printf("Command available: \n");
		printf("\tput <filename>\n");
		printf("\tget <output_filename> <file1..file2..etc.>\n");
		printf("\tparitify <output_filename> <file1..file2..etc.>\n\n");
	} else {
		if (strcmp(argv[1], "put") == 0) {
			if (argv[2] == NULL) printf("Please specify a file to put.\n");
			
			printf("Striping %s...\n", argv[2]);
			putFile(argv[2]);
			printf("Process completed.\n");
		} else if (strcmp(argv[1], "get") == 0) {
			const char *files[argc-3];
			int i;
			for (i = 3; i < argc; i++) {
				files[i-3] = argv[i];
			}
			
			printf("Retrieving to %s...\n", argv[2]);
			getFile(argv[2], argc-3, files);
			printf("Process completed.\n");
		} else if (strcmp(argv[1], "paritify") == 0) {
			const char *files[argc-3];
			int i;
			for (i = 3; i < argc; i++) {
				files[i-3] = argv[i];
			}
			/*int index = atoi(argv[2]);
			int digits = log10((double) index) + 1;
			char *parity_file = malloc(strlen("out/vanilla/") + digits);
			sprintf(parity_file, "out/vanilla/%d", index);*/

			printf("Reconstructing missing element to %s...\n", argv[2]);
			createFileFromXOR(argv[2], argc-3, files);
			printf("Process completed.\n");
		} else if (strcmp(argv[1], "paritify2") == 0) {
			const char *files[argc-4];
			int i;
			for (i = 4; i < argc; i++) {
				files[i-4] = argv[i];
			}
			/*int index = atoi(argv[2]);
			int digits = log10((double) index) + 1;
			char *parity_file = malloc(strlen("out/vanilla/") + digits);
			sprintf(parity_file, "out/vanilla/%d", index);*/

			unsigned int drive_lost = *argv[3];
			printf("Reconstructing missing element to %s...\n", argv[2]);
			createFileFromXOR2(argv[2], argc-4, drive_lost, files);
			printf("Process completed.\n");
		} else if (strcmp(argv[1], "galois") == 0) {
			printf("Using data: %s / %x and %s / %x\n\n", argv[2], *argv[2], argv[3], *argv[3]);
			printf("Calculating Q:\n");
			char buffer_galois1[sizeof(argv[2])];
			char buffer_galois2[sizeof(argv[3])];
			
			galois_w08_region_multiply(argv[2], 1, sizeof(argv[2]), NULL, 0);
			memcpy(buffer_galois1, argv[2], sizeof(argv[2]));
			printf("\tg^0 . A = %x\n", *buffer_galois1);
			
			galois_w08_region_multiply(argv[3], 2, sizeof(argv[3]), NULL, 0);
			memcpy(buffer_galois2, argv[3], sizeof(argv[3]));
			printf("\tg^1 . B = %x\n", *buffer_galois2);
			
			char buffer_XORed[sizeof(argv[2])];
			*buffer_XORed = (char) (*buffer_galois1 ^ *buffer_galois2);
			printf("\tQ = (g^0 . A) + (g^1 . B)\n\tQ = %x + %x = %x\n", *buffer_galois1, *buffer_galois2, *buffer_XORed);
			
			printf("Calculating B:\n");
			
			char temp[sizeof(buffer_XORed)];
			*temp = *buffer_XORed ^ *buffer_galois1;
			//printf("\t g^1 . B = Q + g^0 . A = %x\n", *temp);
			
			char buffer_B[sizeof(temp)];
			galois_w08_region_multiply(temp, -2, sizeof(temp), buffer_B, 0);
			//memcpy(buffer_B, temp, sizeof(temp));
			printf("\t B = (Q + g^0 . A) . g^-1\n\t B = %x . g^-1 = %x / %s\n\n", *temp, *buffer_B, *buffer_B);
			
			printf("Calculating A:\n");
			
			*temp = *buffer_XORed ^ *buffer_galois2;
			printf("\t g^1 . A = Q + g^1 . B = %x\n", *temp);
			
			char buffer_A[sizeof(temp)];
			galois_w08_region_multiply(temp, -1, sizeof(temp), buffer_A, 0);
			printf("\t A = (Q + g^1 . B) . g^0 = %x / %s\n\n", *buffer_A, *buffer_A);
		} else {
			printf("Invalid arguments specified\n");
		}
	}
	
	return 0;
}

/* Get size in bytes of the specified file */
long getFileSize(const char *filename){
    struct stat statBuffer;

    if (stat(filename, &statBuffer) != 0)
    {
        printf("\n\nERROR: Unable to obtain attributes of file %s.", filename);
        return -1;
    }
    return (long) statBuffer.st_size;
}

// split file into chunk and put them in our storage arrays
int putFile(const char *filename) {
	long file_size;           		//total size of the file
	long slice_size = CHUNK_SIZE;    //size of each slice: 4MB
	long last_slice_size;			//size of the last slice
	long cur_slice_size;			//determine whether the size is 4MB or the remainder (last_slice_size)
	long num_slices;	      		//number of slices
	int slice_index;		  		//used in the loop
	int last_slice_index;
	
	// our mounts hardcoded for now
	char *dirs[] = {"chocolate", "strawberry"};
	
	char *file_out = NULL;
	FILE *fp_in;
	FILE *fp_out;

	char buffer[BUFFER_SIZE];
	int bytes_read;
	int bytes_to_write;
	long bytes_written;
	
	file_size = getFileSize(filename);							// get a size of the file
	num_slices = file_size/slice_size;							// number of slices
	last_slice_index = num_slices + 1;							// index of the last slice
	last_slice_size = file_size - (num_slices*slice_size);		// size of the last slice

	// open original file
	fp_in = fopen(filename, "rb");

	int digits = log10((double) num_slices) + 1; // get the number of digit for mem allocation
	int dir_index = 0; // our raid dirs indexer
	
	for (slice_index = 1; slice_index <= num_slices+1; slice_index++) {
		// allocate memory for the name of the output files
		file_out = (char *) realloc(file_out, (4 + strlen(dirs[dir_index]) + 1 + strlen(filename) + 1 + digits + 1) * sizeof(char));    //sizeof(char) = 1 byte

		//printf("Allocated %d for out filename.\n", (4 + strlen(dirs[dir_index]) + 1 + strlen(filename) + 1 + digits + 1) * sizeof(char));

		sprintf (file_out, "out/%s/%s.%d", dirs[dir_index], filename, slice_index);     //concatenate names for the new output: movie.mp4.1 , movie.mp4.2, ...
		printf("Creating chunk %s...\n", file_out);
		fp_out = fopen(file_out, "wb");						   // create and open a output file
		bytes_written = 0;

		// determine whether the size is 4MB or the remainder (last_slice_size)
		if (slice_index == last_slice_index) {
			cur_slice_size = last_slice_size;   // remainder
		}
		else {
			cur_slice_size = slice_size;        // 4MB
		}

		while (bytes_written < cur_slice_size) {
			bytes_to_write = BUFFER_SIZE;
			bytes_read = fread(buffer, 1, bytes_to_write, fp_in);    // read bytes from file to the buffer
			fwrite(buffer, 1, bytes_read, fp_out);					 // write bytes from buffer to the current slice
			bytes_written += bytes_to_write;
		}
		fclose(fp_out);		// close the output file
		
		dir_index++;
		if (dir_index > 1) dir_index = 0;
	}	
	fclose(fp_in);			// close the original file	
	free(file_out);			// free the memory
}

int getFile(const char *out_file, int total_drives, const char *files[]) {
	FILE *file_out = fopen(out_file, "wb");
	printf("Writing to %s\n", out_file);
	
	char buffer[BUFFER_SIZE];
	int bytes_read;
	long bytes_written;
	
	int i = 0;
	while (i < total_drives) {
		FILE *fp = fopen(files[i], "rb");
		if (fp != NULL) {
			printf("Opened %s...\n", files[i]);
		} else { return; }
		
		long file_size = getFileSize(files[i]);
		
		bytes_written = 0;
		
		while (bytes_written < file_size) {
			bytes_read = fread(buffer, 1, BUFFER_SIZE, fp);
			fwrite(buffer, 1, bytes_read, file_out);					 // write bytes from buffer to the current slice
			bytes_written += BUFFER_SIZE;
		}
		
		fclose(fp);
		i++;
	}
}

/*void createFileFromXOR(const char *out_file, int total_drives, const char *files[]) {
	long file_size = 0;
	
	FILE *file_out = fopen(out_file, "wb");
	printf("Writing to %s\n", out_file);
	
	FILE *fp[total_drives];

	int i = 0;
	while (i < total_drives) {
		printf("Opening %s....", files[i]);
		fp[i] = fopen(files[i], "rb");
		if (fp != NULL) {
			printf("opened.\n");
		} else { return; }
		
		if (getFileSize(files[i]) > file_size) file_size = getFileSize(files[i]);
		//printf("Total File Size: %d\n", file_size);
		i++;
	}
	printf("Reconstructing...\n");

	char buffer[total_drives][1];
	char buffer_writing[1];
	int bytes_read[total_drives];
	int bytes_to_write;
	long bytes_written = 0;

	while (bytes_written < file_size) {
		*buffer_writing = 0;
		
		int j;
		for (j = 0; j < total_drives; j++) {
			bytes_read[j] = fread(buffer[j], 1, 1, fp[j]);
			*buffer_writing = (char) (*buffer_writing ^ *buffer[j]);
		}
		
		//printf("C: %s XOR %s", buffer1, buffer2);
		//printf("%x XOR %x : %x\n", *buffer1, *buffer2, *buffer_writing);
		fwrite(buffer_writing, 1, 1, file_out);					 // write bytes from buffer to the current slice
		bytes_written += 1;
	}
	
	int j;
	for (j = 0; j < i; j++) {
		fclose(fp[j]);
	}
	fclose(file_out);
}*/

void createFileFromXOR(const char *out_file, int total_drives, const char *files[]) {
	long file_size = 0;
	
	FILE *file_out = fopen(out_file, "wb");
	printf("Writing to %s\n", out_file);
	
	FILE *fp[total_drives];

	int i = 0;
	while (i < total_drives) {
		printf("Opening %s....", files[i]);
		fp[i] = fopen(files[i], "rb");
		if (fp != NULL) {
			printf("opened.\n");
		} else { return; }
		
		if (getFileSize(files[i]) > file_size) file_size = getFileSize(files[i]);
		//printf("Total File Size: %d\n", file_size);
		i++;
	}
	printf("Reconstructing...\n");

	char buffer[total_drives][1];
	char buffer_writing[1];
	int bytes_read[total_drives];
	int bytes_to_write;
	long bytes_written = 0;

	// Q = g^x . Dx + ....
	while (bytes_written < file_size) {
		*buffer_writing = 0;
		
		int j;
		for (j = 0; j < total_drives; j++) {
			bytes_read[j] = fread(buffer[j], 1, 1, fp[j]);
			
			// g^x . Dx
			// g^x . buffer[j]
			char buffer_galois[1];
			galois_w08_region_multiply(buffer[j], j, sizeof(buffer[j]), buffer_galois, 0);
			
			// Q = Q + Qx
			*buffer_writing = (char) (*buffer_writing ^ *buffer_galois);
		}
		
		//printf("C: %s XOR %s", buffer1, buffer2);
		printf("%x XOR %x : %x\n", *buffer[0], *buffer[1], *buffer_writing);
		fwrite(buffer_writing, 1, 1, file_out);					 // write bytes from buffer to the current slice
		bytes_written += 1;
	}
	
	int j;
	for (j = 0; j < i; j++) {
		fclose(fp[j]);
	}
	fclose(file_out);
}

void createFileFromXOR2(const char *out_file, int total_drives, unsigned int drive_lost, const char *files[]) {
	long file_size = 0;
	
	FILE *file_out = fopen(out_file, "wb");
	printf("Writing to %s\n", out_file);
	
	FILE *fp[total_drives];

	int i = 0;
	while (i < total_drives) {
		printf("Opening %s....", files[i]);
		fp[i] = fopen(files[i], "rb");
		if (fp != NULL) {
			printf("opened.\n");
		} else { return; }
		
		if (getFileSize(files[i]) > file_size) file_size = getFileSize(files[i]);
		//printf("Total File Size: %d\n", file_size);
		i++;
	}
	printf("Reconstructing...\n");

	char buffer[total_drives][1];
	char buffer_writing[1];
	int bytes_read[total_drives];
	int bytes_to_write;
	long bytes_written = 0;

	// Dj = (Q + Qx) . g^-j
	// j = 0 is our galois file
	while (bytes_written < file_size) {
		*buffer_writing = 0;
		
		int j;
		for (j = 1; j < total_drives; j++) {
			bytes_read[j] = fread(buffer[j], 1, 1, fp[j]);
			
			// Qx = g^x . Dx
			char buffer_galois[1];
			galois_w08_region_multiply(buffer[j], 1, sizeof(buffer[j]), buffer_galois, 0);
			
			// nQ = Q + Qx
			*buffer_writing = (char) (*buffer_writing ^ *buffer_galois);
			
			//printf("Writing from disk: %d\n", j);
		}
		
		// nQ + Q
		bytes_read[0] = fread(buffer[0], 1, 1, fp[0]);
		*buffer_writing = (char) (*buffer_writing ^ *buffer[0]);
		
		// Dj = g^-j . (nQ + Q)
		//char buffer_galois[1];
		galois_w08_region_multiply(buffer_writing, 0-drive_lost, sizeof(buffer_writing), NULL, 0);
		//*buffer_writing = *buffer_galois;
		
		printf("Writing to disk: %x\n", *buffer_writing);
		
		//printf("C: %s XOR %s", buffer1, buffer2);
		//printf("%x XOR %x : %x\n", *buffer1, *buffer2, *buffer_writing);
		fwrite(buffer_writing, 1, 1, file_out);					 // write bytes from buffer to the current slice
		bytes_written += 1;
	}
	
	int j;
	for (j = 0; j < i; j++) {
		fclose(fp[j]);
	}
	fclose(file_out);
}