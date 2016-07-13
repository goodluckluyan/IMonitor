#include <sys/stat.h>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <vector>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "C_LogManage.h"
#include "para/C_RunPara.h"
#include "para/C_Para.h"
#include "C_AdditionalLog.h"
#include "C_constDef.h"
#include "C_ErrorDef.h"

using namespace std;
C_LogManage* C_LogManage::m_pInstance = NULL;

C_LogManage::C_LogManage()
{
	m_bSettingDate = 0;
}

C_LogManage::~C_LogManage()
{
	list<PTMP_LOG>::iterator it = m_listTmpLog.begin();
	for(;it != m_listTmpLog.end(); ++it)
	{
		WriteLog((*it)->iLevel, (*it)->iModule, (*it)->iSubModule,
			(*it)->errorCode,(*it)->strError);
		delete (*it);
	}
	m_listTmpLog.clear();

	list<PLOG_STRUCT>::iterator logit = m_listLog.begin();
	for(;logit != m_listLog.end();++logit)
	{
		LogManage * ptrLM = (*logit)->pLogManage;
		if(ptrLM)
		{
			delete ptrLM;
		}

		PLOG_STRUCT  ptrPS = (*logit);
		if(ptrPS)
		{
			delete ptrPS;
		}
	}
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

char* C_LogManage::ft_Printf(const char *fmt, ...)
{
	va_list ptr;
	va_start(ptr,fmt);
	int nSize = ::vsnprintf(NULL,0,fmt,ptr);
	va_end(ptr);

	char * buff =new char[nSize+1];
	va_start(ptr,fmt);
	::vsnprintf(buff,nSize+1,fmt,ptr);
	va_end(ptr);
	return buff;
}


int C_LogManage::WriteLogFmt(int iLevel, int iModule, int iSubModule,c_u_int errorCode, const char *fmt, ...)
{
	va_list ptr;
	va_start(ptr,fmt);
	int nSize = ::vsnprintf(NULL,0,fmt,ptr);
	va_end(ptr);

	char * buff =new char[nSize+1];
	va_start(ptr,fmt);
	::vsnprintf(buff,nSize+1,fmt,ptr);
	va_end(ptr);
	WriteLog(iLevel,iModule,iSubModule,errorCode,buff);
	delete[] buff;
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
// 		int  iTmpNumber = CreateLogNumber(iLevel, iModule, iSubModule, errorCode);
// 		sprintf(a,"0x%08x ",iTmpNumber);
// 		strError = a + strError;
		list<PLOG_STRUCT>::iterator it = m_listLog.begin();
		for(; it != m_listLog.end(); ++it)
		{
			if((*it)->iModule == iModule && (*it)->iSubModule == iSubModule)
			{
				if((*it)->pLogManage == NULL)
				{
					C_AdditionalLog::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_LOGMGR,0,ERROR_MEMORY_NOT_INIT,
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
					C_AdditionalLog::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_LOGMGR,0,ERROR_MEMORY_NOT_INIT,
						     "内存访问错误 pLogManage 没有被初试化！");
					return ERROR_MEMORY_NOT_INIT;
			}
			(*it)->pLogManage->SetLogLevel(iLevel);
			return 0;
		}
	}
	C_AdditionalLog::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_LOGMGR,0,ERROR_NOT_FIND_LOG_TYPE,
						     "与模块编号和子模块编号对应的日志类没有找到！");
					return ERROR_NOT_FIND_LOG_TYPE;
}

