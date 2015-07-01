#include "io.h"
#include "constants.h"
#include <map>
#include <string>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <cctype>
#include <list>
#include <netdb.h>
using namespace std;

struct ProcedureInfo;
struct ServerInfo;

typedef list<ServerInfo> ServerInfoList;
typedef list<ProcedureInfo> ProcedureInfoList;

bool operator==(ProcedureInfo& p1, ProcedureInfo& p2);
bool operator==(ServerInfo& s1, ServerInfo& s2);


struct ProcedureInfo {
  char name[65];
  int argType[64];
  list<ServerInfo*> location;
  ProcedureInfo() {
    memset(name, 0, 65*sizeof(char));
    memset(argType, 0, 64*sizeof(int));
  }
  friend bool operator==(ProcedureInfo& p1, ProcedureInfo& p2);
  bool findServer(ServerInfo& info) {
    list<ServerInfo*>::iterator iter;
    for (iter=location.begin(); iter!=location.end(); iter++) {
      if (*(*iter) == info) {
        return true;
      }
    }
    return false;
  }

  void removeServer(ServerInfo& server) {
    list<ServerInfo*>::iterator iter;
    for (iter=location.begin(); iter!=location.end(); iter++) {
      if (*(*iter) == server) {
        location.erase(iter);
        break;
      }
    }
  }
};

struct ServerInfo {
  char serverID[64];
  int port;
  list<ProcedureInfo*> service;
  ServerInfo() {
    memset(serverID, 0, 64*sizeof(char));
    port = 0;
  }
  friend bool operator== (ServerInfo& s1, ServerInfo& s2);
  bool findProc(ProcedureInfo& info) {
    list<ProcedureInfo*>::iterator iter;
    for (iter=service.begin(); iter!=service.end(); iter++) {
      if (*(*iter) == info) {
        return true;
      }
    }
    return false;
  }
};

