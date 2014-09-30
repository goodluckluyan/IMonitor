#include <sys/stat.h>
#include "C_LogManage.h"
#include "stdio.h"
#include "stdlib.h"
#include "../para/C_RunPara.h"
#include "C_AdditionalLog.h"
#include "../C_ErrorDef.h"
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <vector>
#include "../C_constDef.h"
#include <errno.h>
using namespace std;
C_LogManage* C_LogManage::m_pInstance = NULL;

C_LogManage::C_LogManage()
{
	m_bSettingDate = 0;
}

C_LogManage::~C_LogManage()
{
	
}

C_LogManage* C_LogManage::GetInstance()
{
	if(m_pInstance == NULL)
	{
		m_pInstance = new C_LogManage;
	}
	return m_pInstance;
}

void  C_LogManage::DestoryInstance()
{
	if(m_pInstance != NULL)
	{
		delete m_pInstance;
		m_pInstance = NULL;
	}
}



int C_LogManage::WriteLog(int iLevel, int iModule, int iSubModule,c_u_int errorCode, string strError)
{
	if(m_bSettingDate == 1)
	{
		PTMP_LOG pTmpLog = new TMP_LOG;
		pTmpLog->iLevel = iLevel;
		pTmpLog->iModule = iLevel;
		pTmpLog->iSubModule = iSubModule;
		pTmpLog->errorCode  = errorCode;
		pTmpLog->strError = strError;
		m_TmpLogCS.EnterCS();
		m_listTmpLog.push_back(pTmpLog);
		m_TmpLogCS.LeaveCS();
	}
	else
	{
		char a[12];
		memset(a,0,12);
		int  iTmpNumber = CreateLogNumber(iLevel, iModule, iSubModule, errorCode);
		sprintf(a,"0x%08x ",iTmpNumber);
		strError = a + strError;
		list<PLOG_STRUCT>::iterator it = m_listLog.begin();
		for(; it != m_listLog.end(); ++it)
		{
			if((*it)->iModule == iModule && (*it)->iSubModule == iSubModule)
			{
				if((*it)->pLogManage == NULL)
				{
					C_AdditionalLog::GetInstance()->WriteLog(3,24,0,ERROR_MEMORY_NOT_INIT,
						     "内存访问错误 pLogManage 没有被初试化！");
					return ERROR_MEMORY_NOT_INIT;
				}

				(*it)->m_CS.EnterCS();
				(*it)->pLogManage->WriteLog(iLevel,strError.c_str());
				(*it)->m_CS.LeaveCS();
				return 0;
			}
		}
		
	}
	return -1;
}

int C_LogManage::WriteLog(int iLogNumber, string strError)
{
	int iLevel;
	int iModule;
	int iSubModule;
	int iError;
	iError = SeparateLogNumber(iLogNumber, iLevel, iModule, iSubModule);
	return WriteLog(iLevel, iModule, iSubModule, iError, strError);
}
int C_LogManage::SetLogLevel(int iLevel, int iModele, int iSubModule)
{
	list<PLOG_STRUCT>::iterator it = m_listLog.begin();
	for(; it != m_listLog.end(); ++it)
	{
		if((*it)->iModule == iModele && (*it)->iSubModule == iSubModule)
		{
			if((*it)->pLogManage == NULL)
			{
					C_AdditionalLog::GetInstance()->WriteLog(3,24,0,ERROR_MEMORY_NOT_INIT,
						     "内存访问错误 pLogManage 没有被初试化！");
					return ERROR_MEMORY_NOT_INIT;
			}
			(*it)->pLogManage->SetLogLevel(iLevel);
			return 0;
		}
	}
	C_AdditionalLog::GetInstance()->WriteLog(3,24,0,ERROR_NOT_FIND_LOG_TYPE,
						     "与模块编号和子模块编号对应的日志类没有找到！");
					return ERROR_NOT_FIND_LOG_TYPE;
}
int C_LogManage::InitLogPath(std::string &strBootPath)
{
	m_strBootPath = strBootPath;
	int iResult = -1;

	if((iResult = CheckLogPath(strBootPath)) != 0)
	{
		return iResult;
	}
	SetLogDate();
	if((iResult = DeletePreLog(m_strLogPath)) != 0)
	{
		return  iResult;
	}
	if((iResult = CreateLogStruct(m_strLogPath)) != 0)
	{
		return iResult;
	}
	return 0;
}

