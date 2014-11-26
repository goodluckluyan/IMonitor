
//@file:TMSSensor.h
//@brief:TMS相关操作。
//@author:luyan@oristartech.com
//date:2014-09-17


#ifndef _H_SOFTWARESTATE_
#define _H_SOFTWARESTATE_

#include <iostream>
#include <string>
#include "DataManager.h"
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

class CTMSSensor
{
public:
	
	CTMSSensor();
	
	~CTMSSensor();

	bool StartTMS();

	bool SwitchTMS();
		
	//获取TMS 工作状态
	int GetTMSWorkState();

	bool Init(std::string strIP,int nPort,int nTMSWSPort);

	bool NotifyTMSSMSSwitch(std::string strHallId,std::string strNewIp,unsigned short port);

private:
	bool ShutDownTMS();
	
	bool StartTMS_CurTerminal(std::string strTMSPath);
	bool StartTMS_NewTerminal(std::string strTMSPath);
	int Getpid(std::string strName,std::vector<int>& vecPID);
	int InvokerWebServer(bool bTMSWS,std::string strURI,std::string &xml,std::string &strResponse);
	int GetHttpContent(const std::string &http, std::string &response);
	int SendAndRecvResponse(bool bTMSWS,const std::string &request, std::string &response, int delayTime = 3);
	bool ParseXmlFromOtherMonitor(std::string &retXml,int &nRet);
	bool ParseXmlFromTMS(std::string &retXml,int &nRet);

	bool CallStandbySwitchTMS();
	

private:
	CDataManager * m_ptrDM;
	int m_nPid;

	//对端主机信息
	std::string m_strIP;
	int m_nPort;
	int m_nTMSWBPort;

};


#endif //