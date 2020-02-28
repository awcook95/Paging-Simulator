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

struct Node* findNodeByRW(struct Node* node, char elem){
    if(node->prev == NULL) //skip head node if we started there
        node = node->next;
    while(node->next != NULL){ //while not at the tail node
        if(node->rw == elem){
            return node;
        }
        node = node->next;
    }
    return NULL;
}

void printList(struct Node* node){
    if(node->prev == NULL) //skip head node if we started there
        node = node->next;
    while(node->next != NULL){
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

void lru(FILE* tracefile, int nframes){
    //initialize linked list sentinels and set pointers
    //this list will act as the page table and queue
    //unlike a regular queue, if the address is used again it will be taken out and moved to the back
    struct Node* head = (struct Node*)malloc(sizeof(struct Node));
    struct Node* tail = (struct Node*)malloc(sizeof(struct Node));
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
            
            deleteNode(foundNode);
            enqueue(tail, foundNode->addr, foundNode->rw); //important destinction from fifo, used pages get enqueued again
            free(foundNode); //enqueue constructs a new node, so old copy must be freed
        }
        else if(capacity < nframes){ //adress isn't in the table: if it's not full yet
            enqueue(tail, addr, rw); //add address to the table
            diskReads++;
            capacity++;
        }
        else{ //table is full, must make room
            struct Node* ejectedNode = pop(head); //remove oldest member of the queue
            if(ejectedNode->rw == 'W') //if ejected page was dirty write it to disk
                diskWrites++;
            free(ejectedNode);
            enqueue(tail, addr, rw); //enqueue new address
            diskReads++;
        }
        events++;
    }
    printoutput(nframes, events, diskReads, diskWrites);
}

void fifo(FILE* tracefile, int nframes){
    //start linked list sentinels and set pointers
    //this list will act as the page table and queue
    struct Node* head = (struct Node*)malloc(sizeof(struct Node));
    struct Node* tail = (struct Node*)malloc(sizeof(struct Node));
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
        else if(capacity < nframes){ //adress isn't in the table, if it's not full yet
            enqueue(tail, addr, rw); //add address to the table
            diskReads++;
            capacity++;
        }
        else{ //table is full, must make room
            struct Node* ejectedNode = pop(head); //remove oldest member of the queue
            if(ejectedNode->rw == 'W') //if ejected page was dirty write it to disk
                diskWrites++;
            free(ejectedNode);
            enqueue(tail, addr, rw); //enqueue new address
            diskReads++;
        }
        events++;
    }
    printoutput(nframes, events, diskReads, diskWrites);
}

