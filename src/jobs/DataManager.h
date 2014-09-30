#ifndef DATAMANAGER_INC
#define DATAMANAGER_INC

#include "C_constDef.h"
#include "threadManage/C_CS.h"
#include <map>
// 负责对所有监控数据进行管理
class CDataManager
{
public:
	CDataManager();
	~CDataManager();
	
	bool Init();


	// 设置网卡基本信息
	void SetEthBaseInfo(std::map<std::string,int> &mapEthBaseInfo);

	// 更新另一台主机上运行的调度程序各个模块的监测数据
	bool UpdateOtherMonitorState(bool bMain,int nState);
	bool UpdateOtherTMSState(bool bRun,int nWorkState,int nState);
	bool UpdateOtherSMSState(std::string strHallId,bool bRun,int nState,
		int nPosition,std::string strSplUuid);
	bool UpdateOtherRaidState(int nState,int nReadSpeed,int nWriteSpeed,
		std::vector<int> &vecDiskState);
	bool UpdateOtherEthState(int nConnectState,int nSpeed);
	bool UpdateOtherSwitchState(int nSwitch1State,int nSwitch2State);
	bool UpdateOtherSpeedLmtState(bool bEnableIngest,int nSpeedLimit);
	bool UpdateOtherSMSEWState(int nState,std::string  strInfo,std::string  strHall);


	// 更新本机的各个监测数据
	// 更新磁盘陈列的监测数据
	bool UpdateDevStat(DiskInfo &df);

	// 更新网卡监测数据
	bool UpdateNetStat(std::vector<EthStatus> &vecEthStatus);

	// 更新sms状态
	bool UpdateSMSStat();

	// 更新TMS状态
	bool UpdateTMSStat();

	// 读取监测参数
	bool GetDevStat();
	bool GetNetStat();
	bool GetSMSStat();
	bool GetTMSStat();

	//其它方法
private:

	//DiskStatus;
	DiskInfo m_df;

	std::map<std::string,EthStatus> m_mapEthStatus;

	C_CS m_csDisk;
	C_CS m_csNet;
	C_CS m_csSMS;
	C_CS m_csTMS;


};

#endif