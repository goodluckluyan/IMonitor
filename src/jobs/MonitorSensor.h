//@file:MonitorSensor.h
//@brief: ��һ̨���ȳ����״̬��⡣
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

	// ��ʼ��
	bool Init(std::string strIP,int nPort);
	
	// ��ȡ��һ̨�����ĵ��ȳ����״̬
	bool GetOtherMonitorState(int nStateType,bool bNoticeDM = true);

private:

	// ��ȡhttp�е�xml 
	int GetHttpContent(const std::string &http, std::string &response);

	// �ѵ���xml����http��ʽ���͵�����˲����շ���xml
	int SendAndRecvResponse(const std::string &request, std::string &response, int delayTime=3);

	// ����webservice�ӿ�
	int InvokerWebServer(std::string &xml,std::string &strResponse);

	// ����Monitor״̬
	bool ParseOtherMonitorState(std::string &retXml,bool &bMain,int &nState );

	// ����TMS״̬
	bool ParseOtherMonitorTMSState(std::string &retXml,bool &bRun,int &nWorkState,int &nState);

	// ����SMS״̬
	bool ParseOtherMonitorSMSState(std::string &retXml,std::vector<SMSStatus>& vecSMSStatus);

	// �������̳���״̬
	bool ParseOtherMonitorRaidState(std::string &retXml,int &nState,int &nReadSpeed,int &nWriteSpeed,
		std::vector<int> &vecDiskState);

	// ����SMS�����쳣״̬
	bool ParseOtherMonitorSMSEWState(std::string &retXml,int &nState,
		std::string & strInfo, std::string &strHall);

	// �����ٶ�����״̬
	bool ParseOtherMonitorSpeedLmtState(std::string &retXml,bool &bEnableIngest,int &nSpeedLimit);

	// ����������״̬
	bool ParseOtherMonitorSwitchState(std::string &retXml,int &nSwitch1State,int & nSwitch2State);

	// ��������״̬
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