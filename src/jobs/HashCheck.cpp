#include "HashCheck.h"


extern bool g_bQuit;
CHashCheck::CHashCheck()
{
	pthread_mutex_init(&m_mutx,NULL);
	//pthread_mutex_init(&m_mutxMap,NULL);
	pthread_cond_init(&m_cond,NULL);
}
CHashCheck::~CHashCheck()
{
	pthread_mutex_lock(&m_mutx);
	pthread_cond_signal(&m_cond);
	pthread_mutex_unlock(&m_mutx);

	pthread_mutex_destroy(&m_mutx);
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


	stHashTaskInfo &task = m_lstHaskTask.front();
	std::string strErr;
	task.nResult = HashDcp( task.strPath, task.strUUID , task.strErrInfo);
	m_lstHaskTask.pop_front();

	//pthread_mutex_lock(m_mutxMap);
	//m_mapTaskInfo[task.strUUID] = task;
	//pthread_mutex_unlock(m_mutxMap);
}


// ªÒ»°
bool CHashCheck::GetDcpHashCheckResult(std::string &strPKIUUID,int &nPercent,int &nResult,std::string &strErrInfo)
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

	Content::Hashinfo hash;
	std::string path;
	GetHashPercent(path, strPKIUUID, hash ,strErrInfo );
	nResult = hash.status;
	nPercent = hash.percent;
	return bRet;	
}


