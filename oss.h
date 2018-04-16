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
#include <semaphore.h>

//macro definations

#define SHM_SIZE 2000
#define BILLION 1000000000L //taken from book

//governs maximum amouut of processes and resources.  change to increase/decrease
#define maxProcesses 18
#define maxResources 20

//structures


typedef struct {
	int resourceRequest;
	int resourceRelease;
	bool blocked;
	int maxClaimsAllowed[maxResources];
	int RTLocation;
} processes;
typedef struct {
	int numResources[maxResources];			//stores how many of each resource there is
	int allocated[maxProcesses][maxResources];	//stores how many resources each processes allocated 
	int maxCanRequest[maxProcesses][maxResources]; 		//maximum a process can request
	int available[maxResources];		//maximum available resources
	int pidArray[maxProcesses];			//keep track of child pids
	processes process[maxProcesses];
} descriptor;



typedef struct {
	long mesg_type;				//controls who cam retreive a message
	int pid;
	int RTLocation;
	int release;
	int request;
	bool terminate;
	bool granted;
} Message;

typedef struct
{
    int front, rear, size;
    unsigned capacity;
    int* array;
    int RTLocation;
    int pid;
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
Message message;
int messageBoxID;
int shmidSimClock;
int shmidSemaphore;
int shmidDescriptor;

//message queue key
key_t messageQueueKey;

//Shared Memory Keys
key_t keySimClock;
key_t semaphoreKey;
key_t descriptorKey;

//semaphore


#endif

