/**
 * @file LogManage.cpp
 * @brief Implementation of LogManage class defined in LogManage.h
 *
 *
 * @author xiehua@ceopen.cn
 * @version 1.0
 * @date 2008-11-20
 *
 */
#include <sys/stat.h>
#include <syslog.h>
#include "LogManage.h"
#include <string>
#include "para/C_RunPara.h"

extern int g_LogLevel ;
extern int g_nRunType;
using namespace std;

char* LogManage::logLevelStr[] = 
{
	"DEBUG",
	"INFO",
	"ERROR",
	"FATAL"
};

LogManage::LogManage(int inLevel, unsigned long inMaxLogFileSize, const char *inLogDir, const char *inBaseLogName)
:m_pFile(NULL),
m_logLevel(inLevel),
m_maxLogFileSize(inMaxLogFileSize)
{
	memset(m_logCreateTime, 0, sizeof(m_logCreateTime));

	memset(m_logFullPath, 0, sizeof(m_logFullPath));

	memset(m_logDir, 0, sizeof(m_logDir));

	//get the data outside LogManage
	memcpy(m_logDir, inLogDir, MAX_DIR_LENGTH);

	memcpy(m_baseLogName, inBaseLogName, MAX_BASE_LOGNAME_LEN);
}

LogManage::~LogManage()
{
	//close the log file
	CloseLog();
}

void LogManage::WriteLog(int logLevel, const char* inLogContent)
{

	char*		theTime;
	//time_t		timeInSec;
	char*		totalMsg;		
	char*		msgData;
	int			msgLen;
	theTime		=	NULL;
	msgData		=	NULL;
	totalMsg	=	NULL;

	//check the log level, if it is lower then the specified level, do not write the log
// 	if(logLevel < m_logLevel)
// 	{
// 		return;
// 	}

	//check if the log file is open. Open the log file if it has already been closed
	if (m_pFile == NULL)
	{
		EnableLog(); //re-open log file before we write
	}

	//check the opened file, if the size of the log is bigger than specified size, roll
	//rename it, close the log file, and create a new log file, open it.
	if(-1 == CheckRollLog())
	{
		return;
	}

	//get current time as string
	theTime = new char[MAX_TIMESTRING_LENGTH];

	if(!theTime)
	{
		return;
	}
  /*delete  by wangzhongping at 2102-7-29
	timeInSec = time(NULL);

	strftime(theTime, MAX_TIMESTRING_LENGTH, "%Y-%m-%d %H:%M:%S", localtime(&timeInSec));
	delete end*/
	
	//Add by wangzhongping at 2102-7-29
	C_RunPara *pRunPara = C_RunPara::GetInstance();
	string strTime;
	pRunPara->GetCurTime(strTime);
	strcpy(theTime,strTime.c_str());
	//Add end .

	//generate the main message string
	msgLen = strlen(inLogContent) + 1;

	msgData = new char[msgLen];
	memset(msgData, 0, msgLen);

	strncpy(msgData, inLogContent, msgLen);
	

	if(logLevel >= m_logLevel && logLevel >= 0 && logLevel <= 3)
	{
		ft_fPrintf("%s %s: %s\n", logLevelStr[logLevel], theTime, msgData);
		::fflush(m_pFile);
	}
	
	//运行在交互模式下则输出
	if(g_nRunType == 0 && logLevel >= g_LogLevel && logLevel >= 0 && logLevel <= 3)
	{
		char * ptrPntMsg = NULL;
		ptrPntMsg = ft_Printf("%s%s:%s\n", logLevelStr[logLevel], theTime, msgData);
		printf("%s",ptrPntMsg);
		if(ptrPntMsg)
		{
			delete[] ptrPntMsg;
			ptrPntMsg = NULL;
		}
	}
	
	if(msgData)
	{
		delete []msgData;

		msgData = NULL;
	}
	
	if(theTime)
	{
		delete []theTime;

		theTime = NULL;
	}
}

int LogManage::CheckRollLog()
{
    //returns false if an error occurred, true otherwise
    if ( NULL == m_pFile )
        return LOG_OK;

    //now check size based log rolling
    unsigned long theCurrentPos = ::ftell(m_pFile);
// 	struct stat filestate;
//  fstat(fileno(m_pFile),&filestate);
// 	unsigned long theCurrentPos = filestate.st_size;

    //max_transfer_log_size being 0 is a signal to ignore the setting.
    if ((m_maxLogFileSize != 0) && (theCurrentPos > ((unsigned long)m_maxLogFileSize)))
        return this->RollLog();

    return LOG_OK;
}

