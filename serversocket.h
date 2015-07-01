#include <string>
#include <vector>
#include <netinet/in.h>
#include "constants.h"

#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H
class serversocket{

public:
	char servername[100];
	int max;
	int serverfd;
	int port;
	//struct sockaddr_in addr;
	//struct hostent *hp;
	fd_set socketlist, read_fds;
	int fdmax;

	serversocket(int a);
	std::string getaddress();
	int getport();
	int getmaxconnection();
	Message* getmessage();
};

#endif
