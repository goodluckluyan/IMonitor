

#include <string>
#include <cstdlib>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <algorithm>
#include <sstream>
#include "TMSSensor.h"
#include "para/C_Para.h"
#include "utility/C_TcpTransport.h"
#include "utility/C_HttpParser.h"
#include "C_ErrorDef.h"
#include "MonitorSensor.h"
#include "log/C_LogManage.h"

#define  LOG(errid,msg)  C_LogManage::GetInstance()->WriteLog(LOG_FATAL,LOG_MODEL_JOBS,0,errid,msg)
#define  LOGINFFMT(fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(LOG_INFO,LOG_MODEL_JOBS,0,0,fmt,##__VA_ARGS__)
#define  LOGFAT(errid,msg)  C_LogManage::GetInstance()->WriteLog(LOG_FATAL,LOG_MODEL_JOBS,0,errid,msg)


using namespace std;
using namespace xercesc;


CTMSSensor::CTMSSensor():
m_ptrDM(NULL)
,m_nPid(-1)
{};

CTMSSensor::~CTMSSensor()
{
	xercesc::XMLPlatformUtils::Terminate();
}

// 设置对端主机信息
bool CTMSSensor ::Init(std::string strIP,int nPort,int nTMSWSPort)
{
	if( strIP.empty() || nPort <= 0 || nTMSWSPort <=0)
	{
		return false;
	}
	
	m_strIP = strIP;
	m_nPort = nPort;
	m_nTMSWBPort = nTMSWSPort;

	try
	{
		xercesc::XMLPlatformUtils::Initialize();
	}
	catch( xercesc::XMLException& e )
	{
		char* message = xercesc::XMLString::transcode( e.getMessage() );
		xercesc::XMLString::release( &message );
	}

	StartTMS();
	return true;
}


//获取TMS 运行状态
int CTMSSensor::GetTMSWorkState()
{	
	int state = 0;
	char buf[20] = {0};
	char command[] = "pidof  -s  Tms20_DeviceService";

	m_nPid = 0; //-1;
	
	FILE *fp = popen(command,"r");
	if(fp==NULL)
	{
		LOGINFFMT("failed to popen %s:%s\n",command,strerror(errno));
		state = -1;
		return 0;//////////////return -1;
	}
	
	if(fgets(buf,sizeof(buf)-1,fp)!=NULL)
	{
		LOGINFFMT("[ Tms20_DeviceService ] Pid = %s",buf);
		int result(0);
		result = sscanf( buf,"%d", &m_nPid );
		if(result == 0)//sscanf失败，则返回0
		{
			string error = "Error:pidof -s  Tms20_DeviceService\n";
			LOGINFFMT( "%s" , error.c_str() );
			state = -1;
		}
// 		if ( pid != 0 )
// 		{
// 			fprintf( stderr, "[ %s ] is: <%u>\n", "Tms20_DeviceService", ( unsigned int )pid );  
// 			LOGINFFMT( "[ %s ] is: <%u>\n", "Tms20_DeviceService", ( unsigned int )pid  );
// 		}
	}
	else
	{
		state = -1;
	}
	pclose(fp);

	m_ptrDM = CDataManager::GetInstance();
	if(m_ptrDM != NULL)
	{
		m_ptrDM->UpdateTMSStat(state);
	}
	
	return 0;/////////return 0;
}

// 切换tms
bool CTMSSensor::SwitchTMS()
{
	bool bRet = false;
	if(m_nPid == 0)
	{
		 bRet = StartTMS();
	}
	else
	{
		 bRet =ShutDownTMS();
	}

	if(C_Para::GetInstance()->m_bMain)
	{
		LOGINFFMT("Call Standby SwitchTMS Interface !");
		if( CallStandbySwitchTMS())
		{
			bRet &= true;
			LOGINFFMT("Call Standby SwitchTMS Interface OK!");
		}
		else
		{
			bRet &= false;
			LOGINFFMT("Call Standby SwitchTMS Interface Fail!");
		}
	}
	return bRet;
}


