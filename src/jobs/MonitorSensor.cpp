#include"MonitorSensor.h"
#include"C_ErrorDef.h"
#include"utility/C_HttpParser.h"
#include"utility/C_TcpTransport.h"
#include"log/C_LogManage.h"
#include "utility/IPMgr.h"
#include "para/C_Para.h"

#define  LOGFAT(errid,msg)  C_LogManage::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_JOBS,0,errid,msg)
#define  LOGFATFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_FATAL,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)
#define  LOGINFFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_INFO,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)

CMonitorSensor::CMonitorSensor()
{
	m_mapStateType[TASK_NUMBER_GET_OTHERMONITOR_STATUS] = "<ns2:GetMontorState></ns2:GetMontorState>";
	m_mapStateType[TASK_NUMBER_GET_OTHERMONITOR_TMS_STATUS] = "<ns2:GetTMSState></ns2:GetTMSState>";
	m_mapStateType[TASK_NUMBER_GET_OTHERMONITOR_SMS_STATUS] = "<ns2:GetSMSState></ns2:GetSMSState>";
	m_mapStateType[TASK_NUMBER_GET_OTHERMONITOR_RAID_STATUS] = "<ns2:GetRaidtate></ns2:GetRaidtate>";
	m_mapStateType[TASK_NUMBER_GET_OTHERMONITOR_ETH_STATUS] = "<ns2:GetEthState></ns2:GetEthState>";
	m_mapStateType[TASK_NUMBER_GET_OTHERMONITOR_SWITCH_STATUS] = "<ns2:GetSwitchState></ns2:GetSwitchState>";
	m_mapStateType[TASK_NUMBER_GET_OTHERMONITOR_SPEEDLIMIT_STATUS] = "<ns2:GetIngestSpeedLimit></ns2:GetIngestSpeedLimit>";
	m_mapStateType[TASK_NUMBER_GET_OTHERMONITOR_SMSEW_STATUS] = "<ns2:GetWorkState_CS></ns2:GetWorkState_CS>";
	m_ptrDM = NULL;
}

CMonitorSensor::~CMonitorSensor()
{
	xercesc::XMLPlatformUtils::Terminate();
}

// 初始化
bool CMonitorSensor::Init(std::string strIP,int nPort)
{
	if( strIP.empty() || nPort <= 0)
	{
		return false;
	}

	m_strIP = strIP;
	m_nPort = nPort;

// 	char buff[64]={'\0'};
// 	snprintf(buff,128,"http://%s:%d/?wsdl",strIP.c_str(),nPort);
	m_strURI ="/";//buff;
	
	m_ptrDM = CDataManager::GetInstance();

	try
	{
		xercesc::XMLPlatformUtils::Initialize();
	}
	catch( xercesc::XMLException& e )
	{
		char* message = xercesc::XMLString::transcode( e.getMessage() );
		xercesc::XMLString::release( &message );
	}
}

// 把调用xml串以http方式发送到服务端并接收返回xml
int CMonitorSensor::SendAndRecvResponse(const std::string &request, std::string &response, int delayTime)
{
	if(m_strIP.empty())
	{
		return 0;
	}

	std::string strIP=m_strIP;
	TcpTransport tcp;
	int result = tcp.TcpConnect(strIP.c_str(), m_nPort);
	if(result < 0)
	{	
		LOGFATFMT(ULOG_ERROR,"CMonitorSensor::SendAndRecvResponse TcpConnect %s:%d Fail !\n",strIP.c_str(), m_nPort);
	
		IPMgr::GetInstance()->GetNextOtherIP(strIP);
		if(strIP!=m_strIP)
		{
			result = tcp.TcpConnect(strIP.c_str(), m_nPort);
			if(result < 0)
			{
				LOGFATFMT(ULOG_ERROR,"CMonitorSensor::SendAndRecvResponse TcpConnect %s:%d Fail !\n",strIP.c_str(), m_nPort);
				return  ERROR_SENSOR_TCP_CONNECT;
			}
			else
			{
				m_strIP=strIP;
				LOGFATFMT(ULOG_ERROR,"CMonitorSensor::Set Default Other IP=%s!\n",strIP.c_str());
			}
		}
	
		if(result<0)
		{
			return  ERROR_SENSOR_TCP_CONNECT;
		}
		
	}

	result = tcp.BlockSend(request.c_str(), request.size());
	if(result < 0)
	{
		LOGFATFMT(0,"CMonitorSensor::SendAndRecvResponse Tcp Send %s Fail !\n",request.c_str());
		return  ERROR_SENSOR_TCP_SEND;
	}


	char buffer[1024];
	timeval timeOut;
	timeOut.tv_sec = delayTime;
	timeOut.tv_usec = 0;
	response.clear();
	while((result = tcp.SelectRecv(buffer, 1024-1, timeOut)) >= 0)
	{
		if(result == 0)
			break;
		buffer[result] = 0;
		response += buffer;

		if(response.find(":Envelope>") != std::string::npos)
			break;
		timeOut.tv_sec = 2;
		timeOut.tv_usec = 0;
	}

	return result <0 ? ERROR_SENSOR_TCP_RECV : 0;
	
}