int LogManage::RollLog()
{
	//close the old file.
    if (m_pFile)
	{
        this->CloseLog();
	}
 
    //rename the old file
    int result = this->RenameLogFile(m_logFullPath);
    if (LOG_OK == result)
	{
		this->CloseLog();
        this->EnableLog();//re-opens log file
	}

    return result;
}

int LogManage::RenameLogFile(char* inFileName)
{
	int	theErr;
	char	newLogFullPath[MAX_LOGFILE_FULLPATH_LENGTH];
	char	tempLogFullPath[MAX_LOGFILE_FULLPATH_LENGTH];	
	theErr	=	0;

	memset(newLogFullPath, 0, sizeof(newLogFullPath));
	memset(tempLogFullPath, 0, sizeof(tempLogFullPath));
	sprintf(tempLogFullPath, "%s%s%s", m_logDir, m_baseLogName, m_logCreateTime);

	//loop until we find a unique name to rename this file
    //and append the log number and suffix	
	for(int x = 0; x<=1000; x++)
	{
		if (x  == 1000) //we don't have any digits left, so just reuse the "---" until tomorrow...
		{
			//add a bogus log number and exit the loop
			sprintf(tempLogFullPath, "---.log");

			break;
		}

		//add the log number & suffix
		sprintf(newLogFullPath, "%s-%03d.log", tempLogFullPath, x);

		//assume that when ::access returns -1, it is because
		//the file doesn't exist. Once that happens, we have a unique name
		theErr = ::access(newLogFullPath, 0);
		if(theErr == 0)
		{
			struct stat filestate;
			stat(newLogFullPath,&filestate);
			syslog(LOG_INFO,"%s : %d",newLogFullPath,filestate.st_size);
			if(filestate.st_size < m_maxLogFileSize)
			{
				break;
			}
		}
		else
		{
			break;
		}
    }
	
		
	//rename the file. Use posix rename function
	//int result = ::rename(inFileName, newLogFullPath);
	strcpy(inFileName,newLogFullPath);

// 	if (result == -1)
// 	{
// 		return LOG_FAIL;
// 	}
// 	else
//	{
		return LOG_OK;
//	}
}

int LogManage::EnableLog()
{
    char*	extension;
	extension =".log";

	//make the full path string
	sprintf(m_logFullPath, "%s%s%s", m_logDir, m_baseLogName, extension);
 
	//create a new file if it does not exist
	//open a log file if it exists and set the pointer to the end of the file
	m_pFile = ::fopen(m_logFullPath, "a+");//open for "append"
	
	if (NULL != m_pFile)
	{
		/*delete  by wangzhongping at 2102-7-29
		time_t timeInSec = time(NULL);

		strftime(m_logCreateTime, sizeof(m_logCreateTime), "%Y%m%d", localtime(&timeInSec));
		delete end*/
		//add by wangzhongping at 2102-7-29
		string str;
		C_RunPara::GetInstance()->GetCurDate(str);
		strcpy(m_logCreateTime,str.c_str());
	
		//add end
		return LOG_OK;
	}
	else
	{
		return LOG_FAIL;
	}
}

void LogManage::CloseLog()
{
	if (m_pFile != NULL)
	{
		::fclose(m_pFile);
		m_pFile = NULL;
	}
}

int LogManage::ft_fPrintf(const char *fmt, ...)
{
	if(!m_pFile)
	{
		return -1;
	}

	va_list args;

	va_start(args, fmt);

	int result = ::vfprintf(m_pFile, fmt, args);

	va_end(args);

	return result;
}

char* LogManage::ft_Printf(const char *fmt, ...)
{
	va_list ptr;
	va_start(ptr,fmt);
	int nSize = ::vsnprintf(NULL,0,fmt,ptr);
	va_end(ptr);

	char * buff= new char[nSize+1];
	va_start(ptr,fmt);
	::vsnprintf(buff,nSize+1,fmt,ptr);
	va_end(ptr);

	return buff;
}

int LogManage::SetLogPath(const char *strLogPath)
{
	 CloseLog();
	 memset(m_logDir, 0, MAX_DIR_LENGTH);
	 strcpy(m_logDir,strLogPath);
	 RollLog();
	 return 0;
}
