//@file:C_HallList.cpp
//@brief: 包含了对C_HallList 所有方法的实现。
//@author:wangzhongping@oristartech.com
//dade:2012-07-12



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
#include <netinet/in.h>
#include <arpa/inet.h>


// using namespace std;
C_HallList* C_HallList::m_pInstance = NULL;

C_HallList::~C_HallList()
{
	return;   
}



int C_HallList::Init( )
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
		node.nRole = nTmp == 1 ? 1 : 2;
		node.strExepath = query.getStringField("exepath");
		node.strConfpath = query.getStringField("confpath");
		query.nextRow();
	}
	
	int nLen = vecSMSInfo.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		SMSInfo &node = vecSMSInfo[i];
		if(ptrPara->m_bMain && node.nRole == 1)
		{
			C_Hall * ptrHall = new C_Hall(node);
			m_Halllist.push_back(ptrHall);
		}
		else if(!ptrPara->m_bMain && node.nRole == 2)
		{
			C_Hall * ptrHall = new C_Hall(node);
			m_Halllist.push_back(ptrHall);
		}

	}
	m_ptrDM = CDataManager::GetInstance();
	if(m_ptrDM != NULL)
	{
		m_ptrDM->SetSMSInfo(vecSMSInfo);
	}


	
	
	return 0;
}


// 获取SMS工作状态
bool C_HallList::GetSMSWorkState()
{
	std::list<C_Hall *>::iterator it = m_Halllist.begin();
	for( ;it != m_Halllist.end() ;it++)
	{
		C_Hall * ptr = *it;
		int nState;
		std::string strInfo;
		ptr->GetSMSWorkState(nState,strInfo);
		if(m_ptrDM != NULL)
		{
			m_ptrDM->UpdateSMSStat(ptr->GetHallID(),nState,strInfo);
		}
	}
	return true;
}

//切换sms
bool C_HallList::SwitchSMS(std::string strHallID)
{
   
}
