#include "oss.h"

unsigned int * simClock;
descriptor *resourceTable;


int FindResource(int location, int value);
int main (int argc, char *argv[]){
	printf("USER %d has launced\n", getpid());
	

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
	int location = atoi(argv[1]);
	bool requestWasGranted = false;
	int index; //for incrementing in loops

	int claim;
	
	printf("USer process %d is located in index of %d of the resource table\n", getpid(), location);
	//while loop runs contiously but on first run, the IF statement fails
	//and then suspends on waiting for a message. 
	//message received and then can process the IF statement
	while (1){
		msgrcv(messageBoxID, &message, sizeof(message), myPID, IPC_NOWAIT);
		requestWasGranted = message.granted;
		int willRequest = (rand() % 100) + 1;

		if (requestWasGranted == true){				
			int willTerminate = (rand() % 100) + 1;
			if (willTerminate >= randomTerminateConstant){
				printf("USER %d is going to terminate\n", getpid());
				//notify OSS that process is terminating
				message.terminate = true; 
				message.mesg_type = getppid();
				//set location of process in resourceTable in message to OSS
				//zero out allocated resources
				for (index = 0; index < maxResources; index++){
					(*resourceTable).allocated[location][index] = 0;
				}
				//send message to OSS
				if(msgsnd(messageBoxID, &message, sizeof(message), 0) ==  -1){
					perror("oss: failed to send message to user");
				}
				//process exits
				exit(1); 
			}
		} //end if(requestWasGranted == true)  block	
 
		if (willRequest >= randomRequestConstant){	
				claim = rand () % 20 + 1; //offset by one so I can use 0 as a sentry value in OSS.
				int newAllocation =  1 + (*resourceTable).allocated[location][claim];
				
				if (newAllocation <= (*resourceTable).maxCanRequest[location][claim]){
					message.mesg_type = getppid();
					message.request = claim;
					message.RTLocation = location;
					message.release = 0;

					if(msgsnd(messageBoxID, &message, sizeof(message), 0) == -1){
						perror("User: failed to send message");
					}
				} else {
					message.mesg_type = getppid();
					message.request = 0;
					message.RTLocation = location;
					message.release = 0;
					
					if(msgsnd(messageBoxID, &message, sizeof(message), 0) == -1){
						perror("User: failed to send message");
					}
				}
			} else {	
				//else block select a resource to release
				//find a resource to release	
				//sets message.release to the resource to release. returns -1 if no resources are found
				message.release = FindResource(location, 0);
				
				if (message.release >= 0){
					message.mesg_type = getpid();
					message.RTLocation = location;
					message.request = 0;
					if(msgsnd(messageBoxID, &message, sizeof(message), 0) ==  -1){
						perror("oss: failed to send message to user");
					}
				} else {
					printf("No resources to release");	
				}	
			}			
	} //end outer while loop

} //end main
/*
int findResource(int allocated[][maxResources], int location){
	int index;
	for (index = 0; index < maxProcesses; index++){
		if (allocated[location][index] != 0){
			return index;
			break;
		} else {
			return -1;
		}
	}	

}
*/

int FindResource(int location, int value){
	int i = 0;

	while (i < maxProcesses && (*resourceTable).allocated[location][i] != value) ++i;
	return (i == maxProcesses ? -1 : i );
	}

void handle(int signo){
	if(signo == SIGINT){
		fprintf(stderr, "User process %d shut down by signal\n", getpid());
		exit(0);
	}
}



	

