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

	// ��ʼ��
	int  Init();

	void DeInit();
	
	// �������
	bool AddInitTask();

	// ִ������
	int Exec(int iCmd,void * ptrPara);

	//��ȡ��ʱ�����ʱ����
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
	// ��ӡ������Ϣ
	void PrintProductInfo();

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
};

#endif