//@file:Tms20_DeviceService.cpp
//@brief: 程序入点，包含main函数
//@author:luyan@oristartech.com
//date:2014-09-12

#include <signal.h>
#include <unistd.h>
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
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
extern int g_nRunType;  // 1为守护进程 0为交互模式
bool g_bReread = false; // 是否重读配置文件
int g_RunState = 0;     // 0为正在启动，1 为运行正常
time_t g_tmDBSynch = 0;
static void sig_fun(int iSigNum);


#define  LOGFAT(errid,msg)  C_LogManage::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,errid,msg)
#define  LOGINFFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_INFO,LOG_MODEL_OTHER,0,errid,fmt,##__VA_ARGS__)


void sig_fun(int iSigNum)
{
	C_LogManage *pLogManage = C_LogManage::GetInstance();
	char strInfo[1024];
	sprintf(strInfo, "revc a signal number:%d ", iSigNum);
	pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,strInfo);

	void *stack_p[20];
	char **stack_info;
	int size;
	size = backtrace( stack_p, sizeof(stack_p));
	stack_info = backtrace_symbols( stack_p, size);
	for(int i=0; i<size; ++i)
	{ 
		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,stack_info[i]); 
	}
	free(stack_info);
	if(0 == g_nRunType)
	{
		fflush(NULL);
	}
}
void sigterm(int signo)
{
  syslog(LOG_ERR,"catch TERM Signal !");
  g_bQuit = true;
}

void sighup(int signo)
{
	syslog(LOG_ERR,"catch HUP Signal !");
	g_bReread = true;
}

