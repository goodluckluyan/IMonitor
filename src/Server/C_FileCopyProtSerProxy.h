/****************************************************************************
*@file: C_FileCopyProtSerProxy.h
*@brief: AQ33_CS往对等方传输文件通信协议服务端代理类
*		 
*@author: zhangchao@oristartech.com
*@dade: 2015-11-12
****************************************************************************/
#ifndef CFILECOPYPROTOCOLSERVERPROXY_H
#define CFILECOPYPROTOCOLSERVERPROXY_H

#include <string>

namespace FileCopyProtocol
{
	class CFileCopyProtocolServer;
}
class CFileCopyProtSerProxy
{
public:
	CFileCopyProtSerProxy();
	~CFileCopyProtSerProxy();

	/**
	*brief:设置服务端口号
	*/
	void SetServerPort(const unsigned short& port );

	/**
	*brief:设置日志输出路径(目录路径)
	*/
	void SetLogPath(const std::string& path );

	/**
	*brief:开启服务
	*/
	bool Start();

	/**
	*brief:关闭服务
	*/
	bool Close();
private:
	FileCopyProtocol::CFileCopyProtocolServer *m_pServer;
};

#endif // CFILECOPYPROTOCOLSERVERPROXY_H
