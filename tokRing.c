/*------------------------------------------------------------
File: tokRing.c

Description:
This program implements the  interface to the token ring network.
It monitors the network as follows:
1) Wait for reception from the network on the R-pair pipe 
   (the standard input, fd=0).
2) If a frame destined for the station write it to the rxBuf buffer.
3) Retransmit the frame on the T-pair pipe (standard output fd 1).
4) If the frame received is the token and txBuf empty, write token 
   to the T-pair pipe.
5) If the frame received is the token and txBuf not empty, write 
   the frame in the txBuf to the T-pair pipe.
6) If the received frame source address is the station's address, 
   write the token on the T-pair pipe.
7) If the destination address of a received frame is the station's 
   address, the frame is written to rxBuf.
The standard error can be used to write messages to screen.
-------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include "tokRing.h"
#include <string.h>

// Some definitions
// for frames
#define SYN '^'       // SYN character - the Token
#define STX '@'       // Start of the frame - start of Xmission
#define ETX '~'       // End of the frame - end of Xmission
#define STX_POS 0     // Position of STX
#define DST_POS 1     // Position of the destination identifier
#define SRC_POS 2     // Position of the source identifier
#define MSG_POS 4     // Position of the frame

//********************** Global variables *****************/
// Message Buffers - Set large enough not to worry about overflow
// Buffer rules:
//     Buffer is empty when first character is nul character '\0'
//     Frames are stored in the buffers
//     Can use extractMsg to remove a message from the buffer
//     Can use strcat() to append a frame to the buffer.
char rxBuf[2*BUFSIZ];  
char txBuf[2*BUFSIZ];  
// This station Identifier
char  stnId;
/*****************************/

// Local Function Prototypes
int readMsg(char *, char *, char *);
int extractMsg(char *, char *, char *, char *);

/*-------------------------------------------------------------
Function: initTokenRing
Parameters: int id - Station identifier.
Returns: Nothing.
Description:
   Sets up the global variables.
-------------------------------------------------------------*/
void initTokenRing(int id)
{
   int tid;

   stnId = id;  // The station identifier
   // Ensure buffers are empty
   // Ensure buffers are empty
   rxBuf[0] = '\0';
   txBuf[0] = '\0';
}

/*-------------------------------------------------------------
Function: xmitMessage
Parameters: char dest - destination of message 
            char *msg - message string to send
Returns: nothing
Description:
   Creates a frame an appends it to the end of txBuf.
-------------------------------------------------------------*/
void xmitMessage(char dest, char *msg)
{
    char frame[BUFSIZ];
    sprintf(frame,"%c%c%c-%s%c",STX,stnId,dest,msg,ETX); // create frame
    strcat(txBuf, frame);
}

/*-------------------------------------------------------------
Function: recvMessage
Parameters: char *source - for returning the source
            char *msg - message string 
Returns: Return value from extractMsg which can be
           MSG_EMPTY - buffer is empty.
           MSG_RECV - message was found.
Description:
    Remove a frame from rxBuf if possible.
-------------------------------------------------------------*/
int recvMessage(char *source, char *msg)
{
    char dest;
    return(extractMsg(rxBuf,msg,source,&dest));
}

/*-------------------------------------------------------------
Function: monitorTokenRing
Parameters: none
Returns:  FINISH - link to LAN broken, MSG_STN - message
          has been received for the station.
Description:
   Monitors the token ring LAN as described at the beginning
   of this file.

   The loop is broken when the standard input is closed (e.g.
   the write end of the pipe is closed) - this is detected by
   readMsg(). In this case the function returns.

   The function returns when a message destined for the station
   is received, i.e. the rxBuf has been updated.  

   Note that readMsg() blocks when the pipe attached to 
   the standard input is empty.
-------------------------------------------------------------*/
int monitorTokenRing()
{
   int i=0;                // index for messages[]
   int flag;               // return flag from readMsg()
   char source;            // source identifier for received message/Ack
   char dest;              // dest identifier for received message/Ack
   char msg[BUFSIZ];       // buffer for received message
   char frame[BUFSIZ];     // for building frames

   // loop that monitors network
   // readMsg blocks when pipe is empty.
   // flag set to FINISH by readMsg when pipe is closed
   do
   {
      flag = readMsg(msg, &source, &dest);
      // Transmitting message
      if(flag == MSG_TOK) // token was received - note msg, source, and dest are meaningless
      {  
         if(extractMsg(txBuf,msg,&source,&dest) == MSG_EMPTY)  // no frames to Xmit
            sprintf(frame,"%c",SYN);
	 else
            sprintf(frame,"%c%c%c-%s%c",STX,source,dest,msg,ETX); // create frame
         write(1,frame,strlen(frame));   // writes to the standard output, i.e. pipe
      }
      // Reception de messages 
      else if(flag == MSG_RECV) 
      {     // Received a message - msg contains it, source gives id station that sent it
         if(source == stnId) // frame sent by this station - need to release token
            sprintf(frame,"%c",SYN);
	 else 
	 {
             sprintf(frame,"%c%c%c-%s%c",STX,dest,source,msg,ETX); // create frame
	     if(dest == stnId) 
             { 
	       strcat(rxBuf, frame);  // save copy if for this station
	       flag = MSG_STN;  // To return so that received message can be processed
             } 
	 }
         write(1,frame,strlen(frame));
      }
      else if(flag == FINISH) /* do nothing */;
      else // fatal or unknown error
         fprintf(stderr,"Station %c (%d): unknown value returned by readMsg (%d)\n",stnId,getpid(),flag);

   } while( (flag != FINISH) && (flag != MSG_STN));

   return(flag);
}