// 关闭tms
bool CTMSSensor::ShutDownTMS()
{
	if(m_nPid > 0)
	{
		int nRet = kill(m_nPid,9);
		if(nRet == 0)
		{
			LOGINFFMT("Kill Local TMS(%d) Done!\n",m_nPid);
		}
		return nRet == 0 ? true: false;
	}
	else
	{
		return false;
	}
}

// 启动tms
bool CTMSSensor::StartTMS()
{
	if(!C_Para::GetInstance()->m_bMain)
	{
		LOGINFFMT("Standby host can't start TMS!\n");
		return false;
	}

	std::string strTMSPath = C_Para::GetInstance()->m_strTMSPath;
	if(strTMSPath.empty())
	{
		return false;
	}

	int nPos = strTMSPath.rfind('/');
	std::string strEXE = strTMSPath.substr(nPos+1);
	std::vector<int> vecPID;
	Getpid(strEXE,vecPID);

	if(vecPID.size() == 1)
	{
		m_nPid = vecPID[0];
	}
	else
	{
		int nStartType = C_Para::GetInstance()->m_nStartSMSType;
		if(nStartType == 1)
		{
			StartTMS_CurTerminal(strTMSPath);
		}
		else if(nStartType == 2)
		{
			StartTMS_NewTerminal(strTMSPath);
		}
	}

}


// 获取指定进程的pid
int CTMSSensor::Getpid(std::string strName,std::vector<int>& vecPID)
{	
	char acExe[64]={'\0'};
	snprintf(acExe,sizeof(acExe),"pidof %s",strName.c_str());
	FILE *pp = popen(acExe,"r");
	if(!pp)
	{
		LOGINFFMT("%s popen fail",acExe);
		return -1;
	}
	char tmpbuf[128]={'\0'};
	std::vector<std::string> vecBuf;
	while(fgets(tmpbuf,sizeof(tmpbuf),pp)!=NULL)
	{
		vecBuf.push_back(tmpbuf);
	}

	int nLen = vecBuf.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		std::string &strtmp=vecBuf[i];
		int nStart = 0;
		int nPos = strtmp.find(' ',nStart);
		while(nPos != std::string::npos)
		{
			vecPID.push_back(atoi(strtmp.substr(nStart,nPos-nStart).c_str()));
			nStart = nPos+1;
			nPos = strtmp.find(' ',nStart);
		}
		vecPID.push_back(atoi(strtmp.substr(nStart).c_str()));
	}

	pclose(pp);
	return 0;
}

// 打开新终端启动SMS
bool CTMSSensor::StartTMS_NewTerminal(std::string strTMSPath)
{
	int nPos = strTMSPath.rfind('/');
	std::string strEXE = strTMSPath.substr(nPos+1);
	std::string strDir = strTMSPath.substr(0,nPos);
	std::vector<int> vecCurPID;
	if(Getpid(strEXE,vecCurPID) < 0)
	{
		return false;
	}

	char buf[256]={'\0'};
	snprintf(buf,sizeof(buf),"gnome-terminal --title=\"%s\" --working-directory=%s -e \"%s\"",
		"TMS",strDir.c_str(),strTMSPath.c_str());
	LOGINFFMT("%s\n",buf);
	system(buf);

	//等待3秒
	bool bRun = false;
	int exepid = 0;
	time_t tm1;
	time(&tm1);
	while(!bRun)
	{
		sleep(1);
		std::vector<int> vecNowPID;
		if(Getpid(strEXE,vecNowPID) < 0)
			//if(Getpid("top",vecNowPID) < 0)
		{
			return false;
		}

		std::vector<int>::iterator fit;
		for(int i = 0 ;i <vecNowPID.size();i++)
		{
			fit = std::find(vecCurPID.begin(),vecCurPID.end(),vecNowPID[i]);
			if(fit == vecCurPID.end())
			{
				exepid = vecNowPID[i];
				bRun = true;
				LOGINFFMT("Fork Process(%d) Start SMS ... \n",exepid);
				break;
			}
		}

		time_t tm2;
		time(&tm2);
		if( tm2-tm1 > 5)
		{
			LOGINFFMT("waiting 5 sec ,but SMS not run..\n");
			break;
		}
	}

	if(exepid > 0)
	{	
		m_nPid = exepid;
		return true;
	}
	else
	{
		return false;
	}
}