// 获取http中的xml 
int CMonitorSensor::GetHttpContent(const std::string &http, std::string &response)
{
	HttpResponseParser httpResponse;
	int result = httpResponse.SetHttpResponse(http);
	if(result != 0)
	{
		return result;
	}

	response = httpResponse.GetContent();
	return httpResponse.GetStatus();
}

// 调用webservice接口
int CMonitorSensor::InvokerWebServer(std::string &xml,std::string &strResponse)
{
	HttpRequestParser request;
	request.SetMethod("POST");
	request.SetUri(m_strURI.c_str());
	request.SetVersion("HTTP/1.1");
	request.SetHost(m_strIP.c_str());
	request.SetContentType("text/xml; charset=utf-8");
	request.SetContent(xml);
	request.SetSoapAction("");
	std::string strHttp = request.GetHttpRequest();

	int result = SendAndRecvResponse(strHttp, strResponse);
	
	return result;
	
}

// 调用对端接口设置数据同步标记
bool CMonitorSensor::SetOtherDBSynch(std::string dbsynch)
{
	std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	xml += "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" ";
	xml += "xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" ";
	xml += "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
	xml += "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" ";
	xml += "xmlns:ns1=\"http://tempuri.org/mons.xsd/Service.wsdl\" ";
	xml += "xmlns:ns2=\"http://tempuri.org/mons.xsd\"> <SOAP-ENV:Body> ";
	xml += "<ns2:SetDBSynchSign><dbsynch>"+dbsynch+"</dbsynch></ns2:SetDBSynchSign>";
	xml +="</SOAP-ENV:Body></SOAP-ENV:Envelope>";

	// 通过http方式调用另一个调度软件的WebService服务
	std::string strResponse;
	int nInvokeRes = InvokerWebServer(xml,strResponse);
	if( nInvokeRes == ERROR_SENSOR_TCP_RECV || nInvokeRes == ERROR_SENSOR_TCP_CONNECT 
		|| nInvokeRes == ERROR_SENSOR_TCP_SEND)
	{
		// 写错误日志
		LOGFATFMT(ERROR_CALLOTHERWS_CONNFAIL,"Call OtherHost Web Service(SetOtherDBSynch) Failed,Due To Connection Fail!");
		return false;
	}

	// 提取xml
	std::string retXml;
	int result = GetHttpContent(strResponse, retXml);
	if(retXml.empty())
	{
		LOGFATFMT(0,"SetOtherDBSynch:Parse Fail! xml is empty!\n");
		return false;
	}

	int nRet ;
	if(Parser_SetOtherDBSynch(retXml,nRet))
	{
		return nRet == 0;
	}
	else
	{
		return false;
	}

}


