//@file:C_Hall.h
//@brief: 包含类C_Hall。
//C_Hall：封装了影厅中sms相关操作。
//@author:luyan@oristartech.com
//dade:2012-07-12

#ifndef HALL_DEFINE
#define HALL_DEFINE

#include "C_constDef.h"
//#include "timeTask/C_TaskList.h"
#include <pthread.h>
#include <iostream>
#include <string>

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
#include <xercesc/sax/HandlerBase.hpp>



#include "utility/C_HttpParser.h"
#include "utility/C_TcpTransport.h"

class C_Hall
{

 public:   
	 C_Hall(SMSInfo &stSms);
	 ~C_Hall();

public:
	// 初始化
	int Init(bool bRun);

	 // 获取SMS工作状态
	int  GetSMSWorkState( int &state, std::string &info);
	
	// 启动SMS
	bool StartSMS(int &nPid);

	// 关闭SMS
	bool ShutDownSMS();

	// 改变SMS的运行主机信息	
	SMSInfo& ChangeSMSHost(std::string strIP,bool bLocalRun);

	

	// 获取hallid
	std::string GetHallID()
	{
		return m_SMS.strId;
	}

	// 是否为本地运行
	bool IsLocal()
	{
		return m_SMS.stStatus.nRun == 1;
	}

	// 调用对端调度软件的切换接口
	int CallStandbySwitchSMS(std::string strOtherIP,int nPort,std::string strHallID);

	// 获取运行主机及webservice端口
	void GetRunHost(std::string &strIP,int &nPort);
private:

	// 在当前终端启动SMS
	bool StartSMS_CurTerminal(int &nPid);

	// 打开新终端启动SMS
	bool StartSMS_NewTerminal(int &nPid);

	int UsherHttp(std::string &strURI,std::string& strIP,std::string &xml,std::string action,std::string &strRequest);

	int GetHttpContent(const std::string &http, std::string &content);

	int Parser_GetSMSWorkState( const std::string &content, int &state, std::string &info);

	int Parser_SwitchSMS(std::string &content,int &nRet);

	int TcpOperator(std::string strIP,int nPort,const std::string &send, std::string &recv, int overtime);

	int SendAndRecvInfo(const std::string &send, std::string &recv, int overtime);

	int ReceiveCommand(std::string &recv, int waitTime);

	 std::string ExtractXml(const std::string &response);

 	int GetRootChild( const std::string &xml,xercesc::XercesDOMParser *parser,
 		xercesc::ErrorHandler *errHandler, xercesc::DOMElement **rootChild);
 
 	xercesc::DOMElement *GetElementByName( const xercesc::DOMNode *elem, const std::string &name);
 
 	xercesc::DOMElement *FindElementByName( const xercesc::DOMNode *elem, const std::string &name);

	// 获取指定命令的pid
	int Getpid(std::string strName,std::vector<int>& vecPID);

private:
	
	// SMS信息
	SMSInfo m_SMS;

	// 谨慎期间为SMS加锁
	pthread_mutex_t m_mtxSMSInfo;
 
	//for http head
	std::string m_xmlHeader;
	std::string m_xmlnsXsi;
	std::string m_xmlnsXsd;
	std::string m_xmlnsSoap;
	std::string m_envelopeBgn;
	std::string m_envelopeEnd;
	std::string m_bodyBgn;
	std::string m_bodyEnd;

	//tcp protocol
	TcpTransport m_tcp;

	pid_t m_pid;

	// 初始运行标记 
	bool m_bInitRun;

};
#endif //HALL_DEFINE
