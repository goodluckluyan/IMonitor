//@file:DataManager.h
//@brief: 所有状态数据的管理。
//@author:luyan@oristartech.com
//date:2014-09-17

#ifndef DATAMANAGER_INC
#define DATAMANAGER_INC

#include "C_constDef.h"
#include "threadManage/C_CS.h"
#include <map>
// 负责对所有监控数据进行管理
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
	bool UpdateDevStat(DiskInfo &df);

	// 更新网卡监测数据
	bool UpdateNetStat(std::vector<EthStatus> &vecEthStatus);

	// 更新sms状态
	bool UpdateSMSStat(std::string strHallID,int nState,std::string strInfo);
	bool UpdateSMSStat(std::string strHallID,SMSInfo &stSMSInfo);

	// 更新TMS状态
	bool UpdateTMSStat(int state);

	// 读取监测参数
	bool GetDevStat(DiskInfo &df);
	bool GetNetStat(std::map<std::string,EthStatus> &mapEthStatus);
	bool GetSMSStat(std::vector<SMSStatus>& vecSMSState);
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

	void *GetInvokerPtr();
private:
	CDataManager();

	//DiskStatus;
	DiskInfo m_df;

	std::map<std::string,EthStatus> m_mapEthStatus;
	std::map<std::string,SMSInfo> m_mapSmsStatus;
	int m_nTMSState;

	C_CS m_csDisk;
	C_CS m_csNet;
	C_CS m_csSMS;
	C_CS m_csTMS;
	static CDataManager * m_pinstance;
	void * m_ptrInvoker;

};

#endif
