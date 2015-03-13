
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "FileOperator.h"

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

	stFileOperatorInfo &task = m_lstFileOptTask.front();
	char strCmd[1024]={'\0'};
	if(task.enOpt == CP)
	{
		sprintf(strCmd,"cp %s/%s %s/%s",task.strSrcPath.c_str(),task.strUUID.c_str(),
			task.strDesPath.c_str(),task.strUUID.c_str());
	}
	else
	{
		sprintf(strCmd,"rm -rf %s",task.strSrcPath.c_str());
	}
	
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
	
	m_lstFileOptTask.pop_front();

	pthread_mutex_lock(&m_lstDoneMutex);
	m_lstDoneTask.push_back(task);
	pthread_mutex_unlock(&m_lstDoneMutex);

	
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
		if(it->strUUID == strPKIUUID && it->enOpt == CP)
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
		pthread_mutex_lock(&m_mutx);
		std::list<stFileOperatorInfo> tmplst = m_lstFileOptTask;
		pthread_mutex_unlock(&m_mutx);

		std::list<stFileOperatorInfo>::iterator it = tmplst.begin();
		for(;it != tmplst.end();it++)
		{
			if(it->strUUID == strPKIUUID && it->enOpt == CP)
			{
				nResult = 0;
			}
		}
		
	}


}

// 获取
bool CFileOperator::GetDeleteDcpProgress(std::string &strPKIUUID,int &nResult,std::string &strErrInfo)
{
	bool bFind = false;
	pthread_mutex_lock(&m_lstDoneMutex);
	std::list<stFileOperatorInfo>::iterator it = m_lstDoneTask.begin();
	for(;it != m_lstDoneTask.end();it++)
	{
		if(it->strUUID == strPKIUUID && it->enOpt == RM)
		{
			if(it->status==DONE)
			{
				nResult = 1;
				m_lstDoneTask.erase(it);
				break;
			}
		}
	}

	pthread_mutex_unlock(&m_lstDoneMutex);

	if(!bFind)
	{
		pthread_mutex_lock(&m_mutx);
		std::list<stFileOperatorInfo> tmplst = m_lstFileOptTask;
		pthread_mutex_unlock(&m_mutx);

		std::list<stFileOperatorInfo>::iterator it = tmplst.begin();
		for(;it != tmplst.end();it++)
		{
			if(it->strUUID == strPKIUUID && it->enOpt == RM)
			{
				nResult = 0;
			}
		}

	}
}


