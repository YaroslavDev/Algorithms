#include "SocketClient.h"

SocketClient::SocketClient() {}

SocketClient::~SocketClient() {}

int SocketClient::connectToServer(char in_addr[])
{
	mSockAddr.sin_family = AF_INET;
	mSockAddr.sin_addr.S_un.S_addr = inet_addr(in_addr);
	mSockAddr.sin_port = htons(SERVER_PORT);

	return connect(mSocket, (struct sockaddr*)&mSockAddr, sizeof(mSockAddr));
}

int SocketClient::sendMessage(MESSAGE *pMsg)
{
	DWORD disconnect = 0;
	LONG32 nRemainRecv = 0, nXfer, nRemainSend;
	LPBYTE pBuffer;

	nRemainSend = HEADER_SIZE;
	pMsg->msgLen = (long)(strlen((char*)pMsg->record)+1);
	pBuffer = (LPBYTE)pMsg;
	while(nRemainSend > 0 && !disconnect)
	{
		nXfer = send(mSocket, (char*)pBuffer, nRemainSend, 0);
		if( nXfer == SOCKET_ERROR) return 1;
		disconnect = (nXfer==0);
		nRemainSend -= nXfer;
		pBuffer += nXfer;
	}

	nRemainSend = pMsg->msgLen;
	pBuffer = (LPBYTE)pMsg->record;
	while(nRemainSend>0 && !disconnect)
	{
		nXfer = send(mSocket, (char*)pBuffer, nRemainSend, 0);
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

int SocketClient::receiveMessage(MESSAGE* pMsg)
{
	DWORD disconnect = 0;
	LONG32 nRemainRecv = 0, nXfer;
	LPBYTE pBuffer;

	/*	Read the request. First the header, then the request text. */
	nRemainRecv = HEADER_SIZE; 
	pBuffer = (LPBYTE)pMsg;

	while (nRemainRecv > 0 && !disconnect) 
	{
		nXfer = recv (mSocket, (char*)pBuffer, nRemainRecv, 0);
		disconnect = (nXfer == 0);
		nRemainRecv -=nXfer; pBuffer += nXfer;
	}
	
	/*	Read the request record */
	nRemainRecv = pMsg->msgLen;
	/* Exclude buffer overflow */
	nRemainRecv = min(nRemainRecv, MAX_MSG_LEN);

	pBuffer = pMsg->record;	
	while (nRemainRecv > 0 && !disconnect) {
		nXfer = recv (mSocket, (char*)pBuffer, nRemainRecv, 0);
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