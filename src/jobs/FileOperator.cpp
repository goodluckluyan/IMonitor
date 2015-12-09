
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "FileOperator.h"
#include "log/C_LogManage.h"
#include "C_constDef.h"
#define  LOGINFFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_INFO,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)


CFileOperator::CFileOperator()
{
	pthread_mutex_init(&m_mutx,NULL);
	//pthread_mutex_init(&m_mutxMap,NULL);
	pthread_cond_init(&m_cond,NULL);
	pthread_mutex_init(&m_lstDoneMutex,NULL);
}
CFileOperator::~CFileOperator()
{
	pthread_mutex_lock(&m_mutx);
	pthread_cond_signal(&m_cond);
	pthread_mutex_unlock(&m_mutx);

	pthread_mutex_destroy(&m_mutx);
	pthread_cond_destroy(&m_cond);

	pthread_mutex_destroy(&m_lstDoneMutex);
}

bool CFileOperator::AddFileOptTask(stFileOperatorInfo task)
{
	pthread_mutex_lock(&m_mutx);
	m_lstFileOptTask.push_back(task);
	pthread_cond_signal(&m_cond);
	pthread_mutex_unlock(&m_mutx);


}



void CFileOperator::ProcessFileOptTask()
{
	pthread_mutex_lock(&m_mutx);
	if(m_lstFileOptTask.size() == 0)
	{
		pthread_cond_wait(&m_cond,&m_mutx);
	}

	pthread_mutex_unlock(&m_mutx);

	if(m_lstFileOptTask.size() == 0)
	{
		return;
	}
	stFileOperatorInfo &task = m_lstFileOptTask.front();
	char strCmd[1024]={'\0'};
	char strCmdAfter[1024]={'\0'};

	trimall(task.strSrcPath);
	if(task.enOpt == CP)
	{
		trimall(task.strDesPath);
		if(task.strSrcPath.find_last_of('/')!=task.strSrcPath.size()-1)
		{
			task.strSrcPath+="/";
		}
		if(task.strDesPath.find_last_of('/')!=task.strDesPath.size()-1)
		{
			task.strDesPath+="/";
 		}
// 		sprintf(strCmd,"cp -rf %s%s %s%s",task.strSrcPath.c_str(),task.strUUID.c_str(),
// 			task.strDesPath.c_str(),task.strUUID.c_str());
		sprintf(strCmd,"cp -rf %s%s %s  ",task.strSrcPath.c_str(),task.strUUID.c_str(),
			task.strDesPath.c_str());

		sprintf(strCmdAfter,"chown -R admin %s%s & chgrp -R admin %s%s & chmod -R 755 %s%s",task.strDesPath.c_str(),task.strUUID.c_str(),
			task.strDesPath.c_str(),task.strUUID.c_str(),task.strDesPath.c_str(),task.strUUID.c_str());

	}
	else
	{
		sprintf(strCmd,"rm -rf %s%s",task.strSrcPath.c_str(),task.strUUID.c_str()); 
	}
	LOGINFFMT(0,"%s",strCmd);
	LOGINFFMT(0,"%s",strCmdAfter);
	
	task.status = EXEING;
	struct sigaction sa;
	struct sigaction oldsa;
	sa.sa_handler=SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if(sigaction(SIGCHLD,&sa,&oldsa)<0)
	{
		return ;
	}

	pid_t pid;
	if((pid=fork()) == 0)
	{
		int ret = system(strCmd);
		if(0==ret && strlen(strCmdAfter)>0)
		{
			int retafter = system(strCmdAfter);
			LOGINFFMT(0,"%s <Result:%d>",strCmdAfter,retafter);
		}
		exit(ret);
	}

	int nStatus;
	waitpid(pid,&nStatus,NULL);
	if(WIFEXITED(nStatus))
	{
		task.nResult = WEXITSTATUS(nStatus);
	}
	else
	{
		task.nResult = -1;
	}
	task.status = DONE;
	LOGINFFMT(0,"%s <Result:%d>",strCmd,task.nResult);
	
	pthread_mutex_lock(&m_lstDoneMutex);
	m_lstDoneTask.push_back(task);
	pthread_mutex_unlock(&m_lstDoneMutex);

	m_lstFileOptTask.pop_front();
	sigaction(SIGCHLD,&oldsa,NULL);


}


// 获取
bool CFileOperator::GetCopyDcpProgress(std::string &strPKIUUID,int &nResult,std::string &strErrInfo)
{
	bool bFind = false;
	pthread_mutex_lock(&m_lstDoneMutex);
	std::list<stFileOperatorInfo>::iterator it = m_lstDoneTask.begin();
	for(;it != m_lstDoneTask.end();it++)
	{
		if(it->strUUID == strPKIUUID &&it->enOpt == CP)
		{
			if(it->status==DONE)
			{
				nResult = 1;
				m_lstDoneTask.erase(it);
				bFind = true;

				break;
			}
		}
	}
	
	pthread_mutex_unlock(&m_lstDoneMutex);

	if(!bFind)
	{
		bFind = false;
		pthread_mutex_lock(&m_mutx);
		std::list<stFileOperatorInfo> tmplst = m_lstFileOptTask;
		pthread_mutex_unlock(&m_mutx);

		std::list<stFileOperatorInfo>::iterator it = tmplst.begin();
		for(;it != tmplst.end();it++)
		{
			if(it->strUUID == strPKIUUID && it->enOpt == CP)
			{
				bFind = true;
				nResult = 0;
			}
		}

		if(!bFind)
		{
			nResult = -1;
			strErrInfo = "not find this file";
		}
		
	}


}

// 获取
bool CFileOperator::GetDeleteDcpProgress(std::string &strPKIUUID,int &nResult,std::string &strErrInfo)
{
	bool bFind = false;
	pthread_mutex_lock(&m_lstDoneMutex);
	std::list<stFileOperatorInfo >::iterator it = m_lstDoneTask.begin();
	for(;it != m_lstDoneTask.end();it++)
	{
		if(it->strUUID == strPKIUUID && it->enOpt == RM)
		{
			if(it->status==DONE)
			{
				nResult = 1;
				bFind = true;
				m_lstDoneTask.erase(it);
				break;
			}
		}
	}

	pthread_mutex_unlock(&m_lstDoneMutex);

	if(!bFind)
	{
		bFind = false;
		pthread_mutex_lock(&m_mutx);
		std::list<stFileOperatorInfo> tmplst = m_lstFileOptTask;
		pthread_mutex_unlock(&m_mutx);

		std::list<stFileOperatorInfo>::iterator it = tmplst.begin();
		for(;it != tmplst.end();it++)
		{
			if(it->strUUID == strPKIUUID && it->enOpt == RM)
			{
				bFind = true;
				nResult = 0;
			}
		}

		if(!bFind)
		{
			nResult = -1;
			strErrInfo = "not find this file";
		}

	}
}


