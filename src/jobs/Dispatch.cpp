#include "Dispatch.h"
#include <unistd.h>
#include <stdio.h>
#include <algorithm>
#include "log/C_LogManage.h"
#include "Invoke.h"

#define  LOG(errid,msg)  C_LogManage::GetInstance()->WriteLog(LOG_FATAL,LOG_MODEL_JOBS,0,errid,msg)

using namespace xercesc;
CDispatch::CDispatch(void * ptrInvoker)
:m_ptrInvoker(ptrInvoker)
{
	pthread_cond_init(&cond,NULL);
}

CDispatch::~CDispatch()
{
	m_csLDTask.EnterCS();
	pthread_cond_signal(&cond);
	m_csLDTask.LeaveCS();

	pthread_cond_destroy(&cond);
}

// 初始化
bool CDispatch::Init(std::string strPath)
{
	try
	{
		xercesc::XMLPlatformUtils::Initialize();
	}
	catch( xercesc::XMLException& e )
	{
		char* message = xercesc::XMLString::transcode( e.getMessage() );
		xercesc::XMLString::release( &message );
	}

	if(access(strPath.c_str(),R_OK) < 0)
	{
		printf("CDispatch Init Read %s Failed!",strPath.c_str());
		xercesc::XMLPlatformUtils::Terminate();
		return false;
	}

	ParsePolicy(strPath);
	

	xercesc::XMLPlatformUtils::Terminate();
	return true;
}

// 解析调度策略xml
bool CDispatch::ParsePolicy(std::string strPath)
{
	XercesDOMParser *ptrParser = new  XercesDOMParser;
	ptrParser->setValidationScheme(  XercesDOMParser::Val_Never );
	ptrParser->setDoNamespaces( true );
	ptrParser->setDoSchema( false );
	ptrParser->setLoadExternalDTD( false );

	try
	{
		ptrParser->parse(strPath.c_str());
		DOMDocument* ptrDoc = ptrParser->getDocument();	
		std::map<int,std::string> mapTmp;
		mapTmp[RAIDTask] = "Raid";
		mapTmp[ETHTask] = "Eth";
		mapTmp[SMSTask] = "SMS";
		mapTmp[TMSTask] = "TMS";
		mapTmp[IMonitorTask] = "IMonitor";
 
		std::map<int,std::string>::iterator it = mapTmp.begin();
		for(;it != mapTmp.end();it++)
		{
			struct PolicyInfo stPI;
			stPI.nPolicyDevType = it->first;
			if(GetPolicyNode(ptrDoc,it->second,stPI.mapPInfo))
			{
				m_mapPolicy[it->first] = stPI;
			}
		}
	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode(e.getMessage());
		XMLString::release( &message );
		LOG(ERROR_PARSE_MONITORSTATE_XML,message);
		delete ptrParser;
		ptrParser = NULL;
	}
	delete ptrParser;
	ptrParser = NULL;
	return true;
}

