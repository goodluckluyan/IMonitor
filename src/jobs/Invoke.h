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
	
	}

	// 初始化
	int  Init();

	void DeInit();
	
	// 添加任务
	bool AddInitTask();

	// 执行命令
	int Exec(int iCmd,void * ptrPara);

	//获取定时任务的时间间隔
	int GetCheckDelay(int nStateType);

	bool SwitchTMS();

	bool SwitchSMS(std::string strHallID);

	bool SwitchAllSMS();

	void StartTMS();

	void StartALLSMS();

	void Exit();

	void TakeOverMain();

	void ChangeToStdby();

private:
	// 打印帮助信息
	void PrintProductInfo();

	void PrintLogLevel();

	// 处理用户命令输入
	int Controller();

	// 解析用户命令
	void ParseCmd(std::string strCmd, std::vector<std::string> &vecParam);
	
private:

	C_HallList *m_ptrLstHall;
	CheckDisk *m_ptrDisk;
	Test_NetCard *m_ptrNet;
	CDispatch *m_ptrDispatch;
	CMonitorSensor * m_ptrMonitor;
	CTMSSensor * m_ptrTMS;
};

#endif