// 获取另一台主机的调度程序的状态
bool CMonitorSensor::GetOtherMonitorState(int nStateType,bool bNoticeDM)
{
	std::string strStateType ;
	m_csMap.EnterCS();
	std::map<int,std::string>::iterator it = m_mapStateType.find(nStateType);
	if(it == m_mapStateType.end())
	{
		m_csMap.LeaveCS();
		return false;
	}
	strStateType = it->second;
	m_csMap.LeaveCS();

	std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	xml += "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" ";
    xml += "xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" ";
    xml += "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
    xml += "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" ";
    xml += "xmlns:ns1=\"http://tempuri.org/mons.xsd/Service.wsdl\" ";
    xml += "xmlns:ns2=\"http://tempuri.org/mons.xsd\"> <SOAP-ENV:Body> ";
	xml += strStateType;
	xml +="</SOAP-ENV:Body></SOAP-ENV:Envelope>";

	// 通过http方式调用另一个调度软件的WebService服务
	std::string strResponse;
	int nInvokeRes = InvokerWebServer(xml,strResponse);
	if( nInvokeRes == ERROR_SENSOR_TCP_RECV || nInvokeRes == ERROR_SENSOR_TCP_CONNECT 
		|| nInvokeRes == ERROR_SENSOR_TCP_SEND)
	{
		// 写错误日志
		LOGFATFMT(ERROR_CALLOTHERWS_CONNFAIL,"Call OtherHost Web Service(%s) Failed,Due To Connection Fail!",
			strStateType.c_str());
		if(bNoticeDM && nStateType == TASK_NUMBER_GET_OTHERMONITOR_STATUS)
		{
			m_ptrDM->UpdateOtherMonitorState(false,-1,0);
		}

		if(bNoticeDM && nStateType == TASK_NUMBER_GET_OTHERMONITOR_RAID_STATUS && m_ptrDM != NULL)
		{
			std::vector<int> vecDiskState;
			m_ptrDM->UpdateOtherRaidState(-1,-1,-1,vecDiskState);
		}
		return false;
	}

	// 提取xml
	std::string retXml;
	int result = GetHttpContent(strResponse, retXml);
	if(retXml.empty())
	{
		LOGFATFMT(0,"GetOtherMonitorState:Parse Fail! xml is empty!\n");
		return false;
	}

	// 解析xml读取结果
	bool bRet = false; 
	int nRole= C_Para::GetInstance()->GetRole();
	switch(nStateType)
	{
	case TASK_NUMBER_GET_OTHERMONITOR_STATUS:
		{
			bool bMain;
			int nState;
			long lSynch;
			bRet = ParseOtherMonitorState(retXml,bMain,nState,lSynch);
			if(bNoticeDM && bRet && m_ptrDM != NULL)
			{
			   m_ptrDM->UpdateOtherMonitorState(bMain,nState,lSynch);
			}
		}
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_TMS_STATUS:
		{
			if(nRole!=TMPMAINROLE && nRole!=ONLYMAINROLE)
			{
				bool bRun;	
				int nWorkState;
				int nState;	

				bRet = ParseOtherMonitorTMSState(retXml,bRun,nWorkState,nState);
				if(bNoticeDM && bRet && m_ptrDM != NULL)
				{
					m_ptrDM->UpdateOtherTMSState(bRun,nWorkState,nState);
				}
			}
		}
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_SMS_STATUS:
		{
			
			if(nRole!=TMPMAINROLE && nRole!=ONLYMAINROLE)
			{
				std::vector<SMSStatus> vecSMSStatus;
				bRet = ParseOtherMonitorSMSState(retXml,vecSMSStatus);
				if(bNoticeDM && bRet && m_ptrDM != NULL)
				{
					m_ptrDM->UpdateOtherSMSState(vecSMSStatus);
				}
			}
		}
		
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_RAID_STATUS:
		{
			//因为恢复接管时要知道RAID的状态所以在接管状态也要获取状态
			//if(nRole!=TMPMAINROLE && nRole!=ONLYMAINROLE)
			{
				int nState;
				int nReadSpeed;
				int nWriteSpeed;
				std::vector<int> vecDiskState;

				bRet = ParseOtherMonitorRaidState(retXml,nState,nReadSpeed,nWriteSpeed,vecDiskState);
				if(bNoticeDM && bRet && m_ptrDM != NULL)
				{
					m_ptrDM->UpdateOtherRaidState(nState,nReadSpeed,nWriteSpeed,vecDiskState);
				}
			}
			
		}
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_ETH_STATUS:
		{
			if(nRole!=TMPMAINROLE && nRole!=ONLYMAINROLE)
			{
				std::vector<EthStatus> vecEthStatus;
				bRet = ParseOtherMonitorEthState(retXml,vecEthStatus);
				if(bNoticeDM && bRet && m_ptrDM != NULL)
				{
					m_ptrDM->UpdateOtherEthState(vecEthStatus);
				}
			}
		}
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_SWITCH_STATUS:
		{
			if(nRole!=TMPMAINROLE && nRole!=ONLYMAINROLE)
			{
				int nSwitch1State;
				int nSwitch2State;
				bRet = ParseOtherMonitorSwitchState(retXml,nSwitch1State, nSwitch2State);
				if(bNoticeDM && bRet && m_ptrDM != NULL)
				{
					m_ptrDM->UpdateOtherSwitchState(nSwitch1State,nSwitch2State);
				}
			}
		}
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_SPEEDLIMIT_STATUS:
		{
			if(nRole!=TMPMAINROLE && nRole!=ONLYMAINROLE)
			{
				bool bEnableIngest;
				int nSpeedLimit;
				bRet = ParseOtherMonitorSpeedLmtState(retXml,bEnableIngest, nSpeedLimit);
				if(bNoticeDM && bRet && m_ptrDM != NULL)
				{
					m_ptrDM->UpdateOtherSpeedLmtState(bEnableIngest,nSpeedLimit);
				}
			}

		}
		break;
	case TASK_NUMBER_GET_OTHERMONITOR_SMSEW_STATUS:
		{
			if(nRole!=TMPMAINROLE && nRole!=ONLYMAINROLE)
			{
				int nState;
				std::string strInfo;
				std::string strHall;
				bRet = ParseOtherMonitorSMSEWState(retXml,nState, strInfo, strHall);
				if(bNoticeDM && bRet && m_ptrDM != NULL)
				{
					m_ptrDM->UpdateOtherSMSEWState(nState, strInfo, strHall);
				}
			}
		}
		break;

	}
	
	
	return bRet;
}


