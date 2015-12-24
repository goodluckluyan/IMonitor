/****************************************************************************
*@file: C_TcpServer.h
*@brief: Tcp服务器端协议对外接口实现类
*		 
*@author: zhangchao@oristartech.com
*@dade: 2015-11-11
****************************************************************************/
#ifndef CTCPSERVER_H
#define CTCPSERVER_H

#include <pthread.h>
#include"C_ITcpSserver.h"

class CTcpServer:public ICTcpServer
{
public:
    CTcpServer();
    ~CTcpServer();
public:
    virtual bool StartServer();
    virtual bool StartServer(const unsigned short& port );
    virtual bool CloseServer();
    void SetServerPort(const unsigned short& port );
    virtual bool WriteData(const NetData&netData );
    virtual void SetRecvDataObserver(IRecvDataObserver*pObserver );
private:
	void ListenConnect();
	bool Init();
public:
	void ListenNetData();
private:
	int m_serverSocket;
	int m_clientSocket;
	unsigned short m_serverPort;
	NetData m_writeData;
	IRecvDataObserver *m_pObserver;
	pthread_t  m_threadId;
	bool m_bConnectd;
	bool m_bClosed;
};

#endif // CTCPSERVER_H
