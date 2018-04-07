#include "oss.h"

unsigned int * simClock;

int main (int argc, char *argv[]){
	printf("USER has launced\n");
	int myPID = getpid();
	//seed random number generator
	time_t timeSeed;
	srand((int)time(&timeSeed) % getpid()); //%getpid used because children were all getting the same "random" time to run. 

	//signal handling
	if (signal(SIGINT, handle) == SIG_ERR){
		perror("signal failed");
	}

	//Shared Memory Keys
	key_t keySimClock = 59566;
	
	//Get Shared Memory
	if ((shmidSimClock = shmget(keySimClock, SHM_SIZE, 0666)) == -1){
		perror("user: could not get shmidSimClock\n");
		return 1;
	}
	//Attach to shared memory and get simClock time
	//used to determine how long the child lived for
	simClock= (int *)(shmat(shmidSimClock, 0, 0));
	
	//message queue key
	key_t messageQueueKey = 59569;
	
	//get message box
	if ((messageBoxID = msgget(messageQueueKey, 0666)) == -1){
		perror("user: failed to acceess parent message box");
	}

	//record when this process was created


while (1){ //runs continously once process created but outer while loop will be blocked until message received from OSS.  
	
	}

	msgrcv(messageBoxID, &message, sizeof(message), myPID, 0); //retrieve message from message box.  child is blocked unless there is a message to take from box

}
void handle(int signo){
	if(signo == SIGINT){
		fprintf(stderr, "User process %d shut down by signal\n", getpid());
		exit(0);
	}
}



	

