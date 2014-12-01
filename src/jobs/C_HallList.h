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
#include "C_Hall.h"


//using namespace std;

struct stConditionSwitch
{
	std::string strHallID;
	std::string strCond;
	int nVal;
};

class C_HallList
{
public:
	C_HallList()
		:m_ptrDM(NULL)
	{
		pthread_cond_init(&cond,NULL);
	};
   
	
    ~C_HallList();

   	//初始化所有影厅。
    int Init(bool bRunOther);
   
	// 获取SMS工作状态
	bool GetSMSWorkState();

	// 切换sms
	bool SwitchSMS(std::string strHallID,int &nState);

	// 获取hallid
	void GetAllHallID(std::vector<std::string>& vecHallID);

	// 获取运行主机及webservice端口
	bool GetSMSRunHost(std::string strHallID,std::string &strIP,int &nPort);

	// 添加条件等待切换任务
	bool AddCondSwitchTask(std::string strHallID,std::string strCond,int nVal);

	// 执行条件等待切换任务
	bool ProcessCondSwitchTask();

	// 判断切换sms任务已经在条件任务中存在
	bool IsHaveCondSwitchTask(std::string strHallID);

private:

	//启动tomcat
	bool StartTOMCAT(std::string strPath);

	//关闭tomcat
	bool ShutdownTOMCAT(std::string strPath);


	int GetPID(std::string strName,std::vector<int>& vecPID);

	bool GetPIDExeDir(int ,std::string &);

    C_CS m_CS;
	std::map<std::string,int> m_mapHallCurState;

	std::map<std::string,C_Hall *> m_mapHall;
    static C_HallList *m_pInstance; 
	std::string m_WebServiceLocalIP;
	std::string m_WebServiceOtherIP;
	CDataManager *m_ptrDM;

	std::list<stConditionSwitch>  m_lstCondSwitch;
	C_CS m_csCondTaskLst;
	pthread_cond_t cond; 
	C_CS m_csSwitching;

};
#endif //HALL_LIST;
