//@file:Invoke.h
//@brief: 命令适配类，负责对所有命令的的调用。
//@author:luyan@oristartech.com
//date:2014-09-17 

#ifndef INVOKE_INC
 #define INVOKE_INC
#include "C_HallList.h"
#include "CheckDisk.h"
#include "DataManager.h"
#include "Dispatch.h"
#include "check_netcard.h"
#include "MonitorSensor.h"
#include "TMSSensor.h"


#define SAFE_DELETE(ptr) if(ptr != NULL) {delete ptr ; ptr = NULL;}
class CInvoke
{
public:
	CInvoke():
	  m_ptrLstHall(NULL)
	  ,m_ptrDisk(NULL)
	  ,m_ptrNet(NULL)
	  ,m_ptrMonitor(NULL)
	  ,m_ptrDispatch(NULL)
	  ,m_ptrTMS(NULL)
	  {
	  }


	~CInvoke()
	{
		SAFE_DELETE(m_ptrLstHall);
		SAFE_DELETE(m_ptrDisk);
		SAFE_DELETE(m_ptrNet);
		SAFE_DELETE(m_ptrDispatch);
		SAFE_DELETE(m_ptrMonitor);
		SAFE_DELETE(m_ptrTMS);
	}

	// 初始化
	int  Init();
	
	// 添加任务
	bool AddInitTask();

	// 执行命令
	int Exec(int iCmd,void * ptrPara);

	//获取定时任务的时间间隔
	int GetCheckDelay(int nStateType);

private:
	// 打印帮助信息
	void PrintProductInfo();

	// 处理用户命令输入
	int Controller();

	// 解析用户命令
	void ParseCmd(std::string strCmd, std::vector<std::string> &vecParam);
	
private:
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

	C_HallList *m_ptrLstHall;
	CheckDisk *m_ptrDisk;
	Test_NetCard *m_ptrNet;
	CDispatch *m_ptrDispatch;
	CMonitorSensor * m_ptrMonitor;
	CTMSSensor * m_ptrTMS;
};

#endif