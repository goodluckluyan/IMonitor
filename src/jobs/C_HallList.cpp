//@file:C_HallList.cpp
//@brief: �����˶�C_HallList ���з�����ʵ�֡�
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



int C_HallList::Init(CDataManager * ptr )
{
	m_ptrDM = ptr;
	C_Para *ptrPara = C_Para::GetInstance();

	// �����ݿ�
	CppMySQL3DB mysql;
	if(mysql.open(ptrPara->m_strDBServiceIP.c_str(),ptrPara->m_strDBUserName.c_str(),
		ptrPara->m_strDBPWD.c_str(),ptrPara->m_strDBName.c_str()) == -1)
	{
		printf("mysql open failed!\n");
		return false;
	}

	// ��ȡhallinfo��,��ʼ��sms��Ϣ
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

	if(ptr != NULL)
	{
		ptr->SetSMSInfo(vecSMSInfo);
	}


	
	
	return 0;
}


// ��ȡSMS����״̬
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


