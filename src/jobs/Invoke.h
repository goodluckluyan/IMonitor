 #ifndef INVOKE_INC
 #define INVOKE_INC



#include "C_HallList.h"
#include "CheckDisk.h"
#include "DataManager.h"
#include "Dispatch.h"
#include "check_netcard.h"
#include "MonitorSensor.h"
class CInvoke
{
public:
	CInvoke():
	  m_ptrLstHall(NULL)
	  ,m_ptrDisk(NULL)
	  ,m_ptrNet(NULL)
	  ,m_ptrMonitor(NULL)
	  {
	  }

	  CInvoke(C_HallList *ptrLstHall,CheckDisk *ptrDisk,
		  Test_NetCard *ptrNet,CDispatch *ptrDispatch ,CMonitorSensor * ptrMonitor)
		  : m_ptrLstHall(ptrLstHall)
		  ,m_ptrDisk(ptrDisk)
		  ,m_ptrNet(ptrNet)
		  ,m_ptrDispatch(ptrDispatch)
		  ,m_ptrMonitor(ptrMonitor)
	  {
	  }

	~CInvoke()
	{

	}
	
	bool AddInitTask();
	int Exec(int iCmd,void * ptrPara);

	int GetCheckDelay(int nStateType);

private:

	void PrintProductInfo();

	int Controller();

	void ParseCmd(std::string strCmd, std::vector<std::string> &vecParam);
	
private:
	// ������״̬ʱ����
	int m_nDiskCheckDelay;

	// �������״̬ʱ����
	int m_nEthCheckDelay;

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
};

#endif