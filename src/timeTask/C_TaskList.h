//@file:C_TaskList.h
//@brief: C_TaskList �ڲ�ʹ�������װ��C_Task��
//@author: wangzhongping@oristartech.com
//date: 2012-05-23


#ifndef TASK_LIST
#define TASK_LIST


#include <list>
#include "C_Task.h"
#include <string>
#include "C_constDef.h"
class C_TaskList
{
public:
	~C_TaskList();
	static C_TaskList* GetInstance();
	static void DestoryInstance();

	// ��������
	int AddTask(int iTaskNum,  void *pPara, int iStartTime);

	//���ж�ʱ���� ���� C_Task.m_iStartTime <= ��ǰʱ��iCurTime
	//ʱִ�иö�ʱ����
	int RunTasks(int iCurTime);

	// ��ʼ���������
	int InitTaskList(CInvoke * ptrInvoker);
protected:
	C_TaskList();

private:
	static C_TaskList *m_pInstance;
	std::list<C_Task*> m_TackList;
	int GetIdleTask(C_Task **ppTask); 
};
#endif //TASK_LIST
