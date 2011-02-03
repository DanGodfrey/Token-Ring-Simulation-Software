/*------------------------------------------------------------
File: hub.c   (CSI3131 Assignment 1)

Student Name:

Student Number:

Description:  This program creates the station processes
     (A, B, C, and D) and then acts as token ring hub.
-------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#define OK 1
#define PROGRAM_STN "./stn"  // The program that acts like a station
#define MAX_STNS 10        // Maximum number of stations
// Note that the terms reception and transmission are relatif to the station and not the hub
// Note that the descriptors at the same index in the two arrays are related to the adjacent stations,
// for example, fdsRec[2] and fdsTran[2] contain the fds of the pipes connected to adjacent stations
//                                       in the ring.
int fdsRec[MAX_STNS+1];    // file descriptors for writing ends (reception)
int fdsTran[MAX_STNS+1];   // file descriptors for reading ends (transmission)
/* Prototypes */
void createStation(char *);
void hubThreads();
void *listenTran(void *);

/*-------------------------------------------------------------
Function: main
Parameters:
    int ac - number of arguments on the command line
    char **av - array of pointers to the arguments
Description:
    Creates the stations using createStation() and threads using
    using hubThreads().  hubThreads() cancels
    (terminates) the threads after 30 seconds.
-------------------------------------------------------------*/
int main(int ac, char **av)
{
   int ix;      // Array index
   int first;   // First entry

   // Initialization
   fdsRec[0] = -1; // empty list
   fdsTran[0] = -1; // empty list
   
   // Creating the stations
   createStation("stnA.cfg");
   createStation("stnB.cfg");
   createStation("stnC.cfg");
   createStation("stnD.cfg");
   // Shift the entries by one in fdsRec
   first = fdsRec[0];
   for(ix = 0; fdsRec[ix+1] != -1 ; ix++){
       fdsRec[ix] = fdsRec[ix+1];
   }
   fdsRec[ix] = first;  
 
  // fprintf(stderr,"rxfds = {%d,%d,%d,%d}\n",fdsRec[0],fdsRec[1],fdsRec[2],fdsRec[3]);
  // fprintf(stderr,"txfds = {%d,%d,%d,%d}\n",fdsTran[0],fdsTran[1],fdsTran[2],fdsTran[3]);	
  // fflush(stderr); 
// creating threads for the hub
   hubThreads();  
   // On return from the function - all threads are terminated.
   // When the hub process terminates, all write ends of the reception pipes
   // are closed, which should have the stations terminate.
   return(0);  // All is done.
}

/*-------------------------------------------------------------
Function: createStation
Parameters:
    fileConfig - refers to the name of the configuration file
Description:
    Creates a station process (stn) which acts like a station
    according to the content of the configuration file (see stn.c).
    Must create 2 pipes and organize them as follows:
    Transmission pipe:  The write end is attached to the standard
                        output of the station process (stn).
			The read end remains attached to the hub
			process and its file descriptor is stored
			in fdsTran (at its end).
    Reception  pipe:  The read end is attached to the standard
                      input of the station process (stn).
		      The write end remains attached to the hub
		      process and its file descriptor is stored
		      in fdsRec (at its end).
    Note that the fds of the pipes in the fdsTran and fdsRec arrays
    are stored at the same index during the creation of the stations.
    All fds not used in both the station and hub processes are closed.
-------------------------------------------------------------*/
void createStation(char *fileConfig)
{
	int txfd[2],rxfd[2], pid, ret; //initiate variables 
	ret = pipe(txfd); //Create rx pipe 
	if (ret == -1){ //Make sure pipe was created successfully 
		fprintf(stderr,"Failed to create tx pipe for stn"); //errorz
		exit(-1);
	}
	ret = pipe(rxfd); //Create tx pipe 
	if (ret == -1){ //Make sure pipe was created successfully 
		fprintf(stderr,"Failed to create rx pipe for stn"); //errorz
		exit(-1);
	}		
	pid = fork(); //fork another process
	if (pid < 0) { //Make sure fork was successful
		fprintf(stderr, "Failed to fork a child for stn");//errorz
		exit(-1);
	}
	else if (pid == 0){ // child	
		dup2(rxfd[0],0); //attach to stdin
		dup2(txfd[1],1); //attach to stdout
	
		execlp(PROGRAM_STN, "stn",fileConfig,NULL);	
	} 
	else { //Parent
		int i;
		for(i = 0; fdsRec[i] != -1;i++){
		
		}
		if (i < MAX_STNS){
			fdsRec[i] = rxfd[1];
			fdsRec[i+1] = -1;
			fdsTran[i] = txfd[0];
			fdsRec[i+1] = -1;
		}
		else {
			fprintf(stderr, "Failed to create station. Limit Reached.");//errorz
			exit(-1);
		}
	}
	close(rxfd[0]);
	close(txfd[1]);
}

