#include "constants.h"
#include "rpc.h"
#include <iostream>
#include <sstream>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

using namespace std;


void intToChars(int i, char* buffer) {
	memcpy(buffer, &i, 4);
}

int charsToInt(char *byte) {
	int i;
	memcpy(&i, byte, 4);
	return i;
}



//get real type, 2 bytes for input output
int getrealtype(int argtype){
	int output = argtype;
	return (((output<<8)>>24) &0xFF);
}

int getreallength(int argtype){
	int output = argtype;
	return output & 0xFFFF;
}

Message * wraplocrequest(char *name, int *arg_types){
    Message *m = new Message();

    //get arg_types length and name length
    int arg_types_len = 0;
    int name_len = strlen(name);
    if(name_len>FUNCNAME){
        name_len = FUNCNAME;
    }
    while(arg_types[arg_types_len] != 0){
        arg_types_len++;
    }
    arg_types_len++;
    
    //get message length
    int length = FUNCNAME+arg_types_len*sizeof(int);

    //prepare the message
    m->len = length;
    m->type = LOC_REQUEST;
    m->data = new char[length+1];
    int index = 0;
    for(index=0; index<name_len; index++){
        m->data[index] = name[index];
    }
    for(int j = 0; j < FUNCNAME-name_len; j++) {
        m->data[index] = '\0';
        index++;
    }
    for(int k=0; k<arg_types_len; k++){
        char arg_char[BYTESIZE];
        intToChars(arg_types[k],arg_char);
        for(int l=0; l<BYTESIZE; l++){
            m->data[index] = arg_char[l];
            index++;
        }
    }

    if(index != length){
        cerr<<"wtf"<<endl;
    }

    m->data[index+1] = '\0';
    return m;

}

int getargtypelen(int *argTypes){
	int index = 0;
	while(argTypes[index] != 0){
		index++;
	}
	index++;
	return index;
}

int getargslen(int *argTypes){
	int len = 0;
	for(int i=0; i<getargtypelen(argTypes) - 1; i++){
		int realtype = getrealtype(argTypes[i]);
		int reallen = getreallength(argTypes[i]);
		if(reallen == 0){
			reallen =1;
		}
		switch(realtype){
			case ARG_CHAR:
				len += sizeof(char) * reallen;
				break;
			case ARG_SHORT:
				len += sizeof(short) * reallen;
				break;
			case ARG_INT:
				len += sizeof(int) * reallen;
				break;
			case ARG_LONG:
				len += sizeof(long) * reallen;
				break;
			case ARG_DOUBLE:
				len += sizeof(double) * reallen;
				break;
			case ARG_FLOAT:
				len += sizeof(float) * reallen;
				break;
		}
	}
	return len;
}


Message * wrapexecutemessage(char *name, int *argTypes, void **args){
	int argtypelen = getargtypelen(argTypes);
	int argslen = getargslen(argTypes);
	int messagelen = 4*argtypelen + argslen + FUNCNAME;
	Message *m = new Message();
	m->len = messagelen;
	m->type = EXECUTE;
	m->data = new char[messagelen];	
	
	//copy name into data
	int namelen = strlen(name);
	if(namelen > FUNCNAME){
		namelen = FUNCNAME;
	}
	int index=0;
	for(index=0; index<namelen; index++){
		 m->data[index] = name[index];
	}
	//fill the null
	for(;index<FUNCNAME; index++){
		m->data[index] = '\0';
	}
	//finish name copy
	
	for(int j=0; j<argtypelen; j++){
		char argbuf[BYTESIZE]; 
		intToChars(argTypes[j], argbuf);
		for(int k=0; k<BYTESIZE; k++){
			m->data[index] = argbuf[k];
			index++;
		}
	}
	//finish argtype copy
	
	for(int i=0; i<argtypelen-1; i++){
		int realtype = getrealtype(argTypes[i]);
		
		int reallen = getreallength(argTypes[i]);
		if(reallen == 0){
			reallen = 1;
		}
		int bufsize = 0;
		switch(realtype){
			case ARG_CHAR:
				bufsize = sizeof(char) * reallen;
				break;
			case ARG_SHORT:
			 	bufsize = sizeof(short) * reallen;
				break;
			case ARG_INT:
				bufsize = sizeof(int) * reallen;
				break;
			case ARG_LONG:
				bufsize = sizeof(long) * reallen;
				break;
			case ARG_DOUBLE:
				bufsize = sizeof(double) * reallen;
				break;
			case ARG_FLOAT:
				bufsize = sizeof(float) * reallen;
				break;
		}
		memcpy(m->data+index, args[i], bufsize);
		index = index+bufsize;
	}
	return m;
}

void ** unwrapexecutemessage(int *argTypes, char *data){
	int argtypelen = getargtypelen(argTypes);
	int index = 0;	
	void **args = new void *[(argtypelen-1)*sizeof(void*)];
	char * arg = NULL;
	for(int i=0; i<argtypelen; i++){
		int realtype = getrealtype(argTypes[i]);
		int reallen = getreallength(argTypes[i]);
		if(reallen == 0){
			reallen = 1;
		}
		
		int bufsize = 0;
		switch(realtype){
			case ARG_CHAR:
				bufsize = sizeof(char) * reallen;
				break;
			case ARG_SHORT:
			 	bufsize = sizeof(short) * reallen;
				break;
			case ARG_INT:
				bufsize = sizeof(int) * reallen;
				break;
			case ARG_LONG:
				bufsize = sizeof(long) * reallen;
				break;
			case ARG_DOUBLE:
				bufsize = sizeof(double) * reallen;
				break;
			case ARG_FLOAT:
				bufsize = sizeof(float) * reallen;
				break;
		}
		arg = new char[bufsize];
		for(int j=0; j<bufsize; j++){
			arg[j] = data[index];
			index++;
		}
		args[i] = (void *)arg;
	}
	return args;

}




















