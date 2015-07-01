#include "rpc.h"
#include "serversocket.h"
#include "clientsocket.h"
#include "io.h"
#include "constants.h"
#include <string>
#include <iostream>
#include <sstream>
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>


using namespace std;

class Threaddata{
	public:
	char * name;
	int *arg_type;
	void **args;
	skeleton f;
	int fd;
	Message *m;
};

struct ProcedureInfo;
bool operator==(ProcedureInfo& p1, ProcedureInfo& p2);

struct ProcedureInfo {
  char name[65];
  int argType[64];
  skeleton s;
  ProcedureInfo() {
    memset(name, 0, 65*sizeof(char));
    memset(argType, 0, 64*sizeof(int));
  }
  friend bool operator==(ProcedureInfo& p1, ProcedureInfo& p2);
};

bool operator==(ProcedureInfo& p1, ProcedureInfo& p2) {
  bool nameB = !(strcmp(p1.name, p2.name));
    
  const unsigned int* iter1 = (const unsigned int *)p1.argType;
  const unsigned int* iter2 = (const unsigned int *)p2.argType;


  while (*iter1 != 0 && *iter2 != 0 ) {
    if (*iter2 != *iter1) {
      int mask = 0xffff;
      int high1 = (*iter1) & ~mask;
      int high2 = (*iter2) & ~mask;
      int low1 = (*iter1) & mask;
      int low2 = (*iter2) & mask;
      if (!(high1 == high2 && low1 && low2)) {
        break;
      }
    }
    iter1++;
    iter2++;
  }
  bool argB;
  if (*iter1 == 0 && *iter2 == 0) {
    argB = true;
  } else {
    argB = false;
  }
  return argB && nameB;
}


//server socket for listening
serversocket *ssock = NULL;
clientsocket *csock = NULL;
//binder
struct hostent *binderhost;
struct sockaddr_in binderaddress;
string binderaddr;
string binderportstring;
int binderport;
int binderfd;

//local db
vector <ProcedureInfo> skeletondb;
//vector<pthread_t> worklist;

int getbinderinfo(){
	char *addr = getenv("BINDER_ADDRESS");
	char *port = getenv("BINDER_PORT");
	if(addr == NULL){
		
		return 1;
	}
	if(port == NULL){
		
		return 1;
	}

	binderaddr = string(addr);
	binderportstring = string(port);
	stringstream ss(port);
	ss>>binderport;

	return 0;
}


int connecttobinder(){
	int sockfd;
	binderhost = gethostbyname(binderaddr.c_str());

	if(binderhost == NULL){
		cerr<<"can not resolve binderhost"<<endl;
		return 1;
	}

	memcpy(&binderaddress.sin_addr, binderhost->h_addr_list[0], binderhost->h_length);
	binderaddress.sin_family = AF_INET;
	binderaddress.sin_port = htons(binderport);

	binderfd = socket(AF_INET, SOCK_STREAM, 0);

	if(binderfd == -1){
		cerr<<"merror make socket"<<endl;
		return 1;
	}

	if(connect(binderfd, (struct sockaddr*)&binderaddress, sizeof(binderaddress))){
		cerr<<"connect error"<<endl;
		return 1;
	}

	return 0;
}

int sendregisterdata(char *name, int *argTypes){
	
	char servername[INET_ADDRSTRLEN];
	char funcname[FUNCNAME];
	int portnum = ssock->getport();
	strcpy(servername, ssock->getaddress().c_str());
	strcpy(funcname, name);

	//caculate the length of argTypes
	int argtypelen=0;
	for(argtypelen=0; argTypes[argtypelen] != 0; argtypelen++){

	}
	argtypelen++;

	//caculate the total message length
	int messagelen = sizeof(servername) + sizeof(portnum) + sizeof(funcname) + argtypelen * BYTESIZE;

	
	char *datamsg = new char[messagelen+1];

	//convert messagelen, type, and portnum into char
	
	char charport[BYTESIZE];
	intToChars(portnum,charport);
	
	
	//put all message into datamsg
	int index = 0;
	for(index=0; index<BYTESIZE; index++){
		
		datamsg[index] = charport[index];
	}

	//add server ip name
	for(int i=0; i<INET_ADDRSTRLEN; i++){
	  datamsg[index] = servername[i];
		index++;
	}

	//add funcname
	for(int j=0; j<FUNCNAME; j++){
		datamsg[index] = funcname[j];
		index++;
	}

	//add argTypes
	for(int k=0; k<argtypelen; k++){
		char charargtype[BYTESIZE]; 
		intToChars(argTypes[k], charargtype);
		for(int l=0; l<BYTESIZE; l++){
			datamsg[index] = charargtype[l];
			index++;
		}
	}
	
	

	if(sendMessage(binderfd, REGISTER, datamsg, messagelen)){
		cerr<<"in func sendregisterdata: error sendMessage"<<endl;
		return 1;
	}
	
	delete [] datamsg;
	return 0;

}

