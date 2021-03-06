#ifndef DATABASE_H
#define DATABASE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

#include <sqlite3.h>

char DB_DATABASE_NAME[50];

// FARUQ: THIS LIBRARY IS NOT THREAD SAFE!!

// our sqlite3 handle    
sqlite3 *db_handle;

// stores prepared statements
sqlite3_stmt *db_stmt;

// variable to keep track of db status
int db_opened;

// structure for chunks
typedef struct {
	int col;
	int row;
	int order;
} bf_chunk;

// structure for files
typedef struct {
	int _id;
	char filename[255];
	int filesize;
	int total_chunks;
	bf_chunk *chunks; // dynamic array
} bf_file;

// functions for opening and closing db
// COMPULSORY before performing any of the functions below
int db_open();
void db_close();

// creates the tables if it doesn't exist
//		returns: sqlite3 return codes
int db_createTable();

// inserts a file to the table
// 		returns: last_insert_id
int db_insertFile(char *filename, int filesize);

// inserts a chunk to the table
//		returns: sqlite3 return codes
int db_insertChunk(int file_id, int col, int row, int order);

int db_insertChunk_cacheStatement(char **stmt, int file_id, int col, int row, int order);
int db_insertChunk_cacheCommit(char * stmt);

// gets the last index of the chunks table
// 		index[0] - col
//		index[1] - row
//		returns: sqlite3 return codes
int db_getLastIndex(int index[]);

// gets filename from database
//		return: bf_file
bf_file* db_getFile(char *filename);

// gets the current files from the database
//		return: bf_file[]
void db_getFileList(bf_file** files, int* total);

// check if file exist
//		return: -1, 0 or 1
int db_isFileExist(char *filename);
		
int toDigit(int number);
#endif
