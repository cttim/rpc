#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H
#include "constants.h"
#include <string>
//using namespace std;
class clientsocket{
	public:
	char *name;
	int fd;
	int port;
	clientsocket();
	int makeconnection(std::string name, std::string port);
	int getport();
	std::string getaddress();
	int sendmessage(Message *m);
	Message *getmessage();
	bool isconnect();
	int disconnect();
};



#endif
