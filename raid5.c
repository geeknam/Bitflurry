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
	int end_row = start_row + num_slices / (DISK_TOTAL - 1);
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
			} else if (row_index == end_row && slice_iterator > num_slices) {
				printf(".\t");
				continue;
			} else {
				// Print parity or slice number
				if (column_index == parity_at) {
					printf("P\t");
				} else {
					printf("%d\t", slice_iterator);
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

void raid5_getFile(bf_file* file, char *outfile) {
	// ?
}
