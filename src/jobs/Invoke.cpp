#include <fcntl.h>
#include "Invoke.h"
#include "timeTask/C_TaskList.h"
#include "para/C_RunPara.h"
#include "para/C_Para.h"
#include "check_netcard.h"

bool g_bQuit = false;
int g_LogLevel = 0;
#define  LOGFAT(errid,msg)  C_LogManage::GetInstance()->WriteLog(LOG_FATAL,LOG_MODEL_JOBS,0,errid,msg)
#define  LOGINFFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(LOG_INFO,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)


int  CInvoke::Init()
{
	PrintProductInfo();

	// 数据管理模块
	CDataManager *pDM = CDataManager::GetInstance();
	if(!pDM->Init((void *)this))
	{
		return -1;
	}

	// 监测对端调度软件
	bool bRunOther = false;
	C_Para * pPara = C_Para::GetInstance();
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
				bRunOther = true;
				break;
			}

			sleep(2);

			// 如果超时
			time_t tm2;
			time(&tm2);
			if(tm2-tm1 >= 300)
			{
				LOGFAT(ERROR_OTHERMONITOR_NORUN,"Other Monitor Not Run !");
				printf("Other Monitor Not Run !\n");
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

	// 监测SMS模块
	if(m_ptrLstHall == NULL)
	{
		m_ptrLstHall = new C_HallList();
		m_ptrLstHall->Init(bRunOther);
	}

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
			printf("Initial Fail! Check Raid Status Fail!\n");
			return -1;
		}
		else
		{
			printf("Raid Check Done.\n");
		}
	}

	// 网卡监测模块初始化
	if(m_ptrNet == NULL)
	{
		m_ptrNet = new Test_NetCard();
		if(!m_ptrNet->InitAndCheck())
		{
			printf("Initial Fail! Check Eth Status Fail!\n");
			return -1;
		}
		else
		{
			printf("Eth Check Done.\n");
		}
	}
}

void CInvoke::DeInit()
{
	SAFE_DELETE(m_ptrDisk);
	SAFE_DELETE(m_ptrNet);
	SAFE_DELETE(m_ptrLstHall);
	SAFE_DELETE(m_ptrMonitor);
	SAFE_DELETE(m_ptrTMS);
	SAFE_DELETE(m_ptrDispatch);
}

bool CInvoke::AddInitTask()
{
	// 设置各个状态检测的时间间隔
	m_nDiskCheckDelay = 300;
	m_nEthCheckDelay = 10;
	m_nOtherMonitorCheckDelay = 10;
	m_nOtherTMSCheckDelay = 10;
	m_nOtherSMSCheckDelay = 6;
	m_nOtherRAIDCheckDelay = 5;
	m_nOtherEthCheckDelay = 5;
	m_nOtherSwitchCheckDelay = 5;
	m_nOtherSpeedLmtCheckDelay = 5;
	m_nOtherEWCheckDelay = 5;
	m_nHallListCheckDelay = 5;
	m_nTMSCheckDelay = 5;

	C_TaskList * ptrTaskList = C_TaskList::GetInstance();
	C_RunPara * ptrRunPara = C_RunPara::GetInstance();
	 
	// 添加磁盘检测定时任务
	ptrTaskList->AddTask(TASK_NUMBER_GET_DISK_STATUS,NULL,ptrRunPara->GetCurTime()+m_nDiskCheckDelay);

	// 添加网络检测定时任务
	ptrTaskList->AddTask(TASK_NUMBER_GET_NET_STATUS,NULL,ptrRunPara->GetCurTime()+m_nEthCheckDelay);

	// 添加tms检测定时任务
	ptrTaskList->AddTask(TASK_NUMBER_GET_TMS_STATUS,NULL,ptrRunPara->GetCurTime()+m_nTMSCheckDelay);

	// 添加sms检测定时任务
	ptrTaskList->AddTask(TASK_NUMBER_GET_HALL_STATUS,NULL,ptrRunPara->GetCurTime()+m_nOtherSMSCheckDelay);
	
	// 添加对对端调度程序的检测的定时任务
	ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_STATUS,NULL,
		ptrRunPara->GetCurTime() + m_nOtherMonitorCheckDelay);
	ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_TMS_STATUS,NULL,
		ptrRunPara->GetCurTime() + m_nOtherTMSCheckDelay);
// 	ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_SMS_STATUS,NULL,
// 		ptrRunPara->GetCurTime() + m_nOtherSMSCheckDelay);
	ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_RAID_STATUS,NULL,
		ptrRunPara->GetCurTime() + m_nOtherRAIDCheckDelay);
	ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_ETH_STATUS,NULL,
		ptrRunPara->GetCurTime() + m_nOtherEthCheckDelay);
	ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_SWITCH_STATUS,NULL,
		ptrRunPara->GetCurTime() + m_nOtherSwitchCheckDelay);
	ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_SPEEDLIMIT_STATUS,NULL,
		ptrRunPara->GetCurTime() + m_nOtherSpeedLmtCheckDelay);
	ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_SMSEW_STATUS,NULL,
		ptrRunPara->GetCurTime() + m_nOtherEWCheckDelay);

	// 添加调度任务
	ptrTaskList->AddTask(TASK_NUMBER_DISPATCH_ROUTINE,NULL,-1);

	// 添加条件切换处理任务
	ptrTaskList->AddTask(TASK_NUMBER_CONDSWITCH_ROUTINE,NULL,-1);

	// 添加处理用户输入命令
	ptrTaskList->AddTask(TASK_NUMBER_PROCESS_USERINPUT,NULL,0);

}

