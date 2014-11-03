//@file:DataManager.h
//@brief: ����״̬���ݵĹ���
//@author:luyan@oristartech.com
//date:2014-09-17

#ifndef DATAMANAGER_INC
#define DATAMANAGER_INC

#include "C_constDef.h"
#include "threadManage/C_CS.h"
#include <map>
// ��������м�����ݽ��й���
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


	// ��������������Ϣ
	void SetEthBaseInfo(std::map<std::string,int> &mapEthBaseInfo);

	// ����sms������Ϣ
	void SetSMSInfo(std::vector<SMSInfo> vecHall);

	// ������һ̨���������еĵ��ȳ������ģ��ļ������
	bool UpdateOtherMonitorState(bool bMain,int nState);
	bool UpdateOtherTMSState(bool bRun,int nWorkState,int nState);
	bool UpdateOtherSMSState(std::vector<SMSStatus> &vecSMSStatus);
	bool UpdateOtherRaidState(int nState,int nReadSpeed,int nWriteSpeed,
		std::vector<int> &vecDiskState);
	bool UpdateOtherEthState(std::vector<EthStatus> &vecEthStatus);
	bool UpdateOtherSwitchState(int nSwitch1State,int nSwitch2State);
	bool UpdateOtherSpeedLmtState(bool bEnableIngest,int nSpeedLimit);
	bool UpdateOtherSMSEWState(int nState,std::string  strInfo,std::string  strHall);


	// ���±����ĸ����������
	// ���´��̳��еļ������
	bool UpdateDevStat(DiskInfo &df);

	// ���������������
	bool UpdateNetStat(std::vector<EthStatus> &vecEthStatus);

	// ����sms״̬
	bool UpdateSMSStat(std::string strHallID,int nState,std::string strInfo);
	bool UpdateSMSStat(std::string strHallID,SMSInfo &stSMSInfo);

	// ����TMS״̬
	bool UpdateTMSStat(int state);

	// ��ȡ������
	bool GetDevStat(DiskInfo &df);
	bool GetNetStat(std::map<std::string,EthStatus> &mapEthStatus);
	bool GetSMSStat(std::vector<SMSStatus>& vecSMSState);
	int GetTMSStat();

	// ��������
	// ��ӡtms״̬
	void PrintTMSState();

	// ��ӡ����״̬
	void PrintDiskState();

	// ��ӡsms״̬
	void PrintSMSState();

	// ��ӡ����״̬
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
