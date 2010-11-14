#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1048576 // 1Mb 1048576

long getFileSize(char *filename);

main(int argc, char *argv[]){

	long file_size;           		//total size of the file
	long slice_size = 1048576*4;    //size of each slice: 4MB
	long last_slice_size;			//size of the last slice
	long cur_slice_size;			//determine whether the size is 4MB or the remainder (last_slice_size)
	long num_slices;	      		//number of slices
	int slice_index;		  		//used in the loop
	int last_slice_index;			
	
	char *file_in;                 
	char *file_out;
	FILE *fp_in;
	FILE *fp_out;
	
	char buffer[BUFFER_SIZE];
	int bytes_read;
	int bytes_to_write;
	long bytes_written;
	
	
	file_in = argv[1];											// first argument: file to slice
	file_size = getFileSize(file_in);							// get a size of the file
	num_slices = file_size/slice_size;							// number of slices
	last_slice_index = num_slices + 1;							// index of the last slice
	last_slice_size = file_size - (num_slices*slice_size);		// size of the last slice
	
	// open original file
	fp_in = fopen(file_in, "rb");
	
	// allocate memory for the name of the output files
	file_out = (char *) malloc((strlen(file_in) + num_slices) * sizeof(char));    //sizeof(char) = 1 byte
		
	for(slice_index = 1; slice_index <= num_slices+1; slice_index++){
		
		sprintf (file_out, "%s.%d", file_in, slice_index);     //concatenate names for the new output: movie.mp4.1 , movie.mp4.2, ...
		fp_out = fopen(file_out, "wb");						   //create and open a output file
		bytes_written = 0;
		
		//determine whether the size is 4MB or the remainder (last_slice_size)
		if(slice_index == last_slice_index){
			cur_slice_size = last_slice_size;   //remainder
		}
		else{
			cur_slice_size = slice_size;        //4MB
		}
		
		while(bytes_written < cur_slice_size){
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
