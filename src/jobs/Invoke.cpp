#include <fcntl.h>
#include "Invoke.h"
#include "timeTask/C_TaskList.h"
#include "para/C_RunPara.h"
#include "para/C_Para.h"
#include "check_netcard.h"
#include "database/CppMySQL3DB.h"
#include "database/CppMySQLQuery.h"
#include "utility/IPMgr.h"
#include "C_constDef.h"
#include "utility/C_Time.h"


extern time_t g_tmDBSynch;
//extern int g_RunState;
bool g_bQuit = false;
int g_LogLevel = 0;
int g_nRunType = 0; // 1为守护进程 0为交互模式
#define  LOGFAT(errid,msg)  C_LogManage::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_JOBS,0,errid,msg)
#define  LOGINFFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_INFO,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)
IPMgr* IPMgr::m_ptrIpMgr=NULL;
int  CInvoke::Init()
{
	// 运行在交互模式
	if(0 == g_nRunType)
	{
		PrintProductInfo();
	}
	LOGINFFMT(0,"Invoke Init...");

	C_Para * pPara = C_Para::GetInstance();
	IPMgr::GetInstance()->AddIP(pPara->m_strLIP,pPara->m_strOIP);

	// 数据管理模块
	CDataManager *pDM = CDataManager::GetInstance();
	if(!pDM->Init((void *)this,pPara->m_nSSD_Raid_Num,pPara->m_nSATA_Raid_Num))
	{
		return -1;
	}

	// 监测对端调度软件
	bool bOtherRun = false;
	if(m_ptrMonitor == NULL)
	{
		m_ptrMonitor = new  CMonitorSensor();
		m_ptrMonitor->Init(pPara->m_strOIP,pPara->m_nOPort);

		// 等待对端IMonitor启动
		time_t tm1;
		time(&tm1);
		while(1)
		{
			if(m_ptrMonitor->GetOtherMonitorState(TASK_NUMBER_GET_OTHERMONITOR_STATUS))
			{

				long lsynch=pDM->GetSynChID();
				if(lsynch!= 0)
				{
					char buf[64]={'\0'};
					snprintf(buf,64,"%lld",lsynch);
					if(!m_ptrMonitor->SetOtherDBSynch(buf))
					{
						continue;
					}

					while(1)
					{
						if( CheckDBSynch(lsynch))
						{
							LOGFAT(0,"Mysql DB Synch  OK!");
							break;
						}
						else 
						{
							LOGFAT(0,"Mysql DB Synch  Failed! Try Again Wait 5 Sec.");
							sleep(5);
						}
					}
				}
				bOtherRun = true;
				break;
			}

			sleep(2);

			// 如果超时
			time_t tm2;
			time(&tm2);
			if(tm2-tm1 >= 300)
			{
				LOGFAT(ERROR_OTHERMONITOR_NORUN,"Other Monitor Not Run !");
				break;
			}
		}
	}
	

	// 启动TMS
	if(m_ptrTMS == NULL)
	{
		m_ptrTMS  = new CTMSSensor();
		m_ptrTMS->Init(pPara->m_strOIP,pPara->m_nOPort,pPara->m_nTMSWSPort);
	}
	
	int nOtherStatus =pDM->GetOtherIMonitor();

	// 如果没有监测到对端，则立即启动所有sms，否则根据对端返回的sms状态延迟启动
	if(!bOtherRun && m_ptrLstHall == NULL)
	{
		m_ptrLstHall = new C_HallList();
		if(m_ptrLstHall->Init(m_ptrTMS)!=0)
		{
			return -1;
		}

	}
	else if(nOtherStatus > 0)
	{
		// 如果对端为正常运行状态（非启动、非正在接管、非恢复接管、非处理冲突），则延时启动sms。
		// 延时启动时先获取对端的sms状态再按对端状态进行启动sms。如果对端为接管角色只有延时启动完
		// 完成并把GlobalStatus设为1后对端才会进入恢复接管状态
		m_bSMSBootDelay = true;
		LOGFAT(0,"***Check Other Host IMonitor Had Booted Before Long Time,So HallList Delay Boot !");
	}
	else
	{
		GetDBSynchStatus();
		int nDBStatus;
		pDM->GetDBSynchStatus(nDBStatus);

		// 主发现数据库不同步，则忽略按本机数据启动sms
		// 备发现数据库不同步则等待数据库同步或等待主启动完成后从主获取sms的启动位置
		if(nDBStatus==1 && !pPara->IsMain())
		{
			LOGFAT(0,"***Mysql DB Synch  Failed!");
			while(1)
			{
				GetDBSynchStatus();
				pDM->GetDBSynchStatus(nDBStatus);

				if(nDBStatus == 0)
				{
					break;
				}

				LOGFAT(0,"***Mysql DB Synch  Failed! Waiting DB to Synching......");

				if(m_ptrMonitor->GetOtherMonitorState(TASK_NUMBER_GET_OTHERMONITOR_STATUS))
				{
					nOtherStatus =pDM->GetOtherIMonitor();
					if(nOtherStatus > 0)
					{
						// 如果对端为正常运行状态（非启动、非正在接管、非恢复接管、非处理冲突），则延时启动sms。
						// 延时启动时先获取对端的sms状态再按对端状态进行启动sms。如果对端为接管角色只有延时启动完
						// 完成并把GlobalStatus设为1后对端才会进入恢复接管状态
						m_bSMSBootDelay = true;
						LOGFAT(0,"***Check Other Host IMonitor Had Booted Before Long Time,So HallList Delay Boot !");
						break;
					}
				}
				sleep(1);

			}
			
		}

		if(!m_bSMSBootDelay)
		{
			LOGFAT(0,"***Check Other Host IMonitor Booting,So HallList Immediately Boot !");
			m_ptrLstHall = new C_HallList();
			if(m_ptrLstHall->Init(m_ptrTMS)!=0)
			{
				return -1;
			}
		}
		
	}
		
	

	std::string strLocalIP,strOtherIP;
	GetWebServiceIPFormDB(strLocalIP,strOtherIP);
	IPMgr::GetInstance()->AddIP(strLocalIP,strOtherIP);

	// 调度模块
	if(m_ptrDispatch == NULL)
	{
		m_ptrDispatch = new CDispatch(this);
		std::string strPolicyPath = pPara->m_strInipath+"/policy.xml";
		m_ptrDispatch->Init(strPolicyPath);
		pDM->SetDispatchPtr(m_ptrDispatch);
	}

	// 磁盘监测模块初始化
	if(m_ptrDisk == NULL)
	{
		m_ptrDisk = new CheckDisk();
		if(!m_ptrDisk->InitAndCheck())
		{
			LOGFAT(0,"Initial Fail! Check Raid Status Fail!\n");
			return -1;
		}
		else
		{
			LOGINFFMT(0,"Raid Check Done.\n");
		}
	}

	// 网卡监测模块初始化
	if(m_ptrNet == NULL)
	{
		m_ptrNet = new Test_NetCard();
		if(!m_ptrNet->InitAndCheck())
		{
			LOGFAT(0,"Initial Fail! Check Eth Status Fail!\n");
			return -1;
		}
		else
		{
			LOGINFFMT(0,"Eth Check Done.\n");
		}
	}

	if(m_ptrHash == NULL)
	{
		m_ptrHash = new CHashCheck;
	}

	if(m_ptrFO == NULL)
	{
		m_ptrFO = new CFileOperator;
	}

	std::string strPath;
	C_RunPara::GetInstance()->GetExePath(strPath);
	CWatchdog * ptr = new CWatchdog("ingest_cs",strPath);
	m_vecPtrWathdog.push_back(ptr);

	GetDBSynchStatus();
	//GetDBSynchStatus_PIP();
}

void CInvoke::DeInit()
{
	LOGINFFMT(0,"CInvoke::DeInit.");
	SAFE_DELETE(m_ptrDisk);
	SAFE_DELETE(m_ptrNet);
	SAFE_DELETE(m_ptrLstHall);
	SAFE_DELETE(m_ptrMonitor);
	SAFE_DELETE(m_ptrTMS);
	SAFE_DELETE(m_ptrDispatch);
	SAFE_DELETE(m_ptrHash);
	SAFE_DELETE(m_ptrFO);

	int nLen = m_vecPtrWathdog.size();
	for(int i = 0; i < nLen ;i++)
	{
		CWatchdog * ptr = m_vecPtrWathdog[i];
		SAFE_DELETE(ptr);
	}
	m_vecPtrWathdog.clear();
}