void vms(FILE* tracefile, int nframes){
    //create linked list sentinels and set pointers for process 1
    struct Node* p1Head = (struct Node*)malloc(sizeof(struct Node));
    struct Node* p1Tail = (struct Node*)malloc(sizeof(struct Node));
    p1Head->prev = NULL;
    p1Head->next = p1Tail;
    p1Tail->prev = p1Head;
    p1Tail->next = NULL;

    //create linked list sentinels and set pointers for process 2
    struct Node* p2Head = (struct Node*)malloc(sizeof(struct Node));
    struct Node* p2Tail = (struct Node*)malloc(sizeof(struct Node));
    p2Head->prev = NULL;
    p2Head->next = p2Tail;
    p2Tail->prev = p2Head;
    p2Tail->next = NULL;

    int p1ListSize = 0;
    int p2ListSize = 0;
    unsigned addr;
    char rw;
    int events = 0;
    int diskReads = 0;
    int diskWrites = 0;

    while(fscanf(tracefile,"%x %c",&addr,&rw) != EOF){
        unsigned pID = addr >> 28; //get process ID tag as the first hex digit off 32 bit address
        addr = addr >> 12; //addr is the entire virtual address, we just need the VPN from it so, first 20 bits as each page is size 2^12
        if(pID == 3){ //address belongs to process 1
            struct Node* foundNode = findNode(p1Head, addr); //find address in list
            if(foundNode != NULL){ //if address is in the p1 list already
            if(rw == 'W') 
                foundNode->rw = rw;
            }
            else if((p1ListSize + p2ListSize) < nframes){ //adress isn't in the table, if it's not full yet
            enqueue(p1Tail, addr, rw); //add address to the table
            diskReads++;
            p1ListSize++;
            }
            else{ //table is full, must make room
                if(p1ListSize >= p2ListSize){ //if p1 is using more or equal # of frames
                    struct Node* ejectedNode = findNodeByRW(p1Head, 'R'); //prioritize clean pages
                    if(ejectedNode == NULL){
                        if(p1Head->next->rw == 'W')
                            diskWrites++;
                        free(pop(p1Head)); //since all frames are dirty eject the oldest one and delete it
                    }
                    else{
                        if(ejectedNode->rw == 'W')
                            diskWrites++;
                        free(deleteNode(ejectedNode));
                    }
                    enqueue(p1Tail, addr, rw); //add new page
                    diskReads++;
                }
                else{ //p2 is using more frames than p1, so we eject a page from p2
                    struct Node* ejectedNode = findNodeByRW(p2Head, 'R'); //prioritize clean pages
                    if(ejectedNode == NULL){
                        if(p2Head->next->rw == 'W')
                            diskWrites++;
                        free(pop(p2Head)); //since all pages are dirty eject the oldest one and delete it
                    }
                    else{
                        if(ejectedNode->rw == 'W')
                            diskWrites++;
                        free(deleteNode(ejectedNode));
                    }
                    enqueue(p1Tail, addr, rw); //add new page
                    diskReads++;
                    p1ListSize++;
                    p2ListSize--;
                    
                }
            }
        }
        else{ //address belongs to process 2
            struct Node* foundNode = findNode(p2Head, addr); //find address in list
            if(foundNode != NULL){ //if address is in the p1 list already
            if(rw == 'W') 
                foundNode->rw = rw;
            }
            else if((p1ListSize + p2ListSize) < nframes){ //adress isn't in the table, if it's not full yet
            enqueue(p2Tail, addr, rw); //add address to the table
            diskReads++;
            p2ListSize++;
            }
            else{ //table is full, must make room
                if(p2ListSize >= p1ListSize){ //if p2 is using more or equal # of frames
                    struct Node* ejectedNode = findNodeByRW(p2Head, 'R'); //prioritize clean pages
                    if(ejectedNode == NULL){
                        if(p2Head->next->rw == 'W')
                            diskWrites++;
                        free(pop(p2Head)); //since all frames are dirty eject the oldest one and delete it
                    }
                    else{
                        if(ejectedNode->rw == 'W')
                            diskWrites++;
                        free(deleteNode(ejectedNode));
                    }
                    enqueue(p2Tail, addr, rw); //add new page
                    diskReads++;
                }
                else{ //p1 is using more frames than p2, so we eject a page from p1
                    struct Node* ejectedNode = findNodeByRW(p1Head, 'R'); //prioritize clean pages
                    if(ejectedNode == NULL){
                        if(p1Head->next->rw == 'W')
                            diskWrites++;
                        free(pop(p1Head)); //since all pages are dirty eject the oldest one and delete it
                    }
                    else{
                        if(ejectedNode->rw == 'W')
                            diskWrites++;
                        free(deleteNode(ejectedNode));
                    }
                    enqueue(p2Tail, addr, rw); //add new page
                    diskReads++;
                    p2ListSize++;
                    p1ListSize--;
                    
                }
            }
        }
        events++;
    }
    printoutput(nframes, events, diskReads, diskWrites);
}



int main(int argc, char** argv){
    //errors for incorrect cm args
    if(argc != 5){
		printf("Incorrect number of command line arguments, ending program\n");
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
        lru(fptr, nframes);
    }
    else if(strcmp(testType, "fifo") == 0){
        fifo(fptr, nframes);
    }
    else if(strcmp(testType, "vms") == 0){
        vms(fptr, nframes);
    }
    else{
        printf("unknown page replacement algorithm");
    }

    return 0;
}