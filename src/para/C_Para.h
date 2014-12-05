//@file:C_Para.h
//@brief: 包含类C_Para。
//C_Para:读取系统配置文件中的各个参数。
//@author:wangzhongping@oristartech.com
//dade:2012-07-12


#ifndef _TMS20_PARA
#define _TMS20_PARA
#include <string>
using namespace std;
class C_Para
{
public:
 //method:
    static C_Para* GetInstance();
	static void DestoryInstance();
    ~C_Para();
    //读取配置参数。
    int ReadPara();
protected:
     C_Para();
public:    
//Property:  
	//数据库服务器的IP  
    string m_strDBServiceIP;

   	//数据库服务器的端口号
    unsigned short m_usDBServicePort;

    //数据库服务器登陆的用户名。
    string m_strDBUserName;

    //数据库服务器登陆的用户名。
    string m_strDBPWD;
    string m_strDBName;

    //系统线程池中线程的个数。
    unsigned int m_uiThreadCount;

	//日志存放路径
    string m_strLogPath;

	//是主、辅调度程序
	bool m_bMain ;

	//本机webservice服务打开的端口
	//std::string m_strWebServiceIP;
	int m_nWebServicePort;

	//对端的调度程序的IP
	std::string m_strOIP;

	//对端的调度程序的端口
	int m_nOPort ;

	//TMS的Webservice端口
	int m_nTMSWSPort;

	//对端的调度程序wsdl的URI
	std::string m_strOURI;

	//tms启动目录
	std::string m_strTMSPath;

	//tomcat启动目录
	std::string m_strTOMCATPath;

	//主目录
	std::string m_strInipath;

	//启动sms的方式，1为同一终端启动  2为新终端启动
	int m_nStartSMSType ;

	//等待对端调度软件启动检测超时时间,单位秒
	int m_nTimeOutWaitOtherIMonitor;

	//写日志级别
	int m_nWirteLogLevel;

	// 检测磁盘状态时间间隔
	int m_nDiskCheckDelay;

	// 检测网卡状态时间间隔
	int m_nEthCheckDelay;

	// 检测网卡状态时间间隔
	int m_nHallListCheckDelay;

	// 检测网卡状态时间间隔
	int m_nTMSCheckDelay;

	// 检测对端调度程序状态时间间隔
	int m_nOtherMonitorCheckDelay;

	// 检测对端TMS状态时间间隔
	int m_nOtherTMSCheckDelay;

	// 检测对端SMS状态时间间隔
	int m_nOtherSMSCheckDelay;

	// 检测对端磁盘状态时间间隔
	int m_nOtherRAIDCheckDelay;

	// 检测对端网络状态时间间隔
	int m_nOtherEthCheckDelay;

	// 检测对端交换机时间间隔
	int m_nOtherSwitchCheckDelay;

	// 检测对端速度限制时间间隔
	int m_nOtherSpeedLmtCheckDelay;

	// 检测对端异常状态时间间隔
	int m_nOtherEWCheckDelay;
	
private:
    static C_Para *m_pInstance;
};
#endif //_TMS20_PARA
