//@file:C_Hall.cpp
//@brief: ʵ��C_Hall ���еķ�����
//@author:wangzhongping@oristartech.com
//dade:2012-07-12

#include "C_Hall.h"
#include "database/CppMySQL3DB.h"
#include "para/C_RunPara.h"

using namespace std;
C_Hall::C_Hall()
{
   
}

C_Hall::~C_Hall()
{
    
}


// ����Ӱ������
bool C_Hall::SetPara(std::string strHallid,std::string strHallName,SMSInfo stSms,ProjectorInfo stPrjinfo)
{
	return true;
}

// ��ȡSMS����״̬
bool C_Hall::GetWorkState(int &iState,std::string &strInfo)
{
	return true;
}

// ���DCP
bool C_Hall::Ingest(std::string strPath)
{
	return true;
}

// ���KDM
bool C_Hall::IngestKeyForm(std::string strPathName)
{
	return true;
}

// ɾ��KDM
bool C_Hall::DeleteKDM(std::string strKDMID)
{
	return true;
}

// ���Ӱ����Ӧ�豸�Ķ�ʱ����
int C_Hall::AddTask()
{
	return 0;
}

// ����SMS
bool C_Hall::StartSMS()
{
	return true;
}

// ����ӳ��
bool C_Hall::CheckProjector()
{
	return true;
}
