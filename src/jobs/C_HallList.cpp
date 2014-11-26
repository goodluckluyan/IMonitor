
#include "database/CppMySQL3DB.h"
#include "database/CppMySQLQuery.h"
#include "para/C_Para.h"
#include "log/C_LogManage.h"
#include "timeTask/C_TaskList.h"
#include "para/C_RunPara.h"
#include "log/C_LogManage.h"
#include "para/C_RunPara.h"
#include "C_HallList.h"
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>


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

	char buf[128]={'\0'};
	snprintf(buf,sizeof(buf),"/bin/bash %sstartup.sh",strPath.c_str());
	printf("%s\n",buf);
	system(buf);
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
		}
		
	}
	return true;
}

//切换本机的所有SMS
bool C_HallList::SwitchAllSMS()
{
	std::map<std::string,C_Hall *>::iterator it = m_mapHall.begin();
	for(;it != m_mapHall.end();it++)
	{
		C_Hall * ptr = it->second;
		if(ptr->IsLocal())
		{
			SwitchSMS(ptr->GetHallID());
		}
	}
}

//切换SMS
bool C_HallList::SwitchSMS(std::string strHallID)
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
	// 如果在本机运行
	if(ptr->IsLocal())
	{
		bool bRet = ptr->ShutDownSMS();
		if(C_Para::GetInstance()->m_bMain)
		{
			// 调用备机的切换Sms
 			C_Para *ptrPara = C_Para::GetInstance();
 			ptr->CallStandbySwitchSMS(ptrPara->m_strOIP,ptrPara->m_nOPort,strHallID);
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
		}
		int nPid = 0;
		ptr->StartSMS(nPid);
		if(nPid == 0)
		{
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
			}
			else
			{
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