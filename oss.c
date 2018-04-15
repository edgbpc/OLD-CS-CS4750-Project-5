#include "oss.h" //contains all other includes necessary, macro definations, and function prototypes




//Global Variables

char fileName[10] = "data.log";
FILE * fp;
pid_t childPid; //pid id for child processes
unsigned int *simClock; // simulated system clock  simClock [0] = seconds, simClock[1] = nanoseconds
descriptor *resourceTable;
int bitVector[maxProcesses];

void terminateSharedResources();
static int setperiodic(double sec);
void convertTime(unsigned int simClock[]);
int FindIndex(int value);
void printMaxCanRequestTable();
void printAllocationTable();
void calculateNeed(int need[maxProcesses][maxResources], int maxm[maxProcesses][maxResources], int allot[maxProcesses][maxResources]);


//sem_t sem;


int main (int argc, char *argv[]) {
	//keys
	messageQueueKey = 59569;
	keySimClock = 59566;
	semaphoreKey = 59567;
	descriptorKey = 59568;
	int totalProcessesCreated = 0; //keeps track of all processes created	
	int processCount = 0; //current number of processes
	int processLimit = 100; //max number of processes allowed by assignment parameters
	double terminateTime = 10; //used by setperiodic to terminate program
	unsigned int newProcessTime[2] = {0,0};
	int RTIndex;
	Queue* blockedQueue = createQueue(maxProcesses);
	bool messageReceived;

//open file for writing	
	fp = fopen(fileName, "w");
//seed random number generator
	srand(time(NULL));
//signal handler to catch ctrl c
	if(signal(SIGINT, handle) == SIG_ERR){
		perror("Signal Failed");
	}
	if(signal(SIGALRM, handle) == SIG_ERR){
		perror("Signal Failed");
	}	
//set timer. from book
	if (setperiodic(terminateTime) == -1){
		perror("oss: failed to set run timer");
		return 1;
	}
//Create Shared Memory
	if ((shmidSimClock = shmget(keySimClock, SHM_SIZE, IPC_CREAT | 0666)) == -1){
		perror("oss: could not create shared memory for simClock");
		return 1;
	}
	if ((shmidDescriptor = shmget(descriptorKey, (sizeof(descriptor)), IPC_CREAT | 0666)) == -1){
		perror("oss: could not create shared memory for descriptor");
		return 1;
	}
//Initalize semaphore
//	if ((sem_init(&sem, 1, 0)) == -1){
//		perror("oss: could not init sem");
//	}	
//Attach to shared memory
	simClock = shmat(shmidSimClock, NULL, 0);
	simClock[0] = 0; //nanoseconds
	simClock[1] = 0; //seconds	
	resourceTable = shmat(shmidDescriptor, NULL, 0);	
//create mail box
	if ((messageBoxID = msgget(messageQueueKey, IPC_CREAT | 0666)) == -1){
		perror("oss: Could not create child mail box");
		return 1;
	}

	int counter = 0;

//populate number of resources in the resourcTable
	int index;
	for (index = 0; index < maxResources; index++){
		(*resourceTable).numResources[index] = (rand() % 10) + 1;
	}
	while(1){ //setting max loops during development
		//check to see if its time for a new process to start
		if ((simClock[1] == newProcessTime[1] && simClock[0] >= newProcessTime[0]) || simClock[1] >= simClock[1]){

			bool spawnNewProcess = false;
			//find available location in the resouceTable
			
			for (index = 0; index < maxProcesses; index++){
				if(bitVector[index] == 0){
					int jindex;
					for (jindex = 0; jindex < maxResources; jindex++){
						(*resourceTable).maxCanRequest[index][jindex] = (rand()% 3) + 1;
					}
						spawnNewProcess = true;
						bitVector[index] = 1;
						childPid = fork();
						break;
				}
	//		printMaxCanRequestTable();
			}
			if (spawnNewProcess == true){
				if(childPid == -1){
					perror("OSS:  Failed to fork child\n");
					kill(getpid(), SIGINT);
				}
				
				if(childPid == 0){
					//convert index to string to pass to User so it knows its location in the resouceTable
					char intBuffer[3];
					sprintf(intBuffer, "%d", index);
					//printf("%s", intBuffer);
					execl("./user", "user", intBuffer, NULL);
			//		return 1;
				} 

				processCount++;
				(*resourceTable).pidArray[index] = childPid; //storing pid for future use.  not sure if going to use
			}
			
			//set next time for a process
			//get new process time between 1 and 500 milliseconds
			//increments nanoseconds
			//copies current second time
			//convert nanoseconds to seconds

			newProcessTime[0] = (rand() % 500000000) + 1000000 + simClock[0];
			newProcessTime[1] = simClock[1];
			convertTime(simClock);
		
	} //end if statement for checking if its time for new process
		
		//receive a message
//		printf("OSS is waiting for message\n");
		msgrcv(messageBoxID, &message, sizeof(message), getpid(), IPC_NOWAIT);

		
		printf("OSS received %d and %d\n", message.RTLocation, message.request);
		
		//if message contains request 
		if (message.request != 0){
//			printf("OSS received message to claim %d\n", message.request);
//
//			//increment clock to run banker's
			simClock[0] += 100000;
			convertTime(simClock);
		
			//grant a resource request
			(*resourceTable).allocated[message.RTLocation][message.request-1] += 1;

			message.mesg_type = (*resourceTable).pidArray[message.RTLocation];
			message.granted = true;
		
			if(msgsnd(messageBoxID, &message, sizeof(message), 0) ==  -1){
				perror("oss: failed to send message to user");
			}
		}
	//if message contains release
		if (message.release != 0){
			(*resourceTable).allocated[message.RTLocation][message.release-1] -= 1;
		}

		//if message contains terminate
		if (message.terminate == true){
			//set bitVector for location that terminated to 0
			bitVector[message.RTLocation] = 0;
			//add resources being freed to the available resources.  zero out resources from the 2d allocated array
			for(index = 0; index < maxProcesses; index++){
				(*resourceTable).availableResources[index] += (*resourceTable).allocated[message.RTLocation][index];
				(*resourceTable).allocated[message.RTLocation][index] = 0;
			}
		}
		
		//increment clock
		simClock[0] += 1000000;
		convertTime(simClock);
	
	} //outer while iloop

} //end main()
//convertTime
void convertTime(unsigned int simClock[]){
	simClock[1] += simClock[0]/1000000000;
	simClock[0] = simClock[0]%1000000000;
}


