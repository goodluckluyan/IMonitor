//@file:C_Para.h
//@brief: 包含类C_Para。
//C_Para:读取系统配置文件中的各个参数。
//@author:wangzhongping@oristartech.com
//dade:2012-07-12


#ifndef _TMS20_PARA
#define _TMS20_PARA
#include <string>
#include <pthread.h>
using namespace std;

// 主机角色，主机只有MAINROLE一种状态，备机有STDBYROLE和TMPMAINROLE两种状态
enum enHOSTROLE{MAINROLE = 1,ONLYMAINROLE=2,STDBYROLE = 3,TMPMAINROLE = 4};
class C_Para
{
public:
 //method:
    static C_Para* GetInstance();
	static void DestoryInstance();
    ~C_Para();
    //读取配置参数。
    int ReadPara();

	bool IsMain();
	int GetRole();
	bool SetRoleFlag(int nRole);
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
	enHOSTROLE m_enRole ;

	//本机webservice服务打开的端口
	//std::string m_strWebServiceIP;
	int m_nWebServicePort;

	//本地调度程序的IP
	std::string m_strLIP;

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
	//std::string m_strTOMCATPath;

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

	// SSD RAID数量
	int m_nSSD_Raid_Num;

	// Sata RAID数量
	int m_nSATA_Raid_Num;

    // 数据库同步检测
	int m_nDBSynchCheckDelay;
private:
    static C_Para *m_pInstance;
	pthread_rwlock_t m_rwlk_main;
};
class GlobalStatus
{
public:
	static GlobalStatus *GetInstinct()
	{
		return m_globalstatus;
	}
	~GlobalStatus()
	{
		pthread_mutex_destroy(&m_mutx);
		pthread_cond_destroy(&m_cond);
	}

	int GetStatus()
	{
		return m_RunState;
	}

	int SetStatus(int stat)
	{
		int nRet = 0;
		if(stat == 1)
		{
			m_RunState = 1;
			pthread_cond_signal(&m_cond);
		}
		else
		{
			// 状态变为非正常状态时，要求当前状态为正常状态，否则等待！
			pthread_mutex_lock(&m_mutx);
			while(m_RunState != 1)
			{
				pthread_cond_wait(&m_cond,&m_mutx);
				
			}
			m_RunState = stat;
			pthread_mutex_unlock(&m_mutx);
		}
		

	}


private:
	GlobalStatus()
	{
		pthread_mutex_init(&m_mutx,NULL);
		pthread_cond_init(&m_cond,NULL);
		m_RunState = 0;

	}


	int m_RunState;// 0为正在启动，1为运行正常 2为在接管或恢复接管阶段 3为解决冲突阶段
	static GlobalStatus * m_globalstatus;
	pthread_mutex_t m_mutx;
	pthread_cond_t m_cond;
};
#endif //_TMS20_PARA
