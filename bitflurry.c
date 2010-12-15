#include "bitflurry.h"

int main (int argc, char *argv[]) {
	printf("bitflurry\n");
	printf("(c)2010 FANI FYP Team / Republic Polytechnic\n\n");
	if (argv[1] == NULL || strcmp(argv[1], "--help") == 0) {
			printf("Usage: bitflurry <command> <arguments>\n\n");
			printf("Commands available: \n");
			printf("\tinit # it automatically runs if needed now \n");
			printf("\tput <filename> <length>\n");
			printf("\tget <filename> <file1..file2..etc.>\n");
	} else {
		db_open();
		if (strcmp(argv[1], "init") == 0) {
			init(1);
		} else if (strcmp(argv[1], "sqlitetest") == 0) {
			int lastIndex[2];
			db_getLastIndex(lastIndex);
			printf("Last - Row: %d, Col: %d\n\n", lastIndex[1], lastIndex[0]);
		} else if (strcmp(argv[1], "put") == 0) {
			if (argc < 3) return 1;
			
			putFile(argv[2]);
		} else if (strcmp(argv[1], "get") == 0) {
			if (argc < 4) return 1;
			
			printf("Retrieving file: %s...\n", argv[2]);
			getFile(argv[2], argv[3]);
		}
		db_close();
	}
	
	return 0;
}

int init(int force) {
	// Do initialization checks
	printf("Running initializing checks;\n");
	int ret = 0;
	
	printf("Checking for database %s...", DB_DATABASE_NAME);
	if (access(DB_DATABASE_NAME, W_OK) == 0 && !force) {
		printf("pass!\n");
        ret = 1;
	} else {
		printf("doesn't exist!\n");
		printf("Creating database...");
		if (db_createTable() == SQLITE_OK) {
			printf("pass!\n");
			ret = 1;
		} else {
			printf("failed!\n");
			ret = 0;
		}
	}
	
	printf("Checking for output dir: %s...", DISK_PATH);
	DIR *pDir;
	if ((pDir = opendir(DISK_PATH))) {
		printf("pass!\n");
		closedir (pDir);
        ret = 1;
	} else {
		printf("doesn't exist!\n");
		ret = 0;
	}
	
	if (!ret) printf("Fatal Error: bitflurry will now quit.\n");
	else printf("ALL TEST PASSED!\n");
	printf("\n");
	
	return ret;
}

void putFile(char *filename) {
	if (!init(0)) return;
	fs_putFile(filename);
}

void getFile(char *filename, char *outfile) {
	if (!init(0)) return;
	fs_getFile(filename, outfile);
}
