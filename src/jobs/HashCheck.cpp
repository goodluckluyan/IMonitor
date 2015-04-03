#include "HashCheck.h"
#include "log/C_LogManage.h"
#include "C_constDef.h"
#define  LOGINFFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_INFO,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)

CHashCheck::CHashCheck()
{
	pthread_mutex_init(&m_mutx,NULL);
	pthread_mutex_init(&m_mutxDone,NULL);
	//pthread_mutex_init(&m_mutxMap,NULL);
	pthread_cond_init(&m_cond,NULL);
}
CHashCheck::~CHashCheck()
{
	pthread_mutex_lock(&m_mutx);
	pthread_cond_signal(&m_cond);
	pthread_mutex_unlock(&m_mutx);

	pthread_mutex_destroy(&m_mutx);
	pthread_mutex_destroy(&m_mutxDone);
	pthread_cond_destroy(&m_cond);
}

bool CHashCheck::AddHaskTask(stHashTaskInfo task)
{
	pthread_mutex_lock(&m_mutx);
	m_lstHaskTask.push_back(task);
    pthread_cond_signal(&m_cond);
	pthread_mutex_unlock(&m_mutx);
}

void CHashCheck::ProcessHashTask()
{

	pthread_mutex_lock(&m_mutx);
	if(m_lstHaskTask.size() == 0)
	{
		pthread_cond_wait(&m_cond,&m_mutx);
	}

	pthread_mutex_unlock(&m_mutx);

	if(m_lstHaskTask.size()!=0)
	{
		stHashTaskInfo &task = m_lstHaskTask.front();
		std::string strErr;
		LOGINFFMT(0,"Start HashDcp(%s)....",task.strUUID.c_str());
		task.nResult = 0;
		int nResult = HashDcp( task.strPath, task.strUUID , task.strErrInfo);
		if(nResult == 0)
		{
			task.nResult = 1;
		}
		else
		{
			task.nResult = nResult;
		}
		pthread_mutex_lock(&m_mutxDone);
		m_lstDoneTask.push_back(task);
		pthread_mutex_unlock(&m_mutxDone);

		pthread_mutex_lock(&m_mutx);
		m_lstHaskTask.pop_front();
		pthread_mutex_unlock(&m_mutx);
	}
	

	//pthread_mutex_lock(m_mutxMap);
	//m_mapTaskInfo[task.strUUID] = task;
	//pthread_mutex_unlock(m_mutxMap);
}


// ªÒ»°
bool CHashCheck::GetDcpHashCheckResult(std::string &strPKIUUID,int &nResult,int &nPercent,std::string &strErrInfo)
{
	bool bRet = false;
// 	pthread_mutex_lock(m_mutxMap);
// 	std::map<std::string ,stHashTaskInfo>::iterator it = m_mapTaskInfo.find(strPKIUUID);
// 	if(it != m_mapTaskInfo.end())
// 	{
// 		nResult = it->second.nResult;
// 		strErrInfo = it->second.strErrInfo;
// 		m_mapTaskInfo.erase(it);
// 		bRet = true;
// 	}
// 	
// 	pthread_mutex_unlock(m_mutxMap);

	bool bFind=false;
	pthread_mutex_lock(&m_mutxDone);
	std::list<stHashTaskInfo>::iterator it=m_lstDoneTask.begin();
	for(;it!=m_lstDoneTask.end();it++)
	{
		if(it->strUUID==strPKIUUID)
		{
			nResult = it->nResult;
			if(nResult == 1)
			{
				nPercent = 100;
			}
			bFind = true;
			m_lstDoneTask.erase(it);
			break;
		}
	}
	pthread_mutex_unlock(&m_mutxDone);

	if(!bFind)
	{
		pthread_mutex_lock(&m_mutx);
		std::list<stHashTaskInfo>::iterator it=m_lstHaskTask.begin();
		for(;it!=m_lstHaskTask.end();it++)
		{
			if(it->strUUID==strPKIUUID)
			{
				nResult = it->nResult;
				break;
			}
		}
		pthread_mutex_unlock(&m_mutx);
	}

// 	if(!bFind)
// 	{
// 		Content::Hashinfo hash;
// 		std::string path;
// 		GetHashPercent(path, strPKIUUID, hash ,strErrInfo );
// 		nResult = hash.status;
// 		nPercent = hash.percent;
// 	}
	return bRet;	
}