//TAKEN FROM BOOK
static int setperiodic(double sec) {
   timer_t timerid;
   struct itimerspec value;
   if (timer_create(CLOCK_REALTIME, NULL, &timerid) == -1)
      return -1;
   value.it_interval.tv_sec = (long)sec;
   value.it_interval.tv_nsec = (sec - value.it_interval.tv_sec)*BILLION;
   if (value.it_interval.tv_nsec >= BILLION) {
      value.it_interval.tv_sec++;
      value.it_interval.tv_nsec -= BILLION;
   }
   value.it_value = value.it_interval;
   return timer_settime(timerid, 0, &value, NULL);
}

void handle(int signo){
	if (signo == SIGINT || signo == SIGALRM){
		printf("*********Signal was received************\n");
		terminateSharedResources();
		kill(0, SIGKILL);
		wait(NULL);
		exit(0);
	}
}


void terminateSharedResources(){
	//	sem_destroy(&sem);
		printAllocationTable();
		shmctl(shmidSimClock, IPC_RMID, NULL);
	//	shmctl(shmidSemaphore, IPC_RMID, NULL);
		shmctl(shmidDescriptor, IPC_RMID, NULL);
		msgctl(messageBoxID, IPC_RMID, NULL);
		shmdt(simClock);	
		shmdt(resourceTable);
}
//Queue code comes from https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation

Queue* createQueue(unsigned capacity)
{
    Queue* queue = (Queue*) malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0; 
    queue->rear = capacity - 1;  // This is important, see the enqueue
    queue->array = (int*) malloc(queue->capacity * sizeof(int));
    return queue;
}
 
int isFull(Queue* queue){
	  return (queue->size == queue->capacity); 
 }
 
int isEmpty(Queue* queue){
	return (queue->size == 0);
 }
  
