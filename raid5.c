#include "raid5.h"

void raid5_putFile(char *filename) {

	long file_size;           			// Size of the file
	long slice_size = CHUNK_SIZE;    		// Size of each slice: 4MB
	long last_slice_size;				// Size of the last slice
	long cur_slice_size;				// Determine whether the size is 4MB or the remainder (last_slice_size)
	int slice_index = 0;
	int last_slice_index;

	long num_rows;	      				// Rows of RAID5 calculation groups
	long data_cols;						// Columns of striping

	char *file_out = NULL;
	FILE *fp_in;
	FILE *fp_out;

	char buffer[BUFFER_SIZE];
	int bytes_read;
	int bytes_to_write;
	long bytes_written;
	
	char * stmt = NULL;
	stmt = malloc(sizeof(char));
	memset(stmt, 0, sizeof(char));
	//stmt[0] = "";
	
	// File size and number of slices
	file_size = fs_getFileSize(filename);					// Get size of the file
	int num_slices = file_size / slice_size;
	last_slice_index = num_slices + 1;
	last_slice_size = file_size - (num_slices * slice_size);

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
	id = db_insertFile(filename, file_size);
	printf("Striping file %s...\n", filename);
	printf("Progress: 0...");

	int row_index;
	int column_index;
	int parity_at;

	for (row_index = start_row; row_index < end_row; row_index++) {
		parity_at = DISK_TOTAL - 1 - row_index % DISK_TOTAL;
		for (column_index = 0; column_index < DISK_TOTAL; column_index++) {
			// Skip heading and tail columns
			if (
				(row_index == start_row && column_index < start_col) ||
				(row_index == end_row && slice_index > num_slices)
			) {
				continue;
			} else {
				// Print parity or slice number
				if (column_index != parity_at) {
					// Faruq: Dirty fix for clearing extra slice
					// TODO: Better fix!
					if (slice_index > last_slice_index-1) break;
					
					// Write file
					// allocate memory for the name of the output files
					int name_mem = (strlen(DISK_PATH) + strlen(DISK_ARRAY[column_index]) + toDigit(row_index) + 5) * sizeof(char);
					file_out = (char *) realloc(file_out, name_mem);    //sizeof(char) = 1 byte

					snprintf (file_out, name_mem, "%s/%s/%d", DISK_PATH, DISK_ARRAY[column_index], row_index);  //concatenate names for the new output: movie.mp4.1 , movie.mp4.2, ...
					fp_out = fopen(file_out, "wb");						   // create and open a output file
					
					bytes_written = 0;
					
					// determine whether the size is 4MB or the remainder (last_slice_size)
					if (slice_index == last_slice_index) {
						cur_slice_size = last_slice_size;   // remainder
					} else {
						cur_slice_size = slice_size;        // 4MB
					}
					
					while (bytes_written < slice_size) {
						bytes_to_write = BUFFER_SIZE;
						bytes_read = fread(buffer, 1, bytes_to_write, fp_in);    // read bytes from file to the buffer
						if (bytes_read == 0) {
							// Padding to ensure chunk size is always = BUFFER_SIZE
							char pad_buffer[slice_size-bytes_written];
							bytes_read = sizeof(pad_buffer);
							bytes_to_write = bytes_read;
							memset(pad_buffer, '\0', bytes_to_write);
							//fwrite(pad_buffer, 1, bytes_to_write, fp_out);					 // write bytes from buffer to the current slice
						} else {
							fwrite(buffer, 1, bytes_read, fp_out);					 // write bytes from buffer to the current slice
						}
						bytes_written += bytes_read;
					}
					fflush(fp_out);
					fclose(fp_out);		// close the output file
					
					if (fp_out == NULL) {
						printf("Fatal error: Perhaps the storage doesn't exist or bitflurry has no write permission.");
						break;
					}
					bytes_written = 0;
					
					printf("\rProgress: %d%%...", slice_index * 100 / num_slices);
					fflush(stdout);
					
					slice_index++;
					if (db_insertChunk_cacheStatement(&stmt, id, column_index, row_index, slice_index) < 1) break;
				}
			}
		}
	}
	
	fclose(fp_in);	// Close the original file
	free(file_out); // Free memory
	printf(" Done!\n\n");
	
	raid5_reParity(start_row, end_row - 1);
	
	printf("Committing file to database...\n");
	if (db_insertChunk_cacheCommit(stmt) != SQLITE_OK) {
		printf("Unable to commit file slices to database. Filesystem is in an inconsistent state.\n\n");
	} else {
		printf("File transaction completed successfully!\n\n");
	}

}

