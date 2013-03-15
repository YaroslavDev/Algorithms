#ifndef SOCKETBASE_H
#define SOCKETBASE_H

#include <WinSock2.h>
#include <WS2tcpip.h>

//#define minOf( a, b ) ((a)<(b) ? (a):(b))

#define SockAddr struct sockaddr_in
#define SERVER_PORT 50000
#define MAX_MSG_LEN 0x1000
#define MSG_SIZE sizeof(MESSAGE)
#define HEADER_SIZE MSG_SIZE-MAX_MSG_LEN

typedef struct MESSAGE
{
	LONG32 msgLen;
	BYTE record[MAX_MSG_LEN];

} MESSAGE;

class SocketBase
{
public:
	SocketBase();
	~SocketBase();

	virtual void init();
	virtual void shutDown();

protected:
	WSADATA mWSStartData;
	SOCKET mSocket;
	SockAddr mSockAddr;
};

#endif //SOCKETBASE_H