// 添加定时任务
bool CInvoke::AddInitTask()
{
	C_Para * pPara = C_Para::GetInstance();
	C_TaskList * ptrTaskList = C_TaskList::GetInstance();
	C_RunPara * ptrRunPara = C_RunPara::GetInstance();

	// 添加磁盘检测定时任务
	if(0 != pPara->m_nDiskCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_DISK_STATUS,NULL,ptrRunPara->GetCurTime()+pPara->m_nDiskCheckDelay);
	}

	// 添加网络检测定时任务
	if(0 != pPara->m_nEthCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_NET_STATUS,NULL,ptrRunPara->GetCurTime()+pPara->m_nEthCheckDelay);
	}

	// 添加tms检测定时任务
	if(0 != pPara->m_nTMSCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_TMS_STATUS,NULL,ptrRunPara->GetCurTime()+pPara->m_nTMSCheckDelay);
	}

	// 添加sms检测定时任务，如果sms延时启动则不添加此任务
	if(0 != pPara->m_nHallListCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_HALL_STATUS,NULL,ptrRunPara->GetCurTime()+pPara->m_nHallListCheckDelay);
	}
	
	if(0 != pPara->m_nOtherTMSCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_TMS_STATUS,NULL,
			ptrRunPara->GetCurTime() + pPara->m_nOtherTMSCheckDelay);
	}
	
	if(0 != pPara->m_nOtherSMSCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_SMS_STATUS,NULL,
			ptrRunPara->GetCurTime() + pPara->m_nOtherSMSCheckDelay);
	}
 
	if(0 != pPara->m_nOtherRAIDCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_RAID_STATUS,NULL,
			ptrRunPara->GetCurTime() + pPara->m_nOtherRAIDCheckDelay);
	}
	
	if(0 != pPara->m_nOtherEthCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_ETH_STATUS,NULL,
			ptrRunPara->GetCurTime() + pPara->m_nOtherEthCheckDelay);
	}
	
	if(0 != pPara->m_nOtherSwitchCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_SWITCH_STATUS,NULL,
			ptrRunPara->GetCurTime() + pPara->m_nOtherSwitchCheckDelay);
	}
	
	if(0 != pPara->m_nOtherSpeedLmtCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_SPEEDLIMIT_STATUS,NULL,
			ptrRunPara->GetCurTime() + pPara->m_nOtherSpeedLmtCheckDelay);
	}
	
	if(0 != pPara->m_nOtherEWCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_SMSEW_STATUS,NULL,
			ptrRunPara->GetCurTime() + pPara->m_nOtherEWCheckDelay);
	}

	if(0 != pPara->m_nDBSynchCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_DB_SYNCN_STATUS,NULL,
			ptrRunPara->GetCurTime() + pPara->m_nDBSynchCheckDelay);
	}

    // 只所以把添加检测对端调度程序放在后边，是为了防止过早的进行恢复接管操作
	// 添加对对端调度程序的检测的定时任务，必须要有检测对端调度程序的任务。
	if(0 == pPara->m_nOtherMonitorCheckDelay)
	{
		pPara->m_nOtherMonitorCheckDelay = 3;
	}
	ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_STATUS,NULL,
		ptrRunPara->GetCurTime() + pPara->m_nOtherMonitorCheckDelay);

	
	

	// 添加调度任务
	ptrTaskList->AddTask(TASK_NUMBER_DISPATCH_ROUTINE,NULL,-1);

	// 添加条件切换处理任务
	ptrTaskList->AddTask(TASK_NUMBER_CONDSWITCH_ROUTINE,NULL,-1);

	// 添加hash校验处理任务
	ptrTaskList->AddTask(TASK_NUMBER_HASHCHECK_ROUTINE,NULL,-1);

	// 添加文件任务
	ptrTaskList->AddTask(TASK_NUMBER_FILEOPERATION_ROUTINE,NULL,-1);

	// 添加看门狗监测任务
	ptrTaskList->AddTask(TASK_NUMBER_CHECKWATCHDOG,NULL,ptrRunPara->GetCurTime() + 2);


	SetupRebootTimer();

	// 运行在交互模式
	if(0 == g_nRunType)
	{
		// 添加处理用户输入命令
		ptrTaskList->AddTask(TASK_NUMBER_PROCESS_USERINPUT,NULL,0);
	}
}

// 获取时间间隔
int CInvoke::GetCheckDelay(int nStateType)
{
	C_Para * pPara = C_Para::GetInstance();
	int nDelay = 0;
	switch(nStateType)
	{
	case TASK_NUMBER_GET_DISK_STATUS:
		nDelay = pPara->m_nDiskCheckDelay;
		break;
	case TASK_NUMBER_GET_NET_STATUS:
		nDelay = pPara->m_nEthCheckDelay;
		break;
	case TASK_NUMBER_GET_HALL_STATUS:
		nDelay = pPara->m_nHallListCheckDelay ;
		break;
	case TASK_NUMBER_GET_TMS_STATUS:
		nDelay = pPara->m_nTMSCheckDelay ;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_STATUS:
		nDelay = pPara->m_nOtherMonitorCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_TMS_STATUS:
		nDelay = pPara->m_nOtherTMSCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_SMS_STATUS:
		nDelay = pPara->m_nOtherSMSCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_RAID_STATUS:
		nDelay = pPara->m_nOtherRAIDCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_ETH_STATUS:
		nDelay = pPara->m_nOtherEthCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_SWITCH_STATUS:
		nDelay = pPara->m_nOtherSwitchCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_SPEEDLIMIT_STATUS:
		nDelay = pPara->m_nOtherSpeedLmtCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_SMSEW_STATUS:
		nDelay = pPara->m_nOtherEWCheckDelay;
		break;
	case TASK_NUMBER_GET_DB_SYNCN_STATUS:
		nDelay = pPara->m_nDBSynchCheckDelay;
	case TASK_NUMBER_CHECKWATCHDOG:
		nDelay = 2;
		break;
	}
	return nDelay;
}

//任务执行，由不同的模块完成
int CInvoke::Exec(int iCmd,void * ptrPara)
{
	if(  NULL==m_ptrDisk || NULL==m_ptrNet || NULL==m_ptrDispatch || NULL == m_ptrMonitor)
	{
		return  -1;
	}
	
	int nResult = 0;
	switch(iCmd)
	{
	case TASK_NUMBER_DISPATCH_ROUTINE:
		m_ptrDispatch->Routine();
		nResult = 0;
		break;
	case TASK_NUMBER_CONDSWITCH_ROUTINE:
		if(m_ptrLstHall!=NULL)
		{
			m_ptrLstHall->ProcessCondSwitchTask();
		}
		break;
	case TASK_NUMBER_HASHCHECK_ROUTINE:
		m_ptrHash->ProcessHashTask();
		break;
	case TASK_NUMBER_GET_DISK_STATUS:
		m_ptrDisk->ReadMegaSASInfo();
		nResult = 0;
		break;
	case TASK_NUMBER_FILEOPERATION_ROUTINE:
		m_ptrFO->ProcessFileOptTask();
		break;
	case TASK_NUMBER_GET_NET_STATUS:
		m_ptrNet->GetAllEthStatus();
		nResult = 0;
		break;
	case TASK_NUMBER_GET_HALL_STATUS:
		if(m_ptrLstHall==NULL && m_bSMSBootDelay)
		{
			if(HasOtherSMSStatus())
			{
				m_ptrLstHall = new C_HallList();
				m_ptrLstHall->Init(m_ptrTMS,true);
			}
		}
		else if(m_ptrLstHall!=NULL)
		{
			m_ptrLstHall->GetSMSWorkState();			
		}
		break;
	case TASK_NUMBER_GET_TMS_STATUS:
		m_ptrTMS->GetTMSPID();
		break;
	case TASK_NUMBER_GET_DB_SYNCN_STATUS:
		//GetDBSynchStatus_PIP();
		GetDBSynchStatus();
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_STATUS:
	case TASK_NUMBER_GET_OTHERMONITOR_TMS_STATUS:
	case TASK_NUMBER_GET_OTHERMONITOR_SMS_STATUS:
	case TASK_NUMBER_GET_OTHERMONITOR_RAID_STATUS:
	case TASK_NUMBER_GET_OTHERMONITOR_ETH_STATUS:
	case TASK_NUMBER_GET_OTHERMONITOR_SWITCH_STATUS:
	case TASK_NUMBER_GET_OTHERMONITOR_SPEEDLIMIT_STATUS:
	case TASK_NUMBER_GET_OTHERMONITOR_SMSEW_STATUS:
		m_ptrMonitor->GetOtherMonitorState(iCmd);
		nResult = 0;
		break;
	case TASK_NUMBER_PROCESS_USERINPUT:
		Controller();
		break;
	case TASK_NUMBER_REBOOT:
		shutdown(0);//重启
		break;
	case TASK_NUMBER_SHUTDOWN:
		shutdown(1);//关机
		break;
	case TASK_NUMBER_CHECKWATCHDOG:
		CheckByWatchdog();
	default:
	    nResult = 2;
	}
	return nResult;
}

