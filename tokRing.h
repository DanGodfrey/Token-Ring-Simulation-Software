/*----------------------------------------------
File: tokRing.h
Description: Header file for token ring interface
             module.  Provides prototypes and
	     definitions for using the module.
-----------------------------------------------*/

// Definitions
#define FINISH 1
#define MSG_TOK 2
#define MSG_EMPTY 3
#define MSG_RECV 4
#define MSG_STN 5   

// Prototypes
void initTokenRing(int);
void xmitMessage(char, char *);
int recvMessage(char *, char *);
int monitorTokenRing(void);


