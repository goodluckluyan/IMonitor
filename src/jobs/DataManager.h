#ifndef DATAMANAGER_INC
#define DATAMANAGER_INC

#include "C_constDef.h"
#include "threadManage/C_CS.h"
#include <map>
// ��������м�����ݽ��й���
class CDataManager
{
public:
	CDataManager();
	~CDataManager();
	
	bool Init();


	// ��������������Ϣ
	void SetEthBaseInfo(std::map<std::string,int> &mapEthBaseInfo);

	// ������һ̨���������еĵ��ȳ������ģ��ļ������
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


	// ���±����ĸ����������
	// ���´��̳��еļ������
	bool UpdateDevStat(DiskInfo &df);

	// ���������������
	bool UpdateNetStat(std::vector<EthStatus> &vecEthStatus);

	// ����sms״̬
	bool UpdateSMSStat();

	// ����TMS״̬
	bool UpdateTMSStat();

	// ��ȡ������
	bool GetDevStat();
	bool GetNetStat();
	bool GetSMSStat();
	bool GetTMSStat();

	//��������
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