// 启动tms在当前终端
bool CTMSSensor::StartTMS_CurTerminal(std::string strTMSPath)
{
	pid_t pid;
	if((pid = fork()) < 0)
	{
		perror("failed to create process/n");
		return false;
	}
	else if(pid == 0)
	{
		LOGINFFMT("Fork Process(%d) Start TMS ... \n",getpid());
		if(execl(strTMSPath.c_str(),"Tms20_DeviceService","",NULL) < 0)
		{
			perror("execl error");
			exit(0);
		}
	}

	m_nPid = pid;
	return true;
}


// 通过webservice调用对端的切换tms接口
bool CTMSSensor::CallStandbySwitchTMS()
{
	
	std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	xml += "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" ";
	xml += "xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" ";
	xml += "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
	xml += "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" ";
	xml += "xmlns:ns1=\"http://tempuri.org/mons.xsd/Service.wsdl\" ";
	xml += "xmlns:ns2=\"http://tempuri.org/mons.xsd\"> <SOAP-ENV:Body> ";
	xml += "<ns2:ExeSwitchTMSToOther/>";
	xml +="</SOAP-ENV:Body></SOAP-ENV:Envelope>";

	// 通过http方式调用另一个调度软件的WebService服务

// 	char buff[64]={'\0'};
// 	snprintf(buff,128,"http://%s:%d/?wsdl",strIP.c_str(),nPort);
// 	std::string strURI =buff;
	std::string strResponse;
	int nInvokeRes = InvokerWebServer(false,"/",xml,strResponse);
	if( nInvokeRes == ERROR_SENSOR_TCP_RECV || nInvokeRes == ERROR_SENSOR_TCP_CONNECT 
		|| nInvokeRes == ERROR_SENSOR_TCP_SEND)
	{
		// 写错误日志
		return false;
	}

	// 提取xml
	std::string retXml;
	int result = GetHttpContent(strResponse, retXml);

	if(retXml.empty())
	{
		LOGINFFMT("GetOtherMonitorState:Parse Fail! xml is empty!\n");
		return false;
	}

	int nRet ;
	if(ParseXmlFromOtherMonitor(retXml,nRet))
	{
		return nRet == 0 ? true:false;
	}
	else
	{
		return false;
	}
}

// 通过webservice调用对端的切换tms接口
bool CTMSSensor::NotifyTMSSMSSwitch(std::string strHallId,std::string strNewIp,unsigned short port)
{

	std::ostringstream os;
	os<<port;
	std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	xml += "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" ";
	xml += "xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" ";
	xml += "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
	xml += "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" ";
	xml += "xmlns:ns1=\"http://tempuri.org/ns1.xsd\" ";
	xml += "xmlns:ns2=\"http://tempuri.org/mons.xsd\"> <SOAP-ENV:Body> ";
	xml += "<ns1:SwitchSMS><strHallId>"+strHallId+"</strHallId><strNewIp>"+strNewIp+"</strNewIp><port>"+os.str()+"</port></ns1:SwitchSMS>";
	xml +="</SOAP-ENV:Body></SOAP-ENV:Envelope>";

	// 通过http方式调用另一个调度软件的WebService服务
	std::string strResponse;
	std::string strURI = "/";
	int nInvokeRes = InvokerWebServer(true,strURI,xml,strResponse);
	if( nInvokeRes == ERROR_SENSOR_TCP_RECV || nInvokeRes == ERROR_SENSOR_TCP_CONNECT 
		|| nInvokeRes == ERROR_SENSOR_TCP_SEND)
	{
		// 写错误日志
		return false;
	}

	// 提取xml
	std::string retXml;
	int result = GetHttpContent(strResponse, retXml);

	if(retXml.empty())
	{
		LOGINFFMT("GetOtherMonitorState:Parse Fail! xml is empty!\n");
		return false;
	}

	int nRet ;
	if(ParseXmlFromTMS(retXml,nRet))
	{
		return nRet == 0 ? true:false;
	}
	else
	{
		return false;
	}
}

