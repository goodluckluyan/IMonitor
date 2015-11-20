#include "C_RecvDataObserverImp.h"
#include "C_FileCopyProtocolServer.h"

namespace FileCopyProtocol
{
	CRecvDataObserverImp::CRecvDataObserverImp(CFileCopyProtocolServer* pServer )
	{
		m_pServer =pServer;
	}

	CRecvDataObserverImp::~CRecvDataObserverImp()
	{

	}

	void CRecvDataObserverImp::RecvNetData(const NetData& netData )
	{
		m_pServer->Process(netData  );
	}
}
