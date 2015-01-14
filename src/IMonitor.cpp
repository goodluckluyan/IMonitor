//@file:Tms20_DeviceService.cpp
//@brief: ������㣬����main����
//@author:luyan@oristartech.com
//date:2014-09-12

#include <signal.h>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include "threadManage/C_ThreadData.h"
#include "threadManage/C_ThreadManage.h"
#include "para/C_Para.h"
#include "timeTask/C_TaskList.h"
#include "utility/C_Time.h"
#include "para/C_RunPara.h"
#include "log/C_LogManage.h"
#include "database/CppMySQL3DB.h"
#include "utility/C_TcpTransport.h"
#include "execinfo.h"

extern bool g_bQuit;
static void sig_fun(int iSigNum);


void sig_fun(int iSigNum)
{
	C_LogManage *pLogManage = C_LogManage::GetInstance();
	char strInfo[1024];
	sprintf(strInfo, "revc a signal number:%d ", iSigNum);
	pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,strInfo);

	void *stack_p[20];
	char **stack_info;
	int size;
	size = backtrace( stack_p, sizeof(stack_p));
	stack_info = backtrace_symbols( stack_p, size);
	for(int i=0; i<size; ++i)
	{ 
		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,stack_info[i]); 
	}
	free(stack_info);
	fflush(NULL);

}


void InitSigFun(C_LogManage *pLogManage)
{
	if(signal(SIGINT,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGINT"); 
		printf("add signal Number:SIGINT\n");	
	}
	if(signal(SIGALRM,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGALRM"); 
		printf("add signal Number:SIGALRM\n");	
	}
// 	if(signal(SIGHUP,sig_fun) == SIG_ERR)
// 	{
// 		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGHUP"); 
// 		printf("add signal Number:SIGHUP\n");	
// 	}  
	if(signal(SIGPIPE,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGPIPE"); 
		printf("add signal Number:SIGPIPE\n");	
	}  
	if(signal(SIGPOLL,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGPOLL"); 
		printf("add signal Number:SIGPOLL\n");	
	} 
	if(signal(SIGPROF,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGPROF"); 
		printf("add signal Number:SIGPROF\n");	
	}
	if(signal(SIGSTKFLT,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGSTKFLT"); 
		printf("add signal Number:SIGSTKFLT\n");	
	} 
// 	if(signal(SIGTERM,sig_fun) == SIG_ERR)
// 	{
// 		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_WEBSERVICE_CREATE_TRREAD,"add signal Number:SIGTERM"); 
// 		printf("add signal Number:SIGTERM\n");	
// 	}
	if(signal(SIGUSR1,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGUSR1"); 
		printf("add signal Number:SIGUSR1\n");	
	}
	if(signal(SIGUSR2,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGUSR2"); 
		printf("add signal Number:SIGUSR2\n");	
	} 
	if(signal(SIGVTALRM,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGVTALRM"); 
		printf("add signal Number:SIGVTALRM\n");	
	}  
	if(signal(SIGIO,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGIO"); 
		printf("add signal Number:SIGIO\n");	
	} 
	if(signal(SIGABRT,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGABRT"); 
		printf("add signal Number:SIGABRT\n");	
	}

	// �����ӽ��̽���ʱ���͵�SIGCLD�ź� ����ֹ��ʬ����
	signal(SIGCLD,SIG_IGN);
}

//@brief ������㣬����ϵͳ�ĸ��ֳ��Ի�������
//Ȼ�����ϵͳ��ʱ��������������ѯ��
int main(int argc, char** argv)
{
	int iResult = -1;

	//��ȡ�����ļ�
	C_Para *pPara = C_Para::GetInstance();
	iResult = pPara->ReadPara();
	if(iResult != 0)
	{
		printf("read para error ,exit!\n");
		return -1;	
	}

	//��ʼ������ʱ��������---ʱ�����á�
	C_RunPara *pRunPara = C_RunPara::GetInstance();
	pRunPara->Init(); 
	string strInitDate;
	pRunPara->GetCurDate(strInitDate);

	string strExePath;
	pRunPara->GetExePath(strExePath);
	strExePath += '/';
	if(chdir(strExePath.c_str())!= 0)
	{
		printf("set chdir error!\n");
		return -1;
	}

	//���Ի���־ģ�顣
	C_LogManage *pLogManage = C_LogManage::GetInstance();
	iResult = pLogManage->InitLogPath(pPara->m_strLogPath);
	if(iResult != 0)
	{
		printf("Init Log error ,exit!\n");
		return -1; //Add Log and Erorr Ctrl;
	}
	pLogManage->WriteLog(LOG_FATAL,LOG_MODEL_OTHER,0, INFO_IMonitor_START_RUN,"IMonitor system start run!!!");	

	// ��ʼ���źŴ���
	InitSigFun(pLogManage);

	//���Ի��߳�
	C_ThreadManage *pThreadManage = C_ThreadManage::GetInstance();
	iResult = pThreadManage->InitThreadData();
	if(iResult != 0)
	{
		return -1; 
	} 

	iResult = pThreadManage->InitWebserviceThread();
	if(iResult != 0)
	{
		return -1; 
	} 

	// ����ִ����
	CInvoke Invoker;
	if(Invoker.Init() == -1)
	{
		printf("Invoker Init Failed !\n");
		return -1;
	}

	// ��ʼ����ʱ����
	C_TaskList *pTaskList = C_TaskList::GetInstance();
	iResult = pTaskList->InitTaskList(&Invoker);	

	//�������; 
	Invoker.AddInitTask();

	//���Ի���ʱ����
	int iMillisecond = 0;
	int iSleepMillisecond = 0;
	int bDateSet = -1;
	int iCountTime = 0;
	string strTmpDate;
	int iTmsWorkState = 0;
	char strInfo[1024];
	while(!g_bQuit)
	{
		iMillisecond = pRunPara->GetCurMillisecond(bDateSet);
		++iCountTime;
		iCountTime = iCountTime% 3000;

		//ÿ��0�㿪ʼ������־�ļ��ı���·����
		if(bDateSet != 0)
		{
			pLogManage->ReInitLog();
		}

		//��ѯ��ʱ����
		pTaskList->RunTasks(pRunPara->GetCurTime());

		//��ѯ������200���룻
		iSleepMillisecond = 200 - (pRunPara->GetCurMillisecond(bDateSet) - iMillisecond);
		if(iSleepMillisecond <= 0)
		{
			iSleepMillisecond = 0;//Add ErrorCtrl and log;
			printf("^^^^^^^^^^^^^^^^^^^^^^main thread is busy!^^^^^^^^^^^^^^^^^^^^^^^^^^\n"); 
			continue;
		}
		else
		{
			usleep(100000);
		}

		//add thread check 
		if((++iCountTime) == 20)
		{
			//pThreadManage->CheckTimeoutThread();
		} 
	}

	// ��Ҫ�ı�����˳�򣬷��������޷������̹߳���������
	Invoker.DeInit();
	C_TaskList::DestoryInstance();
	C_ThreadManage::DestoryInstance();
	C_LogManage::DestoryInstance();
	C_RunPara::DestoryInstance();
	C_Para::DestoryInstance();
	printf("IMonitor Exit!\n");
	return 0;
}