using namespace xercesc;

int CMonitorSensor::Parser_SetOtherDBSynch(std::string &content,int &nRet)
{
	XercesDOMParser *ptrParser = new  XercesDOMParser;
	ptrParser->setValidationScheme(  XercesDOMParser::Val_Never );
	ptrParser->setDoNamespaces( true );
	ptrParser->setDoSchema( false );
	ptrParser->setLoadExternalDTD( false );
	InputSource* ptrInputsource = new  MemBufInputSource((XMLByte*)content.c_str(), content.size(), "bufId");

	try
	{
		ptrParser->parse(*ptrInputsource);
		DOMDocument* ptrDoc = ptrParser->getDocument();	

		// 读取ret
		DOMNodeList *ptrNodeList = ptrDoc->getElementsByTagName(C2X("ret"));
		if(ptrNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,"Parser_SetOtherDBSynch:没有找到SetDBSynchSign节点");
			return false;
		}
		else
		{

			DOMNode* ptrNode = ptrNodeList->item(0);
			char * ret = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state =  ret;
			if(!str_state.empty())
			{
				nRet = atoi(str_state.c_str());
			}
			XMLString::release(&ret);
		}
	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOGFAT(ERROR_PARSE_MONITORSTATE_XML,message);
		delete ptrParser;
		ptrInputsource = NULL;
		delete ptrInputsource;
		ptrParser = NULL;
	}


	delete ptrParser;
	delete ptrInputsource;
	ptrInputsource = NULL;
	ptrParser = NULL;
	return true;
}

// 解析Monitor状态
bool CMonitorSensor::ParseOtherMonitorState(std::string &retXml,bool &bMain,int &nState,long &lSynch)
{

	XercesDOMParser *ptrParser = new  XercesDOMParser;
	ptrParser->setValidationScheme(  XercesDOMParser::Val_Never );
	ptrParser->setDoNamespaces( true );
	ptrParser->setDoSchema( false );
	ptrParser->setLoadExternalDTD( false );
	InputSource* ptrInputsource = new  MemBufInputSource((XMLByte*)retXml.c_str(), retXml.size(), "bufId");

	try
	{
		ptrParser->parse(*ptrInputsource);
		 DOMDocument* ptrDoc = ptrParser->getDocument();	

		 // 读取bMain节点
		 DOMNodeList *ptrNodeList = ptrDoc->getElementsByTagName(C2X("bMain"));
		if(ptrNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,"ParseOtherMonitorState:没有找到bMain节点");
			return false;
		}
		else
		{

			DOMNode* ptrNode = ptrNodeList->item(0);
			char * pmain = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state =  pmain;
			if(!str_state.empty())
			{
				bMain = atoi(str_state.c_str()) == 1 ?true : false;
			}
			XMLString::release(&pmain);
		}

		// 读取iState节点
		 DOMNodeList *ptrStateNodeList = ptrDoc->getElementsByTagName(C2X("iState"));
		if(ptrStateNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorState:没有找到iState节点");
			return false;
		}
		else
		{

			 DOMNode* ptrNode = ptrStateNodeList->item(0);
			char *pstate =  XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state =  pstate;
			if(!str_state.empty())
			{
				nState = atoi(str_state.c_str());
			}
			XMLString::release(&pstate);
		}

		// 读取iState节点
		DOMNodeList *ptrSynchNodeList = ptrDoc->getElementsByTagName(C2X("lSynch"));
		if(ptrSynchNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorState:没有找到iState节点");
			return false;
		}
		else
		{

			DOMNode* ptrNode = ptrSynchNodeList->item(0);
			char *pstate =  XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_synch =  pstate;
			if(!str_synch.empty())
			{
				lSynch = atoi(str_synch.c_str());
			}
			XMLString::release(&pstate);
		}

	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOGFAT(ERROR_PARSE_MONITORSTATE_XML,message);
		delete ptrParser;
		ptrInputsource = NULL;
		delete ptrInputsource;
		ptrParser = NULL;
	}


	delete ptrParser;
	delete ptrInputsource;
	ptrInputsource = NULL;
	ptrParser = NULL;
	return true;
}

