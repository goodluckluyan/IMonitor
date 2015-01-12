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

	// 获取定时任务的时间间隔
	int GetCheckDelay(int nStateType);

	// 切换tms
	bool SwitchTMS();

	// 切换sms，bDelaySwitch是否
	bool SwitchSMS(std::string strHallID,bool bDelaySwitch,int &nState);

	// 切换所有sms到对端主机
	bool SwitchAllSMS();

	// 启动tms
	void StartTMS();

	// 启动的所有的sms,就是接管sms
	void StartALLSMS(bool bCheckOtherSMSRun=true);

	// 将接管过来的sms复原，就是切回原机
	void SwtichTakeOverSMS();

	// 退出软件
	void Exit();

	// 主机通信失联后从机接管主机 
	void TakeOverMain(bool bCheckOtherSMSRun=true);

	// 从机通信失联后主机接管从机
	void TakeOverStdby(bool bCheckOtherSMSRun=true);

	// 主机恢复后从机从接管状态回恢复 
	void ChangeToStdby();

	// 从机恢复后主机从接管状态回恢复
	void ChangeToMain();

	// 关闭sms，解决冲突时使用
	bool CloseSMS(std::string strHallID);

	// 开启sms，解决冲突时使用
	bool StartSMS(std::string strHallID);

	// 解决sms运行冲突
	bool SolveConflict(std::vector<ConflictInfo> &vecCI);

private:
	// 打印帮助信息
	void PrintProductInfo();

	// 打印日志级别
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