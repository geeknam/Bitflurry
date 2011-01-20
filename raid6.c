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
	id = db_insertFile(filename, file_size);
	printf("Striping file %s:\n", filename);
	printf("\tProgress: 0...");
	//printf("Using file_id: %d\n", id);
	
	int lastIndex[2];
	int start_row = -1;
	int end_row = -1;
	db_getLastIndex(lastIndex);
	//printf("Last - Row: %d, Col: %d\n\n", lastIndex[1], lastIndex[0]);

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

		while (bytes_written < slice_size) {
			bytes_to_write = BUFFER_SIZE;
			bytes_read = fread(buffer, 1, bytes_to_write, fp_in);    // read bytes from file to the buffer
			if (bytes_read == 0) {
				// Padding to ensure chunk size is always = BUFFER_SIZE
				char pad_buffer[slice_size-bytes_written];
				bytes_read = sizeof(pad_buffer);
				bytes_to_write = bytes_read;
				memset(pad_buffer, '\0', bytes_to_write);
				fwrite(pad_buffer, 1, bytes_to_write, fp_out);					 // write bytes from buffer to the current slice
			} else {
				fwrite(buffer, 1, bytes_read, fp_out);					 // write bytes from buffer to the current slice
			}
			bytes_written += bytes_read;
		}
		fflush(fp_out);
		fclose(fp_out);		// close the output file
		
		if (db_insertChunk_cacheStatement(&stmt, id, lastIndex[0], lastIndex[1], slice_index) < 1) break;
		//printf("Inserted for order: %d at (%d, %d)\n", slice_index, lastIndex[1], lastIndex[0]);
		printf("\r\tProgress: %ld%%...", (slice_index + 1) * 100 / (num_slices + 1));
		fflush(stdout);
	}
	
	if (db_insertChunk_cacheCommit(stmt) != SQLITE_OK) {
		printf("Fatal error: Unable to commit chunk to database.");
	}
	
	end_row = lastIndex[1];
	
	printf("Done!\n");
	
	raid6_reParity(start_row, end_row);
	raid6_dp_reParity(start_row, end_row);
	
	printf("\nFile transaction completed successfully.\n");
	//printf("\nDB Insert Completed.\n");
	
	fclose(fp_in);			// close the original file	
	free(file_out);			// free the memory
}

/*void raid6_putFile(char *filename) {
	int lastIndex[2];
	db_getLastIndex(lastIndex);
	raid6_dp_reParity(0, lastIndex[1]);
	printf("done!\n");
	printf("\nFile transaction completed successfully.\n");
}*/

void raid6_reParity(int start_row, int end_row) {
	char *chunkfile = NULL;
	char *parityfile = NULL;
	int buffer;
	int parity_buffer;
	
	FILE* fp[DISK_TOTAL];
	
	int row_index;
	int column_index;
	int parity_at;
	unsigned int parity_row_at;
	
	printf("Calculating parity for affected rows (%d-%d):", start_row, end_row);
	printf("\n\tProgress: 0%%...");
	
	for (row_index = start_row; row_index <= end_row; row_index++) {
		parity_at = DISK_TOTAL - 2;
		printf("\r\tProgress: row %d (%d%%)...", row_index, row_index * 100 / end_row);
		fflush(stdout);
		
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
		for (parity_index = 0; parity_index < CHUNK_SIZE; parity_index += 1) {
			buffer = 0;
			parity_buffer = 0;
			for (column_index = 0; column_index < DISK_TOTAL - 1; column_index++) {
				if (column_index != parity_at) {
					if (fp[column_index] == NULL) continue;
					buffer = fgetc(fp[column_index]);
					if (buffer != EOF) parity_buffer = parity_buffer ^ buffer;
				}
			}
			// Write the calcuated parity back to parity file handle
			fputc(parity_buffer, fp[parity_at]);
		}
		fflush(fp[parity_at]);
	}
	
	// Close all opened handles first
	int k = 0;
	for (k = 0; k < DISK_TOTAL-1; k++) {
		if (fp[k] != NULL) fclose(fp[k]);
		fp[k] = NULL;
	}
	
	free(chunkfile);
	free(parityfile);
	
	printf("Done!\n");
}

