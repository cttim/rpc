#include "serversocket.h"
#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

using namespace std;

serversocket::serversocket(int a){
	
	max = a;

	struct sockaddr_in addr;
	struct hostent *hp;

	gethostname(servername, 100);
	hp = gethostbyname(servername);
	if(hp == NULL){
		cerr<<"in serversocket creation no host"<<endl;
		exit(-1);
	}

	serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if(serverfd < 0){
		cerr<<"in serversocket creation error opening socket"<<endl;
		exit(-1);
	}

	addr.sin_family = hp->h_addrtype;
	addr.sin_port = htons(0);
	addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(serverfd, (struct sockaddr *) &addr, sizeof(addr)) < 0){
		cerr<<"in serversocket creation error binding"<<endl;
		exit(-1);
	}

	
	listen(serverfd, max);
	struct sockaddr_in newas;
	socklen_t length = sizeof(newas);
	getsockname(serverfd, (struct sockaddr*)& newas, &length);
	port = ntohs(newas.sin_port);
	
	
}

string serversocket::getaddress(){
	return servername;
}

int serversocket::getport(){
	return port;
}

int serversocket::getmaxconnection(){
	return max;
}



