// 解析TMS返回xml
bool  CTMSSensor::ParseXmlFromTMS(std::string &retXml,int &nRet)
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

		// 读取ret节点
		DOMNodeList *ptrNodeList = ptrDoc->getElementsByTagName(C2X("ret"));
		if(ptrNodeList == NULL)
		{
			LOG(ERROR_PARSE_MONITORSTATE_XML,"ParseXmlFromTMS:ret");
			return false;
		}
		else 
		{
			if(ptrNodeList->getLength() == 0)
			{
				LOG(ERROR_PARSE_MONITORSTATE_XML,"ParseXmlFromTMS:ret");
				return false;
			}
			DOMNode* ptrNode = ptrNodeList->item(0);
			std::string str_state =  XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			if(!str_state.empty())
			{
				nRet = atoi(str_state.c_str());
			}
			//LOGINFFMT("%s,%s\n",str_name.c_str(),str_state.c_str());
		}
	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOG(ERROR_PARSE_MONITORSTATE_XML,message);
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

// 解析对端调度软件Webservice返回xml
bool  CTMSSensor::ParseXmlFromOtherMonitor(std::string &retXml,int &nRet)
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

		// 读取ret节点
		DOMNodeList *ptrNodeList = ptrDoc->getElementsByTagName(C2X("ret"));
		if(ptrNodeList == NULL)
		{
			LOG(ERROR_PARSE_MONITORSTATE_XML,"ParseOtherMonitorState:没有找到bMain节点");
			return false;
		}
		else
		{

			DOMNode* ptrNode = ptrNodeList->item(0);
			std::string str_state =  XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			if(!str_state.empty())
			{
				nRet = atoi(str_state.c_str());
			}
			//LOGINFFMT("%s,%s\n",str_name.c_str(),str_state.c_str());
		}
	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOG(ERROR_PARSE_MONITORSTATE_XML,message);
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

// 发送webservice调用消息
int CTMSSensor::SendAndRecvResponse(bool bTMSWS,const std::string &request, std::string &response, int delayTime)
{
	if(m_strIP.empty())
	{
		return 0;
	}

	TcpTransport tcp;
	int result= -1;
	if(bTMSWS)
	{
		result = tcp.TcpConnect("127.0.0.1", m_nTMSWBPort,delayTime);

	}
	else
	{
		result = tcp.TcpConnect(m_strIP.c_str(), m_nPort,delayTime);
	}

	if(result < 0)
	{	
		LOGINFFMT("CMonitorSensor::SendAndRecvResponse TcpConnect %s:%d Fail !\n",m_strIP.c_str(), m_nPort);
		return  ERROR_SENSOR_TCP_CONNECT;
	}

	result = tcp.BlockSend(request.c_str(), request.size());
	if(result < 0)
	{
		LOGINFFMT("CMonitorSensor::SendAndRecvResponse Tcp Send %s Fail !\n",request.c_str());
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

// 获取对端调度软件webservcie接口返回的实际内容
int CTMSSensor::GetHttpContent(const std::string &http, std::string &response)
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


// 调用对端webservice的接口
int CTMSSensor::InvokerWebServer(bool bTMSWS,std::string strURI,std::string &xml,std::string &strResponse)
{
	HttpRequestParser request;
	request.SetMethod("POST");
	request.SetUri(strURI.c_str());
	request.SetVersion("HTTP/1.1");
	request.SetHost(m_strIP.c_str());
	request.SetContentType("text/xml; charset=utf-8");
	request.SetContent(xml);
	request.SetSoapAction("");
	std::string strHttp = request.GetHttpRequest();

	int result = SendAndRecvResponse(bTMSWS,strHttp, strResponse);

	return result;

}
