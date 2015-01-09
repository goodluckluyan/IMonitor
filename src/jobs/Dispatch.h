//@file:Dispatch.h
//@brief: 调用策略实现。
//@author:luyan@oristartech.com
//date:2014-09-17
#ifndef DISPATCH_INC
#define DISPATCH_INC
#include <map>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMNode.hpp>	
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/util/XMLString.hpp>
#include <pthread.h>
#include <map>
#include <list>
#include <vector>
#include <string>
#include "threadManage/C_CS.h"
#include "C_constDef.h"


enum enDTriggerType{NULLTask=0,RAIDTask=1,ETHTask=2,SMSTask=3,TMSTask=4,IMonitorTask=5,ETH=6};
enum enCMDType{NULLCMD=0,LOGCmd=1,POLICYCmd =2};
struct DispatchTask
{
	int nDTriggerType ;
	std::vector<stError> vecErr;

	DispatchTask()
		:nDTriggerType(NULLTask)
	{
		
	}

	DispatchTask(const DispatchTask &obj)
	{
		nDTriggerType = obj.nDTriggerType;
		vecErr.clear();
		vecErr = obj.vecErr;
	}

	DispatchTask& operator =(const DispatchTask &obj)
	{
		if(this != &obj)
		{
			nDTriggerType = obj.nDTriggerType;
			vecErr.clear();
			vecErr = obj.vecErr;
		}
		return *this;
	}
};

struct PolicyInfoEle
{
	std::string strErrName;
	std::string strType;
	int nPriority;
	std::string strFault;
	std::vector<std::string> vecAct;

	PolicyInfoEle():
	 nPriority(-1)
	{

	}

    PolicyInfoEle(const PolicyInfoEle &obj)
	{
		strErrName = obj.strErrName;
		strType = obj.strType;
		nPriority = obj.nPriority;
		strFault = obj.strFault;
		vecAct.clear();
		vecAct.insert(vecAct.end(),obj.vecAct.begin(),obj.vecAct.end());
	}
	

	PolicyInfoEle& operator=(const PolicyInfoEle &obj)
	{
		if(this != &obj)
		{
			strErrName = obj.strErrName;
			strType = obj.strType;
			nPriority = obj.nPriority;
			strFault = obj.strFault;
			vecAct.clear();
			vecAct.insert(vecAct.end(),obj.vecAct.begin(),obj.vecAct.end());
		}
		return *this;
		
	}


	bool operator < (const PolicyInfoEle &obj) const 
	{
		return nPriority < obj.nPriority;
	}
};

struct PolicyInfo
{
	int nPolicyDevType;
	std::map<std::string,PolicyInfoEle> mapPInfo;
};

struct ConflictInfo
{
	std::string strHallID;
	int nType ;//冲突类型  1:两机都在运行同一个sms,2:两机都没有运行sms
	int nMainState;
	int nMainSMSSum;
	int nStdbyState;
	int nStdbySMSSum;
};

// 事务处理中心
class CDispatch
{
public:
	CDispatch(void * ptrInvoker );
	~CDispatch();
public:

	bool Init(std::string strPath);
	bool TriggerDispatch(int nTaskType,std::vector<stError> &vecRE);
	bool Routine();
	void AddConflictInfo(std::vector<ConflictInfo> &vecCI);

private:
	void TrimSpace(std::string &str);
	bool ApplyPolicy(int nTaskType,struct DispatchTask &nodeTask,
		std::map<int,std::vector<std::string> >& mapAction);
	bool GetPolicyNode(xercesc::DOMDocument* ptrDoc,std::string strNodeName,
		std::map<std::string,PolicyInfoEle> &mapPInfo);
	bool ParsePolicy(std::string strPath);
	void ExeCmd(std::map<int,std::vector<std::string> > &mapAction);

private:
	std::list<DispatchTask> m_lstDTask;
	C_CS m_csLDTask;
	std::map<int,PolicyInfo> m_mapPolicy;
	pthread_cond_t cond;
	void  * m_ptrInvoker;
	std::vector<ConflictInfo> m_vecConflict;
};

#endif