// 获取时间间隔
int CInvoke::GetCheckDelay(int nStateType)
{
	int nDelay = 0;
	switch(nStateType)
	{
	case TASK_NUMBER_GET_DISK_STATUS:
		nDelay = m_nDiskCheckDelay;
		break;
	case TASK_NUMBER_GET_NET_STATUS:
		nDelay = m_nEthCheckDelay;
		break;
	case TASK_NUMBER_GET_HALL_STATUS:
		nDelay = m_nHallListCheckDelay ;
		break;
	case TASK_NUMBER_GET_TMS_STATUS:
		nDelay = m_nTMSCheckDelay ;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_STATUS:
		nDelay = m_nOtherMonitorCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_TMS_STATUS:
		nDelay = m_nOtherTMSCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_SMS_STATUS:
		nDelay = m_nOtherSMSCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_RAID_STATUS:
		nDelay = m_nOtherRAIDCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_ETH_STATUS:
		nDelay = m_nOtherEthCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_SWITCH_STATUS:
		nDelay = m_nOtherSwitchCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_SPEEDLIMIT_STATUS:
		nDelay = m_nOtherSpeedLmtCheckDelay;
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_SMSEW_STATUS:
		nDelay = m_nOtherEWCheckDelay;
		break;
	}
	return nDelay;
}

//任务执行，由不同的模块完成
int CInvoke::Exec(int iCmd,void * ptrPara)
{
	if( NULL==m_ptrLstHall || NULL==m_ptrDisk 
		|| NULL==m_ptrNet || NULL==m_ptrDispatch || NULL == m_ptrMonitor)
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
		m_ptrLstHall->ProcessCondSwitchTask();
		break;
	case TASK_NUMBER_GET_DISK_STATUS:
		m_ptrDisk->ReadMegaSASInfo();
		nResult = 0;
		break;
	case TASK_NUMBER_GET_NET_STATUS:
		m_ptrNet->GetAllEthStatus();
		nResult = 0;
		break;
	case TASK_NUMBER_GET_HALL_STATUS:
		m_ptrLstHall->GetSMSWorkState();
		break;
	case TASK_NUMBER_GET_TMS_STATUS:
		m_ptrTMS->GetTMSWorkState();
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
	default:
	    nResult = 2;
	}
	return nResult;
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
	std::string strMORS = C_Para::GetInstance()->m_bMain ? "MAIN" :"STDBY";
	printf("#-----------------------------------------------------------------------------#\n");
	printf("#                      <<<<<IMonitor1.0(%5s)>>>>                          #\n",strMORS.c_str());
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
	printf("# log	-0:print log level LOG_DEBUG\n");
	printf("# log	-1:print log level LOG_INFO\n");
	printf("# log	-2:print log level LOG_ERROR\n");
	printf("# log	-3:print log level LOG_FATAL\n");
	printf("#-----------------------------------------------------------------------------#\n");
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
				printf("Log Level Change To 0:LOG_DEBUG\n");
			}
			else if(vecParam[1] == "1")
			{
				g_LogLevel = 1;
				printf("Log Level Change To 1:LOG_INFO\n");
			}
			else if(vecParam[1] == "2")
			{
				g_LogLevel = 2;
				printf("Log Level Change To 2:LOG_ERROR\n");
			}
			else if(vecParam[1] == "3")
			{
				g_LogLevel = 3;
				printf("Log Level Change To 3:LOG_FATAL\n");
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

// 切换SMS
bool CInvoke::SwitchSMS(std::string strHallID)
{
	if(m_ptrLstHall->IsHaveCondSwitchTask(strHallID))
	{
		return false;
	}

	if(m_ptrLstHall != NULL)
	{
		LOGINFFMT(ERROR_POLICYTRI_SMSSWITCH,"Switch SMS(%s)! ",strHallID.c_str());

		int nState;
		 if(m_ptrLstHall->SwitchSMS(strHallID,nState))
		 {
			 std::string strNewIP;
			 int nNewPort = 0;
			 m_ptrLstHall->GetSMSRunHost(strHallID,strNewIP,nNewPort);
			 if(!strNewIP.empty() && nNewPort > 0 && C_Para::GetInstance()->m_bMain)
			 {
				 m_ptrTMS->NotifyTMSSMSSwitch(strHallID,strNewIP,nNewPort);
			 }
		 }
		 else
		 {
			 if(nState == 2 && C_Para::GetInstance()->m_bMain)// 因为sms busy切换失败
			 {
				 m_ptrLstHall->AddCondSwitchTask(strHallID,"state",101);
			 }
		 }
	}
	else
	{
		return false;
	}
}

// 切换本机上的所有SMS,属于策略中自动切换
bool CInvoke::SwitchAllSMS()
{
	if(m_ptrLstHall != NULL)
	{
		LOGFAT(ERROR_POLICYTRI_ALLSMSSWITCH,"Fault Of Policys Trigger Switch ALLSMS!");
		printf("Fault Of Policys Trigger SwitchAllSMS!\n");		
		std::vector<std::string> vecHallID;
		m_ptrLstHall->GetAllLocalRunHallID(vecHallID);
		for(int i = 0 ;i < vecHallID.size();i++)
		{
			if(C_Para::GetInstance()->m_bMain )
			{
				SwitchSMS(vecHallID[i]);
			}
			else 
			{
				m_ptrLstHall->SwitchSMSByStdby(vecHallID[i]);
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
	printf("Fault Of Policys Trigger Exit! 5 Sec Waiting \n");
	for(int i = 5;i > 0 ;i--)
	{
		sleep(1);
		printf("%d sec ....\n",i);
	}
	
	g_bQuit = true;
}

// 开始TMS
void CInvoke::StartTMS()
{
	printf("Fault Of Policys Trigger StartTMS!\n");
	LOGFAT(ERROR_POLICYTRI_TMSSTARTUP,"Fault Of Policys Trigger StartTMS!");
	m_ptrTMS->StartTMS();
}