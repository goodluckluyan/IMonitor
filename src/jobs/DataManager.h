//@file:DataManager.h
//@brief: ����״̬���ݵĹ���
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

	// �����������ָ��
	void SetDispatchPtr(CDispatch *ptr)
	{
		m_ptrDispatch = ptr;
	}


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
	bool UpdateDevStat(std::map<int,DiskInfo> &mapdf);

	// �����̳����Ƿ��д���
	bool CheckRaidError(std::vector<stError> &vecErr);

	// ���������������
	bool UpdateNetStat(std::vector<EthStatus> &vecEthStatus);

	// ���Eth��״̬
	void CheckEthError(std::vector<stError> &vecErr);

	// ����sms״̬
	bool UpdateSMSStat(std::string strHallID,int nState,std::string strInfo);
	bool UpdateSMSStat(std::string strHallID,SMSInfo &stSMSInfo);

	// ����TMS״̬
	bool UpdateTMSStat(int state);

	// ��ȡ������
	bool GetDevStat(std::map<int,DiskInfo> &df);
	bool GetNetStat(std::map<std::string,EthStatus> &mapEthStatus);
	bool GetSMSStat(std::vector<SMSStatus>& vecSMSState);
	bool GetSMSStat(std::string strHallID,SMSInfo& smsinfo);
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

	// ��ȡInvokerָ��
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

	// ��¼�Զ����������쳣������ʱ��
	time_t m_tmOtherHostFail;
	int m_nOterHostFail;
	time_t m_tmCheckTMSNoRun;
	int m_nCheckTMSNoRun;

	// �Զ�����״̬ -1:Ϊδ֪����0��Ϊ�Զ��������� 1��Ϊ�������
	int m_nOtherMonitorState;

	std::map<std::string,std::list<ConflictInfo> > m_maplstConfilict;

};

#endif