void *work(void *workthreaddata){
	//get the data
	
	Threaddata *data = (Threaddata*) workthreaddata;
	char *name = data->name;
	
	int *arg_type = data->arg_type;
	void **args = data->args;
	skeleton f = data->f;
	int fd = data->fd;
	//do the func
	
	int result = f(arg_type, args);
	
	
	if(result < 0){
		
		char reasonbuf[4];
		intToChars(result, reasonbuf);
		sendMessage(fd, EXECUTE_FAILURE, reasonbuf, 4);
		return (void *) NULL;
	}
	
	int argtypelen = getargtypelen(arg_type);
	int argslen = getargslen(arg_type);
	int length = FUNCNAME+4*argtypelen + argslen;
	char * buf = new char[length];
	int bufindex = 0;
	
	
	//copy func name
	int namelen = strlen(name);
	if(namelen>FUNCNAME){
		namelen = FUNCNAME;
	}
	for(int i=0; i<FUNCNAME; i++){
		buf[bufindex] = name[i];
		bufindex++;
	}
	

	//copy argtype
	char argtypebuf[BYTESIZE];
	for(int j=0; j<argtypelen; j++){
		intToChars(arg_type[j], argtypebuf);
		for(int k=0; k<BYTESIZE; k++){
			buf[bufindex] = argtypebuf[k];
			bufindex++;
		}
		//delete [] argtypebuf;
		//argtypebuf = NULL;
	}
	
	int nbytes=0;
	for(int l=0; l<argtypelen -1; l++){
		int realtype = getrealtype(arg_type[l]);
		int reallen = getreallength(arg_type[l]);
		if(reallen == 0){
			reallen = 1;
		}
		switch(realtype){
			case ARG_CHAR:
            			nbytes = sizeof(char)*reallen;
            			break;
          		case ARG_SHORT:
            			nbytes = sizeof(short)*reallen;
            			break;
          		case ARG_INT:
            			nbytes = sizeof(int)*reallen;
            			break;
          		case ARG_LONG:
            			nbytes = sizeof(long)*reallen;
            			break;
          		case ARG_DOUBLE:
            			nbytes = sizeof(double)*reallen;
            			break;
          		case ARG_FLOAT:
            			nbytes = sizeof(float)*reallen;
            			break;
		}
		
		memcpy(buf+bufindex, args[l], nbytes);
		bufindex = bufindex+nbytes;
	}
	
	 
	
	if(sendMessage(fd, EXECUTE_SUCCESS, buf, length)){
		cerr<<"in func work: error sendMessage"<<endl;
	}
	
	//delete [] data->m->data;
	//delete  data->m;
	delete [] buf;
	delete [] arg_type;
	delete [] args;
	delete data;
}


