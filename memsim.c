/*
command line arguments format:
memsim <tracefile> <nframes> <rdm|lru|fifo|vms> <debug|quiet>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//helper functions
void printoutput(int nframes, int events, int reads, int writes){
    printf("total memory frames: %d", nframes);
    printf("events in trace: %d", events);
    printf("total disk reads: %d", reads);
    printf("total disk writes: %d", writes);
}

//replacement algorithms
void rdm(){
    

}

void lru(){

}

void fifo(){

}

void vms(){

}



int main(int argc, char** argv){
    //errors for incorrect cm args
    if(argc > 5){
		printf("Too many command line arguments, ending program\n");
		return 0;
	}
	else if(argc < 5){
		printf("Too few command line arguments, ending program\n");
		return 0;
    }

    //get command line args
    char* tracefile = argv[1];
    int nframes = atoi(argv[2]);
    char* testType = argv[3];
    char* debugType = argv[4];
    
    //open trace file
    FILE* fptr;
    if((fptr = fopen(tracefile, "r")) == NULL)
        printf("error opening file");

    /*
    scan memory addresses + read or write
    unsigned addr; 
    char rw;
    fscanf(file,"%x %c",&addr,&rw);
    */

    /*
    format for quiet output:
    total memory frames: 12 
    events in trace: 1002050 
    total disk reads: 1751 
    total disk writes: 932 
    */

    return 0;
}