
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/resource.h>
#include "database/CppMySQL3DB.h"
#include "para/C_Para.h"
#include "log/C_LogManage.h"
#include "C_HallList.h"

#define  LOG(errid,msg)  C_LogManage::GetInstance()->WriteLog(LOG_FATAL,LOG_MODEL_JOBS,0,errid,msg)
#define  LOGERRFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(LOG_ERROR,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)
#define  LOGINFFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(LOG_INFO,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)
const std::string smsname = "oristar_sms_server";

// using namespace std;
C_HallList* C_HallList::m_pInstance = NULL;

#define MAINRUN 0
#define STDBYRUN 1

C_HallList::~C_HallList()
{
	//ShutdownTOMCAT(C_Para::GetInstance()->m_strTOMCATPath);
	std::map<std::string,C_Hall*>::iterator it = m_mapHall.begin();
	for(;it != m_mapHall.end();it++)
	{
		C_Hall* ptr = it->second;
		if(ptr->IsLocal())
		{
			//bool bRet = ptr->ShutDownSMS();
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

// 初始化
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
	CppMySQLQuery query = mysql.querySQL("select * from devices where device_type=3 and device_model=\"AQ33CS\"",nResult);
	int nRows = 0 ;
	if((nRows = query.numRow()) == 0)
	{
		LOGERRFMT(ERROR_READSMSTABLE_NOROW,"C_HallList Initial failed,hallinfo talbe no rows!\n");
		return false;
	}

	std::vector<SMSInfo> vecSMSInfo;
	query.seekRow(0);
	for(int i = 0 ;i < nRows ; i++)
	{
		SMSInfo node;
		node.strId = query.getStringField("hall_id");
		std::string strIP = query.getStringField("ip");
		int nPort = atoi(query.getStringField("port"));
		std::string strIP2 = query.getStringField("ip2");
		int nPort2 = atoi(query.getStringField("port"));
		int nTmp = query.getIntField("default_position");
		

		// 如查对方调度软件没有启动，则在本机启动所有sms
		if(bRunOther)
		{
			node.nRole = nTmp == MAINRUN ? 1 : 2;
		}
		else
		{
			node.nRole = ptrPara->IsMain() ? 1 : 2;
		}
		if(node.nRole == 1)
		{
			node.strIp = strIP;
			node.nPort = nPort;
		}
		else
		{
			node.strIp = strIP2;
			node.nPort = nPort2;
		}
		node.stStatus.hallid = node.strId;
		node.strExepath = query.getStringField("exepath");
		node.strConfpath = query.getStringField("confpath");
		query.nextRow();
		vecSMSInfo.push_back(node);
	}

	// 如果已有sms运行，则关联
	std::vector<int> vecPID;
	std::map<int,std::string> mapDir;
	GetPID(smsname,vecPID);
	for(int i = 0 ;i < vecPID.size();i++)
	{
		std::string strDir;
		if(!GetPIDExeDir(vecPID[i],strDir))
		{
			continue;
		}

		mapDir[vecPID[i] ] = strDir;
	}

	int nLen = vecSMSInfo.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		SMSInfo &node = vecSMSInfo[i];
	
		bool bRun = ptrPara->IsMain() ? node.nRole == 1 : node.nRole == 2;
		C_Hall * ptrHall = new C_Hall(node);
		
		// 是否已经存在
		std::map<int,std::string>::iterator it = mapDir.begin();
		for(;it != mapDir.end();it++)
		{
			if(it->second == node.strExepath)
			{
				LOGINFFMT(0,"SMS %s(%d) has been ,To Associate!",it->second.c_str(),it->first);
				break;
			}
		}

		//关联已经存在的sms
		if(it != mapDir.end())
		{
			node.stStatus.nRun = ptrHall->Init(bRun,it->first);
		}
		else//根据bRun标记是否新建运行sms
		{
			node.stStatus.nRun = ptrHall->Init(bRun);
		}
		
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
	ExeShell_Fork(ptrPara->m_strTOMCATPath,"shutdown.sh");
	ExeShell_Fork(ptrPara->m_strTOMCATPath,"startup.sh");

	return 0;
}

//启动tomcat
bool C_HallList::ExeShell_Fork(std::string strPath,std::string strShell)
{
	C_Para *ptrPara = C_Para::GetInstance();
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
		// 关闭所有父进程打开的文件描述符，以免子进程继承父进程打开的端口。
		if(rl.rlim_max == RLIM_INFINITY)
		{
			rl.rlim_max = 1024;
		}
		for(int i = 3 ;i < rl.rlim_max;i++)
		{
			close(i);
		}

		char buf[128]={'\0'};
		snprintf(buf,sizeof(buf),"/bin/bash %s%s",strPath.c_str(),strShell.c_str());
		printf("%s\n",buf);
		system(buf);
		exit(0);
	}
	
	return true;
}


int C_HallList::GetPID(std::string strName,std::vector<int>& vecPID)
{	
	char acExe[64]={'\0'};
	snprintf(acExe,64,"pidof %s",strName.c_str());
	FILE *pp = popen(acExe,"r");
	if(!pp)
	{
		printf("popen fail\n");
		return -1;
	}
	char tmpbuf[128]={'\0'};
	std::vector<std::string> vecBuf;
	while(fgets(tmpbuf,sizeof(tmpbuf),pp)!=NULL)
	{
		vecBuf.push_back(tmpbuf);
	}

	int nLen = vecBuf.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		std::string &strtmp=vecBuf[i];
		int nStart = 0;
		int nPos = strtmp.find(' ',nStart);
		while(nPos != std::string::npos)
		{
			vecPID.push_back(atoi(strtmp.substr(nStart,nPos-nStart).c_str()));
			nStart = nPos+1;
			nPos = strtmp.find(' ',nStart);
		}
		vecPID.push_back(atoi(strtmp.substr(nStart).c_str()));
	}

	pclose(pp);
	return 0;
}


