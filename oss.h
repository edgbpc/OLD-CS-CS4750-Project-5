#ifndef OSS_HEADER_FILE
#define OSS_HEADER_FILE


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>
#include <sys/msg.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <limits.h>

//macro :definations

#define SHM_SIZE 100
#define BILLION 1000000000L //taken from book


//structures


typedef struct {
} Message;

typedef struct
{
    int front, rear, size;
    unsigned capacity;
    int* array;
} Queue;

//prototypes
void handle(int signo);

//Queue Prototypes
Queue* createQueue(unsigned capacity);
int isFull(Queue* queue);
int isEmpty(Queue* queue);
void enqueue(Queue* queue, int item);
int dequeue(Queue* queue);
int front(Queue* queue);
int rear(Queue* queue);
//variables
//
//
Message message;
int messageBoxID;
int shmidSimClock;
int shmidPCB;

//message queue key
key_t messageQueueKey;

//Shared Memory Keys
key_t keySimClock;
key_t keyPCB;




#endif

