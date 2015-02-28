#ifndef _H_HASHCHECK_ 
#define _H_HASHCHECK_ 
#include <pthread.h>
#include <list>
#include <map>
#include "contentcheck/content.h"

struct stHashTaskInfo
{
	std::string strPath;
	std::string strUUID;
	int nResult ;
	std::string strErrInfo;
};

class CHashCheck
{
public:
	CHashCheck();
	~CHashCheck();

	bool AddHaskTask(stHashTaskInfo task);
	void ProcessHashTask();
	bool GetDcpHashCheckResult(std::string &strPKIUUID,int &nPercent,
		int &nResult,std::string &strErrInfo);
private:
	pthread_cond_t m_cond;
	pthread_mutex_t m_mutx;

	std::list<stHashTaskInfo> m_lstHaskTask;

// 	pthread_mutex_t m_mutxMap;
// 	std::map<std:string,stHashTaskInfo> m_mapTaskInfo;

};


#endif