bool CInvoke::HasOtherSMSStatus()
{
	CDataManager * ptrDM = CDataManager::GetInstance();
	bool bDirty=true;
	C_Para *ptrPara = C_Para::GetInstance();
	std::vector<SMSStatus> vecSMSStatus;
	
	time_t tmUpdate=ptrDM->GetOtherSMSstatus(vecSMSStatus);
	time_t tmCur;
	time(&tmCur);
	if(tmCur-tmUpdate < ptrPara->m_nOtherSMSCheckDelay)
	{
		LOGINFFMT(0,"HasOtherSMSStatus Get Other SMS Lastest Status(uptm:%ld:curtm:%ld)(delay:%d)!",
			tmUpdate,tmCur,ptrPara->m_nOtherSMSCheckDelay);
		bDirty=false;
	}
	return !bDirty;
}

//解析输入字符串
void CInvoke::ParseCmd(std::string strCmd, std::vector<std::string> &vecParam)
{
	int nStart, nEnd;
	std::string strParam;
	nStart = 0;
	nEnd = (int)strCmd.find(" ");
	if (nEnd <= 0)
	{
		nEnd = (int)strCmd.size();
	}
	if (nEnd <= nStart)
	{
		return;
	}
	strParam = strCmd.substr(nStart, nEnd - nStart);
	vecParam.push_back(strParam);

	nStart = nEnd + 1;
	while (nStart < (int)strCmd.size() - 1)
	{
		std::string strTmp = strCmd.substr(nStart, strCmd.size() - nStart);
		nStart = (int)strTmp.find("-");
		if (nStart < 0)
		{
			break;
		}
		nStart++;
		nEnd = (int)strTmp.find(" ");
		if (-1 == nEnd)
		{
			nEnd = (int)strTmp.size();
		}
		if (nEnd <= nStart)
		{
			break;
		}
		strParam = strTmp.substr(nStart, nEnd - nStart);
		vecParam.push_back(strParam);
		strCmd = strTmp;
		nStart = nEnd + 1;
	}
}


// 打印产品信息
void CInvoke::PrintProductInfo()
{
	std::string strMORS = C_Para::GetInstance()->IsMain() ? "MAIN" :"STDBY";
	printf("#-----------------------------------------------------------------------------#\n");
	printf("#                      <<<<<IMonitor1.0(%5s)>>>>                       #\n",strMORS.c_str());
	printf("#                                                                             #\n");
	printf("#-----------------------------------------------------------------------------#\n");
	printf("# Command Usage:                                                              #\n");
	printf("# help:print help info\n");
	printf("# print -t:print TMS status\n");
	printf("# print -d:print RAID status\n");
	printf("# print -s:print SMS status\n");
	printf("# print -e:print Eth status\n");
	printf("# log	-n:0-3 print log level\n");
	printf("#-----------------------------------------------------------------------------#\n");
}

void CInvoke::PrintLogLevel()
{
	printf("#-----------------------------------------------------------------------------#\n");
	printf("# log Usage:																  #\n");
	printf("# log	-0:print log level ULOG_DEBUG\n");
	printf("# log	-1:print log level ULOG_INFO\n");
	printf("# log	-2:print log level ULOG_ERROR\n");
	printf("# log	-3:print log level ULOG_FATAL\n");
	printf("#-----------------------------------------------------------------------------#\n");
}

void CInvoke::PrintVersionInfo()
{
	std::string strMORS = C_Para::GetInstance()->IsMain() ? "MAIN" :"STDBY";
	LOGINFFMT(0,"#-----------------------------------------------------------------------------#");
	LOGINFFMT(0,"#                      <<<<<IMonitor1.0.0.5>>>>                               #");
	LOGINFFMT(0,"#-----------------------------------------------------------------------------#");
	LOGINFFMT(0,"%s",strMORS.c_str());
	LOGINFFMT(0,"修改:");
	LOGINFFMT(0,"	1、串行化splitbrain时的恢复，先主恢复接管再备恢复接管");
	LOGINFFMT(0,"	2、增加重启接口和定时重启接口");
	LOGINFFMT(0,"	3、对全局状态进行加锁控制并由GlobalStatus类管理");
	LOGINFFMT(0,"	4、通过使用GlobalStatus来进行主备状态的互斥，防止竞态条件");
	LOGINFFMT(0,"	5、启动时如果对端已经启动则按对端的报告的sms的状态启动");
	LOGINFFMT(0,"#-----------------------------------------------------------------------------#");

}


//接收用户输入控制的线程函数
int CInvoke::Controller () 
{
	int nModule = 0;
	int fdStdin;
	if((fdStdin = open("/dev/stdin", O_RDWR | O_NONBLOCK)) <= 0)
	{
		printf("can not open stdin file !");
	}
	char c[50];

	std::vector<std::string> vecParam;
	std::string user_input;
	while (true)
	{

		if(g_bQuit )
		{
			break;
		}

		int nSize = read(fdStdin, c, 50);
		if(nSize <= 0) 
		{
			usleep(100000);
			continue;
		}
		user_input.append(c, nSize);
		if ('\n' != c[nSize -1])
		{
			continue;
		}

		user_input.resize(user_input.size() - 1);
		vecParam.clear();
		ParseCmd(user_input, vecParam);
		user_input.clear();

		if (vecParam.size() < 1)
		{
			continue;
		}		
		if ("quit" == vecParam[0]) 
		{
			g_bQuit = true;
			break;
		}
		
		if("help" == vecParam[0])
		{
			PrintProductInfo();
			continue;
		}

		if ("print" == vecParam[0] && vecParam.size() >= 2)
		{
			CDataManager * ptr = CDataManager::GetInstance();
			
			if(vecParam[1] == "d")
			{
				ptr->PrintDiskState();
			}
			else if(vecParam[1] == "s")
			{
				ptr->PrintSMSState();
			}
			else if(vecParam[1] == "e")
			{
				ptr->PrintEthState();
			}
			else if(vecParam[1] == "t")
			{
				ptr->PrintTMSState();
			}
			else
			{
				PrintProductInfo();
			}
	
		}
		if("log" == vecParam[0] && vecParam.size() >=2)
		{
			if(vecParam[1] == "0")
			{
				g_LogLevel = 0;
				printf("Log Level Change To 0:ULOG_DEBUG\n");
			}
			else if(vecParam[1] == "1")
			{
				g_LogLevel = 1;
				printf("Log Level Change To 1:ULOG_INFO\n");
			}
			else if(vecParam[1] == "2")
			{
				g_LogLevel = 2;
				printf("Log Level Change To 2:ULOG_ERROR\n");
			}
			else if(vecParam[1] == "3")
			{
				g_LogLevel = 3;
				printf("Log Level Change To 3:ULOG_FATAL\n");
			}
			else
			{
				PrintLogLevel();
			}
		}
		
	}
	printf("Stop input! IMonitor will be exit!\n");
	close(fdStdin);
	return 0;
}

