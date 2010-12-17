#include "config.h"

void cfg_getValueForKey(char *key, char *val) {
	// Assume our key+value is not more than 256 characters
	char lineStr[256];
	char aText[128], bText[128];

	FILE *conFile; 

	conFile = fopen(CONFIG_FILE, "r");       // open text file for read

	// check for input[1]
	if (key != NULL) {
		// check if text file is empty, return Error if true
		if (conFile == NULL) {
			printf("Error");
		}

		// gets text line by line and stores in string
		while (fgets(lineStr, 256, conFile) != NULL) {
			// skip lines starting with '#'
			if(lineStr[0] == '#') {
			    continue;
			}

			// gets the variable name(a) and its value(b)
			sscanf(lineStr, "%s %s", aText, bText);
    
			// compare input[1] with first string to find correct line
			if (strcmp(aText, key) == 0) {
				strcpy(val, bText);
				break;
			}
		}
	} else {
		return;
	}
	// close text file
	fclose(conFile);
	
	return;
}

int cfg_setValueForKey(char *key, char *val) {
	// Assume our key+value is not more than 100 characters
	char lineStr[256];
	char tmpStr[256];
	char aText[128], bText[128];

	FILE *tmpFile;
	FILE *conFile; 

	//tmpnam (buffer);                // temporary name for temporary file
	char *buffer = mktemp("bitflurryconf_tmpXXXXXX");
	tmpFile = fopen(buffer, "w+");       // open temporary file for write and read
	conFile = fopen(CONFIG_FILE, "r");       // open text file for read

	// check for input[1]
	if (key != NULL) {
		// check if text file is empty, return Error if true
		if (conFile == NULL) {
			printf("Error");
		}

		// gets text line by line and stores in string
		while (fgets(lineStr, 256, conFile) != NULL) {
			// skip lines starting with '#'
			if(lineStr[0] == '#') {
			    fputs(lineStr, tmpFile);
			    continue;
			}

			// gets the variable name(a) and its value(b)
			sscanf(lineStr, "%s %s", aText, bText);
    
			// compare input[1] with first string to find correct line
			if (strcmp(aText, key) == 0) {
				//  check for input[2]    
				if (val != NULL) {
					// replacing string value(b) with new input[2]
					//printf("Current Value -> %s = %s\n", aText, bText);
					sprintf(tmpStr, "%s %s\n", aText, val);
					fputs(tmpStr, tmpFile);
					//printf("New Value -> %s = %s\n", aText, val);
				} else {
					fputs(lineStr, tmpFile);
				}
			} else {  
				fputs(lineStr, tmpFile);
			}
		}
		// close text file
		fclose(conFile);

		// check if temporary file is empty, return Error if true
		if (tmpFile == NULL) {
			printf("Error");
		} else {
			conFile = fopen(CONFIG_FILE, "w");       // re-open text file for write
			rewind(tmpFile);

			//  gets text line by line and stores in string
			while (fgets(lineStr, 256, tmpFile) != NULL) {
				// writes from temporary file to text file
				fputs(lineStr, conFile);
			}

			fclose(conFile);            // close text file
		}
	} else {
		return 0;
	}
	
	fclose(tmpFile);            // close temporary file
	remove(buffer);             // remove temporary file
	
	return 1;
}