void raid6_dp_reParity(int start_row, int end_row) {
	int row_adjustment = 0;
	//printf("Start Row: %d\n", start_row);
	if (start_row % (DISK_TOTAL-2) > 0) row_adjustment -= (start_row % (DISK_TOTAL-2));

	//printf("Start Row after Adjustments: %d\n", (start_row + row_adjustment));
	char *chunkfile = NULL;
	char *parityfile = NULL;
	int buffer;
	int parity_buffer;
	
	int row_index;
	int column_index;
	int parity_at;
	unsigned int parity_row_at;
	
	FILE* fp[DISK_TOTAL];
	
	printf("Calculating d-parity for affected rows (%d-%d):", start_row + row_adjustment, end_row);
	printf("\n\tProgress: 0%%...");
	
	for (row_index = start_row + row_adjustment; row_index <= end_row; row_index=row_index+(DISK_TOTAL-2)) {
		parity_at = DISK_TOTAL - 1;
		//printf("\r\tParity for rows %d-%d, ", row_index, row_index+DISK_TOTAL-3);
		
		int i;
		int skipped = 0;
		int parity_row_offset = ((row_index/(DISK_TOTAL-2))%(DISK_TOTAL-2)) - 1;
		int data_col_start = 1;
		for (i = 0; i < (DISK_TOTAL-1); i++) {
			data_col_start--;
			if (data_col_start < 0) data_col_start = (DISK_TOTAL-2);
			
			// Open file handles for diagonal blocks
			printf("\r\tParity for rows %d-%d, set %d (%d%%)...", row_index, row_index+DISK_TOTAL-3, row_index+i-skipped, (row_index+i-skipped-1) * 100 / end_row);
			fflush(stdout);
			int j = data_col_start;
			int j_count = -1;
			int skip = 1;
			int used[DISK_TOTAL-1];
			int k;
			for (k = 0; k < DISK_TOTAL-1; k++) {
				used[k] = 0;
			}
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
				if (fp[data_col]) {
					used[data_col] = 1;
					//printf("\tdata_col: %s [%d]\n", chunkfile, data_col);
					//printf("O");
				} else {
					//printf("M");
				}
				//printf("(%d, %d), ", data_row, data_col);
			}
			
			if (skip) { if (i>0) { skipped++; } /*printf("\r");*/ continue; }
			
			// Parity Row?
			parity_row_offset++;
			if (parity_row_offset >= DISK_TOTAL-2) parity_row_offset = 0;
			if (row_index > 0) {
				parity_row_at = row_index + parity_row_offset;
			} else parity_row_at = i;
		
			// DP
			chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[parity_at]) + toDigit(parity_row_at) + 3) * sizeof(char));
			sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[parity_at], parity_row_at);

			/*if (parity_row_at != 1) continue; else {
				//printf("PARITYPARITYPARITYPARITYPARITYPARITYPARITY\n");
			}*/
			
			fp[parity_at] = fopen(chunkfile, "wb");
			//printf("\tDP_col: %s [%d]\n\n", chunkfile, parity_at);
			//printf("DP(%d, %d)", parity_row_at, parity_at);

			// Read buffer by buffer
			//int print_once = 0;
			long parity_index;
			for (parity_index = 0; parity_index < CHUNK_SIZE; parity_index += 1) {
				buffer = 0;
				parity_buffer = 0;
				//if (!print_once) printf("\t");
				for (column_index = 0; column_index < DISK_TOTAL-1; column_index++) {
					//if (!print_once) printf("\tused: %d; ", used[column_index]);
					if (used[column_index] == 1) {
						//if (fp[column_index] == NULL) continue;
						//if (!print_once) printf("\t%d", column_index);
						buffer = fgetc(fp[column_index]);
						if (buffer != EOF) parity_buffer = parity_buffer ^ buffer;
					} else {
						//if (!print_once) printf("\tM");
					}
				}
				//if (!print_once) printf("\tDP");
				//print_once = 1;
				// Write the calcuated parity back to parity file handle
				fputc(parity_buffer, fp[parity_at]);
				//printf("Byte %ld OK!\n", parity_index);
			}
			//printf("\n");
			
			fflush(fp[parity_at]);
			// Close all opened handles first
			k = 0;
			for (k = 0; k < DISK_TOTAL; k++) {
				if (used[k] == 1) fclose(fp[k]);
				fp[k] = NULL;
			}
		}
	}
	
	printf("Done!\n");
	
	free(chunkfile);
	free(parityfile);
}

