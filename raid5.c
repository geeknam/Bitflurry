#include "raid5.h"

void raid5_putFile(char *filename) {

	long file_size;           			// Size of the file
	long slice_size = CHUNK_SIZE;    		// Size of each slice: 4MB
	long last_slice_size;				// Size of the last slice
	long cur_slice_size;				// Determine whether the size is 4MB or the remainder (last_slice_size)

	long num_rows;	      				// Rows of RAID5 calculation groups
	long data_cols;						// Columns of striping

	char *file_out = NULL;
	FILE *fp_in;
	FILE *fp_out;

	char buffer[BUFFER_SIZE];
	int bytes_read;
	int bytes_to_write;
	long bytes_written;
	
	// File size and number of slices
	file_size = fs_getFileSize(filename);					// Get size of the file
	int num_slices = file_size / slice_size;


	// Get last used row & column from the database
	int lastIndex[2];
	db_getLastIndex(lastIndex);
	int start_row = lastIndex[1];
	int start_col = lastIndex[0];
	
	// Start on the next empty slice
	start_col++;
	if (start_col >= DISK_TOTAL - 1) {
		start_col = 0;
		start_row++;
	}
	
	// Calculate row to end on
	int end_row = start_row + (int) ceil((double) num_slices / (double) (DISK_TOTAL - 1));
	int end_col;

	fp_in = fopen(filename, "rb");							// open original file

	// Make sure there is no duplicate filename in the database
	while (db_isFileExist(filename)) {
		char *filename_new = malloc((strlen(filename) + 5) * sizeof(char));
		sprintf(filename_new, "%s.new", filename);
		
		filename = filename_new;
	}
	
	int id = 0;
	id = db_insertFile(filename);
	printf("Putting file %s...\n", filename);
	//printf("Using file_id: %d\n", id);
	printf("Last - Row: %d, Col: %d\n\n", lastIndex[1], lastIndex[0]);

	printf("Progress: 0%%\n");

	int row_index;
	int column_index;
	int slice_iterator = 0;
	int parity_at;

	for (row_index = start_row; row_index <= end_row; row_index++) {
		parity_at = DISK_TOTAL - 1 - row_index % DISK_TOTAL;
		for (column_index = 0; column_index < DISK_TOTAL; column_index++) {
			// Skip heading and tail columns
			if (row_index == start_row && column_index < start_col) {
				printf(".\t");
				continue;
			} else if (row_index == end_row && slice_iterator - 1 > num_slices) {
				printf(".\t");
				continue;
			} else {
				// Print parity or slice number
				if (column_index == parity_at) {
					printf("P%d\t", row_index);
				} else {
					printf("%d\t", slice_iterator);
					
					// Write file
					// allocate memory for the name of the output files
					file_out = (char *) realloc(file_out, (strlen(DISK_PATH) + strlen(DISK_ARRAY[column_index]) + toDigit(row_index) + 3) * sizeof(char));    //sizeof(char) = 1 byte

					sprintf (file_out, "%s/%s/%d", DISK_PATH, DISK_ARRAY[column_index], row_index);  //concatenate names for the new output: movie.mp4.1 , movie.mp4.2, ...
					fp_out = fopen(file_out, "wb");						   // create and open a output file
					
					while (bytes_written < cur_slice_size) {
						bytes_to_write = BUFFER_SIZE;
						bytes_read = fread(buffer, 1, bytes_to_write, fp_in);    // read bytes from file to the buffer
						fwrite(buffer, 1, bytes_read, fp_out);					 // write bytes from buffer to the current slice
						bytes_written += bytes_to_write;						
					}
					fclose(fp_out);		// close the output file
					
					if (fp_out == NULL) {
						printf("Fatal error: Perhaps the storage doesn't exist or bitflurry has no write permission.");
						break;
					}
					bytes_written = 0;
					
					slice_iterator++;
					if (db_insertChunk(id, column_index, row_index, slice_iterator) != SQLITE_OK) break;
				}
			}
		}
					
		printf("\n");
	}
/*
	while (bytes_written < cur_slice_size) {
		bytes_to_write = BUFFER_SIZE;
		bytes_read = fread(buffer, 1, bytes_to_write, fp_in);    // read bytes from file to the buffer
		fwrite(buffer, 1, bytes_read, fp_out);					 // write bytes from buffer to the current slice
		bytes_written += bytes_to_write;
	}
	fclose(fp_out);		// close the output file
*/

		
	raid5_reParity(start_row, end_row);
	printf("done!\n");
	printf("\nFile transaction completed successfully.\n");
	//printf("\nDB Insert Completed.\n");
	
	fclose(fp_in);			// close the original file	
	free(file_out);			// free the memory

/*
	for (slice_index = 0; slice_index <= num_slices; slice_index++) {
		lastIndex[0]++;
		if (lastIndex[0] >= DISK_TOTAL) { lastIndex[1]++; lastIndex[0] = 0; }
		//printf("Writing on [%d, %d]\n", lastIndex[1], lastIndex[0]);
		
		// allocate memory for the name of the output files
		file_out = (char *) realloc(file_out, (strlen(DISK_PATH) + strlen(DISK_ARRAY[lastIndex[0]]) + toDigit(lastIndex[1]) + 3) * sizeof(char));    //sizeof(char) = 1 byte

		sprintf (file_out, "%s/%s/%d", DISK_PATH, DISK_ARRAY[lastIndex[0]], lastIndex[1]);  //concatenate names for the new output: movie.mp4.1 , movie.mp4.2, ...
		fp_out = fopen(file_out, "wb");						   // create and open a output file
		if (fp_out == NULL) {
			printf("Fatal error: Perhaps the storage doesn't exist or bitflurry has no write permission.");
			break;
		}
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
		
		if (db_insertChunk(id, lastIndex[0], lastIndex[1], slice_index) != SQLITE_OK) break;
		//printf("Inserted for order: %d at (%d, %d)\n", slice_index, lastIndex[1], lastIndex[0]);
		printf("%d...", ((slice_index+1)*100)/(num_slices+1));
	}
*/

}

