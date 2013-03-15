#include "SocketBase.h"

SocketBase::SocketBase()
	: mSocket(INVALID_SOCKET)
{
	init();
}

SocketBase::~SocketBase()
{
	shutDown();
}

void SocketBase::init()
{
	WSAStartup( MAKEWORD(2,0), &mWSStartData);
	ZeroMemory( &mSockAddr, sizeof(mSockAddr) );
	mSocket = socket(AF_INET, SOCK_STREAM, 0);
}

void SocketBase::shutDown()
{
	shutdown(mSocket, SD_BOTH);
	closesocket(mSocket);
	mSocket = INVALID_SOCKET;
	WSACleanup();
}