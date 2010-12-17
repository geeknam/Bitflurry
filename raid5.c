#include "raid5.h"

void raid5_putFile(char *filename) {

	long file_size;           			// Size of the file
	long slice_size = CHUNK_SIZE;    		// Size of each slice: 4MB
	long last_slice_size;				// Size of the last slice
	long cur_slice_size;				// Determine whether the size is 4MB or the remainder (last_slice_size)

	long num_rows;	      				// Rows of RAID5 calculation groups
	long data_cols;						// Columns of striping
	long start_row;						// Very important - needed to start parity at the right position

	// Used in the loop
	int parity_col;
	int disk_col;
	int slice_index;
	int last_slice_index;

	char *file_out = NULL;
	FILE *fp_in;
	FILE *fp_out;

	int bytes_read;
	int bytes_to_write;
	long bytes_written;

	// Calculate dimensions for RAID5 stripe
	file_size = fs_getFileSize(filename);					// Get size of the file
	data_cols = DISK_TOTAL - 1;						// Calculate number of columns used for *DATA*
	num_rows = (int) ceil((double) (file_size / slice_size) / (double) data_cols);	// Calculate number of rows (rounded up to fit)
	
	int lastIndex[2];
	db_getLastIndex(lastIndex);
	
	start_row = lastIndex[1];
	
	// num_slices = file_size / slice_size;		// Number of slices
	// last_slice_index = num_slices + 1;						// index of the last slice
	last_slice_size = file_size - (num_rows * data_cols * slice_size);		// size of the last slice

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

	int cur_row;
	int cur_col;
	for (cur_row = 0; cur_row < num_rows; cur_row++) {

		parity_col = (data_cols) - ((start_row + cur_row + 1) % (data_cols + 1));	// Calculate parity column

		for (cur_col = 0; cur_col < data_cols + 1; cur_col++) {
		
			lastIndex[0]++;

			if (cur_col < parity_col) disk_col = cur_col;
			if (cur_col > parity_col) disk_col = cur_col - 1;
			if (cur_col != parity_col) {
				//file_out = (char *) realloc(file_out, (strlen(DISK_PATH) + strlen(DISK_ARRAY[lastIndex[0]]) + toDigit(lastIndex[1]) + 3) * sizeof(char));
				//sprintf(file_out, "%s/%s/%d", DISK_PATH, DISK_ARRAY[lastIndex[0]], lastIndex[1]);  //concatenate names for the new output: movie.mp4.1
				printf("%d\t", cur_row * data_cols + disk_col);
				if (db_insertChunk(id, lastIndex[0], lastIndex[1], cur_row * data_cols + disk_col) != SQLITE_OK) break;
			} else {
				printf("P\t");
			}

			// allocate memory for the name of the output files
			//file_out = (char *) realloc(file_out, (strlen(DISK_PATH) + strlen(DISK_ARRAY[lastIndex[0]]) + toDigit(lastIndex[1]) + 3) * sizeof(char));    //sizeof(char) = 1 byte

		}
		
		lastIndex[1]++; lastIndex[0] = 0;
		
		// Progress indication
		printf("%d%%\n", ((cur_row + 1) * 100) / num_rows);

	}
	
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
