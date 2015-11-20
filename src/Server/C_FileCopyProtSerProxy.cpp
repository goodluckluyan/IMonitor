#include <sys/wait.h> 
#include "C_FileCopyProtSerProxy.h"
#include "C_FileCopyProtocolServer.h"
#include <stdio.h>

void* start(void *ptr)
{
	FileCopyProtocol::CFileCopyProtocolServer *p =static_cast<FileCopyProtocol::CFileCopyProtocolServer*>(ptr );
	if (p )
	{
		p->Start();
	}
	return NULL;
}

CFileCopyProtSerProxy::CFileCopyProtSerProxy():m_pServer(NULL )
{
	m_pServer =new FileCopyProtocol::CFileCopyProtocolServer;
}

CFileCopyProtSerProxy::~CFileCopyProtSerProxy()
{
	if (m_pServer )
	{
		delete m_pServer;
	}
}

void CFileCopyProtSerProxy::SetServerPort(const unsigned short& port )
{
	m_pServer->SetServerPort(port );
}

void CFileCopyProtSerProxy::SetLogPath(const std::string& path )
{
	m_pServer->SetLogPath(path );
}

bool CFileCopyProtSerProxy::Start()
{
	pthread_t threadId;
	int threadFlag =pthread_create(&threadId, NULL,start,m_pServer );
	pthread_detach(threadId);
	return !threadFlag;
}

bool CFileCopyProtSerProxy::Close()
{
	return m_pServer->Close();
}

