#include "database.h"

int db_open() {
	int retval = 0;

	retval = sqlite3_open(DB_DATABASE_NAME, &db_handle);
    // If connection failed, handle returns NULL
    if (retval)
    {
        printf("Error: Database connection failed.\n");
        return -1;
    }

	db_opened = 1;
	return retval;
}

void db_close() {
	sqlite3_close(db_handle);
	db_opened = 0;
}

int db_createTable() {
	int retval = 0;
	char *zErrMsg = NULL;
	
    // If db is not opened failed, handle returns NULL
    if (db_opened == 0)
    {
        printf("DB Error: Database connection not opened.\n");
        return -1;
    }

	// Create the SQL query for creating a table
    char create_table[] = "\
		CREATE TABLE IF NOT EXISTS `files` (\
		  `_id` INTEGER PRIMARY KEY,\
		  `filename` varchar(255) NOT NULL\
		);\
		CREATE TABLE IF NOT EXISTS `chunks` (\
		  `col` int(11) NOT NULL,\
		  `row` int(11) NOT NULL,\
		  `file_id` int(11) NOT NULL,\
		  `order` int(11) NOT NULL default '0',\
		  PRIMARY KEY  (`col`,`row`)\
		)";
    
    // Execute the query for creating the table
    retval = sqlite3_exec(db_handle, create_table, 0, 0, &zErrMsg);
	if (retval != SQLITE_OK) {
		printf("DB Error: Failed to create table. SQL Error: %s\n", zErrMsg);
		free(zErrMsg);
	}

	return retval;
}

int db_insertFile(char *filename) {
	int retval = 0;
	char *zErrMsg = NULL;
	
	char *insert_file;
	insert_file = (char *) malloc((41 + strlen(filename) + 1) * sizeof(char));
	if (insert_file == NULL) return -1;
	sprintf(insert_file, "INSERT INTO files (filename) VALUES (\"%s\");", filename);
	
	//printf("Query: %s\n", insert_file);
	retval = sqlite3_exec(db_handle, insert_file, 0, 0, &zErrMsg);
	if (retval != SQLITE_OK) {
		printf("DB Error: Failed to insert data to table. SQL Error: %s\n", zErrMsg);
		free(zErrMsg);
		return 0;
	}
	
	retval = sqlite3_last_insert_rowid(db_handle);
	
	//free(insert_file);
	return retval;
}

int db_insertChunk(int file_id, int col, int row, int order) {
	int retval = 0;
	char *zErrMsg = NULL;
	
	char *insert_chunk = NULL;
	insert_chunk = malloc((32 + toDigit(file_id) + toDigit(col) + toDigit(row) + toDigit(order) + 1) * sizeof(char));
	if (insert_chunk == NULL) return -1;
	sprintf(insert_chunk, "INSERT INTO chunks VALUES (%d, %d, %d, %d);", col, row, file_id, order);
	
	//printf("Query: %s\n", insert_chunk);
	retval = sqlite3_exec(db_handle, insert_chunk, 0, 0, &zErrMsg);
	if (retval != SQLITE_OK) {
		printf("DB Error: Failed to insert data to table. SQL Error: %s\n", zErrMsg);
		free(zErrMsg);
		return retval;
	}
	
	free(insert_chunk);
	return retval;
}

int db_getLastIndex(int index[]) {
	int retval = 0;
	
	// default values to return
	index[0] = -1;
	index[1] = 0;
	
	// If db is not opened failed, handle returns NULL
    if (db_opened == 0)
    {
        printf("DB Error: Database connection not opened.\n");
        return -1;
    }
	
	// Self explanatory codes from here on...
	retval = sqlite3_prepare_v2(db_handle, "SELECT * FROM chunks ORDER BY row DESC, col DESC LIMIT 1", -1, &db_stmt, 0);
	if (retval)
    {
        printf("DB: Unknown error while retrieving data.\n");
        return -1;
    }

	// Read the number of rows fetched
    int cols = sqlite3_column_count(db_stmt);

    while (1)
    {
        // fetch a row's status
        retval = sqlite3_step(db_stmt);

        if (retval == SQLITE_ROW)
        {
            // SQLITE_ROW means fetched a row

            int col;
			for(col = 0; col < cols; col++)
            {
				const char *col_name = sqlite3_column_name(db_stmt, col);
                int val = sqlite3_column_int(db_stmt, col);
                //printf("%s = %d\t", sqlite3_column_name(db_stmt, col), val);

				// Assign the corresponding col & row values to the array
				if (strcmp(col_name, "col") == 0) {
					index[0] = val;
				} else if (strcmp(col_name, "row") == 0) {
					index[1] = val;
				}
            }
			//printf("\n");
        }
        else if(retval == SQLITE_DONE)
        {
            // All rows finished
            //printf("DB: All rows fetched.\n");
            break;
        }
        else
        {
            // Some error encountered
            printf("DB: Unknown error while retrieving data.\n");
            return retval;
        }
    }
	
	return retval;
}

