//@file:C_HallList.h
//@brief: 包含类C_HallList。
//C_HallList:内部封装了影院内影厅的链表，提供添加、删除、修改影厅设备等操作。
//同时封装对影厅所有设备的操作。
//@author:wangzhongping@oristartech.com
//dade:2012-07-12



 #ifndef HALL_LIST
 #define HALL_LIST

#include "threadManage/C_CS.h"
#include "C_constDef.h"
#include "DataManager.h"
#include "C_Hall.h"
#include <string>
#include <list>
//using namespace std;


class C_HallList
{
protected:
	C_HallList();
    
public:
	
    ~C_HallList();
    static C_HallList *GetInstance();
	static void DestoryInstance();

   	//初始化所有影厅。
    int Init(CDataManager *);
   
	//初试化各个影厅的计划任务；
    int InitHallsTask();

	//添加影厅
	int AddHall(HALL_PARA &hallPara, std::string &strError);

	// 获取影厅状态
	bool GetHallStat(std::string &strHallID);

	// 获取SMS工作状态
	bool GetSMSWorkState(std::string &strHallID,int &iState,std::string &strInfo);

	// 添加DCP
	bool SMSIngest(std::string &strHallID,std::string strPath);

	// 添加KDM
	bool SMSIngestKeyForm(std::string &strHallID,std::string strPathName);

	// 删除KDM
	bool SMSDeleteKDM(std::string &strHallID,std::string strKDMID);

public:
	// 创建影厅
	int CreateHall(HALL_PARA &hallPara, std::string &strError);

private:
	 
    C_CS m_CS;
	std::list<C_Hall *> m_Halllist;
    static C_HallList *m_pInstance; 
	CDataManager *m_ptrDM;

};
#endif //HALL_LIST;