// 解析内部节点
bool CDispatch::GetPolicyNode(DOMDocument* ptrDoc,std::string strNodeName,std::map<std::string,PolicyInfoEle> &mapPInfo)
{
	DOMNodeList *ptrDevNode = ptrDoc->getElementsByTagName(XMLString::transcode (strNodeName.c_str()));
	if(ptrDevNode->getLength() == 0)
	{
		return false;
	}

	DOMNodeList *ptrDevChildNode = ptrDevNode->item(0)->getChildNodes();
	int nLen = ptrDevChildNode->getLength();	
	for(int i =0; i < nLen ;i++)
	{
		DOMNode *node = ptrDevChildNode->item(i);
		if (node->getNodeType() != DOMNode::ELEMENT_NODE) 
		{
			continue;
		}
		
		PolicyInfoEle pinfo;
		char * nodeName = XMLString::transcode(node->getNodeName());//获取该节点的名称
		std::string nodeName_str = nodeName;
		pinfo.strErrName = nodeName_str;

		//如果该节点下有属性，则遍历其属性
		if (node->hasAttributes()) 
		{
			DOMNamedNodeMap *pAttributes = node->getAttributes();
			const XMLSize_t nSize = pAttributes->getLength();

			//遍历该节点下的所有属性
			for (XMLSize_t i = 0; i < nSize; ++i) 
			{
				DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(i);
				char *name = XMLString::transcode(pAttributeNode->getName());//获取属性名
				std::string name_str = name;
				if (name_str == "type") 
				{
					char *attrValue = XMLString::transcode(pAttributeNode->getValue());//获取属性值
					pinfo.strType = attrValue;
					XMLString::release(&attrValue);//释放资源
				}
				if (name_str == "priority") 
				{
					char *attrValue = XMLString::transcode(pAttributeNode->getValue());//获取属性值
					pinfo.nPriority = atoi(attrValue);
					XMLString::release(&attrValue);//释放资源
				}
				XMLString::release(&name);
			}
			XMLString::release(&nodeName);
		}

		if (node->hasChildNodes()) 
		{
			DOMNode * child;
			for (child = node->getFirstChild(); child != 0; child = child->getNextSibling()) 
			{
				if (child->getNodeType() != DOMNode::ELEMENT_NODE) 
				{
					continue;
				}

				char * nodeName = XMLString::transcode(child->getNodeName());//获取该节点的名称
				std::string nodeName_str = nodeName;
				if(!child->hasAttributes())
				{
					continue;
				}
				DOMNamedNodeMap *pAttributes = child->getAttributes();
				DOMAttr *pAttributeNode = (DOMAttr*) pAttributes->item(0);
				char *name = XMLString::transcode(pAttributeNode->getName());//获取属性名
				std::string name_str = name;
				if (name_str == "value") 
				{
					char *attrValue = XMLString::transcode(pAttributeNode->getValue());//获取属性值
					if(nodeName_str == "fault")
					{
						pinfo.strFault = attrValue;
					}
					else if(nodeName_str == "action")
					{
						pinfo.strAct = attrValue;
					}
					XMLString::release(&attrValue);//释放资源
				}

				XMLString::release(&nodeName);
			}
		}
		mapPInfo[pinfo.strErrName] = pinfo;
	}
	return true;
}

// 添加raid触发调度任务
bool CDispatch::TriggerDispatch(int nTaskType,std::vector<stError> &vecErr)
{
	struct DispatchTask nodeTask;
	nodeTask.nDTriggerType = (enDTriggerType)nTaskType;
	nodeTask.vecErr = vecErr;
	m_csLDTask.EnterCS();
	m_lstDTask.push_back(nodeTask);
	pthread_cond_signal(&cond);
	m_csLDTask.LeaveCS();

	return true;
}



// 执行调度任务
bool CDispatch::Routine()
{
	struct DispatchTask nodeTask;
	m_csLDTask.EnterCS();
	if(m_lstDTask.size() == 0)
	{
		pthread_cond_wait(&cond,&(m_csLDTask.m_CS));
	}

	if(m_lstDTask.size() == 0)
	{
		m_csLDTask.LeaveCS();
		return false;
	}
	nodeTask = m_lstDTask.front();
	m_lstDTask.pop_front();
	
	m_csLDTask.LeaveCS();

	switch(nodeTask.nDTriggerType)
	{
	case RAIDTask:
	case ETHTask:	
	case SMSTask:	
	case TMSTask:
	case IMonitorTask:
		{
			std::map<int,std::vector<std::string> > mapAction;
			ApplyPolicy(nodeTask.nDTriggerType,nodeTask,mapAction);
			ExeCmd(mapAction);
		}
		break;
	}
}