bf_file* db_getFile(char *filename) {
	int retval = 0;
	
	int total_chunks = 0;
	
	// If db is not opened failed, handle returns NULL
    if (db_opened == 0)
    {
        printf("DB Error: Database connection not opened.\n");
        return NULL;
    }
	
	// Construct our query
	//		- Count the number of expected results to create our array
	char *select_query = NULL;
	select_query = realloc(select_query, (strlen("SELECT COUNT(*) FROM chunks WHERE file_id IN (SELECT _id FROM files WHERE filename = \"\") ORDER BY `order` ASC") + strlen(filename) + 1) * sizeof(char));
	if (select_query == NULL) return NULL;
	sprintf(select_query, "SELECT COUNT(*) FROM chunks WHERE file_id IN (SELECT _id FROM files WHERE filename = \"%s\") ORDER BY `order` ASC", filename);
	
	//printf("Query 1: %s\n", select_query);
	
	// Self explanatory codes from here on...
	retval = sqlite3_prepare_v2(db_handle, select_query, -1, &db_stmt, 0);
	if (retval)
    {
        printf("DB: Unknown error while retrieving data.\n");
        return NULL;
    }

	// Read the number of cols fetched
	int cols = sqlite3_column_count(db_stmt);
	
	while (1)
    {
        // fetch a row's status
        retval = sqlite3_step(db_stmt);

        if (retval == SQLITE_ROW)
        {
            // SQLITE_ROW means fetched a row
			
            int col;
			for (col = 0; col < cols; col++)
            {
				const char *col_name = sqlite3_column_name(db_stmt, col);
                int val = sqlite3_column_int(db_stmt, col);
                //printf("%s = %d\t", sqlite3_column_name(db_stmt, col), val);
				
				total_chunks = val;
            }
			//printf("\n");
        } else if(retval == SQLITE_DONE) { break; }
        else {
            // Some error encountered
            printf("DB: Unknown error while retrieving data.\n");
			return NULL;
        }
    }

	if (total_chunks <= 0) return NULL;

	// Second Query
	//		Get our real results
	select_query = realloc(select_query, (strlen("SELECT * FROM chunks WHERE file_id IN (SELECT _id FROM files WHERE filename = \"\") ORDER BY `order` ASC") + strlen(filename) + 1) * sizeof(char));
	if (select_query == NULL) return NULL;
	sprintf(select_query, "SELECT * FROM chunks WHERE file_id IN (SELECT _id FROM files WHERE filename = \"%s\") ORDER BY `order` ASC", filename);
	
	//printf("Query 2: %s\n", select_query);
	
	retval = sqlite3_prepare_v2(db_handle, select_query, -1, &db_stmt, 0);
	if (retval)
    {
        printf("DB: Unknown error while retrieving data.\n");
        return NULL;
    }

	// Read the number of cols fetched
    cols = sqlite3_column_count(db_stmt);
	//printf("%d rows returned\n", total_chunks);
	
	// the bf_file to return
	bf_file * file = malloc(sizeof(bf_file));
	if (file == NULL) return NULL;
	strcpy(file->filename, filename);
	file->total_chunks = total_chunks;
	
	// the power(horror) of dynamic array
	bf_chunk *chunks = NULL;
	chunks = malloc(sizeof(bf_chunk) * total_chunks);
	if (chunks == NULL) return NULL;
	
	int row = 0;

    while (1)
    {
        // fetch a row's status
        retval = sqlite3_step(db_stmt);
		
        if (retval == SQLITE_ROW)
        {
            // SQLITE_ROW means fetched a row
			
            int col;
			for (col = 0; col < cols; col++)
            {
				const char *col_name = sqlite3_column_name(db_stmt, col);
                int val = sqlite3_column_int(db_stmt, col);
                //printf("%s = %d\t", sqlite3_column_name(db_stmt, col), val);

				// Assign the corresponding col & row values to the array
				if (strcmp(col_name, "col") == 0) {
					chunks[row].col = val;
				} else if (strcmp(col_name, "row") == 0) {
					chunks[row].row = val;
				} else if (strcmp(col_name, "file_id") == 0) {
					file->_id = val;
				} else if (strcmp(col_name, "order") == 0) {
					chunks[row].order = val;
				}
            }
			//printf("\n");
			row++;
        }
        else if(retval == SQLITE_DONE)
        {
            // All rows finished
            //printf("DB: All rows fetched.\n");
			file->chunks = chunks;
            break;
        }
        else
        {
            // Some error encountered
            printf("DB: Unknown error while retrieving data.\n");
			return NULL;
        }
    }
	
	//free(select_query);
	return file;
}