void raid5_reParity(int start_row, int end_row) {
	
	int bytes_read;
	long bytes_written;
	char *chunkfile = NULL;
	char *parityfile = NULL;
	char buffer[BUFFER_SIZE];
	char parity_buffer[BUFFER_SIZE];
	
	FILE** fp = malloc(sizeof(FILE*) * (DISK_TOTAL));
	
	int row_index;
	int column_index;
	int parity_at;
	int buffer_at;
	
	for (row_index = start_row; row_index <= end_row; row_index++) {
	
		parity_at = DISK_TOTAL - 1 - row_index % DISK_TOTAL;
		printf("Recalculating parity for row %d at %d...\n", row_index, parity_at);
		
		// Open file handles for each column
		for (column_index = 0; column_index < DISK_TOTAL; column_index++) {
			
			chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[column_index]) + toDigit(row_index) + 3) * sizeof(char));
			sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[column_index], row_index);
			
			printf("Chunk File is at %s\n.", chunkfile);
			
			if (column_index == parity_at) {
				fp[column_index] = fopen(chunkfile, "wb");
			} else {
				fp[column_index] = fopen(chunkfile, "rb");
			}
		}

		printf("All files opened.\n");

		// Read buffer by buffer
		long parity_index;
		for (parity_index = 0; parity_index < CHUNK_SIZE; parity_index += BUFFER_SIZE) {
			printf("Parity: %d.. ", parity_index);
			for (column_index = 0; column_index < DISK_TOTAL; column_index++) {
				printf("Column: %d\n", column_index);
				if (column_index != parity_at) {
					printf("Processing...");
					if (fp[column_index] == NULL) continue;
					printf("File: %s\n", fp[column_index]);
					fread(buffer, 1, BUFFER_SIZE, fp[column_index]);
					// Loop over buffer and perform byte XOR
					for (buffer_at = 0; buffer_at < BUFFER_SIZE; buffer_at++) {
						parity_buffer[buffer_at] = parity_buffer[buffer_at] ^ buffer[buffer_at];
					}
				}
			}
			// Write the calcuated parity back to parity file handle
			fwrite(parity_buffer, 1, BUFFER_SIZE, fp[parity_at]);
		}
		
	}
	
	free(fp);
}

void raid5_getFile(bf_file* file, char *outfile) {

	char buffer[BUFFER_SIZE];
	int bytes_read;
	long bytes_written;

	char *chunkfile = NULL;
	
	if (!to_stdout) printf("Getting %s...\n", file->filename);
	//printf("file_id: %d\n", file->_id);
	//printf("filename: %s\n", file->filename);
	//printf("total chunks: %d\n", file->total_chunks);
	//printf("chunks:\n");
	
	FILE *file_out;
	if (strcmp(outfile, "-") == 0) {
		if (!to_stdout) printf("Writing to stdout\n");
		to_stdout = 1;
		file_out = stdout;
		//file_out = freopen("/dev/null", "wb", stdout);
	} else {
		printf("Writing to %s\n", outfile);
		file_out = fopen(outfile, "wb");
	}
	
	if (!to_stdout) printf("Progress: 0...");
	int i;
	for (i = 0; i < file->total_chunks; i++) {
		//printf("\t[%d,%d] %d\n", file->chunks[i].row, file->chunks[i].col, file->chunks[i].order);//
		
		chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[file->chunks[i].col]) + toDigit(file->chunks[i].row) + 3) * sizeof(char));
		sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[file->chunks[i].col],file->chunks[i].row);
		
		FILE *fp = fopen(chunkfile, "rb");
		if (fp != NULL) {
			//printf("Opened %s...\n", chunkfile);
		} else { return; }

		long file_size = fs_getFileSize(chunkfile);

		bytes_written = 0;

		while (bytes_written < file_size) {
			bytes_read = fread(buffer, 1, BUFFER_SIZE, fp);
			fwrite(buffer, 1, bytes_read, file_out);					 // write bytes from buffer to the current slice
			if (to_stdout) fflush(stdout);
			bytes_written += BUFFER_SIZE;
		}
		fclose(fp);
		
		if (!to_stdout) printf("%d...", ((i+1)*100)/file->total_chunks);
	}

	if (!to_stdout) printf("done!\n");
	free(file);
	free(chunkfile);
	fclose(file_out);
	
	if (!to_stdout) printf("\nFile transaction completed successfully.\n");

}
