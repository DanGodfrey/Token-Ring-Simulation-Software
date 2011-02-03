/*------------------------------------------------------------
File: stn.c

Description: This program implements the station process that
sends messages to another station process.  The content of the 
configuration file determines the station identifer (first data line)
and the identifier of the station to which messages are sent (second
data line).  Other data lines in the configuration files are
messages to be sent (lines starting with # or empty are ignored).

After each message is sent the station process waits for an 
acknowledgement (Ack message).  All communication is done using
the standard input and standard output.  The station process can still
print to the screen using the standard error. When the station process
receives a messages, it reponds by returning an acknowledgement.
-------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include "tokRing.h"
#include <string.h>

// Some definitions
#define MSGS_MAX 10 // Maximum number of messages
#define TRUE 1
#define FALSE 0
#define ACKNOWLEDGMENT "Ack"

// Prototypes
void readFile(FILE *, char *, char *, char *[], char *);
void communication(char, char, char *[]);

/*-------------------------------------------------------------
Function: main
Parameters: 
    int ac - number of arguments on the command line
    char **av - array of pointers to the arguments
Description:
   Evaluates the command line arguments and opens the configuration
   file.  The function readFile() configures the station/destination 
   identifiers and reads in the messages.  If no error is found in the
   configuration file, communication() is called to exchange messages
   with the other station processes.
-------------------------------------------------------------*/
int main(int ac, char **av)
{
   char dest;                   // destination identifier
   char idStn;                  // station identificatier
   char *messages[MSGS_MAX+1];  // array of pointers to messages - terminated with NULL
   char msgsBuffer[BUFSIZ];     // buffer of messages
   FILE *fp;

   if(ac != 2)
   {
       fprintf(stderr,"Usage: stn <fileName>\n");
   }
   else
   {
      fp = fopen(av[1],"r");
      if(fp == NULL) 
      {
        sprintf(msgsBuffer,"stn (%s)",av[1]);
        perror(msgsBuffer);
      }
      else
      {
         readFile(fp, &idStn, &dest, messages, msgsBuffer);
	 fclose(fp);
	 if(idStn != '\0' && dest != '\0') 
         { 
	   initTokenRing(idStn);
	   communication(idStn, dest, messages);
         } 
	 else fprintf(stderr,"File corrupted\n");
      }
   }
}
/*-------------------------------------------------------------
Function: readFile
Parameters: 
	fp	 - file pointeur
	idStnPt  - pointeur to return station identifier
	destPt	 - pointeur to return destination identifier
	msgs     - array of pointers to messages for transmission
	msgsBuf  - pointer to buffer to store messages
Description:
   Read all lines in the file. All empty lines and those starting with # are ignored.
   First line: use the first character as the station id
   Second line: use the first character as the destination id
   Other lines: are the messages.
   (care must be taken with inserting spaces in the file).
-------------------------------------------------------------*/
void readFile(FILE *fp, char *idStnPt, char *destPt, char *msgs[], char *msgsBuf)
{
    char line[BUFSIZ];   // for reading in a line from the file
    char *pt = msgsBuf;  // pointer to add messages to the message buffer msgsBuf
    int i = 0;           // index into the pointer array

    // Some initialization
    *idStnPt = '\0';  
    *destPt = '\0';
    msgs[i] = NULL;  // empty list
    while(fgets(line, BUFSIZ-1, fp) != NULL)
    {
       if(*line != '\n' && *line != '#' && *line != '\0')  // to ignore lines
       {
           if(*idStnPt == '\0') // found first line
	       *idStnPt = *line;  // get first character in the line
	   else if(*destPt == '\0') // found second line
	       *destPt = *line;  // get first character in the line
	   else // all other lines are messages to be saved
	   {
	      line[strlen(line)-1] = '\0';  // remove the \n at the end of the line
	      if(i < MSGS_MAX && strlen(line)+strlen(msgsBuf)+1 < BUFSIZ) // in order not to exceed limites 
	      {
                  strcpy(pt,line);        // copy messages into the buffer
		  msgs[i++] = pt;         // save its address in the array
		  msgs[i] = NULL;         // end the liste in the array with NULL
		  pt += strlen(line)+1;   // point to next free space in buffer; the +1 is for the '\0'
	      }
	   }
       }
    }
}

/*-------------------------------------------------------------
Function: communication
Parameters: 
	idStn    - station identifier
	dest	 - destination identifier
	messages - array of pointers to messages for transmission
Description:
   In a loop send the messages found in the array "messages". Between
   the transmission of each message wait for an acknowledgement (note
   that ackFlag ensures that an acknowldegement has been received
   before transmitting the next message).
   When a message is received, print to the screen (using standard
   error) the message and send an acknowledgement to the source
   of the message.
   The loop is broken when the standard input is closed (e.g.
   the write end of the pipe is closed) - this is detected by
   recvMessage().
   Note that recvMessage() blocks when the pipe attached to 
   the standard input is empty.
-------------------------------------------------------------*/
void communication(char idStn, char dest, char *messages[])
{
   int i=0;                // index for messages[]
   int ackFlag = TRUE;     // acknowledgement flag
   int flag;          // return flag from recvMessage()
   char source;            // source identificateur for received message/Ack
   char msg[BUFSIZ];       // buffer for received message

   // loop for transmission and reception
   do
   {
      // Message reception
      flag = recvMessage(&source, msg); 
      if(flag == MSG_EMPTY) /* do nothing */;  // no messages
      else if(flag == MSG_RECV)  // Received a message
      {
         if(strcmp(msg,ACKNOWLEDGMENT) == 0) 
         {  // received the acknowledgment
            if(source == dest)
	    {
	       ackFlag = TRUE;   
               fprintf(stderr,"Station %c (%d): Received from station %c an acknowledgement\n", 
                       idStn, getpid(), source, msg);
	    }
	    else fprintf(stderr, "Station %c (%d): received an Ack from %c - ignored\n",idStn,getpid(),source);
         } 
         else
         {     // Received a message - msg contains it, source gives id station that sent it
            fprintf(stderr,"Station %c (%d): Received from station %c >%s<\n", idStn, getpid(), source, msg);
	    xmitMessage(source,ACKNOWLEDGMENT);
         }
      }
      else // fatal or unknown error
         fprintf(stderr,"Station %c (%d): unknown value returned by recvMessage (%d)\n",idStn,getpid(),flag);

      // Transmission of messages 
      if(ackFlag && (messages[i] != NULL))
      {  // Send message
         xmitMessage(dest,messages[i]); 
         fprintf(stderr,"Station %c (%d): Sent to station %c >%s<\n",idStn,getpid(),dest,messages[i]);
         ackFlag = FALSE;            // becomes TRUE at the arrival of an ack
	 i++;                        // points to next message for next time
      }
      flag = monitorTokenRing();  
   } while(flag != FINISH); 
}
