#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>  
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "C_TcpServer.h"
#include "C_Log.h"
void* RecvNetData(void *ptr)
{
	CTcpServer *p =static_cast<CTcpServer*>(ptr );
	p->ListenNetData();
	return NULL;
}

CTcpServer::CTcpServer():m_serverSocket(0 ),
    m_serverPort(0 ),
    m_clientSocket(0 ),
    m_pObserver(NULL ),
	m_bConnectd(false ),
	m_bClosed(true ),
	m_threadId(0)
{

}

CTcpServer::~CTcpServer()
{

}

bool CTcpServer::StartServer()
{
	bool flag =false;
	if(m_serverPort&&m_pObserver )
	{
		flag=Init();
		m_bClosed =false;
		if(flag )
		{
			CLog::Write(Normal, "Server start Sucess");
			ListenConnect();
		}else
		{
			CLog::Write(Error, "Server start Error");
		}
    }
    return flag;
}

bool CTcpServer::StartServer(const unsigned short& port )
{
	SetServerPort(port );
	bool flag =StartServer();
	return flag;
}

bool CTcpServer::CloseServer()
{
	if(m_threadId)
	{
		pthread_cancel(m_threadId );
	}
	
	shutdown(m_serverSocket,2);
	int no =close(m_serverSocket );
	no &=close(m_clientSocket );
	m_bClosed =true;
	while(true)
	{
		if(m_serverSocket == 0)
		{
			break;
		}
		sleep(1);
	}
	return !no;
}

void CTcpServer::SetServerPort(const unsigned short& port )
{
	m_serverPort =port;
}

bool CTcpServer::WriteData(const NetData&netData )
{
	if(m_bConnectd )
	{
		int len=send(m_clientSocket, netData.buffer, netData.sz, 0 );
		if(len!=netData.sz)
		{
			close(m_clientSocket );
			m_bConnectd=false;
			std::string strErr =strerror(errno );
			CLog::Write(Error, "send error,client Socket closed! ErMsg:"+strErr );
			return false;
		}
		return true;
	}
	return false;
}

void CTcpServer::SetRecvDataObserver(IRecvDataObserver*pObserver )
{
	m_pObserver =pObserver;
}

bool CTcpServer::Init( )
{
	struct sockaddr_in serverAddr;
	bool flag =true;
	do
	{
		if( (m_serverSocket=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP ) )<0 )
		{
			CLog::Write(Error, "socket create error!");
			flag =false;
			break;
		}
		int opt=1;
		setsockopt(m_serverSocket,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
		memset(&serverAddr, 0, sizeof(serverAddr) );
		serverAddr.sin_family=AF_INET;
		serverAddr.sin_port=htons(m_serverPort );
		serverAddr.sin_addr.s_addr=INADDR_ANY;
		if(bind(m_serverSocket, (struct sockaddr* )&serverAddr,
				sizeof(serverAddr ) )==-1 )
		{
			CLog::Write(Error, "bind error!");
			flag =false;
			break;
		}

		if(listen(m_serverSocket, 1 )<0)
		{
			CLog::Write(Error, "listen error!");
			flag =false;
			break;
		}
	}while(false );
	return flag;
}

void CTcpServer::ListenNetData()
{
	NetData netData;
	int dataLen =0;
	while(true )
	{
		if(m_bConnectd)
		{
			memset(netData.buffer, 0, MAXBUFFERSZ);
			netData.sz =0;
			dataLen =0;
			int len =recv(m_clientSocket, netData.buffer, 8, 0);
			if(len> 0)
			{
				netData.sz+=len;
				time_t timeOne =time(NULL );
				while(netData.sz<8)
				{
					time_t timeTwo =time(NULL);
					if ( (timeTwo-timeOne) >30 )
					{
						CLog::Write(Error,"Timeout waiting for data!");
						break;
					}
					len =recv(m_clientSocket, netData.buffer+netData.sz, 8-netData.sz, 0);
					netData.sz+=len;
				}
				memcpy(&dataLen, netData.buffer+4, 4 );
				if (dataLen<0||dataLen>MAXBUFFERSZ )
				{
					CLog::Write(Error,"The HeaderLen is Error!");
					continue;
				}
				while( netData.sz<dataLen )
				{
					len =recv(m_clientSocket, netData.buffer+netData.sz, dataLen-netData.sz, 0);
					netData.sz +=len;
				}
				m_pObserver->RecvNetData(netData );
			}else if (len==0)
			{
				CLog::Write(Error, "client exit!");
				close(m_clientSocket );
				m_bConnectd=false;
				return;
				//pthread_cancel(m_threadId );
			}else
			{
				close(m_clientSocket );
				m_bConnectd=false;
				std::string strErr =strerror(errno );
				CLog::Write(Error, "recv error,client exit! ErMsg:"+strErr );
				return;
				//pthread_cancel(m_threadId );
			}
		}else
		{
			break;
		}
	}
}

void CTcpServer::ListenConnect()
{
	struct sockaddr_in clientAddr;
	unsigned  int lenth =sizeof(struct sockaddr );
	while(true )
	{
		if (m_bClosed )
		{
			m_serverSocket = 0;
			break;
		}
		struct timeval tv;
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(m_serverSocket,&fdset);
		if(select(m_serverSocket+1,&fdset,NULL,NULL,&tv)<=0)
		{
			continue;
		}
		if(!FD_ISSET(m_serverSocket,&fdset))
		{
			continue;
		}
		if( (m_clientSocket=accept(m_serverSocket,
			(struct sockaddr* )&clientAddr,
			&lenth ) )<0 )
		{
			CLog::Write(Error, "accept error!");
			m_bConnectd =false;
		}else
		{
			CLog::Write(Normal, "Connected Sucess!");
			m_bConnectd =true;
			int threadFlag =pthread_create(&m_threadId, NULL,RecvNetData,this );
			pthread_detach(m_threadId );
		}
	}

}

ICTcpServer* createTcpServer( )
{
	return new CTcpServer;
}

void releaseTcpServer(ICTcpServer* pServer )
{
	if(pServer)
	{

		delete pServer;
		pServer =NULL;
	}
}
