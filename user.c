#include "oss.h"

unsigned int * simClock;
descriptor *resourceTable;

int findResource(descriptor *resourceTable, int location);

int main (int argc, char *argv[]){
	printf("USER has launced\n");
	int myPID = getpid();
	int randomTerminateConstant = 90; //change this to increase or decrease chance process will terminate.  currently 50% to terminate
	int randomRequestConstant = 50;
	//seed random number generator
	time_t timeSeed;
	srand((int)time(&timeSeed) % getpid()); //%getpid used because children were all getting the same "random" time to run. 

	//signal handling
	if (signal(SIGINT, handle) == SIG_ERR){
		perror("signal failed");
	}
	//Shared Memory Keys
	keySimClock = 59566;
	semaphoreKey = 59567;
	descriptorKey = 59568;
	//Get Shared Memory
	if ((shmidSimClock = shmget(keySimClock, SHM_SIZE, 0666)) == -1){
		perror("user: could not get shmidSimClock\n");
		return 1;
	}
	if ((shmidDescriptor = shmget(descriptorKey, SHM_SIZE, 0666)) == -1){
		perror("user: could not get shmidSimClock\n");
		return 1;
	}
	
	//get the shared Clock and resourceTable
	simClock= (int *)(shmat(shmidSimClock, 0, 0));
	resourceTable = (descriptor *)(shmat(shmidDescriptor, 0, 0));
	
	//message queue key
	key_t messageQueueKey = 59569;
	//get message box
	if ((messageBoxID = msgget(messageQueueKey, 0666)) == -1){
		perror("user: failed to acceess parent message box");
	}


	int counter = 0;
	int location;
	bool messageReceived;
	int index; //for incrementing in loops

	//while loop runs contiously but on first run, the IF statement fails
	//and then suspends on waiting for a message. 
	//message received and then can process the IF statement
	while (counter < 3){  
		if (messageReceived == true){
			messageReceived = false; //reset flag to false
			location = message.RTLocation;
	
			//if granted a request
			int willTerminate = (rand() % 100) + 1;
			int willRequest = (rand() % 100) + 1;
			if (willTerminate >= randomTerminateConstant){
				message.terminate = true;
				message.RTLocation = location;
			
				if(msgsnd(messageBoxID, &message, sizeof(message), IPC_NOWAIT) ==  -1){
					perror("oss: failed to send message to user");
				}
			
				exit(1);
			//else if checks if User is going to request or release a resource
			} else if (willRequest >= randomRequestConstant){	
			//determine which resource to request
				message.request = rand() % 20;
				message.mesg_type = getppid();
			//else block select a resource to release
			} else {
			//find a resource to release	
				//sets message.release to resource that User is going to release
				//returns -1 if no resource could be found to release
				if ((message.release = findResource(resourceTable, location)) == -1){
					printf("no resources to release\n");
				
				} 
			}	
			if(msgsnd(messageBoxID, &message, sizeof(message), IPC_NOWAIT) ==  -1){
				perror("oss: failed to send message to user");
			}

		counter++; //for development purposes. controlling max loops
		} //end outer if 

	 //retrieve message from message box.  child is blocked unless there is a message to take from box
		msgrcv(messageBoxID, &message, sizeof(message), myPID, 0);
		messageReceived = true;
	} //end while loopi
} //end main

int findResource(descriptor *resourceTable, int location){
	int index;
	for (index = 0; index < maxProcesses; index++){
		if ((*resourceTable).allocated[location][index] != 0){
			return index;
			break;
		} else {
			return -1;
		}
	}	

}

void handle(int signo){
	if(signo == SIGINT){
		fprintf(stderr, "User process %d shut down by signal\n", getpid());
		exit(0);
	}
}



	