void InitSigFun(C_LogManage *pLogManage)
{
	if(signal(SIGINT,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGINT"); 
	}
	if(signal(SIGALRM,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGALRM"); 
	}
//  防止关闭终端后，程序在后台运行，所有不处理SIGHUP信号，使用默认处理方式即结束进程
// 	if(signal(SIGHUP,sig_fun) == SIG_ERR)
// 	{
// 		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGHUP"); 
// 	}  
	if(signal(SIGPIPE,SIG_IGN) == SIG_ERR)
	{
		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGPIPE"); 
	}  
	if(signal(SIGPOLL,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGPOLL"); 
	} 
	if(signal(SIGPROF,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGPROF"); 
	}
	if(signal(SIGSTKFLT,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGSTKFLT"); 
	} 
// 	if(signal(SIGTERM,sig_fun) == SIG_ERR)
// 	{
// 		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_WEBSERVICE_CREATE_TRREAD,"add signal Number:SIGTERM"); 
// 	}
	if(signal(SIGUSR1,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGUSR1"); 
	}
	if(signal(SIGUSR2,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGUSR2"); 
	} 
	if(signal(SIGVTALRM,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGVTALRM"); 
	}  
	if(signal(SIGIO,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGIO"); 
	} 
	if(signal(SIGABRT,sig_fun) == SIG_ERR)
	{
		pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0,ERROR_SIGCATCH_FUN,"add signal Number:SIGABRT"); 
	}

	// 忽略子进程结束时发送的SIGCLD信号 ，防止僵尸进程
	signal(SIGCLD,SIG_IGN);
}

void daemonize(const char *cmd)
{
	int					i, fd0, fd1, fd2;
	pid_t				pid;
	struct rlimit		rl;
	struct sigaction	sa;

	// Clear file creation mask.
	umask(660);

	
	// Get maximum number of file descriptors.
	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
	{
		printf("%s: can't get file limit", cmd);
		exit(-1);
	}

	
	//Become a session leader to lose controlling TTY.
	if ((pid = fork()) < 0)
	{
		printf("%s: can't fork", cmd);
		exit(-1);
	}
	else if (pid != 0) /* parent */
	{
		exit(0);
	}
	setsid();

	
	// Ensure future opens won't allocate controlling TTYs.
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0)
	{
		printf("%s: can't ignore SIGHUP");
		exit(-1);
	}
	if ((pid = fork()) < 0)
	{
		printf("%s: can't fork", cmd);
		exit(-1);
	}
	else if (pid != 0) /* parent */
	{
		exit(0);
	}

	
	//  Change the current working directory to the root so
	//  we won't prevent file systems from being unmounted. 
// 	if (chdir("/") < 0)
// 	{
// 		printf("%s: can't change directory to /");
// 		exit(-1);
// 	}

	
	// Close all open file descriptors.
	if (rl.rlim_max == RLIM_INFINITY)
	{
		rl.rlim_max = 1024;
	}
	for (i = 0; i < rl.rlim_max; i++)
	{
		close(i);
	}

	
	 //Attach file descriptors 0, 1, and 2 to /dev/null.
	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);

	
	// Initialize the log file.
	openlog(cmd, LOG_CONS, LOG_DAEMON);
	if (fd0 != 0 || fd1 != 1 || fd2 != 2) 
	{
		syslog(LOG_ERR, "unexpected file descriptors %d %d %d",fd0, fd1, fd2);
		exit(1);
	}
}

// 设置文件锁，保证只有一个实例
int already_running(void)
{
	int fd;
	char buf[16];
	fd = open("/var/run/imonitor_cs.pid",O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

	struct flock lock;
	lock.l_type=F_WRLCK;
	lock.l_start = 0;
	lock.l_whence = SEEK_SET;
	lock.l_len = 0;
	if(fcntl(fd,F_SETLK,&lock)<0)
	{
		if(errno ==EACCES || errno == EAGAIN)
		{
			close(fd);
			return 1;
		}
		printf("can't lock %s","/var/run/imonitor_cs.pid");
		return 1;
	}

	ftruncate(fd,0);
	sprintf(buf,"%ld",(long)getpid());
	write(fd,buf,strlen(buf)+1);
	return 0;
}


//@brief 程序入点，进行系统的各种初试化操作。
//然后产生系统定时器；进行任务轮询；
int main(int argc, char** argv)
{

	if(argc == 2)
	{
	   if(strcmp(argv[1],"-daemon")==0)
	   {
		   daemonize("oristar_imonitor");
		   g_nRunType = 1;
		
		   // 注册SIGHUP信号处理程序
		   struct sigaction sa;
		   sa.sa_handler = sigterm;
		   sigemptyset(&sa.sa_mask);
		   sigaddset(&sa.sa_mask,SIGHUP);
		   sa.sa_flags = 0;
		   if(sigaction(SIGTERM,&sa,NULL) < 0)
		   {
			   syslog(LOG_ERR,"can't catch SIGTERM:%s",strerror(errno));
			   exit(1);
		   }
			
		   // 注册SIGHUP信号处理程序
		   sa.sa_handler = sighup;
		   sigemptyset(&sa.sa_mask);
		   sigaddset(&sa.sa_mask,SIGTERM);
		   sa.sa_flags = 0;
		   if(sigaction(SIGHUP,&sa,NULL) < 0)
		   {
			   syslog(LOG_ERR,"can't catch SIGHUP:%s",strerror(errno));
			   exit(1);
		   }
	   }
	}

	if(already_running())
	{
		return -1;
	}
	int iResult = -1;

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
		if (0 == g_nRunType)
		{
			printf("set chdir error!\n");		
		}
		else
		{
			syslog(LOG_ERR,"set chdir error!\n");
		}
		return -2;
	}


	//读取配置文件
	C_Para *pPara = C_Para::GetInstance();
	iResult = pPara->ReadPara();
	if(iResult != 0)
	{
		if (0 == g_nRunType)
		{
			printf("read para error ,exit!\n");
		}
		else
		{
			syslog(LOG_ERR,"read para error ,exit!\n");
		}
		return -3;	
	}

	//初试化日志模块。
	C_LogManage *pLogManage = C_LogManage::GetInstance();
	iResult = pLogManage->InitLogPath(pPara->m_strLogPath);
	if(iResult != 0)
	{
		if (0 == g_nRunType)
		{
			printf("Init Log error ,exit!\n");
		}
		else
		{
			syslog(LOG_ERR,"Init Log error ,exit!\n");
		}

		return -4; //Add Log and Erorr Ctrl;
	}
	pLogManage->WriteLog(ULOG_FATAL,LOG_MODEL_OTHER,0, INFO_IMonitor_START_RUN,"IMonitor system start run!!!");	

	// 启动延时，为了解决主从机之间竞态条件的出现，防止启动sms的冲突。
	srand(time(NULL));
	int ws = 0;
	if(pPara->GetRole()!=MAINROLE)
	{
		ws = 6+rand()%5;
	}
	else
	{
		ws= 2+rand()%3;
	}

	LOGINFFMT(0,"IMinitor Is Booting ,Please Wait (%ds)/....",ws);
	for(int i=ws;i>=0;i--)
	{
		if(0 == g_nRunType)
		{
			if(i>=10)
				printf("\b\b\b\b(%d)",i);
			else
				printf("\b\b\b(%d)",i);


			fflush(0);
		}
		
		sleep(1);
	}

	// 初始化信号处理
	InitSigFun(pLogManage);

	//初试化线程
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
	

	// 创建执行体
	CInvoke Invoker;
	if(Invoker.Init() == -1)
	{
		LOGFAT(0,"Invoker Init Failed !\n");
		return -1;
	}

	// 初始化定时任务
	C_TaskList *pTaskList = C_TaskList::GetInstance();
	iResult = pTaskList->InitTaskList(&Invoker);	

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
		++iCountTime;
		iCountTime = iCountTime% 3000;

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
			LOGINFFMT(0,"^^^^^^^^^^^^^^^^^^^^^^main thread is busy!^^^^^^^^^^^^^^^^^^^^^^^^^^\n"); 
			continue;
		}
		else
		{
			usleep(100000);
		}

		//add thread check 
// 		if((++iCountTime) == 20)
// 		{
// 			pThreadManage->CheckTimeoutThread();
// 		} 

		if(g_bReread)
		{
			pPara->ReadPara();
			g_bReread = false;
			LOGINFFMT(LOG_ERR,"RereadPara Set Log Level:%d",C_Para::GetInstance()->m_nWirteLogLevel);
			C_LogManage::GetInstance()->SetLogLevel(C_Para::GetInstance()->m_nWirteLogLevel);
		}
	}

	// 不要改变析构顺序，否则会出现无法结束线程管理类的情况
	Invoker.DeInit();
	C_TaskList::DestoryInstance();
	C_ThreadManage::DestoryInstance();
	C_LogManage::DestoryInstance();
	C_RunPara::DestoryInstance();
	C_Para::DestoryInstance();
	LOGINFFMT(0,"IMonitor Exit!\n");
	return 0;
}

