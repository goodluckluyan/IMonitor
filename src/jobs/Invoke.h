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
		SAFE_DELETE(m_ptrLstHall);
		SAFE_DELETE(m_ptrDisk);
		SAFE_DELETE(m_ptrNet);
		SAFE_DELETE(m_ptrDispatch);
		SAFE_DELETE(m_ptrMonitor);
		SAFE_DELETE(m_ptrTMS);
	}

	// ��ʼ��
	int  Init();
	
	// �������
	bool AddInitTask();

	// ִ������
	int Exec(int iCmd,void * ptrPara);

	//��ȡ��ʱ�����ʱ����
	int GetCheckDelay(int nStateType);

private:
	// ��ӡ������Ϣ
	void PrintProductInfo();

	// �����û���������
	int Controller();

	// �����û�����
	void ParseCmd(std::string strCmd, std::vector<std::string> &vecParam);
	
private:
	// ������״̬ʱ����
	int m_nDiskCheckDelay;

	// �������״̬ʱ����
	int m_nEthCheckDelay;

	// �������״̬ʱ����
	int m_nHallListCheckDelay;

	// �������״̬ʱ����
	int m_nTMSCheckDelay;

	// ���Զ˵��ȳ���״̬ʱ����
	int m_nOtherMonitorCheckDelay;

	// ���Զ�TMS״̬ʱ����
	int m_nOtherTMSCheckDelay;

	// ���Զ�SMS״̬ʱ����
	int m_nOtherSMSCheckDelay;

	// ���Զ˴���״̬ʱ����
	int m_nOtherRAIDCheckDelay;

	// ���Զ�����״̬ʱ����
	int m_nOtherEthCheckDelay;

	// ���Զ˽�����ʱ����
	int m_nOtherSwitchCheckDelay;

	// ���Զ��ٶ�����ʱ����
	int m_nOtherSpeedLmtCheckDelay;

	// ���Զ��쳣״̬ʱ����
	int m_nOtherEWCheckDelay;

	C_HallList *m_ptrLstHall;
	CheckDisk *m_ptrDisk;
	Test_NetCard *m_ptrNet;
	CDispatch *m_ptrDispatch;
	CMonitorSensor * m_ptrMonitor;
	CTMSSensor * m_ptrTMS;
};

#endif