int C_LogManage::SetLogDate()
{
	C_RunPara * pRunPara = C_RunPara::GetInstance();
	string strDate;
	pRunPara->GetCurDate(strDate);
	string str = strDate.substr(strDate.length()-2,2);
	m_strLogPath = m_strBootPath + '/' + str + '/';
	return 0;
}
int C_LogManage::CheckLogPath(std::string &strBootPath)
{
	struct stat StatBuf;
	int iResult = -1;
	iResult = stat(strBootPath.c_str(),&StatBuf);
	int err = 0;
	char strTmp[512];
	if(iResult < 0)
	{
		err = errno;
		sprintf(strTmp,"CheckLogPath() Error! errno:%d erorInfo:%s\n",err,strerror(err));
		printf(strTmp);
		C_AdditionalLog::GetInstance()->WriteLog(3,24,0,ERROR_GET_LOG_BOOT_PATH, strTmp);
		return ERROR_GET_LOG_BOOT_PATH;
	}
	if(S_ISDIR(StatBuf.st_mode) == 0)
	{
		sprintf(strTmp,"CheckLogPath() Error! %s is not path!\n", strBootPath.c_str());
		printf(strTmp);
		C_AdditionalLog::GetInstance()->WriteLog(3,24,0,ERROR_GET_LOG_BOOT_PATH, strTmp);
		return ERROR_GET_LOG_BOOT_PATH;
	}
	mode_t tmpMode = StatBuf.st_mode;
	string strSubPath; 
	char tmp[4];
	tmp[3] = '\0';
	for(int i=1; i<=31; ++i)
	{
		sprintf(tmp,"/%02d",i);
		strSubPath = strBootPath + tmp;
		iResult = stat(strSubPath.c_str(),&StatBuf);
		if((iResult == 0 && S_ISDIR(StatBuf.st_mode) == 0) || 
			(iResult != 0 && errno == ENOENT))
		{
			iResult = mkdir(strSubPath.c_str(),tmpMode);
			if(iResult != 0)
			{
				err = errno;
				sprintf(strTmp,"CheckLogPath() Error! mkdir(%s) error! errno:%d,errorInfo:%s \n",
				       strSubPath.c_str(),err,strerror(err));
				printf(strTmp);
				C_AdditionalLog::GetInstance()->WriteLog(3,24,0,ERROR_CREATE_LOG_PATH, strTmp);
				return ERROR_CREATE_LOG_PATH;
			}
		}
		else if(iResult != 0 && errno != ENOENT)
		{
				err = errno;
				sprintf(strTmp,"CheckLogPath() Error! stat(%s) error! errno:%d,errorInfo:%s \n",
				       strSubPath.c_str(),err,strerror(err));
				printf(strTmp);
				C_AdditionalLog::GetInstance()->WriteLog(3,24,0,ERROR_GET_LOG_PATH, strTmp);
				return ERROR_GET_LOG_PATH;
		} 
		else
		{
			continue;
		}
	}
	return 0;		
}

int C_LogManage::DeletePreLog(string &strPath)
{
	DIR *dp;
	struct dirent *dirp;
	int err;
	struct stat StatBuf;
	string strName;
	C_RunPara *pRunPara = C_RunPara::GetInstance();
	int iResult = -1;
	char strTmp[512];
	if((dp = opendir(strPath.c_str())) == NULL)
	{
				err = errno;
				sprintf(strTmp,"DeletePreLog() Error! opendir(%s) error! errno:%d,errorInfo:%s \n",
				       strPath.c_str(),err,strerror(err));
				printf(strTmp);
				C_AdditionalLog::GetInstance()->WriteLog(3,24,0,ERROR_DELETE_PRE_LOG, strTmp);
				return ERROR_DELETE_PRE_LOG;
	}
	while((dirp = readdir(dp)) != NULL)
	{
		strName = strPath +  + dirp->d_name;
		iResult = stat(strName.c_str(), &StatBuf);
		if(iResult != 0)
		{
			err = errno;
			sprintf(strTmp,"DeletePreLog() Error! stat(%s) error! errno:%d erorInfo:%s\n",
			strName.c_str(),err,strerror(err));
				printf(strTmp);
				C_AdditionalLog::GetInstance()->WriteLog(3,24,0,ERROR_DELETE_PRE_LOG, strTmp);
				return ERROR_DELETE_PRE_LOG;
		}
		if(S_ISREG(StatBuf.st_mode)!= 0 &&  StatBuf.st_mtime < pRunPara->GetZeroTime())
		{
				if(unlink(strName.c_str())!= 0)
				{
						err = errno;
						printf("DeletePreLog() Error! unlink(%s) error! errno:%d erorInfo:%s\n",
						strName.c_str(),err,strerror(err));
				printf(strTmp);
				C_AdditionalLog::GetInstance()->WriteLog(3,24,0,ERROR_DELETE_PRE_LOG, strTmp);
				return ERROR_DELETE_PRE_LOG;
				}
		}
	}
	return 0;
}

