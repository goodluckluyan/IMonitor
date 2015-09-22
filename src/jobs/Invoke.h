//@file:Invoke.h
//@brief: ���������࣬�������������ĵĵ��á�
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
	  ,m_ptrHash(NULL)
	  ,m_ptrFO(NULL)
	  {
	  }


	~CInvoke()
	{
	
	}

	// ��ʼ��
	int  Init();

	void DeInit();
	
	// �������
	bool AddInitTask();

	// ִ������
	int Exec(int iCmd,void * ptrPara);

	// ��ȡ��ʱ�����ʱ����
	int GetCheckDelay(int nStateType);

	// �л�tms
	bool SwitchTMS();

	// �л�sms��bDelaySwitch�Ƿ�
	bool SwitchSMS(std::string strHallID,bool bDelaySwitch,int &nState);

	// �л�����sms���Զ�����
	bool SwitchAllSMS();

	// ����tms
	void StartTMS();

	// ���������е�sms,���ǽӹ�sms
	void StartALLSMS(bool bCheckOtherSMSRun=true,bool bLocalHost=false);

	// ���ӹܹ�����sms��ԭ�������л�ԭ��
	void SwtichTakeOverSMS();

	// �˳����
	void Exit();

	// ����ͨ��ʧ����ӻ��ӹ����� 
	void TakeOverMain(bool bCheckOtherSMSRun=true);

	// �ӻ�ͨ��ʧ���������ӹܴӻ�
	void TakeOverStdby(bool bCheckOtherSMSRun=true);

	// �����ָ���ӻ��ӽӹ�״̬�ػָ� 
	void ChangeToStdby();

	// �ӻ��ָ��������ӽӹ�״̬�ػָ�
	void ChangeToMain();

	// �ر�sms�������ͻʱʹ��
	bool CloseSMS(std::string strHallID);

	// ����sms�������ͻʱʹ��
	bool StartSMS(std::string strHallID);

	// ���sms���г�ͻ
	bool SolveConflict(std::vector<ConflictInfo> &vecCI);

	// hashУ��
	int DcpHashCheck(std::string strPath,std::string strPklUuid,std::string &strErrInfo);

	// ��ȡhashУ��Ľ���
	int GetHashCheckPercent(std::string strPklUuid,int &nResult,int &nPercent,std::string &strErrInfo);

	// ����dcp
	int CopyDcp(std::string PklUuid,std::string srcPath,std::string desPath,int &result ,std::string &errinfo);

	// ɾ��dcp
	int DeleteDcp(std::string PklUuid,std::string Path,int &result ,std::string &errinfo);

	// ��ȡ����dcp�Ľ���/���
	bool GetCopyDcpProgress(std::string &strPKIUUID,int &nResult,std::string &strErrInfo);

	// ��ȡɾ��dcp�Ľ���/���
	bool GetDeleteDcpProgress(std::string &strPKIUUID,int &nResult,std::string &strErrInfo);

	// ��������ͬ����ǣ������һ̨�������ִ˱�־˵�����ݿ�ͬ����ɡ�
	bool UpdateDBSynch(std::string dbsynch);

	// ������ݿ��Ƿ�ͬ��
	bool CheckDBSynch(long lSynch);

	// ֪ͨtms smsλ��
	bool NoticTMSSMSPos();
private:
	// ��ӡ������Ϣ
	void PrintProductInfo();

	// ��ӡ��־����
	void PrintLogLevel();

	// �����û���������
	int Controller();

	// �����û�����
	void ParseCmd(std::string strCmd, std::vector<std::string> &vecParam);
	
private:

	C_HallList *m_ptrLstHall;
	CheckDisk *m_ptrDisk;
	Test_NetCard *m_ptrNet;
	CDispatch *m_ptrDispatch;
	CMonitorSensor * m_ptrMonitor;
	CTMSSensor * m_ptrTMS;
	CHashCheck * m_ptrHash;
	CFileOperator * m_ptrFO;
};

#endif