// 解析TMS状态
bool CMonitorSensor::ParseOtherMonitorTMSState(std::string &retXml,bool &bRun,int &nWorkState,int &nState)
{

	XercesDOMParser *ptrParser = new  XercesDOMParser;
	ptrParser->setValidationScheme(  XercesDOMParser::Val_Never );
	ptrParser->setDoNamespaces( true );
	ptrParser->setDoSchema( false );
	ptrParser->setLoadExternalDTD( false );
	InputSource* ptrInputsource = new  MemBufInputSource((XMLByte*)retXml.c_str(), retXml.size(), "bufId");

	try
	{
		ptrParser->parse(*ptrInputsource);
		DOMDocument* ptrDoc = ptrParser->getDocument();

		// 读取bRun节点
		DOMNodeList *ptrNodeList = ptrDoc->getElementsByTagName(C2X("bRun"));
		if(ptrNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorTMSState:没有找到bRun节点");
			return false;
		}
		else
		{

			DOMNode* ptrNode = ptrNodeList->item(0);
			char *prun = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state =  prun;
			if(!str_state.empty())
			{
				bRun = atoi(str_state.c_str()) == 1 ?true : false;
			}
			XMLString::release(&prun);
		}


		// 读取iWorkState节点
		DOMNodeList *ptrWorkStateNodeList = ptrDoc->getElementsByTagName(C2X("iWorkState"));
		if(ptrWorkStateNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorTMSState:没有找到iWorkState节点");
			return false;
		}
		else
		{

			DOMNode* ptrNode = ptrWorkStateNodeList->item(0);
			char * pstate = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state =  pstate;
			if(!str_state.empty())
			{
				nWorkState = atoi(str_state.c_str());
			}
			XMLString::release(&pstate);
		}

		// 读取iState节点
		DOMNodeList *ptrStateNodeList = ptrDoc->getElementsByTagName(C2X("iState"));
		if(ptrStateNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorTMSState:没有找到iState节点");
			return false;
		}
		else
		{

			DOMNode* ptrNode = ptrStateNodeList->item(0);
			char * pstate = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state = pstate ;
			if(!str_state.empty())
			{
				nState = atoi(str_state.c_str());
			}
			XMLString::release(&pstate);
		}

	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOGFAT(ERROR_PARSE_MONITORSTATE_XML,message);
		delete ptrParser;
		ptrInputsource = NULL;
		delete ptrInputsource;
		ptrParser = NULL;
	}


	delete ptrParser;
	delete ptrInputsource;
	ptrInputsource = NULL;
	ptrParser = NULL;
	return true;
}

// 解析SMS状态
bool CMonitorSensor::ParseOtherMonitorSMSState(std::string &retXml,std::vector<SMSStatus> &vecSMSStatus)
{

	XercesDOMParser *ptrParser = new  XercesDOMParser;
	ptrParser->setValidationScheme(  XercesDOMParser::Val_Never );
	ptrParser->setDoNamespaces( true );
	ptrParser->setDoSchema( false );
	ptrParser->setLoadExternalDTD( false );
	InputSource* ptrInputsource = new  MemBufInputSource((XMLByte*)retXml.c_str(), retXml.size(), "bufId");

	try
	{
		ptrParser->parse(*ptrInputsource);
		DOMDocument* ptrDoc = ptrParser->getDocument();	

		DOMNodeList *ptrNodeList = ptrDoc->getElementsByTagName(C2X("vecret"));
		int nLen = (int)ptrNodeList->getLength();
		for(int i = 0 ;i < nLen ;i++)
		{
			SMSStatus node;
			DOMNode * ptrNode = ptrNodeList->item(i);
			DOMNodeList* ptrCL = ptrNode->getChildNodes();

			//HallId
			DOMNode* ptrHallIdNode = ptrCL->item(0);
			char * pid = XMLString::transcode(ptrHallIdNode->getFirstChild()->getNodeValue());
			node.hallid =  pid;
			XMLString::release(&pid);
	
			// bRun
			DOMNode* ptrRunNode = ptrCL->item(1);
			char* prun = XMLString::transcode(ptrRunNode->getFirstChild()->getNodeValue());
			std::string str_run = prun ;
			if(!str_run.empty())
			{
				node.nRun = atoi(str_run.c_str()) ;
			}
			XMLString::release(&prun);

			// 读取state节点
			DOMNode* ptrStatNode = ptrCL->item(2);
			char * pstate = XMLString::transcode(ptrStatNode->getFirstChild()->getNodeValue());
			std::string str_state =  pstate;
			if(!str_state.empty())
			{
				node.nStatus = atoi(str_state.c_str())>>8;
			}
			XMLString::release(&pstate);
		
			// 读取position节点
			DOMNode* ptrPosNode = ptrCL->item(3);
			char *ppos = XMLString::transcode(ptrPosNode->getFirstChild()->getNodeValue());
			std::string str_pos =  ppos;
			if(!str_pos.empty())
			{
				node.nPosition = atoi(str_pos.c_str());
			}
			XMLString::release(&ppos);

			// 读取strsqluuid节点
			DOMNode* ptrUUIDNode = ptrCL->item(4);
			char *puuid = XMLString::transcode(ptrUUIDNode->getFirstChild()->getNodeValue());
			node.strSPLUuid =  puuid;
			XMLString::release(&puuid);

			vecSMSStatus.push_back(node);
		}
	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOGFAT(ERROR_PARSE_MONITORSTATE_XML,message);
		delete ptrParser;
		ptrInputsource = NULL;
		delete ptrInputsource;
		ptrParser = NULL;
	}


	delete ptrParser;
	delete ptrInputsource;
	ptrInputsource = NULL;
	ptrParser = NULL;
	return true;
}

