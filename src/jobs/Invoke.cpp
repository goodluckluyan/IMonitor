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
extern time_t g_tmDBSynch;
extern int g_RunState;
bool g_bQuit = false;
int g_LogLevel = 0;
int g_nRunType = 0; // 1Ϊ�ػ����� 0Ϊ����ģʽ
#define  LOGFAT(errid,msg)  C_LogManage::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_JOBS,0,errid,msg)
#define  LOGINFFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_INFO,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)
IPMgr* IPMgr::m_ptrIpMgr=NULL;
int  CInvoke::Init()
{
	// �����ڽ���ģʽ
	if(0 == g_nRunType)
	{
		PrintProductInfo();
	}
	LOGINFFMT(0,"Invoke Init...");

	C_Para * pPara = C_Para::GetInstance();
	IPMgr::GetInstance()->AddIP(pPara->m_strLIP,pPara->m_strOIP);

	// ���ݹ���ģ��
	CDataManager *pDM = CDataManager::GetInstance();
	if(!pDM->Init((void *)this,pPara->m_nSSD_Raid_Num,pPara->m_nSATA_Raid_Num))
	{
		return -1;
	}

	// ���Զ˵������
	bool bRunOther = false;
	if(m_ptrMonitor == NULL)
	{
		m_ptrMonitor = new  CMonitorSensor();
		m_ptrMonitor->Init(pPara->m_strOIP,pPara->m_nOPort);

		// �ȴ��Զ�IMonitor����
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
				bRunOther = true;
				break;
			}

			sleep(2);

			// �����ʱ
			time_t tm2;
			time(&tm2);
			if(tm2-tm1 >= 300)
			{
				LOGFAT(ERROR_OTHERMONITOR_NORUN,"Other Monitor Not Run !");
				break;
			}
		}
	}
	

	// ����TMS
	if(m_ptrTMS == NULL)
	{
		m_ptrTMS  = new CTMSSensor();
		m_ptrTMS->Init(pPara->m_strOIP,pPara->m_nOPort,pPara->m_nTMSWSPort);
	}

	// ���SMSģ��
	if(m_ptrLstHall == NULL)
	{
		m_ptrLstHall = new C_HallList();
		if(m_ptrLstHall->Init(m_ptrTMS)!=0)
		{
			return -1;
		}

		if(!bRunOther)
		{
			int nRole = pPara->GetRole();
			if(nRole== (int)STDBYROLE)
			{
				TakeOverMain(false);
			}
			else if(nRole == (int)MAINROLE)
			{
				TakeOverStdby(false);
			}

// 			std::vector<std::string> vecLocalRun;
// 			m_ptrLstHall->GetAllLocalRunHallID(vecLocalRun);
// 			for(int i = 0;i<vecLocalRun.size();i++)
// 			{
// 				std::string strNewIP;
// 				int nNewPort = 0;
// 				m_ptrLstHall->GetSMSRunHost(vecLocalRun[i],strNewIP,nNewPort);
// 				if(!strNewIP.empty() && nNewPort > 0 && C_Para::GetInstance()->IsMain())
// 				{
// 					bool bRet = m_ptrTMS->NotifyTMSSMSSwitch(vecLocalRun[i],strNewIP,nNewPort);
// 					LOGINFFMT(0,"Init:NotifyTMSSMSSwitch< %s Switch To %s:%d Host Result:%d>",vecLocalRun[i].c_str(),
// 						strNewIP.c_str(),nNewPort,bRet?1:0);
// 				}
// 			}
		}
		
	}

	std::string strLocalIP,strOtherIP;
	m_ptrLstHall->GetWebServiceIP(strLocalIP,strOtherIP);
	IPMgr::GetInstance()->AddIP(strLocalIP,strOtherIP);

	// ����ģ��
	if(m_ptrDispatch == NULL)
	{
		m_ptrDispatch = new CDispatch(this);
		std::string strPolicyPath = pPara->m_strInipath+"/policy.xml";
		m_ptrDispatch->Init(strPolicyPath);
		pDM->SetDispatchPtr(m_ptrDispatch);
	}

	// ���̼��ģ���ʼ��
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

	// �������ģ���ʼ��
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

	GetDBSynchStatus();
	//GetDBSynchStatus_PIP();
}