int C_LogManage::SetLogLevel(int iLevel)
{
	list<PLOG_STRUCT>::iterator it = m_listLog.begin();
	for(; it != m_listLog.end(); ++it)
	{

		if((*it)->pLogManage == NULL)
		{
			C_AdditionalLog::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_LOGMGR,0,ERROR_MEMORY_NOT_INIT,
				"内存访问错误 pLogManage 没有被初试化！");
			return ERROR_MEMORY_NOT_INIT;
		}
		(*it)->pLogManage->SetLogLevel(iLevel);

	}
	return 0;
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
		//printf(strTmp);
		C_AdditionalLog::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_LOGMGR,0,ERROR_GET_LOG_BOOT_PATH, strTmp);
		return ERROR_GET_LOG_BOOT_PATH;
	}
	if(S_ISDIR(StatBuf.st_mode) == 0)
	{
		sprintf(strTmp,"CheckLogPath() Error! %s is not path!\n", strBootPath.c_str());
		//printf(strTmp);
		C_AdditionalLog::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_LOGMGR,0,ERROR_GET_LOG_BOOT_PATH, strTmp);
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
				//printf(strTmp);
				C_AdditionalLog::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_LOGMGR,0,ERROR_CREATE_LOG_PATH, strTmp);
				return ERROR_CREATE_LOG_PATH;
			}
		}
		else if(iResult != 0 && errno != ENOENT)
		{
				err = errno;
				sprintf(strTmp,"CheckLogPath() Error! stat(%s) error! errno:%d,errorInfo:%s \n",
				       strSubPath.c_str(),err,strerror(err));
				//printf(strTmp);
				C_AdditionalLog::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_LOGMGR,0,ERROR_GET_LOG_PATH, strTmp);
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
				//printf(strTmp);
				C_AdditionalLog::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_LOGMGR,0,ERROR_DELETE_PRE_LOG, strTmp);
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
				//printf(strTmp);
				C_AdditionalLog::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_LOGMGR,0,ERROR_DELETE_PRE_LOG, strTmp);
				return ERROR_DELETE_PRE_LOG;
		}
		if(S_ISREG(StatBuf.st_mode)!= 0 &&  StatBuf.st_mtime < pRunPara->GetZeroTime())
		{
				if(unlink(strName.c_str())!= 0)
				{
						err = errno;
						//printf("DeletePreLog() Error! unlink(%s) error! errno:%d erorInfo:%s\n",
						//strName.c_str(),err,strerror(err));
				//printf(strTmp);
				C_AdditionalLog::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_LOGMGR,0,ERROR_DELETE_PRE_LOG, strTmp);
				return ERROR_DELETE_PRE_LOG;
				}
		}
	}
	closedir(dp);
	return 0;
}

#define LOGMAXSIZE 5000000
int C_LogManage::CreateLogStruct(std::string &strPath)
{
	int nWriteLogLevel = C_Para::GetInstance()->m_nWirteLogLevel;
	PLOG_STRUCT pLog = NULL;
	string strTmpPath = strPath;
	
	pLog = new LOG_STRUCT;
	pLog->iModule = LOG_MODEL_THREADMGR;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "ThreadManage";
	pLog->pLogManage = new LogManage(nWriteLogLevel, LOGMAXSIZE, strTmpPath.c_str(), "ThreadManage");
	m_listLog.push_back(pLog);
	
	pLog = new LOG_STRUCT;
	pLog->iModule = LOG_MODEL_TIMETASK;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "TimeTask";
	pLog->pLogManage = new LogManage(nWriteLogLevel, LOGMAXSIZE, strTmpPath.c_str(), "TimeTask");
	m_listLog.push_back(pLog);		
	
	pLog = new LOG_STRUCT;
	pLog->iModule = LOG_MODEL_DB;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "DataBase";
	pLog->pLogManage = new LogManage(nWriteLogLevel,LOGMAXSIZE, strTmpPath.c_str(), "DataBase");
	m_listLog.push_back(pLog);		
	
	pLog = new LOG_STRUCT;
	pLog->iModule = LOG_MODEL_WEBS;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "Webservice";
	pLog->pLogManage = new LogManage(nWriteLogLevel, LOGMAXSIZE, strTmpPath.c_str(), "Webservice");
	m_listLog.push_back(pLog);

	pLog = new LOG_STRUCT;
	pLog->iModule = LOG_MODEL_JOBS;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "jobs";
	pLog->pLogManage = new LogManage(nWriteLogLevel, LOGMAXSIZE, strTmpPath.c_str(), "jobs");
	m_listLog.push_back(pLog);
	
	pLog = new LOG_STRUCT;
	pLog->iModule = LOG_MODEL_OTHER;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "Other";
	pLog->pLogManage = new LogManage(nWriteLogLevel, LOGMAXSIZE, strTmpPath.c_str(), "Other");
	m_listLog.push_back(pLog);
	
	pLog = new LOG_STRUCT;
	pLog->iModule = LOG_MODEL_LOGMGR;
	pLog->iSubModule = 0;
	pLog->strLogName = strPath + "LogMgr";
	pLog->pLogManage = new LogManage(nWriteLogLevel, LOGMAXSIZE, strTmpPath.c_str(), "LogMgr");
	m_listLog.push_back(pLog);
	return 0;		
}

int C_LogManage::ModifyLogStruct(std::string &strPath)
{
	list<PLOG_STRUCT>::iterator it = m_listLog.begin();				
	for(; it!=m_listLog.end(); it++)
	{
		(*it)->pLogManage->SetLogPath(strPath.c_str());
	}	
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
