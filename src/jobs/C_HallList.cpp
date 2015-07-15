
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

#define  LOG(errid,msg)  C_LogManage::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_JOBS,0,errid,msg)
#define  LOGERRFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_ERROR,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)
#define  LOGINFFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_INFO,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)
const std::string smsname = "oristar_sms_server";

// using namespace std;
C_HallList* C_HallList::m_pInstance = NULL;

#define MAINDEFRUN 0
#define STDBYDEFRUN 1
extern int g_RunState;

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

	std::map<std::string,C_CS*>::iterator cit = m_mapCS.begin();
	for(;cit!=m_mapCS.end();cit++)
	{
		C_CS * ptr=cit->second;
		if(ptr != NULL)
		{
			delete ptr;
		}
	}
    m_mapCS.clear();

	m_csCondTaskLst.EnterCS();
	pthread_cond_signal(&cond);
	m_csCondTaskLst.LeaveCS();

	pthread_cond_destroy(&cond);
	return;   
}

// 初始化
int C_HallList::Init()
{
	m_ptrDM = CDataManager::GetInstance();
	C_Para *ptrPara = C_Para::GetInstance();

	// 打开数据库
	CppMySQL3DB mysql;
	if(mysql.open(ptrPara->m_strDBServiceIP.c_str(),ptrPara->m_strDBUserName.c_str(),
		ptrPara->m_strDBPWD.c_str(),ptrPara->m_strDBName.c_str()) == -1)
	{
		LOG(0,"mysql open failed!\n");
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
		int nDefPos = query.getIntField("default_position");// 0为主，1为备
		int nCurPos = query.getIntField("cur_position");//1为主，2为备
		
		int nTmp=-1;
		if(nCurPos<0||nCurPos>2)
		{
			nTmp = nDefPos;
		}
		else
		{
			nTmp = nCurPos == 0? nDefPos : nCurPos-1; 
		}

		node.nRole = nTmp == MAINDEFRUN ? (int)MAINRUNTYPE : (int)STDBYRUNTYPE;
		if(node.nRole == (int)MAINRUNTYPE)
		{
			node.strIp = strIP;
			node.nPort = nPort;
		}
		else
		{
			node.strIp = strIP2;
			node.nPort = nPort2;
		}

		if(ptrPara->IsMain())
		{
			m_WebServiceLocalIP = strIP;
			m_WebServiceOtherIP = strIP2;
		}
		else 
		{
			m_WebServiceLocalIP = strIP2;
			m_WebServiceOtherIP = strIP;
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
			 node = ptrHall->Init(bRun,it->first);
			 UpdateDataBase(node.strId,node.nRole);
		}
		else//根据bRun标记是否新建运行sms
		{
			 node = ptrHall->Init(bRun);
			 UpdateDataBase(node.strId,node.nRole);
		}
		
		
		m_mapHall[node.strId]= ptrHall;
		m_mapCS[node.strId]= new C_CS;
		
	}
	m_ptrDM = CDataManager::GetInstance();
	if(m_ptrDM != NULL)
	{
		m_ptrDM->SetSMSInfo(vecSMSInfo);
	}

	// 启动tomcat
// 	ExeShell_Fork(ptrPara->m_strTOMCATPath,"shutdown.sh");
// 	ExeShell_Fork(ptrPara->m_strTOMCATPath,"startup.sh");

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
		LOGINFFMT(0,"%s\n",buf);
		system(buf);
		exit(0);
	}
	
	return true;
}


