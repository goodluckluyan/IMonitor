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
C_HallList::C_HallList()
{
	m_ptrDM = NULL;
 return;   
}

C_HallList::~C_HallList()
{
	return;   
}

C_HallList* C_HallList::GetInstance()
{
   if(m_pInstance == NULL)
   {
       m_pInstance = new C_HallList;
   }
   return m_pInstance;
}

void  C_HallList::DestoryInstance()
{
	if(m_pInstance != NULL)
	{
		delete m_pInstance;
		m_pInstance = NULL;
	}
}


int C_HallList::Init(CDataManager * ptr )
{

	return 0;
}



int C_HallList::AddHall(HALL_PARA &hallPara, std::string &strError)
{

	return 0; 
}


int C_HallList::InitHallsTask()
{

	return 0;
}


int C_HallList::CreateHall(HALL_PARA &hallPara, string &strError)
{

	return 0;
}

// ��ȡӰ��״̬
bool C_HallList::GetHallStat(std::string &strHallID)
{
	return true;
}

// ��ȡSMS����״̬
bool C_HallList::GetSMSWorkState(std::string &strHallID,int &iState,std::string &strInfo)
{
	return true;
}
// ���DCP
bool C_HallList::SMSIngest(std::string &strHallID,std::string strPath)
{
	return true;
}

// ���KDM
bool C_HallList::SMSIngestKeyForm(std::string &strHallID,std::string strPathName)
{
	return true;
}

// ɾ��KDM
bool C_HallList::SMSDeleteKDM(std::string &strHallID,std::string strKDMID)
{
	return true;
}