// 获取进程执行的路径
bool C_HallList::GetPIDExeDir(int nPID,std::string &strDir)
{
	char buff[256]={'\0'};
	char dir[256]={'\0'};
	snprintf(buff,sizeof(buff),"/proc/%d/exe",nPID);
	if(readlink(buff,dir,256) <=0)
	{
		return false;
	}

	strDir = dir ;
	return true;
}

// 获取SMS工作状态
bool C_HallList::GetSMSWorkState()
{
	C_GuardCS guardcs(&m_csSwitching);
	std::map<std::string ,C_Hall *>::iterator it = m_mapHall.begin();
	for( ;it != m_mapHall.end() ;it++)
	{
		C_Hall * ptr = it->second;
		int nState = 0;
		std::string strInfo;
		//if( ptr->IsLocal())
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
void C_HallList::GetAllLocalRunHallID(std::vector<std::string> &vecHallID)
{
	std::map<std::string ,C_Hall *>::iterator it = m_mapHall.begin();
	for( ;it != m_mapHall.end() ;it++)
	{
		if(it->second->IsLocal())
		{
			vecHallID.push_back(it->first);
		}
	}
}

// 启动所有sms
bool C_HallList::StartAllSMS(std::vector<std::string>& vecHallid)
{
	std::map<std::string ,C_Hall *>::iterator it = m_mapHall.begin();
	for( ;it != m_mapHall.end() ;it++)
	{
		C_Hall *ptr=it->second;
		if(ptr->IsLocal())
		{
			continue;	
		}


		// 测试3次获取sms的状态
		int i = 0;
		while(i<3)
		{
			int nState = -1;
			std::string strInfo;
			int nRet = ptr->GetSMSWorkState(nState,strInfo);
			if(nRet >= 0 && nState > 0 && nState != 103 )
			{
				break;
			}
			LOGERRFMT(ERROR_GETSMSSTATUS_FAIL,"StartAllSMS SMS:%s GetState Fail!(nRet=%d,nState=%d",
				ptr->GetHallID().c_str(),nRet,nState);
			i++;
		}

		// 三次获取状态失败才开启sms
		if(i<3)
		{
			LOGERRFMT(ERROR_SMSSWITCH_LOCALRUN,"StartAllSMS:SMS:%s run normal can't local run!",ptr->GetHallID().c_str());
			continue;
		}

		int nPid = 0;
		ptr->StartSMS(nPid);
		LOGINFFMT(ERROR_SMSSWITCH_LOCALRUN,"StartAllSMS:SMS:%s local run !",ptr->GetHallID().c_str());

		if(nPid == 0)
		{
			LOGERRFMT(ERROR_SMSSWITCH_LOCALRUN,"StartAllSMS:SMS:%s local run failed!",ptr->GetHallID().c_str());
			continue;
		}

		// 验证启动是否成功
		C_Para *ptrPara = C_Para::GetInstance();
		char buf[32]={'\0'};
		snprintf(buf,sizeof(buf),"/proc/%d",nPid);
		struct stat dstat;
		if(stat(buf,&dstat) == 0)
		{
			if(S_ISDIR(dstat.st_mode))
			{
				SMSInfo stSMSInfo = ptr->ChangeSMSHost(m_WebServiceLocalIP,true);
				m_ptrDM->UpdateSMSStat(stSMSInfo.strId,stSMSInfo);
				if(ptrPara->GetRole()==(int)MAINROLE)
				{
					UpdateDataBase(stSMSInfo.strId,MAINRUN);
				}
				else if(ptrPara->GetRole()>=(int)STDBYROLE)
				{
					UpdateDataBase(stSMSInfo.strId,STDBYRUN);
				}
				vecHallid.push_back(stSMSInfo.strId);
				LOGINFFMT(ERROR_SMSSWITCH_LOCALRUNOK,"SMS:%s StartAllSMS local run OK!",ptr->GetHallID().c_str());
			}
			else
			{
				LOGERRFMT(ERROR_SMSSWITCH_LOCALRUNFAIL,"SMS:%s StartAllSMS local run failed!",ptr->GetHallID().c_str());
			}
		}

	}
}


//切换SMS nState 返回1:表示没有些hallid 2:表示sms busy 3:启动新sms失败
bool C_HallList::SwitchSMS(std::string strHallID,int &nState)
{
	C_GuardCS guardcs(&m_csSwitching);
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

	LOGINFFMT(ERROR_SMSSWITCH_START,"SMS:%s Switch Start!",strHallID.c_str());
	C_Hall * ptr = fit->second;
	if(m_ptrDM != NULL && C_Para::GetInstance()->IsMain())
	{
		SMSInfo smsinfo;
		m_ptrDM->GetSMSStat(strHallID,smsinfo);
		
		// 正在播放、正在导入、正在验证都禁止切换
		if(smsinfo.stStatus.nStatus == SMS_STATE_PLAYING ||smsinfo.stStatus.nStatus ==SMS_STATE_CPL_RUNNING
			||smsinfo.stStatus.nStatus == SMS_STATE_INGEST_RUNNING)
		{
			LOGERRFMT(ERROR_SMSSWITCH_START,"Sms(%s) is busy cann't switch!",strHallID.c_str());
			nState = 2;
			return false;
		}
	}

	// 如果在本机运行
	C_Para *ptrPara = C_Para::GetInstance();
	if(ptr->IsLocal())
	{
		bool bRet = ptr->ShutDownSMS();
		LOGINFFMT(ERROR_SMSSWITCH_LOCALSHUTDOWN,"SMS:%s Switch local run sms shutdown!",strHallID.c_str());
		if(C_Para::GetInstance()->IsMain())
		{
			// 调用备机的切换Sms`
 			ptr->CallStandbySwitchSMS(ptrPara->m_strOIP,ptrPara->m_nOPort,strHallID);
			LOGINFFMT(ERROR_SMSSWITCH_CALLOTHERSW,"SMS Switch call other switch sms!");
		}
		if(bRet)
		{
		     SMSInfo stSMSInfo = ptr->ChangeSMSHost(m_WebServiceOtherIP,false);
		     m_ptrDM->UpdateSMSStat(stSMSInfo.strId,stSMSInfo);
			 if(ptrPara->GetRole()>=(int)STDBYROLE)
			 {
				 UpdateDataBase(stSMSInfo.strId,STDBYRUN);
			 }
		}
	}
	else//在对端运行
	{
		if(ptrPara->IsMain())
		{
 			ptr->CallStandbySwitchSMS(ptrPara->m_strOIP,ptrPara->m_nOPort,strHallID);
			LOGINFFMT(ERROR_SMSSWITCH_CALLOTHERSW,"SMS Switch:call other switch sms!");
		}
		int nPid = 0;
		ptr->StartSMS(nPid);
		LOGINFFMT(ERROR_SMSSWITCH_LOCALRUN,"SMS:%s Switch:local run !",strHallID.c_str());

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
				if(ptrPara->GetRole() == (int)MAINROLE)
				{
					UpdateDataBase(stSMSInfo.strId,MAINRUN);
				}
				LOGINFFMT(ERROR_SMSSWITCH_LOCALRUNOK,"SMS:%s Switch local run OK!",strHallID.c_str());
			}
			else
			{
				nState = 3;
				LOGERRFMT(ERROR_SMSSWITCH_LOCALRUNFAIL,"SMS:%s Switch local run failed!",strHallID.c_str());
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

// 是否已经存在于延时切换队列中
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
//	LOG(ERROR_SMSBUSY_DELAYSWITCH,(std::string("Switch SMS while SMS busy ,so delay switch SMS!")+strHallID).c_str());
	LOGERRFMT(ERROR_SMSBUSY_DELAYSWITCH,"Switch SMS:%s while SMS busy ,so delay switch SMS!",strHallID.c_str());
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

// 备机调用主机进行切换
int C_HallList::SwitchSMSByStdby(std::string strHallID)
{
	std::map<std::string,C_Hall*>::iterator fit = m_mapHall.find(strHallID);
	if(fit == m_mapHall.end())
	{
		return -1;
	}

	C_Hall * ptrHall = fit->second;
	C_Para *ptrPara = C_Para::GetInstance();
	
	return ptrHall->CallStandbySwitchSMS(ptrPara->m_strOIP,ptrPara->m_nOPort,strHallID);

}

// 更新数据库中的sms运行主机位置
bool C_HallList::UpdateDataBase(std::string strHallID,int nPosition)
{
	// 打开数据库
	C_Para *ptrPara = C_Para::GetInstance();
	CppMySQL3DB mysql;
	if(mysql.open(ptrPara->m_strDBServiceIP.c_str(),ptrPara->m_strDBUserName.c_str(),
		ptrPara->m_strDBPWD.c_str(),ptrPara->m_strDBName.c_str()) == -1)
	{
		LOG(0,"mysql open failed!\n");
		return false;
	}

	char sql[256]={'\0'};
	snprintf(sql,sizeof(sql),"update devices set default_position=%d where hall_id = %s",nPosition,strHallID.c_str());
	int i=0;
	while(i<3)
	{
		int nResult = mysql.execSQL(sql);
		if(nResult != -1)
		{
			LOGINFFMT(0,"update database OK<%s>",sql);
			break;
		}
		else
		{
			LOGINFFMT(0,"update database FAILED<%s>",sql);
		}
		i++;
		sleep(1);
	}

	if(i == 3)
	{
		LOGERRFMT(ERROR_UPDATESMSTABLE_FAILED,"C_HallList Update SMS RUN Position failed!<%s>",sql);
		return false;
	}
	
	return true;
}