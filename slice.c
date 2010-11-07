#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1048576 // 1Mb 1048576

long getFileSize(char *filename);

main(int argc, char *argv[]){

	long file_size;           //total size of the file
	long slice_size;          //size of each slice
	long num_slices = 4;	   //hardcoded number of slices
	int slice_index;			   //used in the loop
	
	char *file_in;                 
	char *file_out;
	FILE *fp_in;
	FILE *fp_out;
	
	char buffer[BUFFER_SIZE];
	int bytes_read;
	int bytes_to_write;
	long bytes_written;
	
	
	file_in = argv[1];						// first argument: file to slice
	file_size = getFileSize(file_in);		// get a size of the file
	slice_size = file_size/num_slices;		// the size of each slice
	
	// open original file
	fp_in = fopen(file_in, "rb");
	
	// allocate memory for the name of the output files
	file_out = (char *) malloc((strlen(file_in) + num_slices) * sizeof(char));    //sizeof(char) = 1 byte
	
	for(slice_index = 1; slice_index <= num_slices; slice_index++){
		
		sprintf (file_out, "%s.%d", file_in, slice_index);     //concatenate names for the new output: movie.mp4.1 , movie.mp4.2, ...
		fp_out = fopen(file_out, "wb");						   //create and open a output file
		
		bytes_written = 0;
		
		while(bytes_written < slice_size){
			bytes_to_write = BUFFER_SIZE;
			bytes_read = fread(buffer, 1, bytes_to_write, fp_in);    //read bytes from file to the buffer
			fwrite(buffer, 1, bytes_read, fp_out);					 //write bytes from buffer to the current slice
			bytes_written += bytes_to_write;
		}
		fclose(fp_out);		//close the output file
	}	
	fclose(fp_in);			//close the original file	
	free(file_out);			//free the memory
}

/* Get size in bytes of the specified file */
long getFileSize(char *filename){
    struct stat statBuffer;

    if (stat(filename, &statBuffer) != 0)
    {
        printf("\n\nERROR: Unable to obtain attributes of file %s.", filename);
        return -1;
    }
    return (long) statBuffer.st_size;
}
