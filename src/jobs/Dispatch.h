//@file:Dispatch.h
//@brief: 调用策略实现。
//@author:luyan@oristartech.com
//date:2014-09-17
#ifndef DISPATCH_INC
#define DISPATCH_INC
#include <map>
#include <pthread.h>
#include <map>
#include <list>
#include <vector>
#include <string>
#include "threadManage/C_CS.h"
#include "C_constDef.h"
#include "parser_xml.h"

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
	time_t tmTime;
};

// 事务处理中心
class CDispatch
{
public:
	CDispatch(void * ptrInvoker );
	~CDispatch();
public:
	// 初始化
	bool Init(std::string strPath);

	// 触发调度,即添加事件
	bool TriggerDispatch(int nTaskType,std::vector<stError> &vecRE);

	// 调度线程
	bool Routine();

	// 添加sms运行冲突信息
	void AddConflictInfo(std::vector<ConflictInfo> &vecCI);

private:
	// 删除字符串两则空格
	void TrimSpace(std::string &str);

	// 应用策略
	bool ApplyPolicy(int nTaskType,struct DispatchTask &nodeTask,
		std::map<int,std::vector<std::string> >& mapAction);

	// 获取策略xml中的结点
	bool GetPolicyNode(xercesc::DOMDocument* ptrDoc,std::string strNodeName,
		std::map<std::string,PolicyInfoEle> &mapPInfo);

	// 解析策略
	bool ParsePolicy(std::string strPath);

	// 执行策略中的动作
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