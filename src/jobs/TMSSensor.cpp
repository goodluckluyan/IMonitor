/***********************************************************
Copyright (C), 2010-2020, DADI MEDIA Co.,Ltd.
ModuleName: SoftwareState.h
FileName: SoftwareState.h
Author: chengyu
Date: 14/09/19
Version:
Description: 获取SMS,TMS 状态信息
Others:
History:
		<Author>		<Date>		<Modification>
		chengyu			14/09/19
***********************************************************/
#include "TMSSensor.h"

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
#include "para/C_Para.h"
#include "utility/C_TcpTransport.h"
#include "utility/C_HttpParser.h"
#include "C_ErrorDef.h"
#include "MonitorSensor.h"
#include "log/C_LogManage.h"

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
bool CTMSSensor ::Init(std::string strURI,std::string strIP,int nPort)
{
	if(strURI.empty() || strIP.empty() || nPort <= 0)
	{
		return false;
	}
	m_strURI = strURI;
	m_strIP = strIP;
	m_nPort = nPort;

	try
	{
		xercesc::XMLPlatformUtils::Initialize();
	}
	catch( xercesc::XMLException& e )
	{
		char* message = xercesc::XMLString::transcode( e.getMessage() );
		xercesc::XMLString::release( &message );
	}

	if(C_Para::GetInstance()->m_bMain)
	{
		StartTMS();
	}
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
		printf("failed to popen %s:%s\n",command,strerror(errno));
		state = -1;
		return 0;//////////////return -1;
	}
	
	if(fgets(buf,19,fp)!=NULL)
	{
		printf("[ Tms20_DeviceService ] Pid = %s\n",buf);
		int result(0);
		result = sscanf( buf,"%d\n", &m_nPid );
		if(result == 0)//sscanf失败，则返回0
		{
			string error = "Error:pidof -s  Tms20_DeviceService\n";
			printf( "%s" , error.c_str() );
			state = -1;
		}
// 		if ( pid != 0 )
// 		{
// 			fprintf( stderr, "[ %s ] is: <%u>\n", "Tms20_DeviceService", ( unsigned int )pid );  
// 			printf( "[ %s ] is: <%u>\n", "Tms20_DeviceService", ( unsigned int )pid  );
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
		printf("Call Standby SwitchTMS Interface !\n");
		if( CallStandbySwitchTMS())
		{
			bRet &= true;
			printf("Call Standby SwitchTMS Interface OK!\n");
		}
		else
		{
			bRet &= false;
			printf("Call Standby SwitchTMS Interface Fail!\n");
		}
	}
	return bRet;
}

bool CTMSSensor::ShutDownTMS()
{
	if(m_nPid > 0)
	{
		int nRet = kill(m_nPid,9);
		if(nRet == 0)
		{
			printf("Kill Local TMS(%d) Done!\n",m_nPid);
		}
		return nRet == 0 ? true: false;
	}
	else
	{
		return false;
	}
}
bool CTMSSensor::StartTMS()
{
	int nStartType = C_Para::GetInstance()->m_nStartSMSType;
	if(nStartType == 1)
	{
		StartTMS_CurTerminal();
	}
	else if(nStartType == 2)
	{
		StartTMS_NewTerminal();
	}
}

int CTMSSensor::Getpid(std::string strName,std::vector<int>& vecPID)
{	
	char acExe[64]={'\0'};
	snprintf(acExe,64,"pidof %s",strName.c_str());
	FILE *pp = popen(acExe,"r");
	if(!pp)
	{
		printf("popen fail\n");
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
bool CTMSSensor::StartTMS_NewTerminal()
{

	std::string strTMSPath = C_Para::GetInstance()->m_strTMSPath;
	if(strTMSPath.empty())
	{
		return false;
	}

	int nPos = strTMSPath.rfind('/');
	std::string strEXE = strTMSPath.substr(nPos+1);
	std::string strDir = strTMSPath.substr(0,nPos);
	std::vector<int> vecCurPID;
	if(Getpid(strEXE,vecCurPID) < 0)
	{
		return false;
	}

	char buf[256]={'\0'};
	snprintf(buf,256,"gnome-terminal --working-directory=%s -e \"%s\"",strDir.c_str(),strTMSPath.c_str());
	//	snprintf(buf,256,"gnome-terminal -e \"%s\"","/usr/bin/top");
	printf("%s\n",buf);
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
				printf("Fork Process(%d) Start SMS ... \n",exepid);
				break;
			}
		}

		time_t tm2;
		time(&tm2);
		if( tm2-tm1 > 5)
		{
			printf("waiting 5 sec ,but SMS not run..\n");
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

bool CTMSSensor::StartTMS_CurTerminal()
{
	std::string strTMSPath = C_Para::GetInstance()->m_strTMSPath;
	if(strTMSPath.empty())
	{
		return false;
	}

	pid_t pid;
	if((pid = fork()) < 0)
	{
		perror("failed to create process/n");
		return false;
	}
	else if(pid == 0)
	{
		printf("Fork Process(%d) Start TMS ... \n",getpid());
		if(execl(strTMSPath.c_str(),"Tms20_DeviceService","",NULL) < 0)
		{
			perror("execl error");
			exit(0);
		}
	}

	m_nPid = pid;
	return true;
}


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
	std::string strResponse;
	int nInvokeRes = InvokerWebServer(xml,strResponse);
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
		printf("GetOtherMonitorState:Parse Fail! xml is empty!\n");
		return false;
	}

	int nRet ;
	if(ParseXml(retXml,nRet))
	{
		return nRet == 0 ? true:false;
	}
	else
	{
		return false;
	}

}

bool  CTMSSensor::ParseXml(std::string &retXml,int &nRet)
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
			C_LogManage::GetInstance()->WriteLog(0,18,0,ERROR_PARSE_MONITORSTATE_XML,"ParseOtherMonitorState:没有找到bMain节点");
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
			//printf("%s,%s\n",str_name.c_str(),str_state.c_str());
		}
	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		C_LogManage::GetInstance()->WriteLog(0,18,0,ERROR_PARSE_MONITORSTATE_XML,message);
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

int CTMSSensor::SendAndRecvResponse(const std::string &request, std::string &response, int delayTime)
{
	if(m_strIP.empty())
	{
		return 0;
	}

	TcpTransport tcp;
	int result = tcp.TcpConnect(m_strIP.c_str(), m_nPort);
	if(result < 0)
	{	
		printf("CMonitorSensor::SendAndRecvResponse TcpConnect %s:%d Fail !\n",m_strIP.c_str(), m_nPort);
		return  ERROR_SENSOR_TCP_CONNECT;
	}

	result = tcp.BlockSend(request.c_str(), request.size());
	if(result < 0)
	{
		printf("CMonitorSensor::SendAndRecvResponse Tcp Send %s Fail !\n",request.c_str());
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

int CTMSSensor::InvokerWebServer(std::string &xml,std::string &strResponse)
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
