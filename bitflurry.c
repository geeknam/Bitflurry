#include "bitflurry.h"

int main (int argc, char *argv[]) {
	if (argv[1] == NULL) {
			printf("bitflurry\n");
			printf("(c)2010 FANI FYP Team / Republic Polytechnic\n\n");
			printf("Usage: bitflurry <command> <arguments>\n\n");
			printf("Command available: \n");
			printf("\tinit # must be ran before using the other commands!\n");
			printf("\tput <filename> <length>\n");
			printf("\tget <filename> <file1..file2..etc.>\n");
	} else {
		db_open();
		if (strcmp(argv[1], "init") == 0) {
			db_createTable();
		} else if (strcmp(argv[1], "sqlitetest") == 0) {
			int lastIndex[2];
			db_getLastIndex(lastIndex);
			printf("Last - Row: %d, Col: %d\n\n", lastIndex[1], lastIndex[0]);
		} else if (strcmp(argv[1], "put") == 0) {
			if (argc < 4) return 1;
			
			int length = atoi(argv[3]);
			printf("Inserting File %s with %d chunks...\n", argv[2], length);
			
			int id = 0;
			id = db_insertFile(argv[2]);
			printf("Using file_id: %d\n", id);
			
			int lastIndex[2];
			db_getLastIndex(lastIndex);
			printf("Last - Row: %d, Col: %d\n\n", lastIndex[1], lastIndex[0]);
			
			int i;
			for (i = 0; i < length; i++) {
				lastIndex[0]++;
				if (lastIndex[0] >= DISK_TOTAL) { lastIndex[1]++; lastIndex[0] = 0; }
				
				if (db_insertChunk(id, lastIndex[0], lastIndex[1], i) != SQLITE_OK) break;
				
				printf("Inserted for order: %d at (%d, %d)\n", i, lastIndex[1], lastIndex[0]);
			}
			printf("\nDB Insert Completed.\n");
		} else if (strcmp(argv[1], "get") == 0) {
			if (argc < 3) return 1;
			
			printf("Retrieving file: %s...\n", argv[2]);
			bf_file * file = db_getFile(argv[2]);
			
			if (file != NULL) {
				printf("file_id: %d\n", file->_id);
				printf("filename: %s\n", file->filename);
				printf("total chunks: %d\n", file->total_chunks);
				printf("chunks:\n");
				int i;
				for (i = 0; i < file->total_chunks; i++) {
					printf("\t[%d,%d] %d\n", file->chunks[i].row, file->chunks[i].col, file->chunks[i].order);
				}
				
				free(file);
			} else {
				printf("File not found.\n");
			}
		}
		db_close();
	}
	
	return 0;
}

