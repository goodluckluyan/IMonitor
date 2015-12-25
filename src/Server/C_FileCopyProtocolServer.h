/****************************************************************************
*@file: C_FileCopyProtocolServer.h
*@brief: AQ33_CS往对等方传输文件通信协议实现类
*		 
*@author: zhangchao@oristartech.com
*@dade: 2015-11-12
****************************************************************************/
#ifndef C_FILECOPYPROTOCOLSERVER_H
#define C_FILECOPYPROTOCOLSERVER_H

#include <string>
#include <map>
#include "C_ITcpSserver.h"
#include "C_RecvDataObserverImp.h"
#include "ConstDef.h"
#include "C_WriteDataThread.h"
namespace FileCopyProtocol
{
	class CFileCopyProtocolServer
	{
	public:
		CFileCopyProtocolServer();
		~CFileCopyProtocolServer();
	public:
		void SetServerPort(const unsigned short& port );
		void SetLogPath(const std::string& path );
		bool Start();
		bool Close();
	public:
		void Process(const NetData& data );
	private:
		void Init();
		/**
		*brief：在对等机器上创建文件
		*/
		void CreateFile(const NetData& data );

		/**
		*brief：传输文件
		*/
		void TransferFile(const NetData& data );

		/**
		*brief：文件传输完成关闭文件
		*/
		void TransferFileFinish(const NetData& data );

		/**
		*brief：文件传输错误时删除整个路径
		*/
		void RemoveDirectoryAll(const NetData& data );

		/**
		*brief：删除整个目录下的所有文件
		*/
		void RemoveDirectory(const NetData& data );
		
		/**
		*brief：建立目录，如果目标目录存在则不再创建
		*/
		void CreateDirectoryNotRm(const NetData& data );
		
		/**
		*brief：建立目录,存在先删除，再创建
		*/
		void CreateDirectoryRm(const NetData& data );

		/**
		*brief：修改目录权限
		*/
		void ChangeDirectoryOwn(const NetData& data );

		/**
		*brief：返回修改结果
		*/
		void RecToClient(const unsigned int& typeNo, const Errorno& erno );

	private:
		ICTcpServer *m_pTcpServer;
		CRecvDataObserverImp *m_pRecvDataObserver;
		unsigned short m_serverPort;
		std::string m_curpath;//当前文件路径
		std::map<int, std::string> m_errNoErrMsgMap;
		std::string m_LogPath;//日志文件输出文件夹
		CWriteDataThread *m_pWriteThread;
	};
}
#endif // C_FILECOPYPROTOCOLSERVER_H
