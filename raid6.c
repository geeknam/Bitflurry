#include "raid6.h"

void raid6_putFile(char *filename) {
	long file_size;           			//total size of the file
	long slice_size = CHUNK_SIZE;    	//size of each slice: 4MB
	long last_slice_size;				//size of the last slice
	long cur_slice_size;				//determine whether the size is 4MB or the remainder (last_slice_size)
	long num_slices;	      			//number of slices
	int slice_index;		  			//used in the loop
	int last_slice_index;

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

	file_size = fs_getFileSize(filename);						// get a size of the file
	num_slices = file_size/slice_size;							// number of slices
	last_slice_index = num_slices + 1;							// index of the last slice
	last_slice_size = file_size - (num_slices*slice_size);		// size of the last slice

	fp_in = fopen(filename, "rb");								// open original file

	// Make sure there is no duplicate filenames in the database
	while (db_isFileExist(filename)) {
		char *filename_new = malloc((strlen(filename) + 5) * sizeof(char));
		sprintf(filename_new, "%s.new", filename);
		
		filename = filename_new;
	}
	
	int id = 0;
	id = db_insertFile(filename);
	printf("Putting file %s...\n", filename);
	//printf("Using file_id: %d\n", id);
	
	int lastIndex[2];
	int start_row = -1;
	int end_row = -1;
	db_getLastIndex(lastIndex);
	//printf("Last - Row: %d, Col: %d\n\n", lastIndex[1], lastIndex[0]);

	printf("Progress: 0...");
	for (slice_index = 0; slice_index <= num_slices; slice_index++) {
		lastIndex[0]++;
		if (lastIndex[0] >= DISK_TOTAL-2) { lastIndex[1]++; lastIndex[0] = 0; }
		if (start_row < 0) start_row = lastIndex[1];
		//printf("Writing on [%d, %d]\n", lastIndex[1], lastIndex[0]);
		
		// allocate memory for the name of the output files
		int name_mem = (strlen(DISK_PATH) + strlen(DISK_ARRAY[lastIndex[0]]) + toDigit(lastIndex[1]) + 5) * sizeof(char);
		file_out = (char *) realloc(file_out, name_mem);    //sizeof(char) = 1 byte

		snprintf (file_out, name_mem, "%s/%s/%d", DISK_PATH, DISK_ARRAY[lastIndex[0]], lastIndex[1]);  //concatenate names for the new output: movie.mp4.1 , movie.mp4.2, ...
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
		
		if (db_insertChunk_cacheStatement(&stmt, id, lastIndex[0], lastIndex[1], slice_index) < 1) break;
		//printf("Inserted for order: %d at (%d, %d)\n", slice_index, lastIndex[1], lastIndex[0]);
		printf("%d...", (int) (((slice_index+1)*100)/(num_slices+1)));
		fflush(stdout);
	}
	
	if (db_insertChunk_cacheCommit(stmt) != SQLITE_OK) {
		printf("Fatal error: Unable to commit chunk to database.");
	}
	
	end_row = lastIndex[1];
	
	raid6_reParity(start_row, end_row);
	printf("done!\n");
	printf("\nFile transaction completed successfully.\n");
	//printf("\nDB Insert Completed.\n");
	
	fclose(fp_in);			// close the original file	
	free(file_out);			// free the memory
}

