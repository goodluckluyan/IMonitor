//@file:C_TaskList.h
//@brief: C_TaskList 内部使用链表封装了C_Task。
//@author: wangzhongping@oristartech.com
//date: 2012-05-23


#ifndef TASK_LIST
#define TASK_LIST


#include <list>
#include "C_Task.h"
#include <string>
#include "C_constDef.h"
#include "threadManage/C_CS.h"
class C_TaskList
{
public:
	~C_TaskList();
	static C_TaskList* GetInstance();
	static void DestoryInstance();

	// 增加任务
	int AddTask(int iTaskNum,  void *pPara, int iStartTime);

	// 增加任务可以设置任务类型
	int AddTask(int iTaskNum,  void *pPara, int iStartTime,int nType);


	//运行定时任务 条件 C_Task.m_iStartTime <= 当前时间iCurTime
	//时执行该定时任务
	int RunTasks(int iCurTime);

	// 初始化任务队列
	int InitTaskList(CInvoke * ptrInvoker);

	// 删除任务
	bool DeleteTask(int nCommandNumber);
protected:
	C_TaskList();

private:
	static C_TaskList *m_pInstance;
	std::list<C_Task*> m_TackList;
	int GetIdleTask(C_Task **ppTask); 
	C_CS m_cs;
};
#endif //TASK_LIST
