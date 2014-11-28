
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/resource.h>
#include "database/CppMySQL3DB.h"
#include "database/CppMySQLQuery.h"
#include "para/C_Para.h"
#include "log/C_LogManage.h"
#include "timeTask/C_TaskList.h"
#include "para/C_RunPara.h"
#include "log/C_LogManage.h"
#include "para/C_RunPara.h"
#include "C_HallList.h"

#define  LOG(errid,msg)  C_LogManage::GetInstance()->WriteLog(LOG_FATAL,LOG_MODEL_JOBS,0,errid,msg)

// using namespace std;
C_HallList* C_HallList::m_pInstance = NULL;

C_HallList::~C_HallList()
{
	//ShutdownTOMCAT(C_Para::GetInstance()->m_strTOMCATPath);
	std::map<std::string,C_Hall*>::iterator it = m_mapHall.begin();
	for(;it != m_mapHall.end();it++)
	{
		C_Hall* ptr = it->second;
		if(ptr->IsLocal())
		{
			bool bRet = ptr->ShutDownSMS();
		}

		delete ptr;
	}	
	m_mapHall.clear();

	m_csCondTaskLst.EnterCS();
	pthread_cond_signal(&cond);
	m_csCondTaskLst.LeaveCS();

	pthread_cond_destroy(&cond);
	return;   
}



int C_HallList::Init(bool bRunOther )
{
	m_ptrDM = CDataManager::GetInstance();
	C_Para *ptrPara = C_Para::GetInstance();

	// 打开数据库
	CppMySQL3DB mysql;
	if(mysql.open(ptrPara->m_strDBServiceIP.c_str(),ptrPara->m_strDBUserName.c_str(),
		ptrPara->m_strDBPWD.c_str(),ptrPara->m_strDBName.c_str()) == -1)
	{
		printf("mysql open failed!\n");
		return false;
	}

	// 读取hallinfo表,初始化sms信息
	int nResult;
	CppMySQLQuery query = mysql.querySQL("select * from hallinfo",nResult);
	int nRows = 0 ;
	if((nRows = query.numRow()) == 0)
	{
		printf("C_HallList Initial failed,hallinfo talbe no rows!\n");
		return false;
	}

	std::vector<SMSInfo> vecSMSInfo;
	query.seekRow(0);
	for(int i = 0 ;i < nRows ; i++)
	{
		SMSInfo node;
		node.strId = query.getStringField("hallid");
		node.strIp = query.getStringField("ip");
		node.nPort = atoi(query.getStringField("port"));
		int nTmp = query.getIntField("role");

		// 如查对方调度软件没有启动，则在本机启动所有sms
		if(bRunOther)
		{
			node.nRole = nTmp == 1 ? 1 : 2;
		}
		else
		{
			node.nRole = ptrPara->m_bMain ? 1 : 2;
		}
		node.stStatus.hallid = node.strId;
		node.strExepath = query.getStringField("exepath");
		node.strConfpath = query.getStringField("confpath");
		query.nextRow();
		vecSMSInfo.push_back(node);
	}
	//std::vector<int> vecPID;
	//Getpid("sms",vecPID);
	int nLen = vecSMSInfo.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		SMSInfo &node = vecSMSInfo[i];
	
		bool bRun = ptrPara->m_bMain ? node.nRole == 1 : node.nRole == 2;
		C_Hall * ptrHall = new C_Hall(node);
		node.stStatus.nRun = ptrHall->Init(bRun);
		m_mapHall[node.strId]= ptrHall;

		if(bRun)
		{
			m_WebServiceLocalIP = node.strIp;
		}
		else
		{
			m_WebServiceOtherIP = node.strIp;
		}
	}
	m_ptrDM = CDataManager::GetInstance();
	if(m_ptrDM != NULL)
	{
		m_ptrDM->SetSMSInfo(vecSMSInfo);
	}

	// 启动tomcat
	StartTOMCAT(ptrPara->m_strTOMCATPath);

	return 0;
}

//启动tomcat
bool C_HallList::StartTOMCAT(std::string strPath)
{
	if(strPath.empty())
	{
		return false;
	}
	
	if(strPath.at(strPath.size()-1) != '/')
	{
		strPath +="/";
	}

	struct rlimit rl;
	if(getrlimit(RLIMIT_NOFILE,&rl)<0)
	{
		return false;
	}
	pid_t pid;
	
	if((pid = fork()) == 0)
	{
		// 关闭所有父进程打开的文件描述符
		if(rl.rlim_max == RLIM_INFINITY)
		{
			rl.rlim_max = 1024;
		}
		for(int i = 0 ;i < rl.rlim_max;i++)
		{
			close(i);
		}

		char buf[128]={'\0'};
		snprintf(buf,sizeof(buf),"/bin/bash %sstartup.sh",strPath.c_str());
		printf("%s\n",buf);
		system(buf);
		exit(0);
	}
	
	return true;
}

