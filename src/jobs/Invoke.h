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
#include "HashCheck.h"
#include "FileOperator.h"
#include "watchdog.h"
#define SAFE_DELETE(ptr) if(ptr != NULL) {delete ptr ; ptr = NULL;}
#define INGEST_MODULE



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
	  ,m_ptrHash(NULL)
	  ,m_ptrFO(NULL)
	  ,m_bSMSBootDelay(false)
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
	void StartALLSMS(bool bCheckOtherSMSRun=true,bool bLocalHost=false);

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

	// hash校验
	int DcpHashCheck(std::string strPath,std::string strPklUuid,std::string &strErrInfo);

	// 获取hash校验的进度
	int GetHashCheckPercent(std::string strPklUuid,int &nResult,int &nPercent,std::string &strErrInfo);

	// 拷贝dcp
	int CopyDcp(std::string PklUuid,std::string srcPath,std::string desPath,int &result ,std::string &errinfo);

	// 删除dcp
	int DeleteDcp(std::string PklUuid,std::string Path,int &result ,std::string &errinfo);

	// 获取拷贝dcp的进度/结果
	bool GetCopyDcpProgress(std::string &strPKIUUID,int &nResult,std::string &strErrInfo);

	// 获取删除dcp的进度/结果
	bool GetDeleteDcpProgress(std::string &strPKIUUID,int &nResult,std::string &strErrInfo);

	// 更新数据同步标记，如果另一台主机发现此标志说明数据库同步完成。
	bool UpdateDBSynch(std::string dbsynch);

	// 检测数据库是否同步
	bool CheckDBSynch(long lSynch);

	// 通知tms sms位置
	bool NoticTMSSMSPos();

	// 获取sms位置信息
	bool GetSMSPosition(std::string strHallID,std::string &strPos,int& nPort);

	// 重启或关闭服务器
	int ShutdownServer(int nType,int &state,std::string &strDesc);

	// 定时重启服务器
	int TimingRebootServer(int nDay,int nWeek,int nHour,int nMinute,
		int nRepeat,int nRepeatCnt,int &state,std::string &strDesc);

	// 添加定时重启任务
	int SetupRebootTimer();

	// 打印版本信息
	void PrintVersionInfo();

    // kill TMS 不涉及位置变化
    bool KillTMS();

private:
	// 打印帮助信息
	void PrintProductInfo();

	// 打印日志级别
	void PrintLogLevel();

	// 处理用户命令输入
	int Controller();

	// 解析用户命令
	void ParseCmd(std::string strCmd, std::vector<std::string> &vecParam);

	bool GetDBSynchStatus();

	bool GetDBSynchStatus_PIP();

	//重启或关机 0：为重启 1：关机
	void shutdown(int nType);

	// 从数据库中获取本机和对端的ip
	bool GetWebServiceIPFormDB(std::string &WebServiceLocalIP,std::string &WebServiceOtherIP);

	// 判断对端sms的状态是否可用
	bool HasOtherSMSStatus();

	// 看门狗监测
	int CheckByWatchdog();

    // 对第二天重启的任务执行后要把数据库状态重置，设成不操作
    bool  ResetTimeRebootStatus();



	
private:
	std::vector<CWatchdog*> m_vecPtrWathdog;
	C_HallList *m_ptrLstHall;
	CheckDisk *m_ptrDisk;
	Test_NetCard *m_ptrNet;
	CDispatch *m_ptrDispatch;
	CMonitorSensor * m_ptrMonitor;
	CTMSSensor * m_ptrTMS;
	CHashCheck * m_ptrHash;
	CFileOperator * m_ptrFO;
	bool m_bSMSBootDelay;
};

#endif
