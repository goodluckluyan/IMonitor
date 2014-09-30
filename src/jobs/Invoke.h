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
	// 检测磁盘状态时间间隔
	int m_nDiskCheckDelay;

	// 检测网卡状态时间间隔
	int m_nEthCheckDelay;

	// 检测对端调度程序状态时间间隔
	int m_nOtherMonitorCheckDelay;

	// 检测对端TMS状态时间间隔
	int m_nOtherTMSCheckDelay;

	// 检测对端SMS状态时间间隔
	int m_nOtherSMSCheckDelay;

	// 检测对端磁盘状态时间间隔
	int m_nOtherRAIDCheckDelay;

	// 检测对端网络状态时间间隔
	int m_nOtherEthCheckDelay;

	// 检测对端交换机时间间隔
	int m_nOtherSwitchCheckDelay;

	// 检测对端速度限制时间间隔
	int m_nOtherSpeedLmtCheckDelay;

	// 检测对端异常状态时间间隔
	int m_nOtherEWCheckDelay;

	C_HallList *m_ptrLstHall;
	CheckDisk *m_ptrDisk;
	Test_NetCard *m_ptrNet;
	CDispatch *m_ptrDispatch;
	CMonitorSensor * m_ptrMonitor;
};

#endif