void enqueue(Queue* queue, int item){
       if (isFull(queue))
		  return;
       queue->rear = (queue->rear + 1)%queue->capacity;
       queue->array[queue->rear] = item;
       queue->size = queue->size + 1;
       printf("%d enqueued to queue\n",item);
}
                                
int dequeue(Queue* queue){
	if (isEmpty(queue))
		return INT_MIN;
        int item = queue->array[queue->front];
        queue->front = (queue->front + 1)%queue->capacity;
        queue->size = queue->size - 1;
        return item;
}
                                                             
int front(Queue* queue){
	if (isEmpty(queue))
	        return INT_MIN;	
	return queue->array[queue->front];
}
int rear(Queue* queue){
        if (isEmpty(queue))
                return INT_MIN;
        return queue->array[queue->rear];
}

//this code came from https://stackoverflow.com/questions/25003961/find-array-index-if-given-value
int FindIndex(int value){
	int i = 0;

	while (i < maxProcesses && bitVector[i] != value) ++i;
	return (i == maxProcesses ? -1 : i );
	}

void printMaxCanRequestTable(){
	printf("\t\t\t max Can Request table\n");
	printf("Columns represent each resource available\n");
	int i;
	int j;
	for (i = 0; i < maxProcesses; i++){
		printf("P%d\t", i);
		for (j = 0; j < maxResources; j++){
			printf(" %d", (*resourceTable).maxCanRequest[i][j]);
		}
	printf("\n");
	}
}

void printAllocationTable(){
	printf("\t\t\t Allocation Table \n");
	printf("Columns represent each resource available\n");
	int i;
	int j;
	for (i = 0; i < maxProcesses; i++){
		printf("P%d\t", i);
		for (j = 0; j < maxResources; j++){
			printf(" %d", (*resourceTable).allocated[i][j]);
		}
	printf("\n");
	}
}

//code from https://www.geeksforgeeks.org/program-bankers-algorithm-set-1-safety-algorithm/
void calculateNeed(int need[maxProcesses][maxResources], int maxm[maxProcesses][maxResources], int allot[maxProcesses][maxResources]){
	int i,j;
	for (i = 0; i < maxProcesses; i++){
		for (j = 0; j < maxResources; j++){
			need[i][j] = maxm[i][j] - allot[i][j];
		}
	}
}
bool isSafe(int processes[], int avail[], int maxm[][maxResources], int allot[][maxResources]){
	int index;
	int need[maxProcesses][maxResources];
	calculateNeed(need, maxm, allot);

	bool finish[maxProcesses] = {0};

	int safeSeq[maxProcesses];

	int work[maxResources];
	int i;
	for (i = 0; i < maxResources; i++){
		work[i] = avail[i];
	}

	int count = 0;
	while (count < maxProcesses){
		int p;
		bool found = false;
		for (p = 0; p < maxProcesses; p++){
			if (finish[p] == 0) {
				int j;
				for (j = 0; j < maxResources; j++){
					if(need[p][j] > work[j]){
						break;
					}
				}
			if (j == maxResources){
				int k;
				for (k = 0; k < maxResources; k++){
					work[k] += allot[p][k];
				}
				safeSeq[count++] = p;
				finish[p] = 1;
				found = true;
			}
		}
	}

	if (found == false){
			printf("System is not in safe state\n");
			return false;
		}
	}

	printf("System is in safe state\n");

	return true;
}

/*
 KEPT FOR FUTURE USE
//getopt
	char option;
	while ((option = getopt(argc, argv, "s:hl:t:")) != -1){
		switch (option){
			case 's' : maxProcess = atoi(optarg);
				break;
			case 'h': print_usage();
				exit(0);
				break;
			case 'l': 
				strcpy(fileName, optarg);
				break;
			case 't':
				terminateTime = atof(optarg);
				break;
			default : print_usage(); 
				exit(EXIT_FAILURE);
		}
	}

void print_usage(){
	printf("Execution syntax oss -options value\n");
	printf("Options:\n");
	printf("-h: this help message\n");
	printf("-l: specify file name for log file.  Default: data.log\n");
	printf("-s: specify max limit of processes that can be ran.  Default: 5\n");
	printf("-t: specify max time duration in seconds.  Default: 20\n");
}
*/

