//@file:C_ThreadManage.h
//@brief: 包含C_ThreadManage：通过链表封装了C_ThreadData。
//@author: wangzhongping@oristartech.com
//date: 2012-05-23

#ifndef THREAD_MANAGE
#define THREAD_MANAGE
#include "C_ThreadData.h"
#include <list>
using namespace std;
class C_ThreadManage
{
protected:
    C_ThreadManage();
public:
    ~C_ThreadManage();
    static C_ThreadManage *GetInstance();
	static void DestoryInstance();
    int InitThreadData();
	void  DeInit();

    //得到空闲的线程。
    //返回值 
    //pThreadData:C_ThreadData 指针的地址。
    int GetIdlThread(C_ThreadData **pThreadData);
    int GetTimeoutThead(C_ThreadData **pThreadData,int iCurTime);
    int GetFinishThread(C_ThreadData **pThreadData);
    int GetThreadData(pthread_t &threadId,C_ThreadData **pThreadData);
    int InitWebserviceThread();
    int InitWebserviceThreadJMZX();
	int InitWebserviceThreadCenter();////duheqing 2014-01-10
    void CheckTimeoutThread();
    
//20140305 by xiaozhengxiu add
    void create_ssl_lock(void);
    void ssl_lock_cleanup(void);
//20140305 by xiaozhengxiu add end		
private:
   list<C_ThreadData* > m_ThreadDataList;
   static C_ThreadManage* m_pInstance;
   list<C_ThreadData*>::iterator m_itIdl;
   list<C_ThreadData*>::iterator m_itTimeout;
   list<C_ThreadData*>::iterator m_itFinish;
   pthread_t  m_hWebserviceThread;
   C_CS m_cs;
};
#endif //THREAD_MANAGE