void raid6_fsck() {
	int recovered_sp = 0;
	int recovered_dp = 0;
	
	printf("Performing fsck:\n");
	printf("\tPlease wait...");
	while (recovered_sp < 1 || recovered_dp < 1) {
		recovered_dp = raid6_fsck_dp();
		recovered_sp = raid6_fsck_sp();
	}
	
	if (recovered_sp == 1 && recovered_dp == 1) printf("Array rebuild process completed successfully.\n");
	else printf("Array rebuild process has failed. Perhaps more than 2 disks are missing!\n");
}

int raid6_fsck_sp() {
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
	
	FILE* fp[DISK_TOTAL];
	
	int row_index;
	int column_index;
	int buffer_at;
	
	int num_missing;
	int missing_at_row[2];
	int missing_at_col[2];
	
	int eof = 0;
	
	int fully_recovered = 1;
	
	// Massive loop
	for (row_index = 0; row_index <= start_row; row_index++) {
		num_missing = 0;
		missing_at_row[0] = -1;
		missing_at_row[1] = -1;
		missing_at_col[0] = -1;
		missing_at_col[1] = -1;
		//printf("\r\tProgress: verifying row %d (%d%%)...", row_index, row_index * 100 / start_row);
		
		int used[DISK_TOTAL-1];
		int k;
		for (k = 0; k < DISK_TOTAL-1; k++) {
			used[k] = 0;
		}
		
		// Open file handles for each column
		for (column_index = 0; column_index < DISK_TOTAL-1; column_index++) {
			chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[column_index]) + toDigit(row_index) + 3) * sizeof(char));
			if (chunkfile == NULL) return;
			sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[column_index], row_index);
			
			//printf("Chunk File is at %s\n.", chunkfile);
			
			fp[column_index] = fopen(chunkfile, "rb");
			if (fp[column_index] == NULL) {
				if (row_index > lastIndex[1]
					|| (row_index == lastIndex[1] && column_index > lastIndex[0] && column_index != DISK_TOTAL-2)) {
						//printf("Invalid; (%d, %d) vs (%d, %d): ", row_index, column_index, lastIndex[1], lastIndex[0]);
						continue;
				} else {
					if (num_missing < 2) {
						missing_at_row[num_missing] = row_index;
						missing_at_col[num_missing] = column_index;
						num_missing++;
					}
				}
			} else {
				used[column_index] = 1;
			}
		}
		if (num_missing == 0) {
			//printf("OK!");
		} else if (num_missing == 2) {
			fully_recovered = 0;
			//printf("\r\tMarked (%d, %d), (%d, %d) for DP fsck\n", missing_at_row[0], missing_at_col[0], missing_at_row[1], missing_at_col[1]);
			//fflush(stdout);
		} else if (num_missing == 1){
			//printf("Recovering (%d, %d) with 1P... ", missing_at_row[0], missing_at_col[0]);
			printf("\r\t[SP] Recovering (%d, %d)...", missing_at_row[0], missing_at_col[0]);
			fflush(stdout);
			//printf("\tProgress: verifying row %d... (%d%%)", row_index, row_index * 100 / start_row);
			
			chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[missing_at_col[0]]) + toDigit(missing_at_row[0]) + 3) * sizeof(char));
			sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[missing_at_col[0]], missing_at_row[0]);
			fp[missing_at_col[0]] = fopen(chunkfile, "wb");
			
			// XOR missing file
			int print_once = 0;
			long buffer_index;
			//printf("\n");
			for (buffer_index = 0; buffer_index < CHUNK_SIZE; buffer_index += 1) {
				buffer = 0;
				missing_buffer = 0;
				for (column_index = 0; column_index < DISK_TOTAL-1; column_index++) {
					if (column_index != missing_at_col[0] && used[column_index] == 1) {
						//if (!print_once) printf("\t%d", column_index);
						buffer = fgetc(fp[column_index]);
						if (buffer != EOF) missing_buffer = missing_buffer ^ buffer;
					} else {
						//if (!print_once) printf("\tM");
					}
				}
				//if (!print_once) printf("\tP\n");
				print_once = 1;
				
				// Write the calcuated parity back to parity file handle
				fputc(missing_buffer, fp[missing_at_col[0]]);
			}
			fflush(fp[missing_at_col[0]]);
			printf("recovered.\n");
			fflush(stdout);
			//printf("\tProgress: verifying row %d... (%d%%)", row_index, row_index * 100 / start_row);
		} else {
			printf("\r\tUnrecoverable!\n", missing_at_row[0], missing_at_col[0]);
			fflush(stdout);
			break;
		}
		
		k = 0;
		for (k = 0; k < DISK_TOTAL; k++) {
			if (used[k] == 1) fclose(fp[k]);
			fp[k] = NULL;
		}
	}
	
	free(chunkfile);
	free(parityfile);
	
	if (fully_recovered == 0) {
		//printf("Continuing recovery...\n");
		return 0;
	} else {
		return 1;
	}
}

