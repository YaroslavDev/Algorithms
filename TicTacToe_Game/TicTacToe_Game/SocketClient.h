#ifndef SOCKETCLIENT_H
#define SOCKETCLIENT_H

#include "SocketBase.h"

class SocketClient : public SocketBase
{
public:
	SocketClient();
	~SocketClient();

	int connectToServer(char in_addr[]);
	int sendMessage(MESSAGE *msg);
	int receiveMessage(MESSAGE *msg);
};

#endif //SOCKETCLIENT_H