void db_getFileList(bf_file** files, int* total) {
	int retval = 0;
	
	// If db is not opened failed, handle returns NULL
    if (db_opened == 0)
    {
        printf("DB Error: Database connection not opened.\n");
        return;
    }

	// Construct our query
	//		- Count the number of expected results to create our array
	// Self explanatory codes from here on...
	retval = sqlite3_prepare_v2(db_handle, "SELECT COUNT(*) FROM files", -1, &db_stmt, 0);
	if (retval)
	{
	    printf("DB: Unknown error while retrieving data.\n");
	    return;
	}

	// Read the number of rows fetched
	int cols = sqlite3_column_count(db_stmt);

	while (1)
	{
	    // fetch a row's status
	    retval = sqlite3_step(db_stmt);

	    if (retval == SQLITE_ROW)
	    {
	        // SQLITE_ROW means fetched a row

	        int col;
			for(col = 0; col < cols; col++)
	        {
				const char *col_name = sqlite3_column_name(db_stmt, col);
	            int val = sqlite3_column_int(db_stmt, col);
	            //printf("%s = %d\t", sqlite3_column_name(db_stmt, col), val);

				*total = val;
	        }
			//printf("\n");
	    }
	    else if(retval == SQLITE_DONE)
	    {
	        // All rows finished
	        //printf("DB: All rows fetched.\n");
	        break;
	    }
	    else
	    {
	        // Some error encountered
	        printf("DB: Unknown error while retrieving data.\n");
	        return;
	    }
	}
	
	// Second query
	//		- Get the real data
	// Self explanatory codes from here on...
	retval = sqlite3_prepare_v2(db_handle, "SELECT * FROM files", -1, &db_stmt, 0);
	if (retval)
	{
	    printf("DB: Unknown error while retrieving data.\n");
	    return;
	}

	// Read the number of rows fetched
	cols = sqlite3_column_count(db_stmt);
	
	bf_file** file_list = malloc(*total * sizeof(int));
	int i = 0;

	while (1)
	{
	    // fetch a row's status
	    retval = sqlite3_step(db_stmt);

	    if (retval == SQLITE_ROW)
	    {
	        // SQLITE_ROW means fetched a row

			bf_file *file = malloc(sizeof(bf_file));
			
	        int col;
			for(col = 0; col < cols; col++)
	        {
				const char *col_name = sqlite3_column_name(db_stmt, col);
	            
	            printf("%s = %s\t", sqlite3_column_name(db_stmt, col), sqlite3_column_text(db_stmt, col));

				if (strcmp(col_name, "_id") == 0) {
					file->_id = sqlite3_column_int(db_stmt, col);
				} else if (strcmp(col_name, "filename") == 0) {
					strcpy(file->filename, sqlite3_column_text(db_stmt, col));
				}
	        }
	
			file_list[i] = file;
			i++;
			printf("\n");
	    }
	    else if(retval == SQLITE_DONE)
	    {
	        // All rows finished
	        //printf("DB: All rows fetched.\n");
			files = file_list;
	        break;
	    }
	    else
	    {
	        // Some error encountered
	        printf("DB: Unknown error while retrieving data.\n");
	        return;
	    }
	}
	
	return;
}

int db_isFileExist(char *filename) {
	int retval = 0;
	int existence = 0;
	
	// If db is not opened failed, handle returns NULL
    if (db_opened == 0)
    {
        printf("DB Error: Database connection not opened.\n");
        return -1;
    }

	// Construct our query
	//		- Count the number of expected results to create our array
	char *select_query = NULL;
	select_query = realloc(select_query, (strlen("SELECT COUNT(*) FROM files WHERE filename = \"\"") + strlen(filename) + 1) * sizeof(char));
	if (select_query == NULL) return -1;
	sprintf(select_query, "SELECT COUNT(*) FROM files WHERE filename = \"%s\"", filename);

	// Self explanatory codes from here on...
	retval = sqlite3_prepare_v2(db_handle, select_query, -1, &db_stmt, 0);
	if (retval)
	{
	    printf("DB: Unknown error while retrieving data.\n");
	    return -1;
	}
	
	free(select_query);

	// Read the number of rows fetched
	int cols = sqlite3_column_count(db_stmt);

	while (1)
	{
	    // fetch a row's status
	    retval = sqlite3_step(db_stmt);

	    if (retval == SQLITE_ROW)
	    {
	        // SQLITE_ROW means fetched a row

	        int col;
			for(col = 0; col < cols; col++)
	        {
				const char *col_name = sqlite3_column_name(db_stmt, col);
	            int val = sqlite3_column_int(db_stmt, col);
	            //printf("%s = %d\t", sqlite3_column_name(db_stmt, col), val);

				existence = val;
	        }
			//printf("\n");
	    }
	    else if(retval == SQLITE_DONE)
	    {
	        // All rows finished
	        //printf("DB: All rows fetched.\n");
	        break;
	    }
	    else
	    {
	        // Some error encountered
	        printf("DB: Unknown error while retrieving data.\n");
	        return -1;
	    }
	}

	return existence;
}

int toDigit(int number) {
	char digits[5];
	sprintf(digits, "%d", number);
	
	return strlen(digits);
}
