#include "C_AdditionalLog.h"
#include "../para/C_RunPara.h"
#include "C_constDef.h"
using namespace std;
C_AdditionalLog* C_AdditionalLog::m_pInstance = NULL;
	
C_AdditionalLog::C_AdditionalLog()
{

}

C_AdditionalLog::~C_AdditionalLog()
{
		if(m_pInstance != NULL)
		{
			delete m_pInstance;
			m_pInstance = NULL;
		}
		if(m_pLog != NULL)
		{
			delete m_pLog;
			m_pLog = NULL;
		}
}

C_AdditionalLog* C_AdditionalLog::GetInstance()
{
		if(m_pInstance == NULL)
		{
			m_pInstance = new C_AdditionalLog;
		}
		return m_pInstance;
}

int C_AdditionalLog::WriteLog(int iLevel, int iModule, int iSubModule,c_u_int errorCode, std::string strError)
{
		char a[12];
		memset(a,0,12);
	  int  iTmpNumber = CreateLogNumber(iLevel, iModule, iSubModule, errorCode);
		sprintf(a,"%d",iTmpNumber);
		strError = a + strError;
		m_CS.EnterCS();
		m_pLog->WriteLog(iLevel,strError.c_str());
		m_CS.LeaveCS();
		return 0;
}

int C_AdditionalLog::Init()
{
	string str;
	C_RunPara::GetInstance()->GetExePath(str);
	m_pLog = new LogManage(UDEFAULT_LOG_LEVEL, 1024*10240, str.c_str(), "Tms20_DeviceService");
	return 0;
}