void raid5_reParity(int start_row, int end_row) {
	
	int bytes_read;
	long bytes_written;
	
	char *chunkfile = NULL;
	int buffer;
	int parity_buffer;
	
	FILE** fp = malloc(sizeof(FILE*) * (DISK_TOTAL));
	
	int row_index = start_row;
	int column_index;
	int parity_at;
	int buffer_at;
	
	printf("Calculating parity for affected rows (%d-%d)...\n", start_row, end_row);
	printf("Progress: 0%%...");
	
	for (row_index = start_row; row_index <= end_row; row_index++) {
	
		parity_at = DISK_TOTAL - 1 - row_index % DISK_TOTAL;
		
		// Open file handles for each column
		for (column_index = 0; column_index < DISK_TOTAL; column_index++) {
			chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[column_index]) + toDigit(row_index) + 3) * sizeof(char));
			sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[column_index], row_index);
			if (column_index == parity_at) {
				fp[column_index] = fopen(chunkfile, "wb");
			} else {
				fp[column_index] = fopen(chunkfile, "rb");
			}
		}

		// Read int by int
		long parity_index;
		for (parity_index = 0; parity_index < CHUNK_SIZE; parity_index++) {
			buffer = 0;
			parity_buffer = 0;
			for (column_index = 0; column_index < DISK_TOTAL; column_index++) {
				if (column_index != parity_at && fp[column_index] != NULL) {
					buffer = fgetc(fp[column_index]);
					if (buffer != EOF) parity_buffer = parity_buffer ^ buffer;
				}
			}
			// Write the calcuated parity back to parity file handle
			fputc(parity_buffer, fp[parity_at]);
		}
		
		printf("\rProgress: %d%%...", row_index * 100 / end_row);
		fflush(stdout);
		
	}
	
	free(fp);
	printf(" Done!\n\n");
	
}

void raid5_fsck() {

	// Get last used row & column from the database
	int lastIndex[2];
	db_getLastIndex(lastIndex);
	int start_row = lastIndex[1];
	int start_col = lastIndex[0];
	
	int bytes_read;
	long bytes_written;
	char *chunkfile = NULL;
	char *parityfile = NULL;
	int buffer;
	int missing_buffer;
	
	FILE** fp = malloc(sizeof(FILE*) * (DISK_TOTAL));
	
	int row_index;
	int column_index;
	int buffer_at;
	
	int missing_at;
	int parity_at;
	
	int eof = 0;
	
	printf("Verifying RAID 5 consistency...\n");
	
	// Massive loop
	for (row_index = 0; row_index <= start_row; row_index++) {
	
		missing_at = -1;
		
		// Open file handles for each column
		for (column_index = 0; column_index < DISK_TOTAL; column_index++) {
			
			
			chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[column_index]) + toDigit(row_index) + 3) * sizeof(char));
			sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[column_index], row_index);
			
			//printf("Chunk File is at %s\n.", chunkfile);
			
			fp[column_index] = fopen(chunkfile, "rb");
			if (
				fp[column_index] == NULL &&
				!(row_index == start_row && column_index >= start_col) // *Not* EOF
			) {
				if (missing_at == -1) {
					missing_at = column_index;
					fp[column_index] = fopen(chunkfile, "wb");
				} else if (missing_at != -2) {
					missing_at = -2; // As long as this is less than -1, it is unrecoverable
					printf("Unrecoverable row %d\n", row_index);
				}
			}

		}
		
		if (missing_at >= 0) {
			printf("Recovering row %d... ", row_index);
			
			// XOR valid columns to obtain missing column
			long buffer_index;
			for (buffer_index = 0; buffer_index < CHUNK_SIZE; buffer_index++) {
				buffer = 0;
				missing_buffer = 0;
				for (column_index = 0; column_index < DISK_TOTAL; column_index++) {
					if (
						column_index != missing_at &&
						!(row_index == start_row && column_index >= start_col) // Don't consider end of drive else we may segfault on a missing col
					) {
						buffer = fgetc(fp[column_index]);
						if (buffer != EOF) missing_buffer = missing_buffer ^ buffer;
					}
				}
				// Write the recovered column back to the file handle
				fputc(missing_buffer, fp[missing_at]);
			}
			printf("Recovered!\n");
		}
		
	}
	
	printf("Done!\n\n");
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
	
	if (!to_stdout) printf("Progress: 0%%...");
	int total_size_written = 0;
	int i;
	for (i = 0; i < file->total_chunks; i++) {
		//printf("\t[%d,%d] %d\n", file->chunks[i].row, file->chunks[i].col, file->chunks[i].order);//
		
		chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[file->chunks[i].col]) + toDigit(file->chunks[i].row) + 3) * sizeof(char));
		sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[file->chunks[i].col],file->chunks[i].row);
		
		FILE *fp = fopen(chunkfile, "rb");
		if (fp == NULL) { return; }

		long file_size = fs_getFileSize(chunkfile);

		bytes_written = 0;

		if (i != file->total_chunks-1) {
			while (bytes_written < file_size) {
				bytes_read = fread(buffer, 1, BUFFER_SIZE, fp);
				fwrite(buffer, 1, bytes_read, file_out);					 // write bytes from buffer to the current slice
				if (to_stdout) fflush(stdout);
				bytes_written += bytes_read;
			}
		} else {
			while (total_size_written+bytes_written < file->filesize) {
				int bytes_to_write = 0;
				bytes_read = fread(buffer, 1, BUFFER_SIZE, fp);
				if (total_size_written + bytes_written + bytes_read > file->filesize) 
					bytes_read = file->filesize - (total_size_written + bytes_written);
				fwrite(buffer, 1, bytes_read, file_out);					 // write bytes from buffer to the current slice
				if (to_stdout) fflush(stdout);
				bytes_written += bytes_read;
			}
		}
		
		total_size_written += bytes_written;
		fclose(fp);
		
		if (!to_stdout) printf("\rProgress: %d%%...", ((i+1)*100)/file->total_chunks);
		fflush(stdout);
	}

	if (!to_stdout) printf(" Done!\n\n");
	free(file);
	free(chunkfile);
	fclose(file_out);
	
	if (!to_stdout) printf("File transaction completed successfully.\n\n");

}