int C_HallList::GetPID(std::string strName,std::vector<int>& vecPID)
{	
	char acExe[256]={'\0'};
	snprintf(acExe,sizeof(acExe),"ps -ef|grep %s|grep -v \"grep %s\"|awk '{print $2}'",strName.c_str(),strName.c_str());
	FILE *pp = popen(acExe,"r");
	if(!pp)
	{
		LOG(0,"popen fail\n");
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
	std::map<std::string ,C_Hall *>::iterator it = m_mapHall.begin();
	for( ;it != m_mapHall.end() ;it++)
	{
		C_Hall * ptr = it->second;
		std::map<std::string,C_CS*>::iterator fcit=m_mapCS.find(it->first);
		if(fcit==m_mapCS.end())
		{
			continue;
		}

		C_CS * ptrCS=fcit->second;
		if((ptrCS!=NULL && !ptrCS->TryLock())||ptrCS == NULL)
		{
			continue;
		}
		
		int nState = 0;
		std::string strInfo;

		ptr->GetSMSWorkState(nState,strInfo);
		if(m_ptrDM != NULL)
		{
			m_ptrDM->UpdateSMSStat(ptr->GetHallID(),nState,strInfo);
		}
		m_csHallCurState.EnterCS();
		m_mapHallCurState[it->first] = nState;
		m_csHallCurState.LeaveCS();
		ptrCS->LeaveCS();
	}
	g_RunState = 1;
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

void C_HallList::GetTakeOverSMS(std::vector<std::string> &vecHallID)
{
	std::map<std::string ,C_Hall *>::iterator it = m_mapHall.begin();
	for( ;it != m_mapHall.end() ;it++)
	{
		if(it->second->IsLocal()&&it->second->GetRunRole()==(int)TAKEOVERRUNTYPE)
		{
			vecHallID.push_back(it->first);
		}
	}
}


// 启动所有sms
bool C_HallList::StartAllSMS(bool bCheckOtherSMSRun,std::vector<std::string>& vecHallid,bool bLocalHost/*=false*/)
{
	std::map<std::string ,C_Hall *>::iterator it = m_mapHall.begin();
	for( ;it != m_mapHall.end() ;it++)
	{
		C_Hall *ptr=it->second;
		if(ptr->IsLocal())
		{
			continue;	
		}
		
		// 如果要判断对端sms的运行状态则在获取不到对端sms状态时才在本机启动sms
		if(bCheckOtherSMSRun)
		{
			// 测试3次获取sms的状态
			int i = 0;
			while(i<3)
			{
				int nState = -1;
				std::string strInfo;
				C_CS * ptrCS = m_mapCS[it->first];

				// 为了防止和正常的获取sms的线程同时获取所以加个互斥
				ptrCS->EnterCS();
				int nRet = ptr->GetSMSWorkState(nState,strInfo);
				ptrCS->LeaveCS();
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
		}
		

		int nPid = 0;
		ptr->StartSMS(nPid,bLocalHost);
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
				SMSInfo stSMSInfo = ptr->ChangeSMSHost(m_WebServiceLocalIP,(int)TAKEOVERRUNTYPE,true);
				m_ptrDM->UpdateSMSStat(stSMSInfo.strId,stSMSInfo);

				// 只有备机需要通过修改数据的方式通知tms sms的位置，因为这时tms没有启动。
				int nRole=ptrPara->GetRole();
				if(nRole>=(int)STDBYROLE)
				{
					UpdateDataBase(stSMSInfo.strId,(int)STDBYRUNTYPE);
				}
				else
				{
					UpdateDataBase(stSMSInfo.strId,(int)MAINRUNTYPE);
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

//开启或关闭从的SMS bSoC true：开启  false 关闭 
bool C_HallList::StartOrCloseStdBySMS(bool bSoC,std::string strHallID)
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

	C_Para *ptrPara = C_Para::GetInstance();
	if(fit->second->CallStandbyStartOrCloseSMS(bSoC,ptrPara->m_strOIP,ptrPara->m_nOPort,strHallID)==0)
	{
		LOGINFFMT(0,"CallStandbyStartOrCloseSMS:%s(%s) OK!",strHallID.c_str(),bSoC?"Start":"Close");
		return true;
	}
	else
	{
		LOGERRFMT(0,"CallStandbyStartOrCloseSMS:%s(%s) Failed!",strHallID.c_str(),bSoC?"Start":"Close");
		return false;
	}

}


//切换SMS nState 返回1:表示没有些hallid 2:表示sms busy 3:启动新sms失败
bool C_HallList::SwitchSMS(bool bDelaySwitch,std::string strHallID,int &nState)
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

	std::map<std::string,C_CS*>::iterator fcit=m_mapCS.find(fit->first);
	if(fcit==m_mapCS.end())
	{
		nState = 1;
		return false;
	}

	C_CS * ptrCS=fcit->second;
	C_GuardCS guardcs(ptrCS);

	LOGINFFMT(ERROR_SMSSWITCH_START,"SMS:%s Switch Start!",strHallID.c_str());
	C_Hall * ptr = fit->second;

	// 因为只有主机才能发起切换，所有要判断是否为主
	if(m_ptrDM != NULL && C_Para::GetInstance()->IsMain())
	{
		int status;
		std::string info;
		ptr->GetSMSWorkState(status,info);
// 		SMSInfo smsinfo;
// 		m_ptrDM->GetSMSStat(strHallID,smsinfo);

		// 正在播放、正在导入、正在验证都禁止切换
		//if(smsinfo.stStatus.nStatus == SMS_STATE_PLAYING ||smsinfo.stStatus.nStatus ==SMS_STATE_CPL_RUNNING
		//	||smsinfo.stStatus.nStatus == SMS_STATE_INGEST_RUNNING)
		if(status == SMS_STATE_PLAYING ||status ==SMS_STATE_CPL_RUNNING
				||status == SMS_STATE_INGEST_RUNNING)
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
			// 调用备机的切换Sms
 			ptr->CallStandbySwitchSMS(bDelaySwitch,ptrPara->m_strOIP,ptrPara->m_nOPort,strHallID);
			LOGINFFMT(ERROR_SMSSWITCH_CALLOTHERSW,"SMS Switch call other switch sms!");
		}
		if(bRet)
		{
			 if(ptrPara->GetRole()>=(int)STDBYROLE)
			 {
				 SMSInfo stSMSInfo = ptr->ChangeSMSHost(m_WebServiceOtherIP,(int)MAINRUNTYPE,false);
				 m_ptrDM->UpdateSMSStat(stSMSInfo.strId,stSMSInfo);
			 }
			 else
			 {
				 SMSInfo stSMSInfo = ptr->ChangeSMSHost(m_WebServiceOtherIP,(int)STDBYRUNTYPE,false);
				 m_ptrDM->UpdateSMSStat(stSMSInfo.strId,stSMSInfo);
			 }
		}
	}
	else//在对端运行
	{
		if(ptrPara->IsMain())
		{
 			ptr->CallStandbySwitchSMS(bDelaySwitch,ptrPara->m_strOIP,ptrPara->m_nOPort,strHallID);
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
				if(ptrPara->GetRole() <= (int)ONLYMAINROLE)
				{
					SMSInfo stSMSInfo = ptr->ChangeSMSHost(m_WebServiceLocalIP,(int)MAINRUNTYPE,true);
					m_ptrDM->UpdateSMSStat(stSMSInfo.strId,stSMSInfo);
					UpdateDataBase(stSMSInfo.strId,(int)MAINRUNTYPE);
				}
				else
				{
					SMSInfo stSMSInfo = ptr->ChangeSMSHost(m_WebServiceLocalIP,(int)STDBYRUNTYPE,true);
					m_ptrDM->UpdateSMSStat(stSMSInfo.strId,stSMSInfo);
					UpdateDataBase(stSMSInfo.strId,(int)STDBYRUNTYPE);

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
	LOGERRFMT(ERROR_SMSBUSY_DELAYSWITCH,"Switch SMS:%s while SMS busy ,so delay switch SMS!",strHallID.c_str());
	if(IsHaveCondSwitchTask(strHallID))
	{
		LOGINFFMT(0,"Delay switch SMS(%s) task has been!\n ",strHallID.c_str());
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
	m_csHallCurState.EnterCS();
	std::map<std::string,int> mapTmp = m_mapHallCurState;
	m_csHallCurState.LeaveCS();
	
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

		// 防止状态不稳定时所做的切换，连续出现3次时才做切换
		if(bCond)
		{
			node.nTriaggerCnt++;
			LOGINFFMT(0,"Condition Switch Task :Condition OK(%d) Swtich SMS(%s)",
				node.nTriaggerCnt,node.strHallID.c_str());

		}
		else
		{
			node.nTriaggerCnt = 0;
		}

		if(node.nTriaggerCnt>=3)
		{
			LOGINFFMT(0,"Condition Switch Task :Condition OK Swtich SMS(%s)",node.strHallID.c_str());
			int nState;
			SwitchSMS(false,node.strHallID,nState);
			m_lstCondSwitch.erase(it++);
		}
	}
	
	m_csCondTaskLst.LeaveCS();
	return true;
}

// 备机调用主机进行切换
int C_HallList::SwitchSMSByStdby(bool bDelaySwitch,std::string strHallID)
{
	std::map<std::string,C_Hall*>::iterator fit = m_mapHall.find(strHallID);
	if(fit == m_mapHall.end())
	{
		return -1;
	}

	C_Hall * ptrHall = fit->second;
	C_Para *ptrPara = C_Para::GetInstance();
	
	return ptrHall->CallStandbySwitchSMS(bDelaySwitch,ptrPara->m_strOIP,ptrPara->m_nOPort,strHallID);

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
	snprintf(sql,sizeof(sql),"update devices set cur_position=%d where hall_id =\"%s\"",nPosition,strHallID.c_str());
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


//关闭sms
bool C_HallList::CloseSMS(std::string strHallID)
{
	std::map<std::string,C_Hall*>::iterator fit = m_mapHall.find(strHallID);
	if(fit == m_mapHall.end())
	{
		return false;
	}

	std::map<std::string,C_CS*>::iterator fcit=m_mapCS.find(fit->first);
	if(fcit==m_mapCS.end())
	{
		return false;
	}

	C_CS * ptrCS=fcit->second;
	C_GuardCS guardcs(ptrCS);

	C_Hall *ptr = fit->second;
	if(ptr->IsLocal())
	{
		if(!ptr->ShutDownSMS())
		{
			return false;
		}
		if(C_Para::GetInstance()->GetRole()>=(int)STDBYROLE)
		{
			SMSInfo stSMSInfo = ptr->ChangeSMSHost(m_WebServiceOtherIP,(int)MAINRUNTYPE,false);
			m_ptrDM->UpdateSMSStat(stSMSInfo.strId,stSMSInfo);
		}
		else
		{
			SMSInfo stSMSInfo = ptr->ChangeSMSHost(m_WebServiceOtherIP,(int)STDBYRUNTYPE,false);
			m_ptrDM->UpdateSMSStat(stSMSInfo.strId,stSMSInfo);
		}
	}
	else
	{
		return false;
	}
	return true;
}

// 在本机开启sms
bool C_HallList::StartSMS(std::string strHallID)
{
	std::map<std::string,C_Hall*>::iterator fit = m_mapHall.find(strHallID);
	if(fit == m_mapHall.end())
	{
		return false;
	}

	std::map<std::string,C_CS*>::iterator fcit=m_mapCS.find(fit->first);
	if(fcit==m_mapCS.end())
	{
		return false;
	}

	C_CS * ptrCS=fcit->second;
	C_GuardCS guardcs(ptrCS);

	C_Hall *ptr = fit->second;
	if(!ptr->IsLocal())
	{
		int nPid;
		if(!ptr->StartSMS(nPid))
		{
			return false;
		}
		if(C_Para::GetInstance()->GetRole()>=(int)STDBYROLE)
		{
			SMSInfo stSMSInfo = ptr->ChangeSMSHost(m_WebServiceLocalIP,(int)STDBYRUNTYPE,true);
			m_ptrDM->UpdateSMSStat(stSMSInfo.strId,stSMSInfo);
		}
		else
		{
			SMSInfo stSMSInfo = ptr->ChangeSMSHost(m_WebServiceLocalIP,(int)MAINRUNTYPE,true);
			m_ptrDM->UpdateSMSStat(stSMSInfo.strId,stSMSInfo);
		}
	}
	else
	{
		return false;
	}

	return true;
}