#include "oss.h"

unsigned int * simClock;
descriptor *resourceTable;


int findResource(int location, int value);
int main (int argc, char *argv[]){
	printf("USER %d has launched\n", getpid());
	
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
	//semaphoreKey = 59567;
	descriptorKey = 59568;
	//Get Shared Memory
	if ((shmidSimClock = shmget(keySimClock, (2*(sizeof(unsigned int))), 0666)) == -1){
		perror("user: could not get shmidSimClock\n");
		return 1;
	}
	if ((shmidDescriptor = shmget(descriptorKey, (sizeof(descriptor)), 0666)) == -1){
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

	int location = atoi(argv[1]);
	//save location in the resource table to the process structure
	(*resourceTable).process[location].RTLocation = location;
	//copy maxCLaims to the process structure
	int loop;
	for(loop = 0; loop < maxResources; loop++){
		(*resourceTable).process[location].maxClaimsAllowed[loop] = (*resourceTable).maxCanRequest[location][loop];
	}

	int counter = 0;
	bool requestWasGranted = false;
	int index; //for incrementing in loops

	int claim;
	
	//start as not blocked
	(*resourceTable).process[location].blocked = false;

	//while loop runs contiously but on first run, the IF statement fails
	//and then suspends on waiting for a message. 
	//message received and then can process the IF statement
	while (1){
	if ((*resourceTable).process[location].blocked == false){ 
		int willRequest = (rand() % 100) + 1;

//		int willRequest = 100;
		if (requestWasGranted == true){				
			int willTerminate = (rand() % 100) + 1;
			if (willTerminate >= randomTerminateConstant){
				printf("USER %d is going to terminate\n", getpid());
				//notify OSS that process is terminating
				message.terminate = true; 
				message.mesg_type = 5;
				//set location of process in resourceTable in message to OSS
				//zero out allocated resource
				//send message to OSS
				if(msgsnd(messageBoxID, &message, sizeof(message), 0) ==  -1){
					perror("oss: failed to send message to user");
				}
				//process exits
				exit(EXIT_SUCCESS); 
			}
			requestWasGranted = false; //change flag back to false
		} //end if(requestWasGranted == true)  block	
 
		if (willRequest >= randomRequestConstant){	
				claim = rand () % 20;
				int newAllocation =  1 + (*resourceTable).allocated[location][claim];
				
				if (newAllocation <= (*resourceTable).process[location].maxClaimsAllowed[claim]){
					(*resourceTable).process[location].resourceRequest = claim; //save resource requested
					message.mesg_type = 5;
					message.RTLocation = location;
					message.release = 0;
					message.request = 1;
					message.pid = myPID;

					printf("REquested %d\n", (*resourceTable).process[location].resourceRequest);
					printf("message type is%d\n", message.mesg_type);
					printf("%d\n", message.RTLocation);
					printf("%d\n", message.release);
					if(msgsnd(messageBoxID, &message, sizeof(message), 0) == -1){
						perror("User: failed to send message");
					//prevent user from requesting more resources until its granted previous request
					(*resourceTable).process[location].blocked = true;
				//	break;
					}
				} else {
					message.mesg_type = 5;
					message.RTLocation = location;
					message.release = 0;
					message.request = 0;
				}
			} else {	
				//else block select a resource to release
				//find a resource to release	
				//sets message.release to the resource to release. returns -1 if no resources are found
				int releasedResource = findResource(location, 0);
				
				if (releasedResource >= 0){
					(*resourceTable).process[location].resourceRelease = releasedResource;
					message.mesg_type = 5;
					message.RTLocation = location;
					message.request = 0;
					message.release = releasedResource;
					message.pid = myPID;
					if(msgsnd(messageBoxID, &message, sizeof(message), 0) ==  -1){
						perror("oss: failed to send message to user");
					}
				} else {
					printf("No resources to release");	
				}	
			}			
	
	msgrcv(messageBoxID, &message, sizeof(message), myPID, IPC_NOWAIT);
	requestWasGranted = message.granted;
	} //end inner while loop
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

int findResource(int location, int value){
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



	