// 从数据库中获取本机和对端的ip
bool CInvoke::GetWebServiceIPFormDB(std::string &WebServiceLocalIP,std::string &WebServiceOtherIP)
{
	C_Para * ptrPara = C_Para::GetInstance();

	// 打开数据库
	CppMySQL3DB mysql;
	if(mysql.open(ptrPara->m_strDBServiceIP.c_str(),ptrPara->m_strDBUserName.c_str(),
		ptrPara->m_strDBPWD.c_str(),ptrPara->m_strDBName.c_str()) == -1)
	{
		LOGINFFMT(0,"mysql open failed!\n");
		return false;
	}

	// 读取devices表
	int nResult;
	CppMySQLQuery query = mysql.querySQL("select * from devices where device_type=3 and device_model=\"AQ33CS\"",nResult);
	int nRows = 0 ;
	if((nRows = query.numRow()) == 0) 
	{
		LOGINFFMT(0,"GetWebServiceIPFormDB failed,devices table no rows!\n");
		return false;
	}

	query.seekRow(0);
	for(int i = 0 ;i < nRows ; i++)
	{
		SMSInfo node;
		node.strId = query.getStringField("hall_id");
		std::string strIP = query.getStringField("ip");
		int nPort = atoi(query.getStringField("port"));
		std::string strIP2 = query.getStringField("ip2");
		int nPort2 = atoi(query.getStringField("port"));
		int nDefPos = query.getIntField("default_position");// 0为主，1为备
		int nCurPos = query.getIntField("cur_position");//1为主，2为备

		if(ptrPara->IsMain())
		{
			WebServiceLocalIP = strIP;
			WebServiceOtherIP = strIP2;
			break;
		}
		else 
		{
			WebServiceLocalIP = strIP2;
			WebServiceOtherIP = strIP;
			break;
		}
		query.nextRow();
	}
	return true;
}

// 切换TMS
bool CInvoke::SwitchTMS()
{
	if(m_ptrTMS != NULL)
	{
		return m_ptrTMS->SwitchTMS();
	}
	else
	{
		return false;
	}
}

bool CInvoke::NoticTMSSMSPos()
{
	if(NULL != m_ptrLstHall)
	{
		std::vector<std::string> vecHallID;
		m_ptrLstHall->GetAllRunHallID(vecHallID);
		int nLen = vecHallID.size();
		for(int i = 0 ;i < nLen ;i++)
		{
			std::string strNewIP;
			int nNewPort = 0;
			m_ptrLstHall->GetSMSRunHost(vecHallID[i],strNewIP,nNewPort);
			if(!strNewIP.empty() && nNewPort > 0 && C_Para::GetInstance()->IsMain())
			{
				bool bRet = m_ptrTMS->NotifyTMSSMSSwitch(vecHallID[i],strNewIP,nNewPort);
				LOGINFFMT(0,"NoticTMSSMSPos:< %s Switch To %s:%d Host Result:%d>",vecHallID[i].c_str(),
					strNewIP.c_str(),nNewPort,bRet?1:0);
			}
		}
		
	}
	
}

// 切换SMS
bool CInvoke::SwitchSMS(std::string strHallID,bool bDelaySwitch,int &nRet)
{
	C_Para *pPara = C_Para::GetInstance();
	int nRole=pPara->GetRole();

	// 只有一台主机运行时不允许切换
	if(nRole == TMPMAINROLE || nRole == ONLYMAINROLE)
	{
		nRet = 3;// 调度软件异常
		return false;
	}

	CDataManager *ptrDM = CDataManager::GetInstance();
	int nOtherStatus =ptrDM->GetOtherIMonitor();

	// 如果备机状态为0不进行切换
	// 对端机状态为0说明，对端正在启动或正在接管、恢复控管、处理冲突
	if(pPara->IsMain() && nOtherStatus <= 0)
	{
		int nIsRestoreSwitch;
		// 如果备机正处于恢复接管时并执行了切换，这种情况允许执行切换。
		if(m_ptrMonitor->AskAboutSlaveRestoreSwitch(nIsRestoreSwitch))
		{
			if(nIsRestoreSwitch==0)
			{
				LOGINFFMT(0,"SwitchSMS:Due To Other IMonitor Status Error(%d) and nIsRestoreSwitch(%d) ,So SwitchSMS Failed!",
					nOtherStatus,nIsRestoreSwitch);
				nRet = 3;// 调度软件异常
				return false;
			}
			else
			{
				LOGINFFMT(0,"SwitchSMS:nIsRestoreSwitch:%d,So continue switch ",nIsRestoreSwitch);
			}
		}
		else
		{
			LOGINFFMT(0,"SwitchSMS:Due To Other IMonitor Status Error(%d) ,So SwitchSMS Failed!",nOtherStatus);
			nRet = 3;// 调度软件异常
			return false;
		}
	}

	if(bDelaySwitch && m_ptrLstHall && m_ptrLstHall->IsHaveCondSwitchTask(strHallID))
	{
		nRet = 2;// sms正在延时切换
		return false;
	}

	if(m_ptrLstHall != NULL)
	{
		LOGINFFMT(ERROR_POLICYTRI_SMSSWITCH,"Switch SMS(%s)! ",strHallID.c_str());
		CDataManager::GetInstance()->StartSwitch();
		int nState;
		 if(m_ptrLstHall->SwitchSMS(bDelaySwitch,strHallID,nState))
		 {
			 std::string strNewIP;
			 int nNewPort = 0;
			 m_ptrLstHall->GetSMSRunHost(strHallID,strNewIP,nNewPort);

			 //LOGINFFMT(0,"****SwitchSMS:GetSMSRunHost< %s Switch To %s Host >",strHallID.c_str(),strNewIP.c_str());
			 if(!strNewIP.empty() && nNewPort > 0 && C_Para::GetInstance()->IsMain())
			 {
				 bool bRet = m_ptrTMS->NotifyTMSSMSSwitch(strHallID,strNewIP,nNewPort);
				 LOGINFFMT(0,"SwitchSMS:NotifyTMSSMSSwitch< %s Switch To %s:%d Host Result:%d>",strHallID.c_str(),
					 strNewIP.c_str(),nNewPort,bRet?0:1);

				 // 如果没能成功则一直尝试
				 int ndelay=2;
				 while(!bRet)
				 {
					 if(ndelay>100)
					 {
						 break;
					 }

					 sleep(ndelay);
					 bRet = m_ptrTMS->NotifyTMSSMSSwitch(strHallID,strNewIP,nNewPort);
					 LOGINFFMT(0,"SwitchSMS:NotifyTMSSMSSwitch< %s Switch To %s:%d Host Result:%d>",strHallID.c_str(),
						 strNewIP.c_str(),nNewPort,bRet?0:1);
					 ndelay=ndelay<<1;					 
					 
				 }
			 }
			 nRet = 0;
			 CDataManager::GetInstance()->EndSwitch();
			 return true;
		 }
		 else
		 {
			 // 因为只有主机才能发起切换，所有要判断是否为主
			 // 因为sms busy切换失败,加入延时切换
			 if(bDelaySwitch && nState == 2 && C_Para::GetInstance()->IsMain())
			 {
				 m_ptrLstHall->AddCondSwitchTask(strHallID,"state",101);
				 nRet = 2;// sms正在延时切换
				 CDataManager::GetInstance()->EndSwitch();
				 return false;
			 }
			 else if(!bDelaySwitch && nState == 2 && C_Para::GetInstance()->IsMain())
			 {
				 nRet = 1;//sms繁忙
				 CDataManager::GetInstance()->EndSwitch();
				 return false;
			 }
			 else
			 {
				 nRet = 3;//sms繁忙
				 CDataManager::GetInstance()->EndSwitch();
				 return false;
			 }
			
		 }
	}
	else
	{
		nRet = 4;// 调度软件出错
		return false;
	}
}