void raid6_reParity(int start_row, int end_row) {
	int row_adjustment = 0;
	//printf("Start Row: %d\n", start_row);
	if (start_row % (DISK_TOTAL-2) > 0) row_adjustment -= (start_row % (DISK_TOTAL-2));

	//printf("Start Row after Adjustments: %d\n", (start_row + row_adjustment));
	
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
	unsigned int parity_row_at;
	int buffer_at;
	
	printf("\nCalculating normal parity...\n");
	for (row_index = start_row; row_index <= end_row; row_index++) {
		parity_at = DISK_TOTAL - 2;
		printf("Recalculating parity for row %d at %d...\n", row_index, parity_at);
		
		// Open file handles for each column
		for (column_index = 0; column_index < DISK_TOTAL - 1; column_index++) {
			
			chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[column_index]) + toDigit(row_index) + 3) * sizeof(char));
			sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[column_index], row_index);
			
			//printf("Chunk File is at %s\n.", chunkfile);
			
			if (column_index == parity_at) {
				fp[column_index] = fopen(chunkfile, "wb");
			} else {
				fp[column_index] = fopen(chunkfile, "rb");
			}
		}

		// Read buffer by buffer
		long parity_index;
		for (parity_index = 0; parity_index < CHUNK_SIZE; parity_index += BUFFER_SIZE) {
			memset(parity_buffer, 0, sizeof(parity_buffer));
			for (column_index = 0; column_index < DISK_TOTAL - 1; column_index++) {
				if (column_index != parity_at) {
					printf("%d\t", column_index);
					if (fp[column_index] == NULL) continue;
					fread(buffer, 1, BUFFER_SIZE, fp[column_index]);
					// Loop over buffer and perform byte XOR
					for (buffer_at = 0; buffer_at < BUFFER_SIZE; buffer_at++) {
						parity_buffer[buffer_at] = parity_buffer[buffer_at] ^ buffer[buffer_at];
					}
				} else {
					printf("P\t");
				}
			}
			// Write the calcuated parity back to parity file handle
			fwrite(parity_buffer, 1, BUFFER_SIZE, fp[parity_at]);
			printf("Byte %ld OK!\n", parity_index);
		}
	}
	
	printf("Calculating double parity...\n");
	for (row_index = start_row + row_adjustment; row_index <= end_row; row_index=row_index+(DISK_TOTAL-2)) {
		parity_at = DISK_TOTAL - 1;
		printf("Recalculating parity for row %d at %d...\n", row_index, parity_at);
		
		int i;
		int skipped = 0;
		int parity_row_offset = ((row_index/(DISK_TOTAL-2))%(DISK_TOTAL-2)) - 1;
		int data_col_start = 1;
		for (i = 0; i < (DISK_TOTAL-1); i++) {
			data_col_start--;
			if (data_col_start < 0) data_col_start = (DISK_TOTAL-2);
			// Open file handles for diagonal blocks
			printf("Calculating DP%d with set: ", row_index+i-skipped);
			int j = data_col_start;
			int j_count = -1;
			int skip = 1;
			while (1) {
				j_count++;
				if (j_count >= (DISK_TOTAL-2)) break;
				
				if (j >= (DISK_TOTAL-2)) j = 0;
				int data_row = row_index + j;
				int data_col = (i + j) % (DISK_TOTAL-1);
				
				j++;
				
				int white_block = ((DISK_TOTAL-2) + data_row) % (DISK_TOTAL-1);
				if (white_block == data_col) continue;
				else skip = 0;
			
				chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[data_col]) + toDigit(data_row) + 3) * sizeof(char));
				sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[data_col], data_row);
			
				fp[data_col] = fopen(chunkfile, "rb");
				printf("(%d, %d), ", data_row, data_col);
			}
			
			if (skip) { if (i>0) { skipped++; } printf("SKIPPED!\n"); continue; }
			printf("\n");
		
			// Parity Row?
			parity_row_offset++;
			if (parity_row_offset >= DISK_TOTAL-2) parity_row_offset = 0;
			if (row_index > 0) {
				parity_row_at = row_index + parity_row_offset;
			} else parity_row_at = i;
			printf("\tParity Row: %d\n", parity_row_at);
		
			// DP
			chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[parity_at]) + toDigit(parity_row_at) + 3) * sizeof(char));
			sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[parity_at], parity_row_at);
			fp[parity_at] = fopen(chunkfile, "wb");

			// Read buffer by buffer
			long parity_index;
			for (parity_index = 0; parity_index < CHUNK_SIZE; parity_index += BUFFER_SIZE) {
				memset(parity_buffer, 0, sizeof(parity_buffer));
				for (column_index = 0; column_index < DISK_TOTAL-1; column_index++) {
					if (column_index != parity_at) {
						//printf("%d\t", column_index);
						if (fp[column_index] == NULL) continue;
						fread(buffer, 1, BUFFER_SIZE, fp[column_index]);
						// Loop over buffer and perform byte XOR
						for (buffer_at = 0; buffer_at < BUFFER_SIZE; buffer_at++) {
							parity_buffer[buffer_at] = parity_buffer[buffer_at] ^ buffer[buffer_at];
						}
					} else {
						//printf("P\t");
					}
				}
				// Write the calcuated parity back to parity file handle
				fwrite(parity_buffer, 1, BUFFER_SIZE, fp[parity_at]);
				//printf("Byte %ld OK!\n", parity_index);
			}
		}
	}
	
	int k = 0;
	while (1) {
		if (k == DISK_TOTAL-1 || fp[k] == NULL) break;
		fclose(fp[k]);
		k++;
	}
	//free(fp);
}