int C_LogManage::CreateLogStruct(std::string &strPath)
{
	PLOG_STRUCT pLog = NULL;
	string strTmpPath = strPath;

	
	pLog = new LOG_STRUCT;
	pLog->iModule = 16;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "CtrlCenter";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "CtrlCenter");
	m_listLog.push_back(pLog);
	
	pLog = new LOG_STRUCT;
	pLog->iModule = 17;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "ThreadManage";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "ThreadManage");
	m_listLog.push_back(pLog);
	
	pLog = new LOG_STRUCT;
	pLog->iModule = 18;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "TimeTask";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "TimeTask");
	m_listLog.push_back(pLog);	
	
	pLog = new LOG_STRUCT;
	pLog->iModule = 19;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "ContentManage";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "ContentManage");
	m_listLog.push_back(pLog);		
	
	pLog = new LOG_STRUCT;
	pLog->iModule = 20;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "DataBase";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "DataBase");
	m_listLog.push_back(pLog);		
	
	pLog = new LOG_STRUCT;
	pLog->iModule = 21;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "Webservice";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "Webservice");
	m_listLog.push_back(pLog);		
	
	pLog = new LOG_STRUCT;
	pLog->iModule = 22;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "DeviceCtrl";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "DeviceCtrl");
	m_listLog.push_back(pLog);	
	
	pLog = new LOG_STRUCT;
	pLog->iModule = 22;
	pLog->iSubModule = 1;
	pLog->strLogName = strPath + "Projector";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "Audio");
	m_listLog.push_back(pLog);	
	
	pLog = new LOG_STRUCT;
	pLog->iModule = 22;
	pLog->iSubModule = 2;
	pLog->strLogName = strPath + "Audio";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "Audio");
	m_listLog.push_back(pLog);	
	
	pLog = new LOG_STRUCT;
	pLog->iModule = 22;
	pLog->iSubModule = 3;
	pLog->strLogName = strPath + "DCS";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "DCS");
	m_listLog.push_back(pLog);	
	
	pLog = new LOG_STRUCT;
	pLog->iModule = 22;
	pLog->iSubModule = 4;
	pLog->strLogName = strPath + "/Power";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "Power");
	m_listLog.push_back(pLog);
	
	pLog = new LOG_STRUCT;
	pLog->iModule = 23;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "Other";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "Other");
	m_listLog.push_back(pLog);
	
	pLog = new LOG_STRUCT;
	pLog->iModule = 25;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "Schedule";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "Schedule");
	m_listLog.push_back(pLog);
	
	pLog = new LOG_STRUCT;
	pLog->iModule = 26;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "CUE";
	pLog->pLogManage = new LogManage(DEFAULT_LOG_LEVEL, 1024*10240, strTmpPath.c_str(), "CUE");
	m_listLog.push_back(pLog);
	return 0;		
}

int C_LogManage::ModifyLogStruct(std::string &strPath)
{
	vector<string> vecstr;
	string str;
	str = strPath + "CtrlCenter";
	vecstr.push_back(str);
	str = strPath + "ThreadManage";
	vecstr.push_back(str);
	str = strPath + "TimeTask";
	vecstr.push_back(str);
	str = strPath + "ContentManage";
	vecstr.push_back(str);
	str = strPath + "DataBase";
	vecstr.push_back(str);
	str = strPath + "Webservice";	
	vecstr.push_back(str);
	str = strPath + "DeviceCtrl";
	vecstr.push_back(str);
	str = strPath + "Projector";
	vecstr.push_back(str);
	str = strPath + "Audio";
	vecstr.push_back(str);
	str = strPath + "DCS";	
	vecstr.push_back(str);
	str = strPath + "Power";	
	vecstr.push_back(str);
	str = strPath + "Other";	
	vecstr.push_back(str);
	str = strPath + "Schedule";	
	vecstr.push_back(str);
	str = strPath + "CUE";	
	vecstr.push_back(str);
	list<PLOG_STRUCT>::iterator it = m_listLog.begin();				
	for(int i=0; i<14; ++i)
	{

		(*it)->pLogManage->SetLogPath((vecstr[i]).c_str());
	}	
	vecstr.clear();
	return 0;
}
int C_LogManage::ReInitLog()
{
	//设置标志位，阻止其他线程进行日志操作。
	m_bSettingDate = 1;
	//如果其他线程正在写日志，等待其结束。
	usleep(200000);
		int iResult = -1;

	if((iResult = CheckLogPath(m_strBootPath)) != 0)
	{
		return iResult;
	}
	SetLogDate();
	if((iResult = DeletePreLog(m_strLogPath)) != 0)
	{
		return  iResult;
	}
	if((iResult = ModifyLogStruct(m_strLogPath)) != 0)
	{
		return iResult;
	}
	m_bSettingDate = 0;
	usleep(100000);
	list<PTMP_LOG>::iterator it = m_listTmpLog.begin();
	for(;it != m_listTmpLog.end(); ++it)
	{
		WriteLog((*it)->iLevel, (*it)->iModule, (*it)->iSubModule,
		        (*it)->errorCode,(*it)->strError);
		delete (*it);
	}
	m_listTmpLog.clear();
	return 0;
}
