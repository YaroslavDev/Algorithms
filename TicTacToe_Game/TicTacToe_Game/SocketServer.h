#ifndef SOCKETSERVER_H
#define SOCKETSERVER_H

#include "SocketBase.h"

#define MAX_CLIENTS 10

class SocketServer : public SocketBase
{
public:
	SocketServer();
	~SocketServer();

	void init();
	void shutDown();
	int bindTo(DWORD address);
	int beginListen();
	int acceptConnection();
	int sendMessage(MESSAGE *msg);
	int receiveMessage(MESSAGE *msg);
private:
	SOCKET mServSock;
};

#endif //SOCKETSERVER_H