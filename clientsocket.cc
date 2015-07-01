#include "clientsocket.h"
#include "constants.h"
#include "io.h"
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sstream>

using namespace std;

clientsocket::clientsocket(){
	fd = -1;
}

bool clientsocket::isconnect(){
	if (fd != -1){
		return true;
	}
	return false;
}

int clientsocket::disconnect(){
	close(fd);
	return 0;
}

int clientsocket::getport(){
	return port;
}

string clientsocket::getaddress(){
	return name;
} 


int clientsocket::makeconnection(string addr, string portnum) {
	if(isconnect()){
		//cout<<"in func makeconnection close open fd"<< endl;
		disconnect();
	}
	
	struct addrinfo pHint, *ppResult;
	memset(&pHint, 0, sizeof(pHint));
	pHint.ai_family = AF_UNSPEC;
	pHint.ai_socktype = SOCK_STREAM;
	if(getaddrinfo(addr.c_str(), portnum.c_str(), &pHint, &ppResult)){
		cerr<<"in func makeconnection: error get addrinfo"<<endl;
		return 1;
	}

	fd = socket(ppResult->ai_family, ppResult->ai_socktype, ppResult->ai_protocol);
	
	if(connect(fd, ppResult->ai_addr, ppResult->ai_addrlen) < 0){
		cerr<<"in func makeconnection: error connecting"<<endl;
		return 1;
	}
	
	
	return 0;
}

int clientsocket::sendmessage(Message *m){
	//this is new send, hopefully work
		
	char *buffer = new char[m->len];
	int bufferindex = 0;
	//start to prepare the buf
	for(int i=0; i<m->len; i++){
		buffer[bufferindex] = m->data[i];
		bufferindex++;
	}


	if(sendMessage(fd, m->type, buffer, m->len)){
		cerr<<"in func sendmessage: sendMessage fail"<<endl;
		return 1;	
	}
	delete [] buffer;
	buffer = NULL;
	delete m;

	return 0;
}

Message *clientsocket::getmessage(){
	Message *m = new Message();
	int type;
	int size=20000;
	m->data = new char [20000];
	
	
	int result = recvMessage(fd, &type, m->data, &size);
	
	if(result){
		cerr<<"in func getmessage: error recvMessage"<<endl;
		return NULL;
	}
	
	m->len = size;
	m->type = type;
	m->fd = fd;
	

	return m;

}
