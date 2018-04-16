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
void printReport();
bool isSafe(int processes[], int avail[], int maxm[][maxResources], int allot[][maxResources]);
//sem_t sem;


//record keeping variables
int requests; //keep track of count of requests
int safetyChecks; //keeps track of times banker's algorithm is ran
int blocks; //keeps track of times request is blocked
int grants; //keeps track of times request is granted 
int terminations; //keeps track of process terminations
int releases; //keeps track of releases
int processCount; //current number of processes

int main (int argc, char *argv[]) {
	//keys
	messageQueueKey = 59569;
	keySimClock = 59566;
	semaphoreKey = 59567;
	descriptorKey = 59568;
	int totalProcessesCreated = 0; //keeps track of all processes created	
	int processLimit = 100; //max number of processes allowed by assignment parameters
	double terminateTime = 3; //used by setperiodic to terminate program
	unsigned int newProcessTime[2] = {0,0};
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
	if ((shmidSimClock = shmget(keySimClock, (2*(sizeof(unsigned int))), IPC_CREAT | 0666)) == -1){
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

	int line;

//populate number of resources in the resourcTable
	int index;
	for (index = 0; index < maxResources; index++){
		(*resourceTable).numResources[index] = (rand() % 8) + 3;
	}

		fprintf(fp, "maxResources: ");
		fflush(fp);
		line++;
	for (index = 0; index < maxResources; index++){
		fprintf(fp, "%d ", (*resourceTable).numResources[index]);
		fflush(fp);
	}
		fprintf(fp, "\n");
		fflush(fp);
		line++;

	while(1){ //setting max loops during development
		//check to see if its time for a new process to start
		if(line >= 10000){
			terminateSharedResources;
			kill(getpid(), SIGINT);
			}	
	

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
					return 1;
				} 

				processCount++;
				(*resourceTable).pidArray[index] = childPid; //store pid
				fprintf(fp, "OSS created process with PID %d at time %d.%d \n", childPid, simClock[1], simClock[0]);
				fprintf(fp, "Pid store in location %d\n", index);
				fflush(fp);
				line++;
				line++;
			} else {
//				fprintf(fp, "OSS attempted to create process. Max processes reach\n");
//				fflush(fp);
//				line++;
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
		msgrcv(messageBoxID, &message, sizeof(message), 5, IPC_NOWAIT);	

		int processIndex = message.RTLocation;
		int pid = message.pid;



		//if message contains request 
		if (message.request != 0){
			requests++;
			printf("OSS received message to claim\n");
	
			int requestedResource = (*resourceTable).process[processIndex].resourceRequest;
			printf("requested resource is %d\n", requestedResource);
				
			fprintf(fp,"OSS received a request from process %d for resource %d\n", pid, (requestedResource-1));
			fflush(fp);
			line++;

			//test resource request
			(*resourceTable).allocated[processIndex][requestedResource] += 1;
			(*resourceTable).available[requestedResource] -= 1;

			printAllocationTable();

			//increment clock to run banker's
			simClock[0] += 10000;
			convertTime(simClock);
			
			//run safety check
			safetyChecks++;
			if (isSafe((*resourceTable).pidArray, (*resourceTable).available, (*resourceTable).maxCanRequest, (*resourceTable).allocated)){
				grants++;
			
				simClock[0] += 10000;
				convertTime(simClock);
			
				(*resourceTable).process[processIndex].blocked = false;
				message.mesg_type = pid;
				message.granted = true;
			
				fprintf(fp, "OSS granted %d to process %d\n", (requestedResource), pid);
				fflush(fp);
				line++;
		
				if(msgsnd(messageBoxID, &message, sizeof(message), 0) ==  -1){
					perror("oss attempted to send message after safety check. resource to be granted");
				}
		
			} else {
				//reverse the allocation
				blocks++;
				(*resourceTable).allocated[processIndex][requestedResource] -= 1;
				(*resourceTable).available[requestedResource] += 1;
				enqueue(blockedQueue, pid);		
				blockedQueue[0].RTLocation = processIndex;	

				(*resourceTable).process[processIndex].blocked = true;

				fprintf(fp, "OSS blocked process %d request\n", pid);
				fflush(fp);
				line++;
				
			}
			message.request = 0;
		}
	//if message contains release or a termination
	  	if (message.release != 0 || message.terminate == true){
	
			if (message.release != 0){
				releases++;
				int releasedResource = (*resourceTable).process[processIndex].resourceRelease;
				(*resourceTable).allocated[processIndex][releasedResource] -= 1;
				(*resourceTable).available[releasedResource] += 1;
				(*resourceTable).process[processIndex].blocked = false;
				
				//increment clock
				simClock[0] += 10000;
				convertTime(simClock);

				fprintf(fp, "Process %d released resource %d\n", pid, releasedResource);
				fflush(fp);
				line++;
				message.release = 0;
				message.mesg_type = pid;
				message.granted = true;
				
				if(msgsnd(messageBoxID, &message, sizeof(message), 0) ==  -1){
					perror("oss attempted to send message after release.");
				}
			}

			//if message contains terminate
			if (message.terminate == true){
				terminations++;
				//set bitVector for location that terminated to 0
				bitVector[processIndex] = 0;
				//add resources being freed to the available resources.  zero out resources from the 2d allocated array
				for(index = 0; index < maxProcesses; index++){
					(*resourceTable).available[index] += (*resourceTable).allocated[processIndex][index];
					(*resourceTable).allocated[processIndex][index] = 0;
				}
				(*resourceTable).process[processIndex].blocked = false;
				
				//increment clock
				simClock[0] += 10000;
				convertTime(simClock);

				fprintf(fp, "Process %d terminated and returned its resources\n", pid);
				fflush(fp);
				line++;
				
			} 
	
			//check blocked Queue
			if (!isEmpty){
				
				//test resource request
				//message.request is offset by one in user.  allows 0 to act as a sentry
				(*resourceTable).allocated[processIndex][(*resourceTable).process[blockedQueue[0].RTLocation].resourceRequest] += 1;
				(*resourceTable).available[(*resourceTable).process[blockedQueue[0].RTLocation].resourceRequest] -= 1;
				int attemptedPid = blockedQueue[0].pid;
				dequeue(blockedQueue);

				//increment clock to run banker's
				simClock[0] += 10000;
				convertTime(simClock);
			
				fprintf(fp, "OSS is checkeding for blocked processes\n");
				fflush(fp);
				line++;

				//run safety check
				safetyChecks++;
				if (isSafe((*resourceTable).pidArray, (*resourceTable).available, (*resourceTable).maxCanRequest, (*resourceTable).allocated)){
					grants++;
			
					simClock[0] += 10000;
					convertTime(simClock);
				
					message.mesg_type = attemptedPid;
					message.granted = true;

					fprintf(fp, "OSS was able to grant blocked process %d request for resource %d\n", attemptedPid, attemptedPid);
					fflush(fp);
					line++;
			
					if(msgsnd(messageBoxID, &message, sizeof(message), 0) ==  -1){
						perror("oss attempted to send message after safety check of a blocked resource");
					}
			
				} else {
					fprintf(fp, "OSS placed process %d back into blocked queue\n", attemptedPid);
					fflush(fp);
					line++;
					enqueue(blockedQueue, attemptedPid);
					blocks++;
				}
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
		printReport();
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

void printReport(){

//record keeping variables
printf("Total Processes created: %d\n", processCount);
printf("Total Requests Made: %d\n",  requests);
printf("Total Safety Checks made: %d\n", safetyChecks); //keeps track of times banker's algorithm is ran
printf("Total Blocks made: %d\n",  blocks);
printf("Total Requests Granted: %d\n", grants); //keeps track of times request is granted 
printf("Total Processes Terminated: %d\n",terminations); //keeps track of process terminations
printf("Total Resources released: %d\n",  releases); //keeps track of releases
}

//code from https://www.geeksforgeeks.org/program-bankers-algorithm-set-1-safety-algorithm/.  adapted slightly
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

	//int safeSeq[maxProcesses];

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