int raid6_fsck_dp() {
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
	
	FILE* fp[DISK_TOTAL];
	
	int row_index;
	int column_index;
	int buffer_at;
	int parity_at;
	unsigned int parity_row_at;
	
	int num_missing;
	int missing_at_row[2];
	int missing_at_col[2];
	
	int eof = 0;
	int fully_recovered = 1;
	
	for (row_index = 0; row_index <= start_row; row_index=row_index+(DISK_TOTAL-2)) {
		parity_at = DISK_TOTAL - 1;
		//printf("Verifying for rows %d-%d: \n", row_index, row_index+DISK_TOTAL-3);
		
		int i;
		int skipped = 0;
		int parity_row_offset = ((row_index/(DISK_TOTAL-2))%(DISK_TOTAL-2)) - 1;
		int data_col_start = 1;
		for (i = 0; i < (DISK_TOTAL-1); i++) {
			data_col_start--;
			if (data_col_start < 0) data_col_start = (DISK_TOTAL-2);
			
			num_missing = 0;
			missing_at_row[0] = -1;
			missing_at_row[1] = -1;
			missing_at_col[0] = -1;
			missing_at_col[1] = -1;
			
			// Open file handles for diagonal blocks
			//printf("\tSet %d: ", row_index+i-skipped);
			int j = data_col_start;
			int j_count = -1;
			int skip = 1;
			int used[DISK_TOTAL-1];
			int k;
			for (k = 0; k < DISK_TOTAL-1; k++) {
				used[k] = 0;
			}
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
				if (fp[data_col] == NULL) {
					if (data_row > lastIndex[1]
						|| (data_row == lastIndex[1] && data_col > lastIndex[0])) {
						//printf("[%d, %d], ", data_row, data_col);
					} else {
						//printf("X{%d, %d}, ", data_row, data_col);
						if (num_missing < 2) {
							missing_at_row[num_missing] = data_row;
							missing_at_col[num_missing] = data_col;
							num_missing++;
						}
					}
				} else {
					used[data_col] = 1;
					//printf("(%d, %d), ", data_row, data_col);
				}
			}
			
			if (skip) { if (i>0) { skipped++; } /*printf("\r");*/ continue; }
			
			// Parity Row?
			parity_row_offset++;
			if (parity_row_offset >= DISK_TOTAL-2) parity_row_offset = 0;
			if (row_index > 0) {
				parity_row_at = row_index + parity_row_offset;
			} else parity_row_at = i;
			
			// DP
			chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[parity_at]) + toDigit(parity_row_at) + 3) * sizeof(char));
			sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[parity_at], parity_row_at);
			fp[parity_at] = fopen(chunkfile, "rb");
			if (fp[parity_at] == NULL) {
				//printf("X{%d, %d}, ", parity_row_at, parity_at);
				if (num_missing < 2) {
					missing_at_row[num_missing] = parity_row_at;
					missing_at_col[num_missing] = parity_at;
					num_missing++;
				}
			} else {
				used[parity_at] = 1;
				//printf("(%d, %d), ", parity_row_at, parity_at);
			}
			
			if (num_missing == 0) {
				//printf("OK!");
			} else if (num_missing == 1) {
				printf("\r\t[DP] Recovering (%d, %d)...", missing_at_row[0], missing_at_col[0]);
				fflush(stdout);
				
				// Missing file
				chunkfile = (char *) realloc(chunkfile, (strlen(DISK_PATH) + strlen(DISK_ARRAY[missing_at_col[0]]) + toDigit(missing_at_row[0]) + 3) * sizeof(char));
				sprintf(chunkfile, "%s/%s/%d", DISK_PATH, DISK_ARRAY[missing_at_col[0]], missing_at_row[0]);
				fp[missing_at_col[0]] = fopen(chunkfile, "wb");
				
				// Read buffer by buffer
				//int print_once = 0;
				long parity_index;
				//printf("\n");
				for (parity_index = 0; parity_index < CHUNK_SIZE; parity_index += 1) {
					buffer = 0;
					missing_buffer = 0;
					for (column_index = 0; column_index < DISK_TOTAL; column_index++) {
						//if (!print_once) printf("\tused: %d; ", used[column_index]);
						if (used[column_index] == 1) {
							//if (fp[column_index] == NULL) continue;
							//if (!print_once) printf("\t%d", column_index);
							buffer = fgetc(fp[column_index]);
							if (buffer != EOF) missing_buffer = missing_buffer ^ buffer;
						} else {
							//if (!print_once) printf("\tM");
						}
					}
					//if (!print_once) printf("\tDP");
					//print_once = 1;
					// Write the calcuated parity back to parity file handle
					fputc(missing_buffer, fp[missing_at_col[0]]);
					//printf("Byte %ld OK!\n", parity_index);
				}
				fflush(fp[missing_at_col[0]]);
				printf("recovered.\n");
				fflush(stdout);
			} else {
				fully_recovered = 0;
				fflush(stdout);
				//printf("... skip first.\n");
			}
			
			// Close all opened handles first
			k = 0;
			for (k = 0; k < DISK_TOTAL; k++) {
				if (used[k] == 1) fclose(fp[k]);
				fp[k] = NULL;
			}
		}
	}
	
	free(chunkfile);
	free(parityfile);
	
	if (fully_recovered == 0) {
		//printf("Continuing recovery...\n");
		return 0;
	} else {
		//printf("Recovery completed!\n");
		return 1;
	}
}

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
	
	if (!to_stdout) printf("\tProgress: 0%%...");
	int total_size_written = 0;
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
		
		if (!to_stdout) printf("\r\tProgress: %d%%...", ((i+1)*100)/file->total_chunks);
		fflush(stdout);
	}

	if (!to_stdout) printf("Done!\n");
	free(file);
	free(chunkfile);
	fclose(file_out);
	
	if (!to_stdout) printf("\nFile transaction completed successfully.\n");

}
