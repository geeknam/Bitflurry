#include "bitflurry.h"

int main (int argc, char *argv[]) {

	// Quiet output so that we do not corrupt stdout get
	if (strcmp(argv[1], "get") == 0) {
		if (strcmp(argv[3], "-") == 0) {
			to_stdout = 1;
		}
	}
	
	if (!to_stdout) {
		printf("bitflurry\n");
		printf("(c)2010 FANI FYP Team / Republic Polytechnic\n\n");
	}
	if (argv[1] == NULL || strcmp(argv[1], "--help") == 0) {
			printf("Usage: bitflurry <command> <arguments>\n\n");
			printf("Commands available: \n");
			printf("\tinit # it automatically runs if needed now \n");
			printf("\tlist\n");
			printf("\tput <filename> <length>\n");
			printf("\tget <filename> <file1..file2..etc.>\n");
	} else {
		loadConfig();
		
		if (!to_stdout) {
			printf("Total Disks: %d\n", DISK_TOTAL);
			printf("Disks Mount Point: %s\n", DISK_PATH);
			printf("Disks Array: \n");
			int i;
			for (i = 0; i < DISK_TOTAL; i++) {
				printf("\t%s,\n", DISK_ARRAY[i]);
			}
			printf("RAID Level: %d\n", DISK_RAID);
			printf("\n");
		}
		
		db_open();
		if (strcmp(argv[1], "init") == 0) {
			init(1);
		} else if (strcmp(argv[1], "sqlitetest") == 0) {
			int lastIndex[2];
			db_getLastIndex(lastIndex);
			printf("Last - Row: %d, Col: %d\n\n", lastIndex[1], lastIndex[0]);
		} else if (strcmp(argv[1], "conftest") == 0) {
			if (argc < 3) return 1;
			
			char *read_val[50];
			
			cfg_getValueForKey(argv[2], read_val);
			printf("Value for %s: %s\n", argv[2], read_val);
			
			if (argc > 3) {
				printf("Setting value of %s to %s.\n", argv[2], argv[3]);
				cfg_setValueForKey(argv[2], argv[3]);
			}
		} else if (strcmp(argv[1], "list") == 0) {
			printf("Files in database: \n");
			bf_file** files = NULL;
			int max = 0;
			db_getFileList(files, &max);
			if (files == NULL) return;
			
			printf("Max: %d\n", max);
			
			int i;
			for (i = 0; i < max; i++) {
				printf("\t- %s\n", files[i]->filename);
				i++;
			}
			printf("\n");
		} else if (strcmp(argv[1], "put") == 0) {
			if (argc < 2) return 1;
			putFile(argv[2]);
		} else if (strcmp(argv[1], "get") == 0) {
			if (argc < 4) return 1;
			
			getFile(argv[2], argv[3]);
		}
		db_close();
	}
	
	return 0;
}

void loadConfig() {
	cfg_getValueForKey("DATABASE_NAME", &DB_DATABASE_NAME);
	
	char disk_total[50] = "0";
	cfg_getValueForKey("DISK_TOTAL", &disk_total);
	DISK_TOTAL = atoi(disk_total);
	
	cfg_getValueForKey("DISK_PATH", &DISK_PATH);
	
	char disk_raid[50] = "0";
	cfg_getValueForKey("DISK_RAID", &disk_raid);
	DISK_RAID = atoi(disk_raid);
	
	char disk_array[50] = "";
	cfg_getValueForKey("DISK_ARRAY", &disk_array);
	
	// 'explode' the config value and put it into array
	char *disk = NULL;
	DISK_ARRAY = malloc(DISK_TOTAL * sizeof(int));
	if (DISK_ARRAY == NULL) return;
	disk = strtok(disk_array, ",");
	int i = 0;
	while (disk != NULL) {
		char *_disk = malloc((strlen(disk)+1) * sizeof(char));
		strcpy(_disk, disk);
		DISK_ARRAY[i] = _disk;
		disk = strtok(NULL, ",");
		i++;
	}
	
	free(disk);
}

int init(int force) {
	// Do initialization checks
	if (!to_stdout) printf("Running initializing checks;\n");
	int ret = 0;
	
	if (!to_stdout) printf("Checking for database %s...", DB_DATABASE_NAME);
	if (access(DB_DATABASE_NAME, W_OK) == 0 && !force) {
		if (!to_stdout) printf("pass!\n");
        ret = 1;
	} else {
		if (!to_stdout) printf("doesn't exist!\n");
		if (!to_stdout) printf("Creating database...");
		if (db_createTable() == SQLITE_OK) {
			if (!to_stdout) printf("pass!\n");
			ret = 1;
		} else {
			if (!to_stdout) printf("failed!\n");
			ret = 0;
		}
	}
	
	if (!to_stdout) printf("Checking for output dirs in %s...", DISK_PATH);
	DIR *pDir;
	if ((pDir = opendir(DISK_PATH))) {
		if (!to_stdout) printf("\n");
		closedir (pDir);		
        ret = 1;

		int i;
		for (i = 0; i < DISK_TOTAL; i++) {
			if (!to_stdout) printf("\t...%s...", DISK_ARRAY[i]);
			char *subdir = malloc((strlen(DISK_PATH) + strlen(DISK_ARRAY[i]) + 2) * sizeof(char));
			sprintf(subdir, "%s/%s", DISK_PATH, DISK_ARRAY[i]);
			if (pDir = opendir(subdir)) {
				if (!to_stdout) printf("pass!\n");
				closedir (pDir);
				free(subdir);
			} else {
				if (!to_stdout) printf("failed!\n");
				ret = 0;
				free(subdir);
				break;
			}
		}
	} else {
		if (!to_stdout) printf("doesn't exist!\n");
		ret = 0;
	}
	
	if (!ret) printf("Fatal Error: bitflurry will now quit.\n");
	else if (!to_stdout) printf("ALL OK!\n");
	if (!to_stdout) printf("\n");
	
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
