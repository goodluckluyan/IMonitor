//@file:C_HallList.h
//@brief: 包含类C_HallList。
//C_HallList:内部封装了影院内影厅的链表，提供添加、删除、修改影厅设备等操作。
//同时封装对影厅所有设备的操作。
//@author:wangzhongping@oristartech.com
//dade:2012-07-12



 #ifndef HALL_LIST
 #define HALL_LIST
#include <string>
#include <list>
#include <pthread.h>
#include "threadManage/C_CS.h"
#include "C_constDef.h"
#include "DataManager.h"
#include "TMSSensor.h"
#include "C_Hall.h"


//using namespace std;

struct stConditionSwitch
{
	std::string strHallID;
	std::string strCond;
	int nVal;
	int nTriaggerCnt;
};


class C_HallList
{
public:
	C_HallList()
		:m_ptrDM(NULL)
		,m_ptrTMS(NULL)
	{
		pthread_cond_init(&cond,NULL);
	};
   
	
    ~C_HallList();

   	// 初始化所有影厅。
    int Init(CTMSSensor * ptrTMS);
   
	// 获取SMS工作状态
	bool GetSMSWorkState();

	// 切换sms
	bool SwitchSMS(bool bDelaySwitch,std::string strHallID,int &nState);


	// 在本机启动所有sms
	bool StartAllSMS(bool bCheckOtherSMSRun,std::vector<std::string>& vecHallid,bool bLocalHost=false);

	// 备机调用主机进行切换
	int SwitchSMSByStdby(bool bDelaySwitch,std::string strHallID);

	// 获取在本机运行的hallid
	void GetAllLocalRunHallID(std::vector<std::string>& vecHallID);

	// 获取运行的hallid
	void GetAllRunHallID(std::vector<std::string> &vecHallID);

	// 获取由本机接管理过来的hallid
	void GetTakeOverSMS(std::vector<std::string> &vecHallID);

	// 获取运行主机及webservice端口
	bool GetSMSRunHost(std::string strHallID,std::string &strIP,int &nPort);

	// 添加条件等待切换任务
	bool AddCondSwitchTask(std::string strHallID,std::string strCond,int nVal);

	// 执行条件等待切换任务
	bool ProcessCondSwitchTask();

	// 判断切换sms任务已经在条件任务中存在
	bool IsHaveCondSwitchTask(std::string strHallID);

	// 关闭本机的sms
	bool CloseSMS(std::string strHallID);

	// 在本机开启sms
	bool StartSMS(std::string strHallID);

	// 开启或关闭从sms bSoC true：开启  false 关闭 
	bool StartOrCloseStdBySMS(bool bSoC,std::string strHallID);

	// 更新数据库sms运行的位置
	bool UpdateDataBase(std::string strHallID,int nPosition);

	// 获取sms的运行位置
	bool GetSMSPosition(std::string strHallID,std::string &strIP,int& nPort);

	// 改变sms运行位置 pos: 1主机 2备机 只有主机才会调用这个函数
	bool ChangeSMSHost(std::string strHallID,int nPos);

private:

	// 在新的里程中执行脚本
	bool ExeShell_Fork(std::string strPath,std::string strShell);

	// 获取指定进程的pid
	int GetPID(std::string strName,std::vector<int>& vecPID);

	// 获取进程的启动目录
	bool GetPIDExeDir(int ,std::string &);

	// 通过service 命令重启自己
	bool RebootMyself();


    C_CS m_csHallCurState;//保护m_mapHallCurState
	std::map<std::string,int> m_mapHallCurState;

	std::map<std::string,C_Hall *> m_mapHall;
    static C_HallList *m_pInstance; 
	std::string m_WebServiceLocalIP;
	std::string m_WebServiceOtherIP;
	CDataManager *m_ptrDM;
	CTMSSensor * m_ptrTMS;

	std::list<stConditionSwitch>  m_lstCondSwitch;
	C_CS m_csCondTaskLst;// 保护条件
	pthread_cond_t cond; 

	std::map<std::string,C_CS*> m_mapCS;// 串行化切换和获取状态
	

};
#endif //HALL_LIST;