// 解析磁盘陈列状态
bool CMonitorSensor::ParseOtherMonitorRaidState(std::string &retXml,int &nState,int &nReadSpeed,int &nWriteSpeed,
								std::vector<int> &vecDiskState)
{
	XercesDOMParser *ptrParser = new  XercesDOMParser;
	ptrParser->setValidationScheme(  XercesDOMParser::Val_Never );
	ptrParser->setDoNamespaces( true );
	ptrParser->setDoSchema( false );
	ptrParser->setLoadExternalDTD( false );
	InputSource* ptrInputsource = new  MemBufInputSource((XMLByte*)retXml.c_str(), retXml.size(), "bufId");

	try
	{
		ptrParser->parse(*ptrInputsource);
		DOMDocument* ptrDoc = ptrParser->getDocument();	

		// 读取status节点
		DOMNodeList *ptrNodeList = ptrDoc->getElementsByTagName(C2X("state"));
		if(ptrNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorRaidState:没有找到status节点");
			return false;
		}
		else
		{
			DOMNode* ptrNode = ptrNodeList->item(0);
			char * pstate = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state =  pstate;
			if(!str_state.empty())
			{
				nState = atoi(str_state.c_str());
			}
			XMLString::release(&pstate);
		}

		// 读取nReadSpeed节点
		DOMNodeList *ptrReadNodeList = ptrDoc->getElementsByTagName(C2X("ReadSpeed"));
		if(ptrReadNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorRaidState:没有找到nReadSpeed");
			return false;
		}
		else
		{
			DOMNode* ptrNode = ptrReadNodeList->item(0);
			char *pspeed = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_speed = pspeed ;
			if(!str_speed.empty())
			{
				nReadSpeed = atoi(str_speed.c_str()) ;
			}
			XMLString::release(&pspeed);
		}

		// 读取nWriteSpeed节点
		DOMNodeList *ptrWriteNodeList = ptrDoc->getElementsByTagName(C2X("WriteSpeed"));
		if(ptrWriteNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorRaidState:没有找到nWriteSpeed节点");
			return false;
		}
		else
		{
			DOMNode* ptrNode = ptrWriteNodeList->item(0);
			char * pwspeed =  XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_wspeed = pwspeed;
			if(!str_wspeed.empty())
			{
				nWriteSpeed = atoi(str_wspeed.c_str());
			}
			XMLString::release(&pwspeed);
		}


		// 读取diskstate节点
		DOMNodeList *ptrDiskNodeList = ptrDoc->getElementsByTagName(C2X("diskState"));
		if(ptrDiskNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorRaidState:没有找到diskstate节点");
			return false;
		}
		else
		{
			for(int i = 0 ;i < ptrDiskNodeList->getLength();i++)
			{
				DOMNode* ptrNode = ptrDiskNodeList->item(i);
				char * pdiskstate = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
				std::string str_state =  pdiskstate;
				if(!str_state.empty())
				{
					vecDiskState.push_back(atoi(str_state.c_str()));
				}
				XMLString::release(&pdiskstate);
			}
		}

	
	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOGFAT(ERROR_PARSE_MONITORSTATE_XML,message);
		delete ptrParser;
		ptrInputsource = NULL;
		delete ptrInputsource;
		ptrParser = NULL;
	}


	delete ptrParser;
	delete ptrInputsource;
	ptrInputsource = NULL;
	ptrParser = NULL;
	return true;
}

