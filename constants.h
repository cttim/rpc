#ifndef __CONSTANTS__
#define __CONSTANTS__

#define BYTESIZE 4
#define FUNCNAME 64			//skeleton func length

#define REGISTER 20
#define REGISTER_SUCCESS 21
#define REGISTER_FAILURE 22

#define LOC_REQUEST 30
#define LOC_SUCCESS 31
#define LOC_FAILURE 32

#define EXECUTE 40
#define	EXECUTE_SUCCESS 41
#define EXECUTE_FAILURE 42
#define TERMINATE 100
#define TERMINATESUCCESS 101
#define SERVERIDENTIFY 1000

//define error code
#define ERRORBINDERINFO -50
#define ERRORBINDERCONNECT -51
#define NOSERVER -52
#define ERRORSENDREG -53
#define ERRORSENDTERM -54
#define DBEMPTY -55
#define ERRORSENDLOCREQUEST -56
#define WHAT -57

class Message{
 public:
        int fd;
	int len;
	int type;
	char *data;
};


//here is helper function
char *intToChars(int integer);
int charsToInt(char *byte);
void intToChars(int i, char* buffer);
int getargtypelen(int *argTypes);
int getargslen(int *argTypes);
int getrealtype(int argtype);
int getreallength(int argtype);
Message * wraplocrequest(char *name, int *arg_types);
Message * wrapexecutemessage(char *name, int *argTypes, void **args);
void ** unwrapexecutemessage(int *argTypes, char *data);
#endif

//serversocket.cc: getmessage
//rpc.cc: execute 
//rpc.cc: work
//done

//start client side
//rpcCall
