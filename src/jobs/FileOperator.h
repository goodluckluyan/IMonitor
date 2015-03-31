#ifndef _H_FILEOPERATOR_ 
#define _H_FILEOPERATOR_ 
#include <pthread.h>
#include <list>
#include <map>
#include <string>
#include <sys/wait.h>

enum enOptType{CP=1,RM=2};
enum enExeStatus{WAIT=1,EXEING=2,DONE=3};
struct stFileOperatorInfo
{
	enOptType enOpt;
	std::string strSrcPath;
	std::string strDesPath;
	std::string strUUID;
	int nResult ;
	std::string strErrInfo;
	enExeStatus status;

	stFileOperatorInfo()
	{
		nResult = -1;
		strSrcPath="";
		strDesPath="";
		strUUID="";
		strErrInfo="";
		status = WAIT;
	}

	stFileOperatorInfo(const stFileOperatorInfo & obj)
	{
		enOpt = obj.enOpt;
		strSrcPath = obj.strSrcPath;
		strDesPath = obj.strDesPath;
		strUUID = obj.strUUID;
		nResult = obj.nResult;
		strErrInfo = obj.strErrInfo;
		status = obj.status;
	}

	stFileOperatorInfo & operator = (const stFileOperatorInfo & obj)
	{
		if(this != &obj)
		{
			enOpt = obj.enOpt;
			strSrcPath = obj.strSrcPath;
			strDesPath = obj.strDesPath;
			strUUID = obj.strUUID;
			nResult = obj.nResult;
			strErrInfo = obj.strErrInfo;
			status = obj.status;
		}
		return *this;
		
	}

};

class CFileOperator
{
public:
	CFileOperator();
	~CFileOperator();

	bool AddFileOptTask(stFileOperatorInfo task);
	void ProcessFileOptTask();
	bool GetCopyDcpProgress(std::string &strPKIUUID,int &nResult,std::string &strErrInfo);
	bool GetDeleteDcpProgress(std::string &strPKIUUID,int &nResult,std::string &strErrInfo);

	void trimall(std::string &str)
	{
		int nStart = str.find_first_not_of(' ');
		int nEnd = str.find_last_not_of(' ');

		str = str.substr(nStart,nEnd-nStart+1);
	}

private:
	pthread_cond_t m_cond;
	pthread_mutex_t m_mutx;

	std::list<stFileOperatorInfo> m_lstFileOptTask;

	pthread_mutex_t m_lstDoneMutex;
	std::list<stFileOperatorInfo > m_lstDoneTask;

};


#endif

