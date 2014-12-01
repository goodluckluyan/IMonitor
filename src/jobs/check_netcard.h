//@file:check_netcard.h
//@brief: 网卡状态监测。
//@author:zhangmiao@oristartech.com
//date:2014-09-17

#ifndef CHECKNETCARD_INC
#define CHECKNETCARD_INC

#include <linux/sockios.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "DataManager.h"

class Test_NetCard
{
public:
	Test_NetCard();
	~Test_NetCard();

	//初始化函数，必须先被调用
	int Init();

	//初始化检测
	bool InitAndCheck();

	// 获取所有可用网卡状态
	bool GetAllEthStatus();
private:

	bool ReadEthinfoTable(std::map<std::string,int>& mapEthBaseInfo);

	//检测网卡的连接状态，rLinkStatus = 1,连接正常，rLinkStatus = 0,连接不正常 
	int Check_EthLinkStatus(const char* eth_name , int& rLinkStatus );

	//网络带宽测量 eth_name=网卡名称必须为"eth0,eth1,eth2,eth3,eth4,eth5"中1个;TranBPS=表示每秒发送字节数，RecvBPS=表示每秒接收字节数
	int Check_NewWork_Flow( std::string eth_name , unsigned long long& TranBPS , unsigned long long& RecvBPS );
	
	
private:

	bool m_bInit;
	std::map<std::string,time_t> m_old_CheckTime;	//保存每个网卡上一次的检测时间戳
	std::vector<std::string> m_vecEth;


	std::map<std::string,unsigned long long> m_oldTx_bytes ;
	std::map<std::string,unsigned long long> m_oldRx_bytes;	//保存每个网卡上一次的发送/或接收的总字节数

	CDataManager * m_ptrDM;


};

#endif



