/****************************************************************************
*@file: C_RecvDataObserverImp.h
*@brief: 接收数据观察者接口实现类
*		 
*@author: zhangchao@oristartech.com
*@dade: 2015-11-12
****************************************************************************/
#ifndef RECVDATAOBSERVERIMP_H
#define RECVDATAOBSERVERIMP_H

#include "C_ITcpSserver.h"

namespace FileCopyProtocol
{
	class CFileCopyProtocolServer;
	class CRecvDataObserverImp:public IRecvDataObserver
	{
	public:
		CRecvDataObserverImp(CFileCopyProtocolServer* pServer );
		~CRecvDataObserverImp();
	public:
		virtual void RecvNetData(const NetData& netData );

	private:
		CFileCopyProtocolServer* m_pServer;
	};
}
#endif // RECVDATAOBSERVERIMP_H