int execute(Message *m){
	
	
	string funcname = (string(m->data)).substr(0, FUNCNAME);

	//map<string,skeleton>::iterator it;
	//it = skeletondb.find(funcname);
	//skeleton f = it->second;
	 
	//get all argtypes
	int argindex = FUNCNAME;
	char *argbuf = new char[BYTESIZE];
	vector<int> argtype;
	while(true){
		for(int i=0; i<BYTESIZE; i++){
			argbuf[i] = m->data[argindex++];
		}
		if(charsToInt(argbuf) == 0){
			
			argtype.push_back(charsToInt(argbuf));
			break;
		}
		
		argtype.push_back(charsToInt(argbuf));
	}
	delete [] argbuf;
	argbuf = NULL;
	int *argtypes = new int [argtype.size()];
	for (int i = 0; i < argtype.size(); i++) {
		
        	argtypes[i] = argtype[i];
    	}

	ProcedureInfo info;
	strcpy(info.name,funcname.c_str());
	int argSize = argtype.size();
	memcpy(info.argType, argtypes, (argSize > 64)? 64*4: argSize*4);

	skeleton f;
	vector<ProcedureInfo>::iterator it;
	for (it = skeletondb.begin(); it != skeletondb.end(); it++) {
		if (*it == info) {
			f = it->s;
			break;
		}
	}
    	
	
	//get all args
	void **args = new void *[(argtype.size() -1) * sizeof (void*)];
	char *arg = NULL;
	int nbytes;
	
	for(int i=0; i<argtype.size() - 1; i++){
		int argrealtype = getrealtype(argtype[i]);
		int length = getreallength(argtype[i]);
		
		if(length == 0){
			length = 1;
		}

		switch (argrealtype) {
          		case ARG_CHAR:
            			nbytes = sizeof(char)*length;
            			break;
          		case ARG_SHORT:
            			nbytes = sizeof(short)*length;
            			break;
          		case ARG_INT:
            			nbytes = sizeof(int)*length;
            			break;
          		case ARG_LONG:
            			nbytes = sizeof(long)*length;
            			break;
          		case ARG_DOUBLE:
            			nbytes = sizeof(double)*length;
            			break;
          		case ARG_FLOAT:
            			nbytes = sizeof(float)*length;
            			break;
        	}

        	arg = new char[nbytes];

        	for(int j=0; j<nbytes; j++){
        		arg[j] = m->data[argindex++];
        	}

        	args[i] = (void *) arg; 
	}
	//finished get all args

	//start to prepare threaddata;
	Threaddata *workthreaddata = new Threaddata();
	
	workthreaddata->name = (char*) funcname.c_str();
	workthreaddata->arg_type = argtypes;
	workthreaddata->args = args;
	workthreaddata->f = f;
	workthreaddata->fd = m->fd;
	workthreaddata->m = m;
	//finished
	
	//start to work
	pthread_t thread;
	pthread_create(&thread, NULL, &work, (void *) workthreaddata);
	return 0;
}


int serverdowork(Message *m){
	switch (m->type){
		case EXECUTE:
			return execute(m);
		case TERMINATE:
			cout<<"should not get here"<<endl;
			break;
		default:
			cout<<"got weird message"<<endl;
			return 0;
	}
}


int requesttoserver(Message *m, char *name, int* argTypes, void** args){
	Message *send_m;
	Message *reply_m;
	int index =0;
	int port;
	//get addr and port num
	
	char portbuf[BYTESIZE];
	strncpy(portbuf, m->data, BYTESIZE);
	port = charsToInt(portbuf);	
	
	index = index+4;
	char addr[INET_ADDRSTRLEN];
	strncpy(addr, m->data+index, INET_ADDRSTRLEN-1);
	addr[INET_ADDRSTRLEN] = '\0';
	
	//convert port to stringport
	string portstring;
	ostringstream convert;
	convert<<port;
	portstring = convert.str();	
	
	//connect to server
	if(csock->makeconnection(addr, portstring)){
		cerr<<"in func requesttoserver: error makeconnection"<<endl;
		return 1;
	}
	//create execute message
	send_m = wrapexecutemessage(name, argTypes, args);
	
	//send the message and get reply
	if(csock->sendmessage(send_m)){
		cerr<<"in func requesttoserver: error sendmessage"<<endl;
		return 1;
	}
	
	reply_m = csock->getmessage();
	
	void **returnargs = NULL;
	switch(reply_m->type){
		case EXECUTE_SUCCESS:
			returnargs = unwrapexecutemessage(argTypes, reply_m->data+FUNCNAME+getargtypelen(argTypes) * BYTESIZE);
			for(int i=0; i<getargtypelen(argTypes) - 1; i++){
				args[i] = returnargs[i];
			}
			return 0;
		case EXECUTE_FAILURE:
			return charsToInt(reply_m->data);
		default:
			return 1; 
	}
	

}

int rpcInit(){

	//make a new socket to listen
	ssock = new serversocket(100);
	//print the address and port
	cout<<"SERVER_ADDRESS "<<ssock->getaddress()<<endl;
	cout<<"SERVER_PORT "<<ssock->getport()<<endl;


	// connect to the binder
	if(getbinderinfo()){
		cerr<<"error getting binder info"<<endl;
		return ERRORBINDERINFO;
	}

	if(connecttobinder()){
		cerr<<"error connecting to binder"<<endl;
		return ERRORBINDERCONNECT;
	}
	
	char servername[INET_ADDRSTRLEN];
	char portbuf[BYTESIZE];
	int portnum = ssock->getport();
	intToChars(portnum, portbuf);
	strcpy(servername, ssock->getaddress().c_str());
	char buf[20];
	for(int i=0; i<BYTESIZE; i++){
		buf[i] = portbuf[i];
	}
	for(int j=0; j<INET_ADDRSTRLEN; j++){
		buf[j+4] = servername[j];
	}	
	
	sendMessage(binderfd, SERVERIDENTIFY, buf, 20);
	
	return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f){
	
	if(ssock == NULL){
		cerr<<"in func rpcRegister: no serversocket exist"<<endl;
		return NOSERVER;
	}

	if(sendregisterdata(name, argTypes)){
		cerr<<"in func rpcRegister: error sendregisterdata"<<endl;
		return ERRORSENDREG;
	}

	//need to confirm msg from server implement!!!!!!!!!!!!!!!!!
	int type;
	int size = 20000;
	char buf[20000];
	recvMessage(binderfd, &type, buf, &size);
	//cout<<"in rpcregister received"<<endl;

	//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	ProcedureInfo key;
	strcpy(key.name, name);
	int argSize = getargtypelen(argTypes);
	memcpy(key.argType, argTypes, (argSize>64)? 64*4: argSize*4);
	key.s = f;
	skeletondb.push_back(key);

	return 0;	
}

