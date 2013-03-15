#include "SocketServer.h"

SocketServer::SocketServer()
	: mServSock(INVALID_SOCKET)
{
}

SocketServer::~SocketServer()
{
	closesocket(mServSock);
	mServSock = INVALID_SOCKET;
}

void SocketServer::shutDown()
{
	SocketBase::shutDown();
}

void SocketServer::init()
{
	SocketBase::init();
}

int SocketServer::bindTo(DWORD address)
{
	mSockAddr.sin_family = AF_INET;
	mSockAddr.sin_addr.S_un.S_addr = htonl(address);
	mSockAddr.sin_port = htons( SERVER_PORT );

	return bind(mSocket, (struct sockaddr *)&mSockAddr, sizeof(mSockAddr));
}

int SocketServer::beginListen()
{
	return listen(mSocket, MAX_CLIENTS);
}

int SocketServer::acceptConnection()
{
	int addr_len = sizeof(mSockAddr);
	mServSock = accept(mSocket, (struct sockaddr*)&mSockAddr, &addr_len);
	if( mServSock==INVALID_SOCKET ) return 1;
	else return 0;
}

int SocketServer::sendMessage(MESSAGE *pMsg)
{
	DWORD disconnect = 0;
	LONG32 nRemainRecv = 0, nXfer, nRemainSend;
	LPBYTE pBuffer;

	nRemainSend = HEADER_SIZE;
	pMsg->msgLen = (long)(strlen((char*)pMsg->record)+1);
	pBuffer = (LPBYTE)pMsg;
	while(nRemainSend > 0 && !disconnect)
	{
		nXfer = send(mServSock, (char*)pBuffer, nRemainSend, 0);
		if( nXfer == SOCKET_ERROR) return 1;
		disconnect = (nXfer==0);
		nRemainSend -= nXfer;
		pBuffer += nXfer;
	}

	nRemainSend = pMsg->msgLen;
	pBuffer = (LPBYTE)pMsg->record;
	while(nRemainSend>0 && !disconnect)
	{
		nXfer = send(mServSock, (char*)pBuffer, nRemainSend, 0);
		if( nXfer == SOCKET_ERROR ) return 1;
		disconnect = ( nXfer==0 );
		nRemainSend -= nXfer;
		pBuffer += nXfer;
	}

	/*nRemainSend = sizeof(pMsg->pos);
	pBuffer = (LPBYTE)&pMsg->pos;
	while(nRemainSend>0 && !disconnect)
	{
		nXfer = send(mSocket, (char*)pBuffer, nRemainSend, 0);
		if( nXfer == SOCKET_ERROR ) return 1;
		disconnect = ( nXfer==0 );
		nRemainSend -= nXfer;
		pBuffer += nXfer;
	}*/

	return disconnect;
}

int SocketServer::receiveMessage(MESSAGE* pMsg)
{
	DWORD disconnect = 0;
	LONG32 nRemainRecv = 0, nXfer;
	LPBYTE pBuffer;

	/*	Read the request. First the header, then the request text. */
	nRemainRecv = HEADER_SIZE; 
	pBuffer = (LPBYTE)pMsg;

	while (nRemainRecv > 0 && !disconnect) 
	{
		nXfer = recv (mServSock, (char*)pBuffer, nRemainRecv, 0);
		disconnect = (nXfer == 0);
		nRemainRecv -=nXfer; pBuffer += nXfer;
	}
	
	/*	Read the request record */
	nRemainRecv = pMsg->msgLen;
	/* Exclude buffer overflow */
	nRemainRecv = min(nRemainRecv, MAX_MSG_LEN);

	pBuffer = pMsg->record;	
	while (nRemainRecv > 0 && !disconnect) {
		nXfer = recv (mServSock, (char*)pBuffer, nRemainRecv, 0);
		disconnect = (nXfer == 0);
		nRemainRecv -=nXfer; pBuffer += nXfer;
	}

	/*nRemainRecv = sizeof(pMsg->pos);
	pBuffer = (LPBYTE)&pMsg->pos;
	while(nRemainRecv>0 && !disconnect)
	{
		nXfer = recv(mSocket, (char*)pBuffer, nRemainRecv, 0);
		disconnect = ( nXfer==0 );
		nRemainRecv -= nXfer;
		pBuffer += nXfer;
	}*/

	return disconnect;
}