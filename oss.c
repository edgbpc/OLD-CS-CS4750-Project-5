#include "oss.h" //contains all other includes necessary, macro definations, and function prototypes


//Global Variables

char fileName[10] = "data.log";
FILE * fp;
pid_t childPid; //pid id for child processes
unsigned int *simClock; // simulated system clock  simClock [0] = seconds, simClock[1] = nanoseconds

void terminateSharedResources();
static int setperiodic(double sec);


int main (int argc, char *argv[]) {
	//keys
	messageQueueKey = 59569;
	keySimClock = 59566;
	int totalProcessesCreated = 0; //keeps track of all processes created	
	int processCount = 0; //current number of processes
	int processLimit = 100; //max number of processes allowed by assignment parameters
	double terminateTime = 3; //used by setperiodic to terminate program
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
//Attach to shared memory and initalize clock to 0
	simClock = shmat(shmidSimClock, NULL, 0);
	
//create mail box
	if ((messageBoxID = msgget(messageQueueKey, IPC_CREAT | 0666)) == -1){
		perror("oss: Could not create child mail box");
		return 1;
	}

int counter = 0;

while(counter < 20){ //setting max loops because program will crash if left to run infinitely.
		//if its time for a new process fork child process
		if ((childPid = fork()) == -1){ 
			perror("oss: failed to fork child");
                        // clena out memory and terminate or continue on but no new process
		}	
		//exec child process
		if (childPid < 0){
			perror("pss: failed to exec");
			exit(1);
		} else 	if (childPid == 0){
			//child process
		} else {
			//parent process sets priority and initalizes the PCBTable Entry
		}		
} //outer while loop



//free  up memory queue and shared memory
terminateSharedResources();

kill(0, SIGKILL); //without this, some user processes continue to run even after OSS terminates
return 0;
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
		kill(0, SIGKILL);
		wait(NULL);
		terminateSharedResources();
		exit(0);
	}
}


void terminateSharedResources(){
		shmctl(shmidSimClock, IPC_RMID, NULL);
		msgctl(messageBoxID, IPC_RMID, NULL);
		shmdt(simClock);	
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