int rpcExecute(){	
	if (ssock==NULL){
		cerr<<"in func rpcExecute: no sscok"<<endl;
		return NOSERVER;
	}

	if(skeletondb.empty()){
		cerr<<"in func rpcExecute: skeletondb is empty"<<endl;
		return DBEMPTY;
	}

	fd_set socketlist;
	fd_set read_fds;
	int fdmax;
	int mainsocket = ssock->serverfd;
	FD_SET(mainsocket, &socketlist);
	fdmax = mainsocket;
	for(;;){
		read_fds = socketlist;
		select(fdmax+1, &read_fds, NULL, NULL, NULL);
		for(int i=0; i<=fdmax; i++){
			if (FD_ISSET(i, &read_fds)) {
				if(i == mainsocket) {
					int client = accept(mainsocket, NULL, 0);
					FD_SET(client, &socketlist);
					if(fdmax<client){
						fdmax = client;
					}
				}else{
					char buffer[2000];
					int type;
					int size = 2000;
					int status = recvMessage(i, &type, buffer, &size);
					if(status<0){
            					close(i);
            					FD_CLR(i, &socketlist);
					}else{
						switch(type){
							case EXECUTE:{
								Message *m = new Message();
								m->type = type;
								m->len = size;
								m->fd = i;
								m->data = new char[size];
								m->data = buffer;
								if(serverdowork(m)){
									cerr<<"in func rpcExecute: error serverdowork"<<endl;
									return 1;
								}
								//cout<<"print statement"<<endl;
								//delete [] m->data;
								//delete m;
								//m = NULL;
								}break;
							case TERMINATE:
								char buf[BYTESIZE];
								sendMessage(i, TERMINATESUCCESS, buf, BYTESIZE);
								return 0;
						}
					}
				}
			}
		}
	}
	
}

int rpcCall(char* name, int* argTypes, void** args){

	Message *m = NULL;
	Message *r = NULL;

	m = wraplocrequest(name, argTypes);

	//case for terminate
	if(m->type == TERMINATE){
		return 0;
	}

	if(getbinderinfo()){
		cerr<<"in func rpcCall: error get binder info"<<endl;
		return ERRORBINDERINFO;
	}

	if(csock == NULL){
		csock = new clientsocket();
	}

	if(getbinderinfo()){
		cerr<<"error getting binder info"<<endl;
		return ERRORBINDERINFO;
	}	

	if(csock->makeconnection(binderaddr, binderportstring)){
		cerr<<"in func rpcCall: error makeconnection"<<endl;
		return ERRORBINDERCONNECT;
	}

	if(csock->sendmessage(m)){
		cerr<<"in func rpcCall: error sendmessage"<<endl;
		return ERRORSENDLOCREQUEST;
	}

	r = csock->getmessage();

	switch(r->type){
		case LOC_SUCCESS:
			return requesttoserver(r, name, argTypes, args);
		case LOC_FAILURE:
			return LOC_FAILURE;
		default:
			cerr<<"in func rpcCAll: wtf it does not make any sense"<<endl;
			return WHAT;
	}
	

}

int rpcTerminate(){
	if(csock == NULL){
		csock = new clientsocket();
	}
	if(csock->isconnect()){
		csock->disconnect();
	}
	
	if(getbinderinfo()){
		cerr<<"error getting binder info"<<endl;
		return ERRORBINDERINFO;
	}
	
	if(csock->makeconnection(binderaddr, binderportstring)){
		cerr<<"in func rpcCall: error makeconnection"<<endl;
		return ERRORBINDERCONNECT;
	}
	
	char buf[BYTESIZE];
	if(sendMessage(csock->fd, TERMINATE, buf, BYTESIZE)){
		//cerr<<"wtf"<<endl;
		return ERRORSENDTERM; 
	}
	return 0;
}








































