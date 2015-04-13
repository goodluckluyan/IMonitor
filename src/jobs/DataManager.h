//@file:DataManager.h
//@brief: 所有状态数据的管理。
//@author:luyan@oristartech.com
//date:2014-09-17

#ifndef DATAMANAGER_INC
#define DATAMANAGER_INC

#include "C_constDef.h"
#include "threadManage/C_CS.h"
#include "Dispatch.h"
#include <map>


class CDataManager
{
public:
	static CDataManager * GetInstance()
	{
		if(m_pinstance == NULL)
		{
			m_pinstance = new CDataManager;
		}
		return m_pinstance;
	}

	~CDataManager();
	
	bool Init(void *);

	// 包含调度类的指针
	void SetDispatchPtr(CDispatch *ptr)
	{
		m_ptrDispatch = ptr;
	}


	// 设置网卡基本信息
	void SetEthBaseInfo(std::map<std::string,int> &mapEthBaseInfo);

	// 设置sms基本信息
	void SetSMSInfo(std::vector<SMSInfo> vecHall);

	// 更新另一台主机上运行的调度程序各个模块的监测数据
	bool UpdateOtherMonitorState(bool bMain,int nState);
	bool UpdateOtherTMSState(bool bRun,int nWorkState,int nState);
	bool UpdateOtherSMSState(std::vector<SMSStatus> &vecSMSStatus);
	bool UpdateOtherRaidState(int nState,int nReadSpeed,int nWriteSpeed,
		std::vector<int> &vecDiskState);
	bool UpdateOtherEthState(std::vector<EthStatus> &vecEthStatus);
	bool UpdateOtherSwitchState(int nSwitch1State,int nSwitch2State);
	bool UpdateOtherSpeedLmtState(bool bEnableIngest,int nSpeedLimit);
	bool UpdateOtherSMSEWState(int nState,std::string  strInfo,std::string  strHall);


	// 更新本机的各个监测数据
	// 更新磁盘陈列的监测数据
	bool UpdateDevStat(std::map<int,DiskInfo> &mapdf);

	// 检测磁盘陈列是否有错误
	bool CheckRaidError(std::vector<stError> &vecErr);

	// 更新网卡监测数据
	bool UpdateNetStat(std::vector<EthStatus> &vecEthStatus);

	// 检测Eth的状态
	void CheckEthError(std::vector<stError> &vecErr);

	// 更新sms状态
	bool UpdateSMSStat(std::string strHallID,int nState,std::string strInfo);
	bool UpdateSMSStat(std::string strHallID,SMSInfo &stSMSInfo);

	// 更新TMS状态
	bool UpdateTMSStat(int state);

	// 读取监测参数
	bool GetDevStat(std::map<int,DiskInfo> &df);
	bool GetNetStat(std::map<std::string,EthStatus> &mapEthStatus);
	bool GetSMSStat(std::vector<SMSStatus>& vecSMSState);
	bool GetSMSStat(std::string strHallID,SMSInfo& smsinfo);
	int GetTMSStat();

	// 其它方法
	// 打印tms状态
	void PrintTMSState();

	// 打印磁盘状态
	void PrintDiskState();

	// 打印sms状态
	void PrintSMSState();

	// 打印网卡状态
	void PrintEthState();

	// 获取Invoker指针
	void *GetInvokerPtr();
private:
	CDataManager();

	bool CheckConfile(std::vector<ConflictInfo> &vecConflict);


	std::map<int,DiskInfo> m_mapdf;

	std::map<std::string,EthStatus> m_mapEthStatus;
	std::map<std::string,SMSInfo> m_mapSmsStatus;
	std::map<std::string,SMSInfo> m_mapOtherSMSStatus;
	int m_nTMSState;

	C_CS m_csDisk;
	C_CS m_csNet;
	C_CS m_csSMS;
	C_CS m_csTMS;
	static CDataManager * m_pinstance;
	CDispatch * m_ptrDispatch;
	void * m_ptrInvoker;

	// 记录对端主机连接异常次数和时间
	time_t m_tmOtherHostFail;
	int m_nOterHostFail;
	time_t m_tmCheckTMSNoRun;
	int m_nCheckTMSNoRun;

	// 对端主机状态 -1:为未知数，0：为对端正在启动 1：为启动完成
	int m_nOtherMonitorState;

	std::map<std::string,std::list<ConflictInfo> > m_maplstConfilict;

};

#endif