//关闭tomcat
bool C_HallList::ShutdownTOMCAT(std::string strPath)
{
	if(strPath.empty())
	{
		return false;
	}

	if(strPath.at(strPath.size()-1) != '/')
	{
		strPath +="/";
	}

	char buf[128]={'\0'};
	snprintf(buf,sizeof(buf),"/bin/bash %sshutdown.sh",strPath.c_str());
	printf("%s\n",buf);
	system(buf);
	return true;
}

// int C_HallList::Getpid(std::string strName,std::vector<int>& vecPID)
// {	
// 	char acExe[64]={'\0'};
// 	snprintf(acExe,64,"pidof %s",strName.c_str());
// 	FILE *pp = popen(acExe,"r");
// 	if(!pp)
// 	{
// 		printf("popen fail\n");
// 		return -1;
// 	}
// 	char tmpbuf[128]={'\0'};
// 	std::vector<std::string> vecBuf;
// 	while(fgets(tmpbuf,sizeof(tmpbuf),pp)!=NULL)
// 	{
// 		vecBuf.push_back(tmpbuf);
// 	}
// 
// 	int nLen = vecBuf.size();
// 	for(int i = 0 ;i < nLen ;i++)
// 	{
// 		std::string &strtmp=vecBuf[i];
// 		int nStart = 0;
// 		int nPos = strtmp.find(' ',nStart);
// 		while(nPos != std::string::npos)
// 		{
// 			vecPID.push_back(atoi(strtmp.substr(nStart,nPos-nStart).c_str()));
// 			nStart = nPos+1;
// 			nPos = strtmp.find(' ',nStart);
// 		}
// 		vecPID.push_back(atoi(strtmp.substr(nStart).c_str()));
// 	}
// 
// 	pclose(pp);
// 	return 0;
// }

// 获取SMS工作状态
bool C_HallList::GetSMSWorkState()
{
	std::map<std::string ,C_Hall *>::iterator it = m_mapHall.begin();
	for( ;it != m_mapHall.end() ;it++)
	{
		C_Hall * ptr = it->second;
		int nState;
		std::string strInfo;
		if( ptr->IsLocal())
		{
			ptr->GetSMSWorkState(nState,strInfo);
			if(m_ptrDM != NULL)
			{
				m_ptrDM->UpdateSMSStat(ptr->GetHallID(),nState,strInfo);
			}
			m_CS.EnterCS();
			m_mapHallCurState[it->first] = nState;
			m_CS.LeaveCS();
		}
		
	}
	return true;
}

// 获取hallid
void C_HallList::GetAllHallID(std::vector<std::string> &vecHallID)
{
	std::map<std::string ,C_Hall *>::iterator it = m_mapHall.begin();
	for( ;it != m_mapHall.end() ;it++)
	{
		vecHallID.push_back(it->first);

	}
}


//切换SMS nState 返回1:表示没有些hallid 2:表示sms busy 3:启动新sms失败
bool C_HallList::SwitchSMS(std::string strHallID,int &nState)
{
	if(strHallID.empty())
	{
		return false;
	}
	std::map<std::string,C_Hall*>::iterator fit = m_mapHall.find(strHallID);
	if(fit == m_mapHall.end())
	{
		nState = 1;
		return false;
	}

	LOG(ERROR_SMSSWITCH_START,(std::string("SMS Switch Start!")+strHallID).c_str());
	C_Hall * ptr = fit->second;
	if(m_ptrDM != NULL && C_Para::GetInstance()->m_bMain)
	{
		SMSInfo smsinfo;
		m_ptrDM->GetSMSStat(strHallID,smsinfo);
		
		// 正在播放、正在导入、正在验证都禁止切换
		if(smsinfo.stStatus.nStatus == SMS_STATE_PLAYING ||smsinfo.stStatus.nStatus ==SMS_STATE_CPL_RUNNING
			||smsinfo.stStatus.nStatus == SMS_STATE_INGEST_RUNNING)
		{
			char buff[64];
			snprintf(buff,sizeof(buff),"Sms(%s) is busy cann't switch!",strHallID.c_str());
			LOG(ERROR_SMSBUSY_NOTSWITCH,buff);
			printf("%s\n",buff);
			nState = 2;
			return false;
		}
	}
	// 如果在本机运行
	if(ptr->IsLocal())
	{
		bool bRet = ptr->ShutDownSMS();
		LOG(ERROR_SMSSWITCH_LOCALSHUTDOWN,(std::string("SMS Switch:local run sms shutdown!")+strHallID).c_str());
		if(C_Para::GetInstance()->m_bMain)
		{
			// 调用备机的切换Sms
 			C_Para *ptrPara = C_Para::GetInstance();
 			ptr->CallStandbySwitchSMS(ptrPara->m_strOIP,ptrPara->m_nOPort,strHallID);
			LOG(ERROR_SMSSWITCH_CALLOTHERSW,"SMS Switch:call other switch sms!");
		}
		if(bRet)
		{
		     SMSInfo stSMSInfo = ptr->ChangeSMSHost(m_WebServiceOtherIP,false);
		     m_ptrDM->UpdateSMSStat(stSMSInfo.strId,stSMSInfo);
		}
	}
	else//在对端运行
	{
		if(C_Para::GetInstance()->m_bMain)
		{
			// 调用备机的切换Sms
 			C_Para *ptrPara = C_Para::GetInstance();
 			ptr->CallStandbySwitchSMS(ptrPara->m_strOIP,ptrPara->m_nOPort,strHallID);
			LOG(ERROR_SMSSWITCH_CALLOTHERSW,"SMS Switch:call other switch sms!");
		}
		int nPid = 0;
		ptr->StartSMS(nPid);
		LOG(ERROR_SMSSWITCH_LOCALRUN,(std::string("SMS Switch:local run !")+strHallID).c_str());
		if(nPid == 0)
		{
			nState = 3;
			return false;
		}

		// 验证启动是否成功
		char buf[32]={'\0'};
		snprintf(buf,sizeof(buf),"/proc/%d",nPid);
		struct stat dstat;
		if(stat(buf,&dstat) == 0)
		{
			if(S_ISDIR(dstat.st_mode))
			{
				SMSInfo stSMSInfo = ptr->ChangeSMSHost(m_WebServiceLocalIP,true);
				m_ptrDM->UpdateSMSStat(stSMSInfo.strId,stSMSInfo);
				LOG(ERROR_SMSSWITCH_LOCALRUNOK,(std::string("SMS Switch:local run OK!")+strHallID).c_str());
			}
			else
			{
				nState = 3;
				LOG(ERROR_SMSSWITCH_LOCALRUNFAIL,(std::string("SMS Switch:local run failed!")+strHallID).c_str());
				return false;
			}
		}

	}
	return true;
}	