void CInvoke::DeInit()
{
	SAFE_DELETE(m_ptrDisk);
	SAFE_DELETE(m_ptrNet);
	SAFE_DELETE(m_ptrLstHall);
	SAFE_DELETE(m_ptrMonitor);
	SAFE_DELETE(m_ptrTMS);
	SAFE_DELETE(m_ptrDispatch);
	SAFE_DELETE(m_ptrHash);
	SAFE_DELETE(m_ptrFO);
}

bool CInvoke::AddInitTask()
{
	C_Para * pPara = C_Para::GetInstance();
	C_TaskList * ptrTaskList = C_TaskList::GetInstance();
	C_RunPara * ptrRunPara = C_RunPara::GetInstance();

	// ��Ӵ��̼�ⶨʱ����
	if(0 != pPara->m_nDiskCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_DISK_STATUS,NULL,ptrRunPara->GetCurTime()+pPara->m_nDiskCheckDelay);
	}

	// ��������ⶨʱ����
	if(0 != pPara->m_nEthCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_NET_STATUS,NULL,ptrRunPara->GetCurTime()+pPara->m_nEthCheckDelay);
	}

	// ���tms��ⶨʱ����
	if(0 != pPara->m_nTMSCheckDelay)
	{
		ptrTaskList->AddTask(TASK_NUMBER_GET_TMS_STATUS,NULL,ptrRunPara->GetCurTime()+pPara->m_nTMSCheckDelay);
	}

	// ���sms��ⶨʱ����
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

    // ֻ���԰���Ӽ��Զ˵��ȳ�����ں�ߣ���Ϊ�˷�ֹ����Ľ��лָ��ӹܲ���
	// ��ӶԶԶ˵��ȳ���ļ��Ķ�ʱ���񣬱���Ҫ�м��Զ˵��ȳ��������
	if(0 == pPara->m_nOtherMonitorCheckDelay)
	{
		pPara->m_nOtherMonitorCheckDelay = 3;
	}
	ptrTaskList->AddTask(TASK_NUMBER_GET_OTHERMONITOR_STATUS,NULL,
		ptrRunPara->GetCurTime() + pPara->m_nOtherMonitorCheckDelay);

	
	

	// ��ӵ�������
	ptrTaskList->AddTask(TASK_NUMBER_DISPATCH_ROUTINE,NULL,-1);

	// ��������л���������
	ptrTaskList->AddTask(TASK_NUMBER_CONDSWITCH_ROUTINE,NULL,-1);

	// ���hashУ�鴦������
	ptrTaskList->AddTask(TASK_NUMBER_HASHCHECK_ROUTINE,NULL,-1);

	// ����ļ�����
	ptrTaskList->AddTask(TASK_NUMBER_FILEOPERATION_ROUTINE,NULL,-1);

	// �����ڽ���ģʽ
	if(0 == g_nRunType)
	{
		// ��Ӵ����û���������
		ptrTaskList->AddTask(TASK_NUMBER_PROCESS_USERINPUT,NULL,0);
	}


}

// ��ȡʱ����
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
		break;
	}
	return nDelay;
}

//����ִ�У��ɲ�ͬ��ģ�����
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
		m_ptrLstHall->GetSMSWorkState();
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
	default:
	    nResult = 2;
	}
	return nResult;
}

//���������ַ���
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