// 切换本机上的所有SMS,属于策略中自动切换
bool CInvoke::SwitchAllSMS()
{
	if(m_ptrLstHall != NULL)
	{
		LOGFAT(ERROR_POLICYTRI_ALLSMSSWITCH,"Fault Of Policys Trigger Switch ALLSMS!");	
		std::vector<std::string> vecHallID;
		m_ptrLstHall->GetAllLocalRunHallID(vecHallID);

		 CDataManager * ptrDM = CDataManager::GetInstance();
		int nOtherStatus =ptrDM->GetOtherIMonitor();
		if(nOtherStatus <= 0)
		{
			LOGINFFMT(0,"SwitchAllSMS:Due To Other IMonitor Status %d ,So SwitchALLSMS Failed!",nOtherStatus);
			return false;
		}

		int nOtherRaidStatus =ptrDM->GetOtherRaidStatus();
		if(0 != nOtherRaidStatus )
		{
			LOGINFFMT(0,"SwitchAllSMS:Due To Other Raid Status %d ,So SwitchALLSMS Failed!",nOtherRaidStatus);
			return false;
		}

		for(int i = 0 ;i < vecHallID.size();i++)
		{
			if(C_Para::GetInstance()->IsMain() )
			{
				int nState;
				SwitchSMS(vecHallID[i],false,nState);
			}
			else 
			{
				m_ptrLstHall->SwitchSMSByStdby(false,vecHallID[i]);
				LOGINFFMT(ERROR_SMSSWITCH_CALLOTHERSW,
					"SwitchAllSMS:Due To This Is STDBY ,So SMS Switch call Main Switch SMS!");
			}
		}
	}
	else
	{
		return false;
	}
	return true;
}


// 退出系统
void CInvoke::Exit()
{
	LOGFAT(ERROR_POLICYTRI_EXIT,"Fault Of Policys Trigger Exit!");
	//printf("Fault Of Policys Trigger Exit! 5 Sec Waiting \n");
	for(int i = 5;i > 0 ;i--)
	{
		sleep(1);
		//printf("%d sec ....\n",i);
	}
	
	g_bQuit = true;
}

// 切换本机接管的sms
void CInvoke::SwtichTakeOverSMS()
{
	if(m_ptrLstHall == NULL)
	{
		return ;
	}

	sleep(3);
	std::vector<std::string> vecSMS;
	m_ptrLstHall->GetTakeOverSMS(vecSMS);
	C_Para * pPara = C_Para::GetInstance();
	CDataManager *pDM = CDataManager::GetInstance();

	std::vector<SMSStatus> vecSMSStatus;

	bool bDirty=true;
	while(bDirty)
	{
		time_t tmUpdate=pDM->GetOtherSMSstatus(vecSMSStatus);
		time_t tmCur;
		time(&tmCur);
		if(tmCur-tmUpdate<pPara->m_nOtherSMSCheckDelay)
		{
			LOGINFFMT(0,"Get Other SMS Lastest Status(uptm:%ld:curtm:%ld)(delay:%d)!",
				tmUpdate,tmCur,pPara->m_nOtherSMSCheckDelay);
			bDirty=false;
		}
		else
		{
			sleep(1);
		}

	}
	

	for(int i=0;i<vecSMS.size();i++)
	{
		SMSStatus OtherStatus;
		if(!vecSMS[i].empty())
		{
			for(int j=0;j<vecSMSStatus.size();j++)
			{
				if(vecSMSStatus[j].hallid==vecSMS[i])
				{
					OtherStatus=vecSMSStatus[j];
					break;
				}
			}
			
			if(!OtherStatus.hallid.empty())
			{
				// 对端没有运行,则执行切换命令
				if(OtherStatus.nRun==0)
				{
					LOGINFFMT(0,"Get Other SMS(%s) Lastest Status(nRun:%d),So Run Switch SMS!",vecSMS[i].c_str(),OtherStatus.nRun);
					if(!C_Para::GetInstance()->IsMain())
					{
						// 调用主的切换接口,支持延时切换
						m_ptrLstHall->SwitchSMSByStdby(true,vecSMS[i]);
					}
					else
					{
						//支持延时切换
						int nState;
						SwitchSMS(vecSMS[i],true,nState);
					}
				}
				else if(OtherStatus.nRun == 2)//对端认为sms在本地运行
				{
					LOGINFFMT(0,"Get Other SMS(%s) Lastest Status(nRun:%d),So Run Switch SMS!",vecSMS[i].c_str(),OtherStatus.nRun);
					if(!C_Para::GetInstance()->IsMain())
					{
						// 调用主的切换接口,支持延时切换
						pDM->SetRestoreSwitch(true);
						m_ptrLstHall->SwitchSMSByStdby(true,vecSMS[i]);
					}
					else
					{
						//支持延时切换
						int nState;
						SwitchSMS(vecSMS[i],true,nState);
					}
				}
				else if(OtherStatus.nRun == 1)//对端相同的sms在运行则开闭本地sms
				{
					LOGINFFMT(0,"Get Other SMS(%s) Lastest Status(nRun:%d),So Run Close SMS!",vecSMS[i].c_str(),OtherStatus.nRun);
					CloseSMS(vecSMS[i]);
				}
			}
			
		}
	}
	pDM->SetRestoreSwitch(false);
}

// 在本机启动所有sms
void CInvoke::StartALLSMS(bool bCheckOtherSMSRun,bool bLocalHost/*=false*/)
{
	LOGFAT(ERROR_POLICYTRI_TMSSTARTUP,"Fault Of Policys Trigger StartALLSMS!");
	if(m_ptrLstHall != NULL)
	{
		std::vector<std::string> vecHallID;
		m_ptrLstHall->StartAllSMS(bCheckOtherSMSRun,vecHallID, bLocalHost);
		if(vecHallID.size() == 0)
		{
			return;
		}

		// 只有主机需要通过调用web接口的方式通知tms sms的位置，因为这时tms已经在主机上启动。
		int nRole=C_Para::GetInstance()->GetRole();
		LOGINFFMT(0,"Role is %d Now",nRole);
		if(nRole == ONLYMAINROLE || nRole == MAINROLE/*TMPMAINROLE*/)
		{
			for(int i = 0;i < vecHallID.size();i++)
			{
				std::string strNewIP;
				int nNewPort = 0;
				m_ptrLstHall->GetSMSRunHost(vecHallID[i],strNewIP,nNewPort);
				if(!strNewIP.empty() && nNewPort > 0)
				{
					bool bRet = false;
					int cnt = 0;
					while(!bRet && cnt <2 )
					{
						bRet = m_ptrTMS->NotifyTMSSMSSwitch(vecHallID[i],strNewIP,nNewPort);
						LOGINFFMT(0,"StartALLSMS:NotifyTMSSMSSwitch< %s Switch To %s:%d Host Result:%d>",vecHallID[i].c_str(),
							strNewIP.c_str(),nNewPort,bRet?1:0);
						cnt++;
						if(!bRet)
						{
							sleep(1);
						}
					}
					
				}
				else
				{
					LOGINFFMT(0,"StartALLSMS Get New SMS(%s) Run IP(%s) Port(%d) Err",
						vecHallID[i].c_str(),strNewIP.c_str(),nNewPort);
				}
			}
		}

	}
}

// 接管主服务器成为主角色
void CInvoke::TakeOverMain(bool bCheckOtherSMSRun)
{
	GlobalStatus::GetInstinct()->SetStatus(2);// 设置运行状态为接管状态
	LOGFAT(ERROR_POLICYTRI_TMSSTARTUP,"Fault Of Policys Trigger TakeOverMain!");

	//为了防止主数据库恢复时200ip变化时sms状态不正确的问题，在接管主机上的sms时，用localhost数据库ip代替200.
	StartALLSMS(bCheckOtherSMSRun,true);
	sleep(1);
	if(C_Para::GetInstance()->GetRole() != TMPMAINROLE)
	{
		C_Para::GetInstance()->SetRoleFlag(TMPMAINROLE);
	}
	GlobalStatus::GetInstinct()->SetStatus(1);
}

// 接管备服务器成为（只有主）角色
void CInvoke::TakeOverStdby(bool bCheckOtherSMSRun)
{
	GlobalStatus::GetInstinct()->SetStatus(2);// 设置运行状态为接管状态
	LOGFAT(ERROR_POLICYTRI_TMSSTARTUP,"Fault Of Policys Trigger TakeOverStdby!");
	StartALLSMS(bCheckOtherSMSRun);
	sleep(1);
	if(C_Para::GetInstance()->GetRole() != ONLYMAINROLE)
	{
		C_Para::GetInstance()->SetRoleFlag(ONLYMAINROLE);
	}
	GlobalStatus::GetInstinct()->SetStatus(1);
}