/*void raid6_fsck() {

	// Get last used row & column from the database
	int lastIndex[2];
	db_getLastIndex(lastIndex);
	int start_row = lastIndex[1];
	int start_col = lastIndex[0];
	
	int bytes_read;
	long bytes_written;
	char *chunkfile = NULL;
	char *parityfile = NULL;
	char buffer[BUFFER_SIZE];
	char missing_buffer[BUFFER_SIZE];
	
	FILE** fp = malloc(sizeof(FILE*) * (DISK_TOTAL));
	
	int row_index;
	int column_index;
	int buffer_at;
	
	int missing_at;
	
	int eof = 0;
	
	// Massive loop
	for (row_index = 0; row_index <= start_row; row_index++) {
	
		missing_at = -1;
		printf("Verifying row %d... ", row_index);
		
		// Open file handles for each column
		for (column_index = 0; column_index < DISK_TOTAL-1; column_index++) {
			
			if (row_index == start_row && column_index >= start_col) {
				eof = 1;
			}
			
			chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[column_index]) + toDigit(row_index) + 3) * sizeof(char));
			sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[column_index], row_index);
			
			//printf("Chunk File is at %s\n.", chunkfile);
			
			fp[column_index] = fopen(chunkfile, "rb");
			if (fp[column_index] == NULL && eof == 0) {
				if (missing_at == -1) {
					missing_at = column_index;
					fp[column_index] = fopen(chunkfile, "wb");
				} else {
					missing_at = -2; // -2 = use DP to recover
				}
			}

		}
		
		if (missing_at == -1) {
			printf("OK!\n");
		} else if (missing_at == -2) {
			printf("2 missing: Proceed to recovery through RAID-DP.\n");
			raid6_fsck_dp(0, 0);
		} else {
			printf("Recovering...\n");
			
			// XOR missing file
			long buffer_index;
			for (buffer_index = 0; buffer_index < CHUNK_SIZE; buffer_index += BUFFER_SIZE) {
				memset(missing_buffer, 0, sizeof(missing_buffer));
				for (column_index = 0; column_index < DISK_TOTAL-1; column_index++) {
				
					if (row_index == start_row && column_index >= start_col) {
						printf(".\t");
						continue;
					}
				
					if (column_index != missing_at) {
						//if (column_index == parity_at) {
						//	printf("P\t");
						//} else {
							printf("%d\t", column_index);
						//}
						fread(buffer, 1, BUFFER_SIZE, fp[column_index]);
						// Loop over buffer and perform byte XOR
						for (buffer_at = 0; buffer_at < BUFFER_SIZE; buffer_at++) {
							missing_buffer[buffer_at] = missing_buffer[buffer_at] ^ buffer[buffer_at];
						}
					} else {
						printf("X\t");
					}
				}
				printf("Recovered bytes %ld\n", buffer_index);
			}
			
			// Write the calcuated parity back to parity file handle
			fwrite(missing_buffer, 1, BUFFER_SIZE, fp[missing_at]);
			printf("Recovered!\n");
		}
		
	}
	
	free(fp);
	
}

void raid6_fsck_dp(int col, int row) {
	
}*/

void raid6_getFile(bf_file* file, char *outfile) {

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
