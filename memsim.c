/*
command line arguments format:
memsim <tracefile> <nframes> <rdm|lru|fifo|vms> <debug|quiet>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//doubly linked list implementation - uses head and tail sentinel nodes
struct Node{
    unsigned addr;
    char rw;
    struct Node* next;
    struct Node* prev;
};

void enqueue(struct Node* tail, unsigned address, char readOrWrite){
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    //assign data members
    newNode->addr = address;
    newNode->rw = readOrWrite;
    //update pointers
    newNode->next = tail;
    newNode->prev = tail->prev;
    newNode->prev->next = newNode;
    tail->prev = newNode;
    
}

struct Node* deleteNode(struct Node* nodeToDelete){
    nodeToDelete->prev->next = nodeToDelete->next;
    nodeToDelete->next->prev = nodeToDelete->prev;
    return nodeToDelete;
}

void insertAfter(struct Node* previousNode, unsigned address, char readOrWrite){
    if (previousNode == NULL){
        printf("Error: previous node can't be null");
        return;
    }
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    //assign data members
    newNode->addr = address;
    newNode->rw = readOrWrite;
    //update pointers
    newNode->prev = previousNode;
    newNode->next = previousNode->next;
    previousNode->next = newNode;
    newNode->next->prev = newNode;


}

void push(struct Node* head, unsigned address, char readOrWrite){
    insertAfter(head, address, readOrWrite);
}

struct Node* pop(struct Node* head){
    return deleteNode(head->next);
}

struct Node* findNode(struct Node* node, unsigned elem){
    if(node->prev == NULL) //skip head node if we started there
        node = node->next;
    while(node->next != NULL){ //while not at the tail node
        if(node->addr == elem){
            return node;
        }
        node = node->next;
    }
    return NULL;
}

void printList(struct Node* node){
    while(node != NULL){
        printf("address: %x RW: %c\n", node->addr, node->next);
        node = node->next;
    }
}

//helper functions

//given the vpn, the array, and size of array it returns the index if found else -1
int find(unsigned elem, unsigned array[], int size){
    int i;
    for(i = 0; i < size; i++){
            if(array[i] == elem)
            return i;
        }
    return -1;
}

void printoutput(int nframes, int events, int reads, int writes){
    printf("total memory frames: %d\n", nframes);
    printf("events in trace: %d\n", events);
    printf("total disk reads: %d\n", reads);
    printf("total disk writes: %d\n", writes);
}

int random(int lower, int upper){
    srand(1);
    int num = (rand() % (upper - lower + 1)) + lower;
    return num;
}

//replacement algorithms
void rdm(FILE* tracefile, int nframes){
    unsigned* pageTable;
    char* dirtyBit;
    pageTable = (unsigned*)malloc(sizeof(unsigned)*nframes);
    dirtyBit = (char*)malloc(sizeof(char)*nframes);
    memset(dirtyBit, 'R',sizeof(char)*nframes);

    int capacity = 0; 
    unsigned addr;
    char rw;
    int events = 0;
    int diskReads = 0;
    int diskWrites = 0;

    while(fscanf(tracefile,"%x %c",&addr,&rw) != EOF){
        addr = addr >> 12; //addr is the entire virtual address, we just need the VPN from it so, first 20 bits as each page is size 2^12
        int i = find(addr, pageTable, nframes); //get index of the page in the page table if it exists
        if(i != -1){ //if address is in the page table already
            if(rw == 'W') 
                dirtyBit[i] = rw;
        }
        else if(capacity < nframes){ //adress isn't in the table, is it full?
            pageTable[capacity] = addr; //add address to the table
            diskReads++;
            capacity++;
            if(rw == 'W') 
                dirtyBit[i] = rw;
        }
        else{ //table is full, must make room
            i = random(0, nframes); //randomly select page to remove
            if(dirtyBit[i] == 'W') //if ejected page was dirty write it to disk
                diskWrites++;
            pageTable[i] = addr; //add new address to table
            dirtyBit[i] = rw;
            diskReads++;
        }
        events++;
    }
    printoutput(nframes, events, diskReads, diskWrites);
}

void lru(){

}

void fifo(FILE* tracefile, int nframes){
    //start linked list sentinels and set pointers
    //this list will act as the page table and queue
    struct Node* head;
    struct Node* tail;
    head->prev = NULL;
    head->next = tail;
    tail->prev = head;
    tail->next = NULL;

    int capacity = 0; 
    unsigned addr;
    char rw;
    int events = 0;
    int diskReads = 0;
    int diskWrites = 0;

    while(fscanf(tracefile,"%x %c",&addr,&rw) != EOF){
        addr = addr >> 12; //addr is the entire virtual address, we just need the VPN from it so, first 20 bits as each page is size 2^12
        struct Node* foundNode = findNode(head, addr); //find address in list
        if(foundNode != NULL){ //if address is in the page table already
            if(rw == 'W') 
                foundNode->rw = rw;
        }
        else if(capacity < nframes){ //adress isn't in the table, is it full?
            pageTable[capacity] = addr; //add address to the table
            diskReads++;
            capacity++;
            if(rw == 'W') 
                dirtyBit[i] = rw;
        }
        else{ //table is full, must make room
            i = random(0, nframes); //randomly select page to remove
            if(dirtyBit[i] == 'W') //if ejected page was dirty write it to disk
                diskWrites++;
            pageTable[i] = addr; //add new address to table
            dirtyBit[i] = rw;
            diskReads++;
        }
        events++;
    }
    printoutput(nframes, events, diskReads, diskWrites);
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

    //call test function
    if(strcmp(testType, "rdm") == 0){
        rdm(fptr, nframes);
    }
    else if(strcmp(testType, "lru") == 0){
        //lru(fptr, nframes);
    }
    else if(strcmp(testType, "fifo") == 0){
        fifo(fptr, nframes);
    }
    else if(strcmp(testType, "vms") == 0){
        //vms(fptr, nframes);
    }
    else{
        printf("unknown page replacement algorithm");
    }

    return 0;
}