// 从（只有主）服务器角色改变成为主角色
void CInvoke::ChangeToMain()
{
	 CDataManager * ptrDM = CDataManager::GetInstance();
	int nOtherRaidStatus =ptrDM->GetOtherRaidStatus();
	if(0 != nOtherRaidStatus )
	{
		LOGINFFMT(0,"ChangeToMain:Due To Other Raid Status %d ,So ChangeToMain Failed!",nOtherRaidStatus);
		return ;
	}

	LOGFAT(ERROR_POLICYTRI_TMSSTARTUP,"****Find STDBYHost Change To MAINHost!****");

	GlobalStatus::GetInstinct()->SetStatus(2);// 设置运行状态为恢复接管状态
	//g_RunState=2;// 设置运行状态为恢复接管状态

	C_Para::GetInstance()->SetRoleFlag(MAINROLE);
	SwtichTakeOverSMS();

	g_tmDBSynch = 0;
	GlobalStatus::GetInstinct()->SetStatus(1);// 设置运行状态为恢复接管状态
	//g_RunState=1;// 设置运行状态为正常运行状态
}

// 从临时主服务器改变成为备角色
void CInvoke::ChangeToStdby()
{
	 CDataManager * ptrDM = CDataManager::GetInstance();
	int nOtherRaidStatus =ptrDM->GetOtherRaidStatus();
	if(0 != nOtherRaidStatus )
	{
		LOGINFFMT(0,"ChangeToStdby:Due To Other Raid Status %d ,So ChangeToStdby Failed!",nOtherRaidStatus);
		return ;
	}

	LOGFAT(ERROR_POLICYTRI_TMSSTARTUP,"****Find MainHost Change To STDBYHost!****");

	GlobalStatus::GetInstinct()->SetStatus(2);// 设置运行状态为恢复接管状态
	//g_RunState=2;// 设置运行状态为恢复接管状态
	C_Para::GetInstance()->SetRoleFlag(STDBYROLE);
	m_ptrTMS->ShutDownTMS();
//	sleep(3);
	SwtichTakeOverSMS();

	g_tmDBSynch = 0;
	GlobalStatus::GetInstinct()->SetStatus(1);// 设置运行状态为恢复接管状态
	//g_RunState=1;// 设置运行状态为正常运行状态
}

// 开始TMS
void CInvoke::StartTMS()
{
	LOGFAT(ERROR_POLICYTRI_TMSSTARTUP,"Fault Of Policys Trigger StartTMS!");
	if(m_ptrTMS != NULL)
	{
		m_ptrTMS->StartTMS();
	}
}

// 关闭指定的sms
bool CInvoke::CloseSMS(std::string strHallID)
{
	LOGINFFMT(0," CloseSMS %s!",strHallID.c_str());
	if(m_ptrLstHall != NULL)
	{
		return m_ptrLstHall->CloseSMS(strHallID);
	}
	else
	{
		return false;
	}
}

// 开启指定的sms
bool CInvoke::StartSMS(std::string strHallID)
{
	LOGINFFMT(0," StartSMS %s!",strHallID.c_str());
	if(m_ptrLstHall != NULL)
	{
		return m_ptrLstHall->StartSMS(strHallID);
	}
	else
	{
		return false;
	}
}

// 解决sms运行冲突，以权重做为关闭还要保留的判断条件，权重小则关闭。
// 正在运行占16，运行数量站相应数量的权重
bool CInvoke::SolveConflict(std::vector<ConflictInfo> &vecCI)
{	

	if(m_ptrLstHall == NULL)
	{
		return false;
	}
	int nLen = vecCI.size();
	for(int i=0;i<nLen ;i++)
	{
		int nMWeight=0;
		int nSWeight=0;
		ConflictInfo &node = vecCI[i];
		LOGINFFMT(0," SolveConflict %s!",node.strHallID.c_str());
		if(node.nMainState>103)
		{
			nMWeight+=16;
		}
		if(node.nStdbyState>103)
		{
			nSWeight+=16;
		}

		nMWeight += 8 - node.nMainSMSSum;
		nSWeight += 8 - node.nStdbySMSSum;
		
      
		if(node.nType==1)// 两端都启动了，则关闭一方
		{
			if(node.nMainState == 101 && node.nStdbyState == 101)
			{
				if(nMWeight < nSWeight)// 权重小的一方关闭
				{
					LOGINFFMT(0," SolveConflict(ALL RUN %s),Close SMS At MainHost(M%d:S%d) ",node.strHallID.c_str(),nMWeight,nSWeight);
					m_ptrLstHall->CloseSMS(node.strHallID);
					node.nMainSMSSum--;
					m_ptrLstHall->ChangeSMSHost(node.strHallID,STDBYRUNTYPE);
					m_ptrLstHall->UpdateDataBase(node.strHallID,STDBYRUNTYPE);

				}
				else
				{
					LOGINFFMT(0," SolveConflict(ALL RUN %s),Close SMS At STDBY(M%d:S%d) ",node.strHallID.c_str(),nMWeight,nSWeight);
					m_ptrLstHall->StartOrCloseStdBySMS(false,node.strHallID);
					node.nStdbySMSSum--;
					m_ptrLstHall->ChangeSMSHost(node.strHallID,MAINRUNTYPE);
					m_ptrLstHall->UpdateDataBase(node.strHallID,MAINRUNTYPE);

				}
			}
			else
			{
				if(node.nMainState == 201)
				{
					LOGINFFMT(0," SolveConflict(ALL RUN %s),Close SMS At STDBY(due to Main Playing) ",node.strHallID.c_str());
					m_ptrLstHall->StartOrCloseStdBySMS(false,node.strHallID);
					node.nStdbySMSSum--;
					m_ptrLstHall->ChangeSMSHost(node.strHallID,MAINRUNTYPE);
					m_ptrLstHall->UpdateDataBase(node.strHallID,MAINRUNTYPE);
				}
				else if(node.nStdbyState == 201)
				{
					LOGINFFMT(0," SolveConflict(ALL RUN %s),Close SMS At MainHost(due to STDBY Playing) ",node.strHallID.c_str());
					m_ptrLstHall->CloseSMS(node.strHallID);
					node.nMainSMSSum--;
					m_ptrLstHall->ChangeSMSHost(node.strHallID,STDBYRUNTYPE);
					m_ptrLstHall->UpdateDataBase(node.strHallID,STDBYRUNTYPE);
				}
			}
			
		}
		else if(node.nType==2) // 两端都没有启动，则在一方开启
		{
			if(nMWeight > nSWeight )// 权重大的一方开启
			{
				LOGINFFMT(0," SolveConflict(ALL NORUN %s),Close SMS At MainHost(M%d:S%d) ",node.strHallID.c_str(),nMWeight,nSWeight);
				m_ptrLstHall->StartSMS(node.strHallID);
				node.nMainSMSSum++;
				m_ptrLstHall->ChangeSMSHost(node.strHallID,MAINRUNTYPE);
				m_ptrLstHall->UpdateDataBase(node.strHallID,MAINRUNTYPE);

			}
			else
			{
				LOGINFFMT(0," SolveConflict(ALL NORUN %s),Close SMS At STDBY(M%d:S%d) ",node.strHallID.c_str(),nMWeight,nSWeight);
				m_ptrLstHall->StartOrCloseStdBySMS(true,node.strHallID);
				node.nStdbySMSSum++;
				m_ptrLstHall->ChangeSMSHost(node.strHallID,STDBYRUNTYPE);
				m_ptrLstHall->UpdateDataBase(node.strHallID,STDBYRUNTYPE);

			}
		}
		
	}
	//g_RunState=1; // 设置全局状态为正常运行状态
	GlobalStatus::GetInstinct()->SetStatus(1);// 设置全局状态为正常运行状态
}