// 解析网卡状态
bool  CMonitorSensor::ParseOtherMonitorEthState(std::string &retXml,std::vector<EthStatus> &vecEthStatus)
{
	XercesDOMParser *ptrParser = new  XercesDOMParser;
	ptrParser->setValidationScheme(  XercesDOMParser::Val_Never );
	ptrParser->setDoNamespaces( true );
	ptrParser->setDoSchema( false );
	ptrParser->setLoadExternalDTD( false );
	InputSource* ptrInputsource = new  MemBufInputSource((XMLByte*)retXml.c_str(), retXml.size(), "bufId");

	try
	{
		ptrParser->parse(*ptrInputsource);
		DOMDocument* ptrDoc = ptrParser->getDocument();	

		DOMNodeList *ptrNodeList = ptrDoc->getElementsByTagName(C2X("vecret"));
		int nLen = (int)ptrNodeList->getLength();
		for(int i = 0 ;i < nLen ;i++)
		{
			EthStatus EthNode;
			DOMNode * ptrNode = ptrNodeList->item(i);
			DOMNodeList* ptrCL = ptrNode->getChildNodes();

			//index
			DOMNode* ptrEthNode = ptrCL->item(0);
			char * peth =  XMLString::transcode(ptrEthNode->getFirstChild()->getNodeValue());
			std::string strEth = peth;
			if(!strEth.empty())
			{
				EthNode.strName = "eth"+strEth;
			}
			XMLString::release(&peth);

			//tasktype
			DOMNode* ptrTypeNode = ptrCL->item(1);
			char *ptype =  XMLString::transcode(ptrTypeNode->getFirstChild()->getNodeValue());
			std::string strType  = ptype;
			if(!strType.empty())
			{
				EthNode.nTaskType = atoi(strType.c_str());
			}
			XMLString::release(&ptype);

			// connect state
			DOMNode* ptrConnectStatusNode = ptrCL->item(2);
			char * pconnt = XMLString::transcode(ptrConnectStatusNode->getFirstChild()->getNodeValue());
			std::string strConnectStatus = pconnt;
			if(!strConnectStatus.empty())
			{
				EthNode.nConnStatue = atoi(strConnectStatus.c_str());
			}
			XMLString::release(&pconnt);

			// speed
			DOMNode* ptrSpeedNode = ptrCL->item(3);
			char *pspeed = XMLString::transcode(ptrSpeedNode->getFirstChild()->getNodeValue());
			std::string strSpeed =  pspeed;
			if(!strSpeed.empty())
			{
				EthNode.nRxSpeed = EthNode.nTxSpeed = atoi(strSpeed.c_str());
			}
			XMLString::release(&pspeed);

			vecEthStatus.push_back(EthNode);
		}
		
	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOGFAT(ERROR_PARSE_MONITORSTATE_XML,message);
		delete ptrParser;
		ptrInputsource = NULL;
		delete ptrInputsource;
		ptrParser = NULL;
	}


	delete ptrParser;
	delete ptrInputsource;
	ptrInputsource = NULL;
	ptrParser = NULL;
	return true;
}


// 解析交换机状态
bool  CMonitorSensor::ParseOtherMonitorSwitchState(std::string &retXml,int &nSwitch1State,int & nSwitch2State)
{
	XercesDOMParser *ptrParser = new  XercesDOMParser;
	ptrParser->setValidationScheme(  XercesDOMParser::Val_Never );
	ptrParser->setDoNamespaces( true );
	ptrParser->setDoSchema( false );
	ptrParser->setLoadExternalDTD( false );
	InputSource* ptrInputsource = new  MemBufInputSource((XMLByte*)retXml.c_str(), retXml.size(), "bufId");

	try
	{
		ptrParser->parse(*ptrInputsource);
		DOMDocument* ptrDoc = ptrParser->getDocument();	

		// 读取Swtch1State节点
		DOMNodeList *ptrS1NodeList = ptrDoc->getElementsByTagName(C2X("Switch1State"));
		if(ptrS1NodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorSwitchState:没有找到Swtch1State节点");
			return false;
		}
		else
		{
			DOMNode* ptrNode = ptrS1NodeList->item(0);
			char *pstate = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state =  pstate;
			if(!str_state.empty())
			{
				nSwitch1State = atoi(str_state.c_str());
			}
			XMLString::release(&pstate);
		}

		// 读取Swtch2State节点
		DOMNodeList *ptrS2NodeList = ptrDoc->getElementsByTagName(C2X("Switch2State"));
		if(ptrS2NodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorSwitchState:没有找到Swtch2State节点");
			return false;
		}
		else
		{
			DOMNode* ptrNode = ptrS2NodeList->item(0);
			char * pstate = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state =  pstate;
			if(!str_state.empty())
			{
				nSwitch2State = atoi(str_state.c_str());
			}
			XMLString::release(&pstate);
		}


	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOGFAT(ERROR_PARSE_MONITORSTATE_XML,message);
		delete ptrParser;
		ptrInputsource = NULL;
		delete ptrInputsource;
		ptrParser = NULL;
	}


	delete ptrParser;
	delete ptrInputsource;
	ptrInputsource = NULL;
	ptrParser = NULL;
	return true;
}


