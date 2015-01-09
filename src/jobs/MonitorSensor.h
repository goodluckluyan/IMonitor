//@file:MonitorSensor.h
//@brief: 另一台调度程序的状态监测。
//@author:luyan@oristartech.com
//date:2014-09-17

#ifndef MONITORSENSOR_INC
#define MONITORSENSOR_INC

#include "DataManager.h"
#include "threadManage/C_CS.h"
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

class Char2XMLCh
{
public :
	//  Constructors and Destructor
	Char2XMLCh()
	{
		fUnicodeForm = NULL;
	}
	Char2XMLCh(const char* toTranscode)
	{
		fUnicodeForm = xercesc::XMLString::transcode(toTranscode);
	}
	~Char2XMLCh()
	{
		if (fUnicodeForm)
		{
			xercesc::XMLString::release(&fUnicodeForm);
			fUnicodeForm = NULL;
		}
	}

	void operator = (const char* chstr)
	{
		fUnicodeForm = xercesc::XMLString::transcode(chstr);
	}

	const XMLCh* unicodeForm() const
	{
		return fUnicodeForm;
	}

	XMLCh* get_xml_str()
	{
		return fUnicodeForm;
	}

private :
	XMLCh* fUnicodeForm;
};

#define C2X(str) Char2XMLCh(str).unicodeForm()

class CMonitorSensor
{
public:
	CMonitorSensor();

	~CMonitorSensor();

	// 初始化
	bool Init(std::string strIP,int nPort);
	
	// 获取另一台主机的调度程序的状态
	bool GetOtherMonitorState(int nStateType,bool bNoticeDM = true);

private:

	// 获取http中的xml 
	int GetHttpContent(const std::string &http, std::string &response);

	// 把调用xml串以http方式发送到服务端并接收返回xml
	int SendAndRecvResponse(const std::string &request, std::string &response, int delayTime=3);

	// 调用webservice接口
	int InvokerWebServer(std::string &xml,std::string &strResponse);

	// 解析Monitor状态
	bool ParseOtherMonitorState(std::string &retXml,bool &bMain,int &nState );

	// 解析TMS状态
	bool ParseOtherMonitorTMSState(std::string &retXml,bool &bRun,int &nWorkState,int &nState);

	// 解析SMS状态
	bool ParseOtherMonitorSMSState(std::string &retXml,std::vector<SMSStatus>& vecSMSStatus);

	// 解析磁盘陈列状态
	bool ParseOtherMonitorRaidState(std::string &retXml,int &nState,int &nReadSpeed,int &nWriteSpeed,
		std::vector<int> &vecDiskState);

	// 解析SMS工作异常状态
	bool ParseOtherMonitorSMSEWState(std::string &retXml,int &nState,
		std::string & strInfo, std::string &strHall);

	// 解析速度限制状态
	bool ParseOtherMonitorSpeedLmtState(std::string &retXml,bool &bEnableIngest,int &nSpeedLimit);

	// 解析交换机状态
	bool ParseOtherMonitorSwitchState(std::string &retXml,int &nSwitch1State,int & nSwitch2State);

	// 解析网卡状态
	bool ParseOtherMonitorEthState(std::string &retXml,std::vector<EthStatus> &vecEthStatus);
	
private:
	std::string m_strURI;
	std::string m_strIP;
	int m_nPort;
	CDataManager * m_ptrDM;

	std::map<int,std::string> m_mapStateType;
	C_CS m_csMap;
};

#endif