// 执行动作
void CDispatch::ExeCmd(std::map<int,std::vector<std::string> > &mapAction)
{
	std::map<int,std::vector<std::string> >::iterator it = mapAction.begin();
	for(;it != mapAction.end();it++)
	{
		switch(it->first)
		{
		case LOGCmd:
			{
				std::vector<std::string> &vecStr = it->second;
				for(int i = 0;i < vecStr.size();i++)
				{
					LOG(ERROR_DEVSTATUS_FAULT,vecStr[i]);
					printf(vecStr[i].c_str());
				}
			}
			break;
		case POLICYCmd:
			{
				std::vector<std::string> &vecStr = it->second;
				vecStr.erase(std::unique(vecStr.begin(),vecStr.end()),vecStr.end());
				for(int i = 0;i < vecStr.size();i++)
				{
					TrimSpace(vecStr[i]);
					if(vecStr[i] == "SwitchAllSMS" && m_ptrInvoker)	
					{
						((CInvoke*)m_ptrInvoker)->SwitchAllSMS();	
					}
					else if(vecStr[i] == "StartTMS" && m_ptrInvoker)
					{
						((CInvoke*)m_ptrInvoker)->StartTMS();
					}
					else if(vecStr[i] == "Exit" && m_ptrInvoker)
					{
						((CInvoke*)m_ptrInvoker)->Exit();
					}
				}
			}
			break;
		}

	}
}

// 删除两侧空格
void CDispatch::TrimSpace(std::string &str)
{
	std::string tmp=str;
	str = tmp.assign(tmp.begin()+tmp.find_first_not_of(' '),tmp.begin()+tmp.find_last_not_of(' ')+1);
}

// 按策略进行调度
bool CDispatch::ApplyPolicy(int nTaskType,struct DispatchTask &nodeTask,std::map<int,std::vector<std::string> >& mapAction)
{	
	if(nodeTask.nDTriggerType != nTaskType)
	{
		return false;
	}

	std::map<int,PolicyInfo>::iterator fit = m_mapPolicy.find(nTaskType);
	if(fit == m_mapPolicy.end())
	{
		return false;
	}

	std::vector<PolicyInfoEle> vecAct;
	PolicyInfo& stPI = fit->second;
	int nLen = nodeTask.vecErr.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		stError &err = nodeTask.vecErr[i];
		std::map<std::string,PolicyInfoEle>::iterator fele = stPI.mapPInfo.find(err.ErrorName);
		if(fele == stPI.mapPInfo.end())
		{
			continue;
		}

		PolicyInfoEle &stPE = fele->second;
		if(stPE.strType == "value" && stPE.strFault == err.ErrorVal)
		{
			char buf[128];
			snprintf(buf,sizeof(buf),"LOG:%s%d value: %s = Policy:%s\n",err.ErrorName.c_str(),err.nOrdinal,
				err.ErrorVal.c_str(),stPE.strFault.c_str());
			mapAction[LOGCmd].push_back(std::string(buf));
		
			//获取动作
			vecAct.push_back(stPE);
		}

		if(stPE.strType == "exrange")
		{
			int nPos = stPE.strFault.find(',');
			if(nPos != std::string::npos)
			{
				int nMin = atoi(stPE.strFault.substr(0,nPos).c_str());
				int nMax = atoi(stPE.strFault.substr(nPos+1).c_str());
				int errvalue = atoi(err.ErrorVal.c_str());
				if(errvalue < nMin && errvalue > nMax)
				{
					char buf[128];
					snprintf(buf,sizeof(buf),"LOG:%s value = %s  = Policy:%s -> %s \n",err.ErrorName.c_str(),
						err.ErrorVal.c_str(),stPE.strFault.c_str(),stPE.strAct.c_str());
					mapAction[LOGCmd].push_back(std::string(buf));

					//获取动作
					vecAct.push_back(stPE);
				}
			}
		}

		if(stPE.strType == "asis")
		{
			if(stPE.strFault.find(err.ErrorVal) != std::string::npos)
			{
				char buf[128];
				snprintf(buf,sizeof(buf),"LOG:%s value = %s  = Policy:%s\n",err.ErrorName.c_str(),
					err.ErrorVal.c_str(),stPE.strFault.c_str());
				mapAction[LOGCmd].push_back(std::string(buf));

				//获取动作
				vecAct.push_back(stPE);
			}
		}
	}

	if(vecAct.size()>0)
	{
		std::sort(vecAct.begin(),vecAct.end());
		for(int i = 0;i < vecAct.size() ;i++)
		{
			mapAction[POLICYCmd].push_back(vecAct[i].strAct);
		}
	}
	return true;
}