/*-------------------------------------------------------------
Function: readMsg
Parameters: 
	msg	  - pointer to buffer for storing received message (frame contents)
	sourcePtr - pointer to a character to receive source identifier
	destPtr   - pointer to a character to receive destination identifier
Description:
    Reads one or more frames from the standard input (i.e. pipe) and stores
    them in buffer (allframes).  If the standard input is closed, return FINISH. 
    Note that the buffer allFrames is static and does not disappear between calls
    to the function.

    If frames have been received, call extractMsg() to extract the
    first message; it returns MSG_TOK if a token is found or
    MSG_RECV if a message is found (message shall be copied to the buffer
    referenced by msg).  Messages are removed from allFrames by 
    extractMsg().  Once allFrames is empty, fill it again from the 
    standard input.

    Thus this function scans the standard input for messages until it
    finds one destined for station process stnId or until the standard input
    is closed.

    See extractMsg() for frame format.
-------------------------------------------------------------*/
int readMsg(char *msg, char *sourcePtr, char *destPtr)
{
   static char allFrames[BUFSIZ];  // all frames read from pipe - static buffer
   int ret;			   // value returned by this function
   int retRead;			   // to store value returned by read and extractMsg function
   char errorMsg[BUFSIZ];         // buffer to build error messages
   
   while(1) // Loop to find a message
   {
      if(*allFrames == '\0') // buffer empty - need to read from the pipe
      {
        retRead = read(0,allFrames,BUFSIZ-1); // blocks when pipe is empty
	if(retRead == -1) 
	{
            sprintf(errorMsg,"Station %c (%d): reading error",errorMsg,stnId);
            perror(errorMsg);
	}
	else if(retRead == 0) // write end of pipe has been closed
	{
	    ret = FINISH;
	    break;  // break out of loop
	}
	// messages are in the buffer
	allFrames[retRead]='\0';  // terminate the string
      }
      // The following line can be used for debugging
      //fprintf(stderr,"Station %c (%d): readMsg >%s<\n", stnId, getpid(), toutesTrames);
      retRead = extractMsg(allFrames, msg, sourcePtr, destPtr);
      if(retRead != MSG_EMPTY) // if MSG_EMPTY, no messages were found in the buffer 
      {
          ret = retRead;  // is MSG_TOK or MSG_RECV
	  break;
      }
      // if we get here, need to read from the pipe again
   }
   return(ret);
}

/*------------------------------------------------
Function: extractMsg

Parameters:
    frameBuf 	- points to buffer of received frames
    msg		- points to buffer to receive a message
    sourcePtr   - to return the identifier of the source 
    destPtr     - to return the identifier of the destination 

Description: 
     Extracts a message from the buffer referenced by frameBuf.
     The message is removed and copied to the buffer referenced
     by msg. Frames with improper destination id are skipped.

     Message format: STX D S <message> ETX
                     SYN - the token
     D gives the ident. of the destination station. 
     S gives the ident. of the station that sent the message 
     <message> - string of characters
     If STX is missing, print an error and skip the message.
------------------------------------------------*/
int extractMsg(char *frameBuf, char *msg, char *sourcePtr, char *destPtr)
{
   char *pt=frameBuf;       // pointer to navigate the buffer of all frames
   int retcd = MSG_EMPTY;  // return value 

   while(1) // find a message for this station
   {
      if(*pt == '\0') // no messages
      {
         retcd = MSG_EMPTY;
	 *frameBuf = '\0';  // empties the all frames buffer - to deal with corruption
         break; // break the loop
      }
      else if(*pt == SYN) // found the token
      {
         retcd = MSG_TOK;
	 pt++;                    // skip the SYN
	 strcpy(frameBuf,pt);      // move unread frames to the start of the buffer
	 break;
      }
      else if(*pt != STX) // found an error - no STX
      {
	  fprintf(stderr,"stn(%c,%d): no STX: >%s<\n",stnId,getpid(),pt);
          while(*pt != ETX && *pt != STX && *pt != '\0') pt++;  // skip until the end or beginning 
	  if(*pt == ETX) pt++; 					// skip the ETX
      }
      else // found a message
      {
         *sourcePtr = *(pt+SRC_POS);                       // to return the source ident.
         *destPtr = *(pt+DST_POS);                         // to return the destination ident.
         retcd = MSG_RECV;
	 pt = pt+MSG_POS;                                  // point to the message
         while(*pt != ETX && *pt != '\0') *msg++ = *pt++;  // copy message into buffer
	 if(*pt == ETX) pt++;                              // skip the ETX
	 *msg = '\0';                                      // terminate the string
	 strcpy(frameBuf,pt);                               // move unread frames to the start of the buffer
	 break;
      }
   }
   return(retcd);
}