// 获取运行主机及webservice端口
bool C_HallList::GetSMSRunHost(std::string strHallID,std::string &strIP,int &nPort)
{
	if(strHallID.empty())
	{
		return false;
	}
	std::map<std::string,C_Hall*>::iterator fit = m_mapHall.find(strHallID);
	if(fit == m_mapHall.end())
	{
		return false;
	}

	C_Hall * ptr = fit->second;
	ptr->GetRunHost(strIP,nPort);
	return true;
}

bool C_HallList::IsHaveCondSwitchTask(std::string strHallID)
{
	m_csCondTaskLst.EnterCS();
	std::list<stConditionSwitch> tmpLst = m_lstCondSwitch;
	m_csCondTaskLst.LeaveCS();

	bool bFind = false;
	std::list<stConditionSwitch>::iterator it = tmpLst.begin();
	for(;it != tmpLst.end();it++)
	{
		if(it->strHallID == strHallID)
		{
			bFind = true;
			break;
		}
	}
	
	return bFind;
}

// 添加条件等待切换任务
bool C_HallList::AddCondSwitchTask(std::string strHallID,std::string strCond,int nVal)
{
	LOG(ERROR_SMSBUSY_DELAYSWITCH,(std::string("Switch SMS while SMS busy ,so delay switch SMS!")+strHallID).c_str());
	if(IsHaveCondSwitchTask(strHallID))
	{
		printf("Delay switch SMS(%s) task has been!\n ",strHallID.c_str());
		return false;
	}

	stConditionSwitch node;
	node.strHallID = strHallID;
	node.strCond = strCond;
	node.nVal = nVal;

	m_csCondTaskLst.EnterCS();
	m_lstCondSwitch.push_back(node);
	pthread_cond_signal(&cond);
	m_csCondTaskLst.LeaveCS();
}

// 执行条件等待切换任务
bool C_HallList::ProcessCondSwitchTask()
{
	m_CS.EnterCS();
	std::map<std::string,int> mapTmp = m_mapHallCurState;
	m_CS.LeaveCS();
	
	m_csCondTaskLst.EnterCS();
	if(m_lstCondSwitch.size() == 0)
	{
		pthread_cond_wait(&cond,&(m_csCondTaskLst.m_CS));
	}

	if(m_lstCondSwitch.size() == 0)
	{
		m_csCondTaskLst.LeaveCS();
		return false;
	}
	std::list<stConditionSwitch>::iterator it = m_lstCondSwitch.begin();
	for(;it != m_lstCondSwitch.end();it++)
	{
		stConditionSwitch& node = *it;
		std::map<std::string,int>::iterator fit = mapTmp.find(node.strHallID);
		if(fit == mapTmp.end())
		{
			continue;
		}
		
		bool bCond = false;
		if(node.strCond == "state")
		{
			bCond = node.nVal == fit->second ? true :false;
		}
		
		if(bCond)
		{
			printf("Condition Switch Task :Condition OK Swtich SMS(%s)",node.strHallID.c_str());
			int nState;
			SwitchSMS(node.strHallID,nState);
			m_lstCondSwitch.erase(it++);
		}
	}
	
	m_csCondTaskLst.LeaveCS();
	return true;
}