/*--------------------------------------------------------------
Function: hubThreads
Description:
   Create 4 threads to listen on each T-pair pipe (i.e to the
   fd's in fdsTran) with the function listenTran.
   Once the threads have been created, sleep to allow all messages
   to be exchanged and then cancel (terminate) the threads.
--------------------------------------------------------------*/
/* Complete this function */
void hubThreads()
{
   	// Declaration of variables
	pthread_t tid[4];
	pthread_attr_t attr;
	int i;
	int params[4][2]; // 2 Dimensional Array to store fd points
	
   // Creating hub threads
	pthread_attr_init(&attr);
	
	for(i = 0; i < 4; i++){
		params[i][1] =  fdsRec[i]; //add rec fd to params
		params[i][0] =  fdsTran[i]; //add tran fd to params
		pthread_create(&tid[i],&attr,listenTran,params[i]); //create thread
	}

   // Write Token to a pipe
	char buf[10];
	strcpy(buf, "^"); //send token symbol
	write(fdsRec[0],buf,1);
	
   // Sleep a bit
   sleep(15);

   // Cancel the threads
	for(i= 0; i < 4; i ++){
		pthread_cancel(tid[i]); // bye-bye
	}
}

/*-------------------------------------------------------------------
Function: listenTran

Description: 
   This function runs in a thread an listens on fdListen file descriptor,
   to a station process on a transmission pipe (station transmits).  
   When data is read from the pipe, it is copied into reception pipe 
   of the adjacent station using the fdSend file descriptor.
-------------------------------------------------------------------*/
void *listenTran(void *fdPairPtr)
{
   int *fdPair = (int *) fdPairPtr;
   int fdListen = fdPair[0];     // Get the fd on which to listen
   int fdSend = fdPair[1];      // Get the fd on which to send
	int num;                      // value returned by read (num of bytes read)
   char buffer[BUFSIZ];          // buffer for reading data
   int i;
  
   while(1)  // a loop
   {
     num = read(fdListen,buffer,BUFSIZ-1);
     if(num == -1) // error in reading 
     {
        sprintf(buffer,"Fatal error in reading on fd %d (%d)",fdListen,getpid());
	perror(buffer);  			/* writes on standard error */
	break;  				/* break the loop */
     }
     else if(num == 0) /* other end of pipe closed - should not happen */
     {
	sprintf(buffer,"Pipe closed (%d)\n",getpid());
        write(2,buffer,strlen(buffer)); 	// write to standard error
	break;  				/* break the loop */
     }
     else // write into the R-pair pipe
     {
       buffer[num] = '\0';  			// terminate the string
       // Following lines can be used for debugging
       //printf("hub: received frame from %d >%s<\n",fdListen,buffer);
       //printf("hub: transmitting frame to %d >%s<\n",fdsRec[i],buffer);
       write(fdSend,buffer,strlen(buffer));
     }
   }
}
