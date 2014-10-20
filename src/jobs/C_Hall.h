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

#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/dom/DOMDocumentType.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMNodeIterator.hpp>
#include <xercesc/dom/DOMNodeList.hpp>
#include <xercesc/dom/DOMText.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>

#include "utility/C_HttpParser.h"
#include "utility/C_TcpTransport.h"

class C_Hall
{

 public:   
	 C_Hall(SMSInfo &stSms);
	 ~C_Hall();

public:
	
	 // 获取SMS工作状态
	int  GetSMSWorkState( int &state, std::string &info);
	
	// 启动SMS
	bool StartSMS();

	// 获取hallid
	std::string GetHallID()
	{
		return m_SMS.strId;
	}
private:

	//Sms
	int SmsInit();

	//拼串 发送给SMS
	std::string GetSMSWorkState_Xml();

	std::string &UsherHttp(const std::string &xml, const std::string &action);

	int GetHttpContent(const std::string &http, std::string &content);

	int Parser_GetSMSWorkState( const std::string &content, int &state, std::string &info);

	int Parser_GetSMSWorkState_Response( xercesc::DOMElement *rootChild, int &state, std::string &info);

	int SendAndRecvInfo(const std::string &send, std::string &recv, int overtime);

	int ReceiveCommand(std::string &recv, int waitTime);

	 std::string ExtractXml(const std::string &response);

	int GetRootChild( const std::string &xml,xercesc::XercesDOMParser *parser,
		xercesc::ErrorHandler *errHandler, xercesc::DOMElement **rootChild);

	xercesc::DOMElement *GetElementByName( const xercesc::DOMNode *elem, const std::string &name);

	xercesc::DOMElement *FindElementByName( const xercesc::DOMNode *elem, const std::string &name);

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

	//for location
	std::string m_usherLocation;
	std::string m_usherNs;

	//http protocol
	HttpRequestParser m_request;
	HttpResponseParser m_response;
	//tcp protocol
	TcpTransport m_tcp;

};
#endif //HALL_DEFINE
