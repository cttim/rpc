#ifndef __IO__
#define __IO__

#ifdef __cplusplus
extern "C" {
#endif

/*
Send message to a socket
This function requires:
1. The connection is already open for this socket
2. size is number of byte of the given buffer
3. size does not excess INT_MAX

Return Value:
	0: Success
	Positive Integer: Number of unsend byte, not able to send due to error

*/
int sendMessage(int socket, int type, char* buffer, int size);

/*
Receive message from a socket
The size variable is a integer pointer.
When calling, *size should be the number of byte in the given buffer.
After return, *size will be the number of byte that actually received,
*Type will be the received message type (See constants.h)
This function requires:
1. *size is number of byte of the given buffer
2. *size does not excess INT_MAX

Return Value:
	0: Success
	-1: Connection Close
	-2: Given buffer size too small
	Positive number: Number of unreceived byte, not able to receive due to error

*/
int recvMessage(int socket, int* type, char* buffer, int* size);

/* Take the next 4 bytes from the buffer and put the int in there */
void IntToChar(int i, char* buffer);

/* Opposite to IntToChar */
int CharToInt(char* buffer);


#ifdef __cplusplus
}
#endif

#endif