int CInvoke::DcpHashCheck(std::string strPath,std::string strPklUuid,std::string &strErrInfo)
{
	struct stat buf;
	if(stat(strPath.c_str(),&buf)<0)
	{
		strErrInfo="path not exsit";
		LOGINFFMT(0,"DcpHashCheck:%s(%s)",strErrInfo.c_str(),strPath.c_str());
		return 1;
	}
	
	if(!S_ISDIR(buf.st_mode))
	{
		strErrInfo="path not directory";
		LOGINFFMT(0,"DcpHashCheck:%s(%s)",strErrInfo.c_str(),strPath.c_str());
		return 1;
	}
	
	if(m_ptrHash != NULL)
	{
		stHashTaskInfo st;
		st.strPath = strPath;
		st.strUUID = strPklUuid;
		m_ptrHash->AddHaskTask(st);
		return 0;
	}
}

int CInvoke::GetHashCheckPercent(std::string strPklUuid,int &nResult,int &nPercent,std::string &strErrInfo)
{
	if(m_ptrHash != NULL)
	{
		m_ptrHash->GetDcpHashCheckResult(strPklUuid,nResult,nPercent,strErrInfo);
		return 0;
	}
	else
	{
		return 1;
	}
}


// 拷贝dcp
int  CInvoke::CopyDcp(std::string PklUuid,std::string srcPath,std::string desPath,int &result ,std::string &errinfo)
{
	char buff[512];
	sprintf(buff,"Copy Task :src=%s dest=%s uuid=%s",
		srcPath.c_str(),desPath.c_str(),PklUuid.c_str());
	LOGINFFMT(0,"%s",buff);

	struct stat buf;
	if(stat(srcPath.c_str(),&buf)<0)
	{
		errinfo="src path not exsit";
		result = 1;
		return 1;
	}

	if(!S_ISDIR(buf.st_mode))
	{
		errinfo="src path not directory";
		result = 1;
		return 1;
	}

	if(stat(desPath.c_str(),&buf)<0)
	{
		errinfo="des path not exsit";
		result = 2;
		return 1;
	}

	if(!S_ISDIR(buf.st_mode))
	{
		errinfo="despath not directory";
		result = 2;
		return 1;
	}

	if(m_ptrFO!=NULL)
	{
		stFileOperatorInfo st;
		st.strSrcPath = srcPath;
		st.strDesPath = desPath;
		st.strUUID = PklUuid;
		st.enOpt = CP;
		m_ptrFO->AddFileOptTask(st);
		result = 0;
		
	}

}

// 删除dcp
int  CInvoke::DeleteDcp(std::string PklUuid,std::string Path,int &result ,std::string &errinfo)
{
	char buff[512];
	sprintf(buff,"DeteteDcp Task :Path=%s uuid=%s",Path.c_str(),PklUuid.c_str());
	LOGINFFMT(0,"%s",buff);

	struct stat buf;
	if(stat(Path.c_str(),&buf)<0)
	{
		errinfo="path not exsit";
		result = 1;
		return 1;
	}

	if(!S_ISDIR(buf.st_mode))
	{
		errinfo="path not directory";
		result = 1;
		return 1;
	}
	if(m_ptrFO!=NULL)
	{
		stFileOperatorInfo st;
		st.strSrcPath = Path;
		st.strUUID = PklUuid;
		st.enOpt = RM;
		m_ptrFO->AddFileOptTask(st);
		result = 0;
		return 0;
	}

}

// 获取拷贝dcp的进度/结果
bool  CInvoke::GetCopyDcpProgress(std::string &strPKIUUID,int &nResult,std::string &strErrInfo)
{
	if(m_ptrFO!=NULL)
	{
		m_ptrFO->GetCopyDcpProgress(strPKIUUID,nResult,strErrInfo);
		return true;
	}
	else
	{
		return false;
	}
}

// 获取删除dcp的进度/结果
bool  CInvoke::GetDeleteDcpProgress(std::string &strPKIUUID,int &nResult,std::string &strErrInfo)
{
	if(m_ptrFO!=NULL)
	{
		m_ptrFO->GetDeleteDcpProgress(strPKIUUID,nResult,strErrInfo);
		return true;
	}
	else
	{
		return false;
	}
}

bool CInvoke::UpdateDBSynch(std::string dbsynch)
{
	// 打开数据库
	C_Para *ptrPara = C_Para::GetInstance();
	CppMySQL3DB mysql;
	if(mysql.open(ptrPara->m_strDBServiceIP.c_str(),ptrPara->m_strDBUserName.c_str(),
		ptrPara->m_strDBPWD.c_str(),ptrPara->m_strDBName.c_str()) == -1)
	{
		LOGINFFMT(0,"mysql open failed!");
		return false;
	}

	char sql[256]={'\0'};
	snprintf(sql,sizeof(sql),"update system_config  set conf_val=\"%s\" where conf_item=\"db_synch\"",dbsynch.c_str());
	int i=0;
	while(i<3)
	{
		int nResult = mysql.execSQL(sql);
		if(nResult != -1)
		{
			LOGINFFMT(0,"CInvoke:update system_config:db_synch database OK<%s>",sql);
			break;
		}
		else
		{
			LOGINFFMT(0,"CInvoke:update system_config:db_synch database FAILED<%s>",sql);
		}
		i++;
		sleep(1);
	}

	if(i == 3)
	{
		LOGINFFMT(0,"CInvoke Update db_synch failed!<%s>",sql);
		return false;
	}
	return true;
}