// ��ӡ��Ʒ��Ϣ
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
	LOGINFFMT(0,"#                      <<<<<IMonitor1.0.0.4>>>>                               #");
	LOGINFFMT(0,"#-----------------------------------------------------------------------------#");
	LOGINFFMT(0,"%s",strMORS.c_str());
	LOGINFFMT(0,"�޸�:");
	LOGINFFMT(0,"	1���������������������ͬsms������");
	LOGINFFMT(0,"	2���������粻�ȶ�ʱ�������ӹܵ��ȶ�ʱ�ָ������Ĺ���");	
	LOGINFFMT(0,"#-----------------------------------------------------------------------------#");

}


//�����û�������Ƶ��̺߳���
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

// �л�TMS
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

// �л�SMS
bool CInvoke::SwitchSMS(std::string strHallID,bool bDelaySwitch,int &nRet)
{
	int nRole=C_Para::GetInstance()->GetRole();

	// ֻ��һ̨��������ʱ�������л�
	if(nRole == TMPMAINROLE || nRole == ONLYMAINROLE)
	{
		nRet = 3;// ��������쳣
		return false;
	}

	CDataManager *ptrDM = CDataManager::GetInstance();
	int nOtherStatus =ptrDM->GetOtherIMonitor();
	if(nOtherStatus <= 0)
	{
		LOGINFFMT(0,"SwitchSMS:Due To Other IMonitor Status Error(%d) ,So SwitchSMS Failed!",nOtherStatus);
		nRet = 3;// ��������쳣
		return false;
	}

	if(bDelaySwitch && m_ptrLstHall->IsHaveCondSwitchTask(strHallID))
	{
		nRet = 2;// sms������ʱ�л�
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

				 // ���û�ܳɹ���һֱ����
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
			 // ��Ϊֻ���������ܷ����л�������Ҫ�ж��Ƿ�Ϊ��
			 // ��Ϊsms busy�л�ʧ��,������ʱ�л�
			 if(bDelaySwitch && nState == 2 && C_Para::GetInstance()->IsMain())
			 {
				 m_ptrLstHall->AddCondSwitchTask(strHallID,"state",101);
				 nRet = 2;// sms������ʱ�л�
				 CDataManager::GetInstance()->EndSwitch();
				 return false;
			 }
			 else if(!bDelaySwitch && nState == 2 && C_Para::GetInstance()->IsMain())
			 {
				 nRet = 1;//sms��æ
				 CDataManager::GetInstance()->EndSwitch();
				 return false;
			 }
			 else
			 {
				 nRet = 3;//sms��æ
				 CDataManager::GetInstance()->EndSwitch();
				 return false;
			 }
			
		 }
	}
	else
	{
		nRet = 4;// �����������
		return false;
	}
}

// �л������ϵ�����SMS,���ڲ������Զ��л�
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


// �˳�ϵͳ
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

// �л������ӹܵ�sms
void CInvoke::SwtichTakeOverSMS()
{
	sleep(3);
	std::vector<std::string> vecSMS;
	m_ptrLstHall->GetTakeOverSMS(vecSMS);
	C_Para * pPara = C_Para::GetInstance();

	std::vector<SMSStatus> vecSMSStatus;

	bool bDirty=true;
	while(bDirty)
	{
		time_t tmUpdate=CDataManager::GetInstance()->GetOtherSMSstatus(vecSMSStatus);
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
				// �Զ�û������,��ִ���л�����
				if(OtherStatus.nRun==0)
				{
					LOGINFFMT(0,"Get Other SMS(%s) Lastest Status(nRun:%d),So Run Switch SMS!",vecSMS[i].c_str(),OtherStatus.nRun);
					if(!C_Para::GetInstance()->IsMain())
					{
						// ���������л��ӿ�,֧����ʱ�л�
						m_ptrLstHall->SwitchSMSByStdby(true,vecSMS[i]);
					}
					else
					{
						//֧����ʱ�л�
						int nState;
						SwitchSMS(vecSMS[i],true,nState);
					}
				}//�Զ����ܵ�sms�������򿪱ձ���sms
				else if(OtherStatus.nRun == 1)
				{
					LOGINFFMT(0,"Get Other SMS(%s) Lastest Status(nRun:%d),So Run Close SMS!",vecSMS[i].c_str(),OtherStatus.nRun);
					CloseSMS(vecSMS[i]);
				}
			}
			
		}
	}
}