// 解析速度限制状态
bool  CMonitorSensor::ParseOtherMonitorSpeedLmtState(std::string &retXml,bool &bEnableIngest,int &nSpeedLimit)
{
	XercesDOMParser *ptrParser = new  XercesDOMParser;
	ptrParser->setValidationScheme(  XercesDOMParser::Val_Never );
	ptrParser->setDoNamespaces( true );
	ptrParser->setDoSchema( false );
	ptrParser->setLoadExternalDTD( false );
	InputSource* ptrInputsource = new  MemBufInputSource((XMLByte*)retXml.c_str(), retXml.size(), "bufId");

	try
	{
		ptrParser->parse(*ptrInputsource);
		DOMDocument* ptrDoc = ptrParser->getDocument();	

		// 读取bEnableIngest节点
		DOMNodeList *ptrNodeList = ptrDoc->getElementsByTagName(C2X("bEnableIngest"));
		if(ptrNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorSpeedLmtState:没有找到bEnableIngest节点");
			return false;
		}
		else
		{
			DOMNode* ptrNode = ptrNodeList->item(0);
			char * pingest = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state =  pingest;
			if(!str_state.empty())
			{
				bEnableIngest = atoi(str_state.c_str()) == 1? true :false;
			}
			XMLString::release(&pingest);
		}

        // 读取speedLimit节点
		DOMNodeList *ptrSLNodeList = ptrDoc->getElementsByTagName(C2X("speedLimit"));
		if(ptrSLNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorSpeedLmtState:没有找到speedLimit节点");
			return false;
		}
		else
		{
			DOMNode* ptrNode = ptrSLNodeList->item(0);
			char * pspeedlimit = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state =  pspeedlimit;
			if(!str_state.empty())
			{
				nSpeedLimit = atoi(str_state.c_str()) ;
			}
			XMLString::release(&pspeedlimit);
		}


	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOGFAT(ERROR_PARSE_MONITORSTATE_XML,message);
		delete ptrParser;
		ptrInputsource = NULL;
		delete ptrInputsource;
		ptrParser = NULL;
	}


	delete ptrParser;
	delete ptrInputsource;
	ptrInputsource = NULL;
	ptrParser = NULL;
	return true;
}

// 解析SMS工作异常状态
bool  CMonitorSensor::ParseOtherMonitorSMSEWState(std::string &retXml,int &nState,
												  std::string & strInfo, std::string &strHall)
{
	XercesDOMParser *ptrParser = new  XercesDOMParser;
	ptrParser->setValidationScheme(  XercesDOMParser::Val_Never );
	ptrParser->setDoNamespaces( true );
	ptrParser->setDoSchema( false );
	ptrParser->setLoadExternalDTD( false );
	InputSource* ptrInputsource = new  MemBufInputSource((XMLByte*)retXml.c_str(), retXml.size(), "bufId");

	try
	{
		ptrParser->parse(*ptrInputsource);
		DOMDocument* ptrDoc = ptrParser->getDocument();

		// 读取status节点
		DOMNodeList *ptrNodeList = ptrDoc->getElementsByTagName(C2X("state"));
		if(ptrNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorSMSEWState:没有找到HallId节点");
			return false;
		}
		else
		{
			DOMNode* ptrNode = ptrNodeList->item(0);
			char *pstate = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state =  pstate;
			if(!str_state.empty())
			{
				nState = atoi(str_state.c_str());
			}
			XMLString::release(&pstate);
		}

		// 读取info节点
		DOMNodeList *ptrInfoNodeList = ptrDoc->getElementsByTagName(C2X("info"));
		if(ptrInfoNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorSMSEWState:没有找到info节点");
			return false;
		}
		else
		{
			DOMNode* ptrNode = ptrInfoNodeList->item(0);
			char *pinfo = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			strInfo =  pinfo;
			XMLString::release(&pinfo);
		}

		// 读取hall节点
		DOMNodeList *ptrHallNodeList = ptrDoc->getElementsByTagName(C2X("hall"));
		if(ptrHallNodeList == NULL)
		{
			LOGFAT(ERROR_PARSE_MONITORSTATE_XML,
				"ParseOtherMonitorSMSEWState:没有找到hall节点");
			return false;
		}
		else
		{
			DOMNode* ptrNode = ptrHallNodeList->item(0);
			char * phall = XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			strHall =  phall;
			XMLString::release(&phall);
		}
	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOGFAT(ERROR_PARSE_MONITORSTATE_XML,message);
		delete ptrParser;
		ptrInputsource = NULL;
		delete ptrInputsource;
		ptrParser = NULL;
	}


	delete ptrParser;
	delete ptrInputsource;
	ptrInputsource = NULL;
	ptrParser = NULL;
	return true;
}