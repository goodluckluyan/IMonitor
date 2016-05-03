/****************************************************************************
*@file: C_ITcpServer.h
*@brief: Tcp服务器端协议对外接口定义
*		 
*@author: zhangchao@oristartech.com
*@dade: 2015-11-11
****************************************************************************/
#ifndef C_ITCPSERVER
#define C_ITCPSERVER

//接收或发送的数据
const int MAXBUFFERSZ =90000; /*9000;*/
typedef struct NetData
{
    int sz;
    char buffer[MAXBUFFERSZ];
}NetData;

//接收数据观察者接口
class IRecvDataObserver
{
public:
    virtual void RecvNetData(const NetData& netData ) =0;
};

class ICTcpServer
{
public:
    virtual ~ICTcpServer( ){}
	/**
	*brief：启动TCP服务端
	*/
    virtual bool StartServer() =0;

	/**
	*brief：启动TCP服务端
	*/
    virtual bool StartServer(const unsigned short& port ) =0;

	/**
	*brief：关闭TCP服务端
	*/
    virtual bool CloseServer() =0;

	/**
	*brief：设置服务端端口
	*/
    virtual void SetServerPort(const unsigned short& port ) =0;

	/**
	*brief：写入要发送的数据
	*/
    virtual bool WriteData(const NetData&netData ) =0;

	/**
	*brief：设置接收数据观察者
	*/
    virtual void SetRecvDataObserver(IRecvDataObserver*pObserver ) =0;
};
extern "C"
{
	ICTcpServer* createTcpServer();
	void releaseTcpServer(ICTcpServer* pServer );
};
#endif // C_ITCPSERVER

