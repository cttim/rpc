#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include "io.h"
#include <stdio.h>

/*

Return Value:
	0: Success
	-1: Connection Close
	Positive number: Number of unreceived byte, not able to receive due to error

*/
int recvBuffer(int s, char *buffer, int length) {
	do {
		int byteRecv = recv(s, buffer, length, 0);
		if (byteRecv <= 0)
			return -1;
		length -= byteRecv;
        buffer += byteRecv;
	} while (length);
	return length;
}

/*

Return Value:
	0: Success
	Positive Integer: Number of unsend byte, not able to send due to error

*/
int sendBuffer(int s, char *buffer, int length) {
	do {
		int sent = send(s, buffer, length, 0);
		if (sent < 0)
			break;
		length -= sent;
		buffer += sent;
	} while (length);
	return length;
}

void IntToChar(int i, char* buffer) {
	// buffer[0] = (i >> 24) & 0xff;
	// buffer[1] = (i >> 16) & 0xff;
	// buffer[2] = (i >> 8) & 0xff;
	// buffer[3] = i & 0xff;
	memcpy(buffer, &i, 4);
}

int CharToInt(char* buffer) {
	int i;
	memcpy(&i, buffer, 4);
	//return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
	return i;
}

int sendMessage(int socket, int type, char* buffer, int size) {
	int newSize = size*sizeof(char) + 2*sizeof(int);
	char* newBuffer = (char*)malloc(newSize);
	int result;
	IntToChar(size, newBuffer);
	IntToChar(type, newBuffer+4);
	memcpy(newBuffer+8, buffer, size);
	result = sendBuffer(socket, newBuffer, newSize);
	free(newBuffer);
	return result;
}

#define NOLEFT 0

static int leftBufferSize = NOLEFT;

int recvMessage(int socket, int* type, char* buffer, int* size) {
  if (leftBufferSize != NOLEFT) {
  	if (*size < leftBufferSize) {
  		return -2;
  	} else {
  		return recvBuffer(socket, buffer, leftBufferSize);
  		leftBufferSize = NOLEFT;
  	}
  } else {
    char header[8];
    int result;

    if ((result = recvBuffer(socket, header, 8))) {
      return result;
    }

    int newSize = CharToInt(header);
    *type = CharToInt(header + 4);
    if (newSize > *size) {
      *size = newSize;
      leftBufferSize = newSize;
      return -2;
    }
    *size = newSize;
    return recvBuffer(socket, buffer, newSize);
  }
}