// 检测数据库是否同步
bool  CInvoke::CheckDBSynch(long lSynch)
{
	// 打开数据库
	C_Para *ptrPara = C_Para::GetInstance();
	CppMySQL3DB mysql;
	if(mysql.open(ptrPara->m_strDBServiceIP.c_str(),ptrPara->m_strDBUserName.c_str(),
		ptrPara->m_strDBPWD.c_str(),ptrPara->m_strDBName.c_str()) == -1)
	{
		LOGINFFMT(0,"mysql open failed!");
		return false;
	}

	char sql[256]={'\0'};
	snprintf(sql,sizeof(sql),"select conf_val from system_config  where conf_item=\"db_synch\"");
	
	int nResult;
	CppMySQLQuery query = mysql.querySQL(sql,nResult);
	int nRows = 0 ;
	if((nRows = query.numRow()) == 0) 
	{
		LOGINFFMT(0,"find not db_synch field from tms.system_config!\n");
		return false;
	}

	query.seekRow(0);
	std::string tmpstr = query.getStringField("conf_val");
	if(atol(tmpstr.c_str())==lSynch)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool CInvoke::GetSMSPosition(std::string strHallID,std::string &strPos,int& nPort)
{
	LOGINFFMT(0," GetSMSPosition %s!",strHallID.c_str());
	if(m_ptrLstHall != NULL)
	{
		return m_ptrLstHall->GetSMSPosition(strHallID,strPos,nPort);
	}
	else
	{
		return false;
	}
	return true;
}

bool CInvoke::GetDBSynchStatus()
{
	struct sigaction sa;
	struct sigaction oldsa;
	sa.sa_handler=SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if(sigaction(SIGCHLD,&sa,&oldsa)<0)
	{
		return false;
	}
	std::string strCmd="bash /usr/local/imonitor/mysqlreplication";
	int nStatus=system(strCmd.c_str());
	sigaction(SIGCHLD,&oldsa,NULL);
	if(nStatus < 0) 
	{ 
		LOGINFFMT(0,"Get DB Synch Status system return error: %s", strerror(errno));
		return false; 
	} 
	if(WIFEXITED(nStatus))
	{
		int nResult = WEXITSTATUS(nStatus);
		CDataManager *pDM = CDataManager::GetInstance();
		pDM->SetDBSynchStatus(nResult);
		LOGINFFMT(0,"Get DB Synch Status: %d",nResult);
	}
	return true;

}

bool CInvoke::GetDBSynchStatus_PIP()
{
	FILE *fp;
	int nStatus=0;
	struct sigaction sa;
	struct sigaction oldsa;
	sa.sa_handler=SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if(sigaction(SIGCHLD,&sa,&oldsa)<0)
	{
		return false;
	}

	std::string strCmd="bash /usr/local/imonitor/mysqlreplication";
	if((fp=popen(strCmd.c_str(),"r"))==NULL)
	{ 
		LOGINFFMT(0,"Get DB Synch Status popen return error: %s", strerror(errno));
		sigaction(SIGCHLD,&oldsa,NULL);
		return false; 
	} 

// 	char buffer[256]={'\0'};
// 	while(fgets(buffer,256,fp)!=NULL)
// 	{
// 		if('\n' == buffer[strlen(buffer)-1]) 
// 		{ 
// 			buffer[strlen(buffer)-1] = '\0'; 
// 		} 
// 		LOGINFFMT(0,"mysqlreplication output: %s", buffer);
// 		memset(buffer,0,256);
// 	}
	nStatus=pclose(fp);
	if(nStatus!=-1&&!WIFEXITED(nStatus))
	{
		int nResult = WEXITSTATUS(nStatus);
		CDataManager *pDM = CDataManager::GetInstance();
		pDM->SetDBSynchStatus(nResult);
		LOGINFFMT(0,"Get DB Synch Status: %d",nResult);
		sigaction(SIGCHLD,&oldsa,NULL);
		return true;
	}
	else
	{
		LOGINFFMT(0,"Get DB Synch Status pclose return error: %s", strerror(errno));
		sigaction(SIGCHLD,&oldsa,NULL);
		return false;
	}
	

}

// 重启或关闭服务器
int CInvoke::ShutdownServer(int nType,int &state,std::string &strDesc)
{
	C_Para * ptrPara = C_Para::GetInstance();
	C_RunPara *ptrRunPara = C_RunPara::GetInstance();
	C_TaskList * ptrTaskList = C_TaskList::GetInstance();
	if(ptrPara->IsMain())
	{
		std::vector<SMSStatus> vecState;
		CDataManager::GetInstance()->GetSMSStat(vecState);
		int len = vecState.size();
		bool bBusy = false;
		for(int i = 0 ;i < len ;i++)
		{
			SMSStatus &node = vecState[i];
			if(node.nStatus != 101)
			{
				bBusy = true;
				break;
			}
		}

		if(bBusy)
		{
			state = 1;
			strDesc = "system busy ,can not reboot!";
			return 1;
		}
		
		// 询问tms是否可以重启
// 		if(!m_ptrTMS->AskTMSReboot())
// 		{
// 			state = 1;
// 			strDesc = "system busy ,can not reboot!";
// 			return 1;
// 		}
// 		else
		{
			//再让主机执行
			ptrTaskList->AddTask(nType==0?TASK_NUMBER_REBOOT:TASK_NUMBER_SHUTDOWN,NULL,ptrRunPara->GetCurTime() + 3,ONCE_TASK);
			LOGINFFMT(0,"-----------Reboot Or Shutdown After 3 Sec -------------------");
			state = 0;
			strDesc = "OK";
			return 0;
		}
	}
	else
	{
		ptrTaskList->AddTask(nType==0?TASK_NUMBER_REBOOT:TASK_NUMBER_SHUTDOWN,NULL,ptrRunPara->GetCurTime() + 3,ONCE_TASK);
		LOGINFFMT(0,"-----------Reboot Or Shutdown After 3 Sec -------------------");
		state = 0;
		strDesc = "OK";
		return 0;
	}
}

//重启或关机 0：为重启 1：关机
void CInvoke::shutdown(int nType)
{
	C_Para * ptrPara = C_Para::GetInstance();
	int nRole = ptrPara->GetRole();

	// 先让备机执行 
	// 成为接管角色后不对对端主机进行操作
	if(nRole == 1)
	{
		int state;
		std::string desc;
		m_ptrMonitor->SlaveReboot(nType,state,desc);
	}

	if(nType == 0)
	{
		LOGINFFMT(0,"-----------exe reboot command-------------------");
		system("reboot");
	}
	else
	{
		LOGINFFMT(0,"-----------exe shutdown command-------------------");
		system("shutdown -h now");
		
	}
}

// 设置定时重启服务器
int CInvoke::TimingRebootServer(int nDay,int nWeek,int nHour,int nMinute,
	int nRepeatType,int nRepeatCnt,int &nState,std::string &strDesc)
{

	m_ptrMonitor->WriteTimerFile(nDay,nWeek,nHour,nMinute,nRepeatType,
		nRepeatCnt,nState,strDesc);

	// 如果不是第二天才生效,则立即设置
	C_Para * ptrPara = C_Para::GetInstance();
	bool bMain = ptrPara->IsMain();

	if(bMain)
	{
		int slave_stat=0;
		std::string slave_Desc;
		m_ptrMonitor->InvokeSlaveTimingReboot(nDay,nWeek,nHour,nMinute,nRepeatType,
			nRepeatCnt,slave_stat,slave_Desc);
	}

	
	if(nRepeatType != 4 && bMain)
	{
		SetupRebootTimer();
	}
	if(nRepeatType == 0 && bMain)
	{
		C_TaskList * ptrTaskList = C_TaskList::GetInstance();
		ptrTaskList->DeleteTask(TASK_NUMBER_REBOOT);
		ptrTaskList->DeleteTask(TASK_NUMBER_SHUTDOWN);
	}
	return 0;
}

int CInvoke::SetupRebootTimer()
{
	int nEnable;
	int nDay;
	int nWeek;
	int nHour;
	int nMinute;
	int nType;
	int nRepeatCnt;
	int nExecnt;

	C_Para * ptrPara = C_Para::GetInstance();
	bool bMain = ptrPara->IsMain();
	if(!bMain)
	{
		return 0;
	}


	int iRet = m_ptrMonitor->ReadRebootTask(nEnable,nDay,nWeek,nHour,nMinute,nType,nRepeatCnt,nExecnt);
	LOGINFFMT(0,"------ReadRebootTask(%d,%d,%d,%d,%d,%d,%d,%d)",nEnable,nDay,nWeek,nHour,nMinute,
		nType,nRepeatCnt,nExecnt);

	if(!nEnable||(nRepeatCnt>0 && nExecnt>=nRepeatCnt))
	{
		LOGINFFMT(0,"------Timing Reboot Cannot Setup!(nEnable:%d||nExecnt>=nRepeatCnt(%d:%d))",nEnable,nExecnt,nRepeatCnt);
		return 0;
	}

	// 删除之前已有的定时重启
	C_TaskList * ptrTaskList = C_TaskList::GetInstance();
	ptrTaskList->DeleteTask(TASK_NUMBER_REBOOT);

	
	C_Time t1,t2;
	std::string strCurDate;
	t1.setCurTime();
	t1.getDateStr(strCurDate); 
	char buf[10]={'\0'};
	snprintf(buf,10," %2d:%2d:00",nHour,nMinute);
	std::string strDT = strCurDate + std::string(buf);
	t2.setTimeStr(strDT);

	switch(nType)
	{
	case 1:
		if(t1.getDay() == nDay && t1.getHour() < nHour)
		{
			ptrTaskList->AddTask(TASK_NUMBER_REBOOT,NULL,
				t2.getTimeInt(),ONCE_TASK);
			LOGINFFMT(0,"-----------Add Timer Reboot(each month day:%d) -------------------",nDay);
			m_ptrMonitor->FixRebootXmlAttribute("ExeCnt",++nExecnt);
		}
		break;
	case 2:
		if(t1.getWeek() == nWeek && t1.getHour() < nHour)
		{

			ptrTaskList->AddTask(TASK_NUMBER_REBOOT,NULL,
				t2.getTimeInt(),ONCE_TASK);
			LOGINFFMT(0,"-----------Add Timer Reboot(each week:%d) -------------------",nWeek);
			m_ptrMonitor->FixRebootXmlAttribute("ExeCnt",++nExecnt);
		}
		break;
	case 3:
	case 4:
		if(t1.getHour() < nHour)
		{
			ptrTaskList->AddTask(TASK_NUMBER_REBOOT,NULL,
				t2.getTimeInt(),ONCE_TASK);
			LOGINFFMT(0,"-----------Add Timer Reboot(each day:%d:%d) -------------------",nHour,nMinute);
			m_ptrMonitor->FixRebootXmlAttribute("ExeCnt",++nExecnt);
		}
		
		break;
	}

}

// 看门狗监测
int CInvoke::CheckByWatchdog()
{
	int nLen = m_vecPtrWathdog.size();
	for(int i = 0 ;i < nLen ; i++)
	{
		CWatchdog * ptrDog = m_vecPtrWathdog[i];
		if(!ptrDog->IsRun())
		{
			int nPid;
			ptrDog->StartInCurTerminal(nPid);
		}
	}
}