bool operator==(ProcedureInfo& p1, ProcedureInfo& p2) {
  bool nameB = !(strcmp(p1.name, p2.name));
    
  const unsigned int* iter1 = (const unsigned int *)p1.argType;
  const unsigned int* iter2 = (const unsigned int *)p2.argType;


  while (*iter1 != 0 && *iter2 != 0 ) {
    //cout << *iter1 << " " << *iter2 << endl;
    if (*iter2 != *iter1) {
      int mask = 0xffff;
      int high1 = (*iter1) & ~mask;
      int high2 = (*iter2) & ~mask;
      int low1 = (*iter1) & mask;
      int low2 = (*iter2) & mask;
      //cout << "low1 " << low1 << "low2 " << low2 << endl; 
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


bool operator== (ServerInfo& s1, ServerInfo& s2) {
  bool idB = !(strcmp(s1.serverID, s2.serverID));
  bool portB = (s1.port == s2.port);
  //cout << idB << " " << portB << endl;
  return idB && portB;
}


#define LINK() { \
  iter2->location.push_back(&(*iter)); \
  iter->service.push_back(&(*iter2)); \
}

class Database {
  ServerInfoList serverList;
  ProcedureInfoList procedureList;
public:
  int locate(ServerInfo& server, ProcedureInfo& procedure) {
    ProcedureInfoList::iterator iter;
    int foundProc = false;
    for (iter = procedureList.begin(); iter != procedureList.end(); iter++) {
      if (*iter == procedure) {
        foundProc = true;
        break;
      }
    }
    if (foundProc) {
      ServerInfo* s = iter->location.front();
      iter->location.pop_front();
      memcpy(server.serverID, s->serverID, 64);
      server.port = s->port;
      iter->location.push_back(s);
      return 0;
    }
    return -1;
  }

  void save(ServerInfo& server, ProcedureInfo& procedure) {
    ServerInfoList::iterator iter;
    int foundServer = false;
    ProcedureInfoList::iterator iter2;
    int foundProc = false;
    // Find server node
    for (iter = serverList.begin(); iter != serverList.end(); iter++) {
      if (*iter == server) {
        foundServer = true;
        break;
      }
    }
    // Find procedure node
    for (iter2 = procedureList.begin(); iter2 != procedureList.end(); iter2++) {
      if (*iter2 == procedure) {
        foundProc = true;
        break;
      }
    }
    // There are in total four condition:
    if (foundServer) {
      if (foundProc) {
        if (!(iter->findProc(*iter2))) {
          LINK()
        }
        //cout << "S P" << endl;
      } else { 
        iter2 = procedureList.insert(procedureList.end(), procedure);
        LINK()
        //cout << "S" << endl;
      }
    } else {
      iter = serverList.insert(serverList.end(), server);
      if (foundProc) {
        LINK()
        //cout << "P" << endl;
      } else {
        //cout << "not found" << endl;
        iter2 = procedureList.insert(procedureList.end(), procedure);
        LINK()
      }
    }
  }

  void eraseServer(ServerInfo& server) {
    ServerInfoList::iterator iter;
    for (iter = serverList.begin(); iter != serverList.end(); iter++) {
      if (*iter == server) {
        list<ProcedureInfo*>::iterator iter2;
        for (iter2 = iter->service.begin(); iter2 != iter->service.end(); iter2++){
          (*iter2)->removeServer(server);
        }
        serverList.erase(iter);
        break;
      }
    }
  } 
};

/*

  Procedure Info -> a list of server info (round robin)
  Server Info -> a list of procedure Info

*/

static Database db;
/*
int main () {
  ServerInfo serverData;
  strcpy(serverData.serverID, "linux.student.cs.uwaterloo.ca");
  serverData.port = 10;
  ProcedureInfo function;
  strcpy(function.name, "addtwonumber");
  function.argType[0] = 3;
  function.argType[1] = 1 << 16 | 1;
  function.argType[2] = 1 << 31 | 0;
  ServerInfo serverData2;
  strcpy(serverData2.serverID, "not the same server");
  serverData2.port = 10;
  ProcedureInfo function2;
  strcpy(function2.name, function.name);
  function2.argTypDatabasee[0] = 3;
  function2.argType[1] = 1 << 16 | 16;
  function2.argType[2] = 1 << 31 | 1;

  //db.save(serverData, function); 
  //db.save(serverData2, function2server); 

  ServerInfo result;
  cout << db.locate(result, function2) << endl;
  // cout << (result == serverData) << endl;
  // db.locate(result, function2);
  // cout << (result == serverData2) << endl;
  // db.locate(result, function);
  // cout << (result == serverData) << endl;
  // db.locate(result, function2);
  // cout << (result == serverData2) << endl;

  return 0;
}
*/

/*

Register:
  port: int
  server_identifier: 64 char
  name: 64 char
  argType: rest

*/


void copyArgType( int* argType, char* buffer, int maxSize) {
  int i = 0;
  while (i < maxSize) {
    *argType = CharToInt(buffer);
    if (*argType == 0) {
      return;
    }
    buffer += 4;
    argType += 1;
    i++;
  }
  cerr << "ERROR: Arg type too large, system does not support!";
}

char* bufferBreaker(char* buffer) {
  while (buffer) {
    buffer++;
  }
  buffer++;
  return buffer;
}

int registerFuc(char* buffer, int size) {
  ServerInfo sInfo;
  // Get port
  sInfo.port = CharToInt(buffer);
  buffer += 4;
  //cout << "Port:" << sInfo.port << endl;

  // Get ServerID
  strcpy(sInfo.serverID, buffer);
  buffer += 16;
  //cout << "Server Name: " << sInfo.serverID << endl;


  ProcedureInfo pInfo;
  // Get Name
  strcpy(pInfo.name, buffer);
  buffer += 64;
  //cout << "Function Name: " << pInfo.name <<  endl;

  // Get argType
  copyArgType(pInfo.argType, buffer, (size-4-16-64)/4);
  
  db.save(sInfo, pInfo);
  return REGISTER_SUCCESS;
}


/*
  Server_name: 64 byte
  argType: rest
*/
int getLocation(char* buffer, int size, char* reply, int replyLen) {
  ServerInfo sInfo;
  ProcedureInfo pInfo;
  strcpy(pInfo.name, buffer);
  buffer += 64;

  copyArgType(pInfo.argType, buffer, 64);

  int result = db.locate(sInfo, pInfo);
  if (result == -1) {
    return LOC_FAILURE;
  }

  memset(reply, 0, replyLen);
  IntToChar(sInfo.port, reply);
  reply += 4;

  strcpy(reply, sInfo.serverID);

  return LOC_SUCCESS;
}

#define MAX_MESSAGE_SIZE 200


void sendTermination(char* hostname, int port) {
  struct addrinfo hints, *head;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  char *ip_s = hostname;
  string port_s;
  stringstream ss;
  ss << port;
  ss >> port_s;

  getaddrinfo(ip_s, port_s.c_str(), &hints, &head); 

  if (connect(sock, head->ai_addr, head->ai_addrlen) == -1)
    cerr << "Cannot establish connection" << endl;
  freeaddrinfo(head);
  char b[10];
  sendMessage(sock, TERMINATE, b, 10);
	char reply[10];
	int size = 10;
	int type;
	if (recvMessage(sock, &type, reply, &size) > 0) {
		cerr << "Error receiving Termination message, skip this server" << endl;
	}
  close(sock);
}


int main(void) {
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = 0;
  int mainSocket = socket(AF_INET, SOCK_STREAM, 0);
  bind(mainSocket, (struct sockaddr*)&addr, sizeof(struct sockaddr_in));

  char name[20];
  gethostname(name, 20);
  cout << "BINDER_ADDRESS " << string(name) << endl;
  struct sockaddr_in sin;
  socklen_t len = sizeof(sin);
  getsockname(mainSocket, (struct sockaddr*)&sin, &len);
  cout << "BINDER_PORT " << ntohs(sin.sin_port) << endl;

  fd_set socketList;
  fd_set read_fds;
  int fdmax;

  listen(mainSocket, 20);
  FD_SET(mainSocket, &socketList);
  fdmax = mainSocket;
  
  map<int, ServerInfo> serverDatabase;

  for (;;) {
    read_fds = socketList;
    select(fdmax + 1, &read_fds, NULL, NULL, NULL);
    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == mainSocket) {
          // New Connection
          int client = accept(mainSocket, NULL, 0);
          FD_SET(client, &socketList);
          if (fdmax < client) fdmax = client;
        } else {
          // New Message
          char buffer[MAX_MESSAGE_SIZE];
          int type;
          int size = MAX_MESSAGE_SIZE;
          // TODO: need error detection here
          int status = recvMessage(i, &type, buffer, &size);
          if (status < 0) {
            // Close connection
            if (serverDatabase.count(i) >= 1) {
              ServerInfo server = serverDatabase.at(i);
              db.eraseServer(server);
              serverDatabase.erase(i);
            }
            close(i);
            FD_CLR(i, &socketList);
          } else {
            // New Message
            switch (type) {
              case REGISTER: {
                registerFuc(buffer, size);
		            char reply[25];
                sendMessage(i, REGISTER_SUCCESS, reply, 20);
              } break;
              case LOC_REQUEST: {
                char reply[25];
                int result = getLocation(buffer, MAX_MESSAGE_SIZE, reply, 25);
                if (result == LOC_SUCCESS) {
                  sendMessage(i, LOC_SUCCESS, reply, 20);
                } else if (result == LOC_FAILURE) {
                  sendMessage(i, LOC_FAILURE, reply, 20);
                } else {
                  cerr << "we should never get here" << endl;
                }
              } break;
              case TERMINATE: {
                map<int, ServerInfo>:: iterator tail;
                for (tail=serverDatabase.begin(); tail!=serverDatabase.end();tail++) {
                  char* hostname = tail->second.serverID;
                  int port = tail->second.port;
                  sendTermination(hostname, port);
                }
                return 0;
              } break;
              case SERVERIDENTIFY: {
                ServerInfo info;
                info.port = CharToInt(buffer);
                memcpy(info.serverID, buffer+4, 16);
                serverDatabase[i] = info;
              } break;
              default:
                break;
            }
          } 
        }
      }
    }
  }
}