// �ڱ�����������sms
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

		// ֻ��������Ҫͨ������web�ӿڵķ�ʽ֪ͨtms sms��λ�ã���Ϊ��ʱtms�Ѿ���������������
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

// �ӹ�����������Ϊ����ɫ
void CInvoke::TakeOverMain(bool bCheckOtherSMSRun)
{
	LOGFAT(ERROR_POLICYTRI_TMSSTARTUP,"Fault Of Policys Trigger TakeOverMain!");

	//Ϊ�˷�ֹ�����ݿ�ָ�ʱ200ip�仯ʱsms״̬����ȷ�����⣬�ڽӹ������ϵ�smsʱ����localhost���ݿ�ip����200.
	StartALLSMS(bCheckOtherSMSRun,true);
	sleep(1);
	if(C_Para::GetInstance()->GetRole() != TMPMAINROLE)
	{
		C_Para::GetInstance()->SetRoleFlag(TMPMAINROLE);
	}
}

// �ӹܱ���������Ϊ��ֻ��������ɫ
void CInvoke::TakeOverStdby(bool bCheckOtherSMSRun)
{
	LOGFAT(ERROR_POLICYTRI_TMSSTARTUP,"Fault Of Policys Trigger TakeOverStdby!");
	StartALLSMS(bCheckOtherSMSRun);
	sleep(1);
	if(C_Para::GetInstance()->GetRole() != ONLYMAINROLE)
	{
		C_Para::GetInstance()->SetRoleFlag(ONLYMAINROLE);
	}
}

// �ӣ�ֻ��������������ɫ�ı��Ϊ����ɫ
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
	g_RunState=2;// ��������״̬Ϊ�ָ��ӹ�״̬
	C_Para::GetInstance()->SetRoleFlag(MAINROLE);
	SwtichTakeOverSMS();

	g_tmDBSynch = 0;
	g_RunState=1;// ��������״̬Ϊ��������״̬
}

// ����ʱ���������ı��Ϊ����ɫ
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
	g_RunState=2;// ��������״̬Ϊ�ָ��ӹ�״̬
	C_Para::GetInstance()->SetRoleFlag(STDBYROLE);
	m_ptrTMS->ShutDownTMS();
	sleep(10);
	SwtichTakeOverSMS();

	g_tmDBSynch = 0;
	g_RunState=1;// ��������״̬Ϊ��������״̬
}

// ��ʼTMS
void CInvoke::StartTMS()
{
	LOGFAT(ERROR_POLICYTRI_TMSSTARTUP,"Fault Of Policys Trigger StartTMS!");
	if(m_ptrTMS != NULL)
	{
		m_ptrTMS->StartTMS();
	}
}

// �ر�ָ����sms
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

// ����ָ����sms
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

// ���sms���г�ͻ����Ȩ����Ϊ�رջ�Ҫ�������ж�������Ȩ��С��رա�
// ��������ռ16����������վ��Ӧ������Ȩ��
bool CInvoke::SolveConflict(std::vector<ConflictInfo> &vecCI)
{	
	

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
      
		if(node.nType==1)// ���˶������ˣ���ر�һ��
		{
			if(nMWeight < nSWeight)// Ȩ��С��һ���ر�
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
		else if(node.nType==2) // ���˶�û������������һ������
		{
			if(nMWeight > nSWeight )// Ȩ�ش��һ������
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
	g_RunState=1; // ����ȫ��״̬Ϊ��������״̬
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


// ����dcp
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

// ɾ��dcp
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

// ��ȡ����dcp�Ľ���/���
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

// ��ȡɾ��dcp�Ľ���/���
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
	// �����ݿ�
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

// ������ݿ��Ƿ�ͬ��
bool  CInvoke::CheckDBSynch(long lSynch)
{
	// �����ݿ�
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