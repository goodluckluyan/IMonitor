//@file:Tms20_DeviceService.cpp
//@brief: 程序入点，包含main函数
//@author:wangzhongping@oristartech.com
//dade:2012-07-12

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
#include <signal.h>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include "execinfo.h"

extern bool g_bQuit;

int CompareCppListenIpAddr();//duheqing 2013-12-27
//20140305 by xiaozhengxiu add
//void init_ssl_env(C_ThreadManage *pThreadManage);
//void clean_ssl_env(C_ThreadManage *pThreadManage);
//20140305 by xiaozhengxiu add end

static void sig_fun(int iSigNum);

//duheqing 2013-12-27
int CompareCppListenIpAddr()
{
	//duheqing 2014-5-26
	string startTime;
	ifstream fin("/proc/uptime");
	fin>>startTime;
	fin.close();

	C_RunPara *pRunPara = C_RunPara::GetInstance();
	if(atoi(startTime.c_str()) < pRunPara->GetOsStartWaitTime())
		sleep(pRunPara->GetOsStartWaitTime() - atoi(startTime.c_str()));

	CppMySQL3DB mySql;
	int result = mySql.openTMS();
	if(result != 0)
		return -1;

	string sql = "select conf_val from system_config where conf_item=\"proxy_host\";";
	CppMySQLQuery recordset = mySql.querySQL(sql.c_str(), result);
	if(result != 0)
		return -1;
	if(recordset.numRow() == 0)
		return -1;

	recordset.seekRow(0);
	string listenIp = recordset.getStringField("conf_val");

	std::vector<std::string> ipList;
	result = GetThisAllIp(ipList);
	if(result != 0)
		return -1;

	if(std::find(ipList.begin(), ipList.end(), listenIp) == ipList.end())
	{
		sql = "update system_config set conf_val=\"\" where conf_item=\"proxy_host\"";
		mySql.execSQL(sql.c_str());
	}
	return std::find(ipList.begin(), ipList.end(), listenIp) == ipList.end() ? -1 : 0;
}


void sig_fun(int iSigNum)
{
	C_LogManage *pLogManage = C_LogManage::GetInstance();
	char strInfo[1024];
	sprintf(strInfo, "revc a signal number:%d ", iSigNum);
	pLogManage->WriteLog(3,17,0,ERROR_WEBSERVICE_CREATE_TRREAD,strInfo);

	void *stack_p[20];
	char **stack_info;
	int size;
	size = backtrace( stack_p, sizeof(stack_p));
	stack_info = backtrace_symbols( stack_p, size);
	for(int i=0; i<size; ++i)
	{ 
		pLogManage->WriteLog(3,17,0,ERROR_WEBSERVICE_CREATE_TRREAD,stack_info[i]); 
	}
	free(stack_info);
	fflush(NULL);

}


void InitSigFun(C_LogManage *pLogManage)
{
	// set signal; wzp
	if(signal(SIGINT,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(3,17,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGINT"); 
		printf("add signal Number:SIGINT\n");	
	}
	if(signal(SIGALRM,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(3,17,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGALRM"); 
		printf("add signal Number:SIGALRM\n");	
	}
	if(signal(SIGHUP,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(3,17,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGHUP"); 
		printf("add signal Number:SIGHUP\n");	
	}  
	if(signal(SIGPIPE,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(3,17,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGPIPE"); 
		printf("add signal Number:SIGPIPE\n");	
	}  
	if(signal(SIGPOLL,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(3,17,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGPOLL"); 
		printf("add signal Number:SIGPOLL\n");	
	} 
	if(signal(SIGPROF,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(3,17,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGPROF"); 
		printf("add signal Number:SIGPROF\n");	
	}
	if(signal(SIGSTKFLT,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(3,17,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGSTKFLT"); 
		printf("add signal Number:SIGSTKFLT\n");	
	} 
	/* if(signal(SIGTERM,sig_fun) == SIG_ERR)
	{
	pLogManage->WriteLog(3,17,0,ERROR_WEBSERVICE_CREATE_TRREAD,"add signal Number:SIGTERM"); 
	printf("add signal Number:SIGTERM\n");	
	}*/
	if(signal(SIGUSR1,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(3,17,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGUSR1"); 
		printf("add signal Number:SIGUSR1\n");	
	}
	if(signal(SIGUSR2,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(3,17,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGUSR2"); 
		printf("add signal Number:SIGUSR2\n");	
	} 
	if(signal(SIGVTALRM,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(3,17,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGVTALRM"); 
		printf("add signal Number:SIGVTALRM\n");	
	}  
	if(signal(SIGIO,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(3,17,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGIO"); 
		printf("add signal Number:SIGIO\n");	
	} 
	if(signal(SIGABRT,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(3,17,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGABRT"); 
		printf("add signal Number:SIGABRT\n");	
	}                 
}

//@brief 程序入点，进行系统的各种初试化操作。
//然后产生系统定时器；进行任务轮询；
int main(int argc, char** argv)
{
	int iResult = -1;

	//system intialize;    
	//读取配置文件
	C_Para *pPara = C_Para::GetInstance();
	iResult = pPara->ReadPara();
	if(iResult != 0)
	{
		printf("read para error ,exit!\n");
		return -1; //Add Log and Erorr Ctrl;s
	}

	//初始化运行时参数设置---时间设置。
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

	//初试化日志模块。
	C_LogManage *pLogManage = C_LogManage::GetInstance();
	iResult = pLogManage->InitLogPath(pPara->m_strLogPath);
	if(iResult != 0)
	{
		printf("Init Log error ,exit!\n");
		return -1; //Add Log and Erorr Ctrl;
	}
	pLogManage->WriteLog(0,16,0, INFO_IMonitor_START_RUN,"IMonitor system start run!!!");	

	// 初始化信号处理
	InitSigFun(pLogManage);


	// 数据管理模块
	CDataManager dm;
	if(!dm.Init())
	{
		return -1;
	}

	// 磁盘监测模块初始化
	CheckDisk ds(&dm);
// 	if(!ds.InitAndCheck())
// 	{
// 		printf("Initial Fail! Check Raid Status Fail!\n");
// 		return -1;
// 	}
// 	else
// 	{
// 		printf("Raid Check Done.\n");
// 	}
	
	// 网卡监测模块初始化
	Test_NetCard ns(&dm);
	if(!ns.InitAndCheck())
	{
		printf("Initial Fail! Check Eth Status Fail!\n");
		return -1;
	}
	else
	{
		printf("Eth Check Done.\n");
	}
	
	// 监测SMS模块
	C_HallList * ptrLstHall = C_HallList::GetInstance();
	ptrLstHall->Init(&dm);

	// 监测对端高度软件
	CMonitorSensor OMonitorSersor;
	OMonitorSersor.Init(pPara->m_strOURI,pPara->m_strOIP,pPara->m_nOPort,&dm);

	//初试化线程
	C_ThreadManage *pThreadManage = C_ThreadManage::GetInstance();
	iResult = pThreadManage->InitThreadData();
	if(iResult != 0)
	{
		return -1; //Add Log and Erorr Ctrl;
	} 

	iResult = pThreadManage->InitWebserviceThread();
	if(iResult != 0)
	{
		return -1; //Add Log and Erorr Ctrl;
	} 


	// 调度模块
	CDispatch dispatch(&dm);
	dispatch.Init();
	CInvoke Invoker(ptrLstHall,&ds,&ns,&dispatch,&OMonitorSersor);

	// 初始化定时任务
	C_TaskList *pTaskList = C_TaskList::GetInstance();
	iResult = pTaskList->InitTaskList(&Invoker);
	if(iResult != 0)
	{
		return -1; //Add Log and Erorr Ctrl;
	}
	
	//添加任务; 
	Invoker.AddInitTask();

	//初试化定时器。
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
		//每天2点至4点之间进行检测
		++iCountTime;
		iCountTime = iCountTime% 3000;
		if(iMillisecond >2*3600*1000 && iMillisecond < 4*3600*1000  && iCountTime == 0)
		{
			pRunPara->GetCurDate(strTmpDate);

			//判断当前时间是否和程序开始运行时间一致。
			if(strTmpDate != strInitDate)
			{
				// 判断当前tms是否空闲。
				if(iTmsWorkState == 0)
				{
					sprintf(strInfo,"2--4 clock process exist iMillisecond:%d strTmpDate:%s strInitDate%s iTmsWorkState%d\n",
						iMillisecond, strTmpDate.c_str(),  strInitDate.c_str(),iTmsWorkState);
					pLogManage->WriteLog(3,17,0,ERROR_WEBSERVICE_CREATE_TRREAD,strInfo);       
					//退出程序等待监控程序重新启动主程序。
					return 0;
				}
				else
				{
					sprintf(strInfo,"2--4 clock process not exist because iTmsWorkState != 0 iMillisecond:%d strTmpDate:%s strInitDate%s iTmsWorkState%d\n",
						iMillisecond, strTmpDate.c_str(),  strInitDate.c_str(),iTmsWorkState);
					pLogManage->WriteLog(3,17,0,ERROR_WEBSERVICE_CREATE_TRREAD,strInfo);   
				}
			}
		}

		//每天0点开始更改日志文件的保存路径。
		if(bDateSet != 0)
		{
			pLogManage->ReInitLog();
		}

		//轮询定时任务。
		pTaskList->RunTasks(pRunPara->GetCurTime());

		//轮询周期是200毫秒；
		iSleepMillisecond = 200 - (pRunPara->GetCurMillisecond(bDateSet) - iMillisecond);
		if(iSleepMillisecond <= 0)
		{
			iSleepMillisecond = 0;//Add ErrorCtrl and log;
			printf("^^^^^^^^ main thread is busy!^^^^^^^^^^^^^^^^^^^^^^^^^^\n"); 
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

	C_TaskList::DestoryInstance();
	C_HallList::DestoryInstance();
	C_ThreadManage::DestoryInstance();
	C_LogManage::DestoryInstance();
	C_RunPara::DestoryInstance();
	C_Para::DestoryInstance();
	printf("IMonitor exit!\n");
	return 0;
}

