
#include "C_Hall.h"
#include <algorithm>
#include <string>
#include <cstdlib>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include "para/C_Para.h"
#include "C_ErrorDef.h"
#include "log/C_LogManage.h"

#define  LOG(errid,msg)  C_LogManage::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_JOBS,0,errid,msg)
#define  LOGERRFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_ERROR,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)
#define  LOGINFFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_INFO,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)

#define BUFFLEN  2048

extern int g_nRunType;
using namespace std;
using namespace xercesc;
C_Hall::C_Hall(SMSInfo &stSms)
{
   m_SMS = stSms;
   m_bInitRun = false;
   m_pid = 0;
}

C_Hall::~C_Hall()
{
}



// 初始化
SMSInfo&  C_Hall::Init(bool bRun,int nPID)
{
	 m_bInitRun = bRun;
	 if(bRun && nPID == 0)
	 {
		 // 本机运行
		int nPid;
		StartSMS(nPid);
		 m_SMS.stStatus.nRun = 1;
	 }
	 else if(bRun && nPID != 0)
	 {
		 // 本机运行
		m_pid = nPID;
		m_SMS.stStatus.nRun = 1;	
	 }
	 else if(!bRun && nPID != 0)//不应在本机运行但在本机运行，则也认为是本机的sms
	 {
		 m_pid = nPID;
		 m_SMS.stStatus.nRun = 1;
		 int nRole = C_Para::GetInstance()->GetRole();
		 if(nRole==(int)MAINROLE)
		 {
			 m_SMS.nRole =  (int)MAINRUNTYPE;
		 }
		 else
		 {
			 (int)STDBYRUNTYPE;
		 }
	 }
	 else 
	 {
		 // 另一台机子运行
		m_SMS.stStatus.nRun = 2;
	 }
	return m_SMS;
}

// 改变SMS的运行主机信息	
SMSInfo& C_Hall::ChangeSMSHost(std::string strIP,int nRunType,bool bLocalRun)
{
	m_SMS.strIp = strIP;
	m_SMS.stStatus.nRun = bLocalRun ? 1:2;
	m_SMS.nRole = nRunType;
	return m_SMS;
}

// 获取运行主机及webservice端口
void C_Hall::GetRunHost(std::string &strIP,int &nPort)
{
	strIP = m_SMS.strIp;
	nPort = m_SMS.nPort;
}

// 启动SMS
bool C_Hall::StartSMS(int &nPid)
{
	int nStartType = C_Para::GetInstance()->m_nStartSMSType;
	if(nStartType == 1 || 1 == g_nRunType)
	{
		return StartSMS_CurTerminal(nPid);
	}
	else if(nStartType == 2 && 0 == g_nRunType)
	{
		return StartSMS_NewTerminal(nPid);
	}
}



// 在当前终端启动SMS
bool C_Hall::StartSMS_CurTerminal(int &nPid)
{
	
	if(m_SMS.strExepath.empty())
	{
		return false;
	}

	struct rlimit rl;
	if(getrlimit(RLIMIT_NOFILE,&rl)<0)
	{
		return false;
	}

	pid_t pid ;
	if((pid = fork()) < 0)
	{
		perror("failed to create process/n");
		return false;
	}
	else if(pid == 0)
	{

		// 关闭所有父进程打开的文件描述符，以免子进程继承父进程打开的端口。
		if(rl.rlim_max == RLIM_INFINITY)
		{
			rl.rlim_max = 1024;
		}
		for(int i = 3 ;i < rl.rlim_max;i++)
		{
			close(i);
		}

		LOGINFFMT(0,"Fork Process(%d) Start SMS(%s) ... \n",getpid(),m_SMS.strId.c_str());
		int nPos = m_SMS.strExepath.rfind('/');
		std::string strEXE = m_SMS.strExepath.substr(nPos+1);	
		std::string strDir = m_SMS.strExepath.substr(0,nPos);
		chdir(strDir.c_str());
		strEXE += "&";
		if(!strEXE.empty() && execl(m_SMS.strExepath.c_str(),strEXE.c_str(),
		m_SMS.strConfpath.c_str(),NULL) < 0)
		{
			perror("execl error");
			exit(0);
		}
	}

	//等待2秒
	sleep(2);

	if(pid > 0)
	{	
        m_pid = pid;
		nPid = pid;
	}
	
	return true;
}

// 获取指定命令的pid
int C_Hall::Getpid(std::string strName,std::vector<int>& vecPID)
{	
	char acExe[256]={'\0'};
	snprintf(acExe,sizeof(acExe),"ps -ef|grep %s|grep -v \"grep %s\"|awk '{print $2}'",strName.c_str(),strName.c_str());
	FILE *pp = popen(acExe,"r");
	if(!pp)
	{
		LOG(0,"popen fail\n");
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
bool C_Hall::StartSMS_NewTerminal(int &nPid)
{

	if(m_SMS.strExepath.empty())
	{
		return false;
	}
	int nPos = m_SMS.strExepath.rfind('/');
	std::string strEXE = m_SMS.strExepath.substr(nPos+1);
	std::string strDir = m_SMS.strExepath.substr(0,nPos);
	std::vector<int> vecCurPID;
	if(Getpid(strEXE,vecCurPID) < 0)
	{
		LOGERRFMT(0,"StartSMS_NewTerminal Getpid Failed (Start SMS:%s)!",m_SMS.strId.c_str());
		return false;
	}
	
	char buf[256]={'\0'};
 	snprintf(buf,sizeof(buf),"sudo gnome-terminal --working-directory=%s --title=\"sms_%s\" -e \"%s %s\"",strDir.c_str(),m_SMS.strId.c_str()
		,m_SMS.strExepath.c_str(),m_SMS.strConfpath.c_str());
//	snprintf(buf,256,"gnome-terminal -e \"%s\"","/usr/bin/top");
	LOGINFFMT(0,"%s\n",buf);
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
		{
			LOGERRFMT(0,"StartSMS_NewTerminal Getpid Failed (Start SMS:%s)!",m_SMS.strId.c_str());
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
				LOGINFFMT(0,"Fork Process(%d) Start SMS_%s ... \n",exepid,m_SMS.strId.c_str());
				break;
			}
		}

		time_t tm2;
		time(&tm2);
		if( tm2-tm1 > 5)
		{
			LOG(0,"waiting 5 sec ,but SMS not run..\n");
			break;
		}
	}
	
	if(exepid > 0)
	{	
		m_pid = exepid;
		nPid = exepid;
		return true;
	}
	else
	{
		return false;
	}
}

// 关闭SMS
bool C_Hall::ShutDownSMS()
{
	if(m_pid > 0)
	{
		int nRet = kill(m_pid,9);
		if(nRet == 0)
		{
			LOGINFFMT(0,"Kill Local SMS(%d) Done!\n",m_pid);
		}
		return nRet == 0 ? true: false;
	}
	else
	{
		return false;
	}

}

int C_Hall::ISSMSRun()
{
	char buf[64]={'\0'};
	snprintf(buf,sizeof(buf),"ps %d|grep %d|awk '{print $1}'",(int)m_pid,(int)m_pid);
	
	FILE *pp = popen(buf,"r");
	if(!pp)
	{
		LOG(0,"popen fail\n");
		return -1;
	}
	char tmpbuf[128]={'\0'};
	std::vector<std::string> vecBuf;
	while(fgets(tmpbuf,sizeof(tmpbuf),pp)!=NULL)
	{
		vecBuf.push_back(tmpbuf);
	}
	pclose(pp);
	LOGINFFMT(0,"%s (%d)",buf,vecBuf.size());
	return vecBuf.size()>0;
}

//获取SMS 运行状态
int C_Hall::GetSMSWorkState( int &state, string &info)
{
	//初始化获取AQ33的ip和port
	if (m_SMS.strIp.empty())
	{
		return -1;
	}

	if(IsLocal())
	{
		int n = -1;
		if(!ISSMSRun())
		{
			int i = 0;
			while(!ISSMSRun() && i<3)
			{
				i++;
			}
			
			if(3==i)
			{
				LOGERRFMT(0,"Check SMS(%s) was Shutdown ,Start It!",m_SMS.strId.c_str());
				int nPID;
				StartSMS(nPID);
			}
			
		}
	}
	

	std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	xml += "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" ";
	xml += "xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" ";
	xml += "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
	xml += "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" ";
	xml += "xmlns:ns1=\"http://tempuri.org/mons.xsd/Service.wsdl\" ";
	xml += "xmlns:ns2=\"http://tempuri.org/mons.xsd\"> <SOAP-ENV:Body> ";
	xml += "<ns1:GetWorkState_CS></ns1:GetWorkState_CS>";
	xml +="</SOAP-ENV:Body></SOAP-ENV:Envelope>";
	
	//refer to wsdl file.
	std::string strUsherLocation = "/oristar/services/Usher";
	std::string strUsherNs = "http://webservices.oristar.com/XP/Usher/2009-09-29/GetWorkState_CS";

	int iResult;
	string response_c;
	string content_c;
	string http;
	LOGINFFMT(0,"GetSMSWorkState %s:%s",m_SMS.strId.c_str(),m_SMS.strIp.c_str());
	UsherHttp(strUsherLocation,m_SMS.strIp, xml, strUsherNs,http);
	iResult = TcpOperator(m_SMS.strIp,m_SMS.nPort, http, response_c, 5);
	if (iResult != 0)
	{
		return iResult;//SoftwareSTATE_ERROR_TCP
	}

	iResult = GetHttpContent( response_c, content_c);
	if (iResult < 0)
	{
		return iResult;//SoftwareSTATE_ERROR_HTTP
	}
	//java定义为,如果sms连接异常, 返回的xml, http抛出500。定义状态为102
	else if (iResult == 500)
	{
		state = 102;
		info = "error";
		return 0;
	}
	
	int nPos = content_c.find("<?");
	std::string strContent = content_c.substr(nPos,content_c.size()-nPos);
	iResult = Parser_GetSMSWorkState( strContent, state, info);
	if (iResult != 0)
	{
		return iResult;//SoftwareSTATE_ERROR_XMLPARSER
	}
	return 0;
}

// 调用对端调度软件的关闭sms接口
// 解析开启或关闭结果 bSoC true：开启  false 关闭 
int  C_Hall::CallStandbyStartOrCloseSMS(bool bSoC,std::string strOtherIP,int nPort,std::string strHallID)
{
	int iResult;
	string response_c;
	string content_c;

	if (strOtherIP.empty())
	{
		return -1;
	}
	std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	xml += "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" ";
	xml += "xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" ";
	xml += "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
	xml += "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" ";
	xml += "xmlns:ns1=\"http://tempuri.org/mons.xsd/Service.wsdl\" ";
	xml += "xmlns:ns2=\"http://tempuri.org/mons.xsd\"> <SOAP-ENV:Body> ";
	if(bSoC)
	{
		xml += "<ns2:ExeStartSMS><strHallID>"+strHallID+"</strHallID></ns2:ExeStartSMS>";
	}
	else
	{
		xml += "<ns2:ExeCloseSMS><strHallID>"+strHallID+"</strHallID></ns2:ExeCloseSMS>";
	}
	xml +="</SOAP-ENV:Body></SOAP-ENV:Envelope>";

	string http ;
	// 	char buff[64]={'\0'};
	// 	snprintf(buff,128,"http://%s:%d/?wsdl",strOtherIP.c_str(),nPort);
	string strURI = "/";//buff;
	UsherHttp(strURI,strOtherIP,xml,"",http);

	iResult = TcpOperator(strOtherIP,nPort, http, response_c, 5);
	if (iResult != 0)
	{
		return iResult;//SoftwareSTATE_ERROR_TCP
	}

	iResult = GetHttpContent( response_c, content_c);
	if (iResult != 0)
	{
		return iResult;//SoftwareSTATE_ERROR_HTTP
	}

	int nRet;
	iResult = Parser_StartOrCloseSMS(bSoC,content_c,nRet );
	if(iResult!=0)
	{
		return iResult;
	}
	return nRet;
}


// 解析开启或关闭结果 bCoS true：开启  false 关闭 
int C_Hall::Parser_StartOrCloseSMS(bool bSoC,std::string &content,int &nRet)
{
	XercesDOMParser *parser = new XercesDOMParser();
	ErrorHandler *errHandler = (ErrorHandler*) new HandlerBase();
	DOMElement *rootChild = NULL;

	int result = GetRootChild( content, parser, errHandler, &rootChild);
	if (result < 0 || rootChild == NULL)
		return -1;

	
	DOMElement *child ;
	if(bSoC)
	{
		child = GetElementByName(rootChild->getFirstChild(), "ExeStartSMS");
	}
	else
	{
		child = GetElementByName(rootChild->getFirstChild(), "ExeCloseSMS");
	}

	if(child == NULL)
	{
		return ERROR_PLAYER_AQ_NEEDSOAPELEM;
	}

	char *p;
	DOMElement *root = GetElementByName(child->getFirstChild(), "ret");
	if ( child == NULL || child->getFirstChild() == NULL || child->getFirstChild()->getNodeValue() == NULL)
	{
		return ERROR_PLAYER_AQ_NEEDSOAPELEM;
	}
	p = (char *)XMLString::transcode(root->getFirstChild()->getNodeValue());
	nRet = atoi(p);
	XMLString::release( &p);


	delete errHandler;
	delete parser;
	return result;
}


// 调用对端调度软件的切换接口
int  C_Hall::CallStandbySwitchSMS(bool bDelaySwitch,std::string strOtherIP,int nPort,std::string strHallID)
{
	int iResult;
	string response_c;
	string content_c;

	if (strOtherIP.empty())
	{
		return -1;
	}
	std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	xml += "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" ";
	xml += "xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" ";
	xml += "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
	xml += "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" ";
	xml += "xmlns:ns1=\"http://tempuri.org/mons.xsd/Service.wsdl\" ";
	xml += "xmlns:ns2=\"http://tempuri.org/mons.xsd\"> <SOAP-ENV:Body> ";
	if(bDelaySwitch)
	{
		xml += "<ns2:ExeSwitchSMSToOtherDelay><strHallID>"+strHallID+"</strHallID></ns2:ExeSwitchSMSToOther>";
	}
	else
	{
		xml += "<ns2:ExeSwitchSMSToOther><strHallID>"+strHallID+"</strHallID></ns2:ExeSwitchSMSToOther>";
	}
	xml +="</SOAP-ENV:Body></SOAP-ENV:Envelope>";

	string http ;
// 	char buff[64]={'\0'};
// 	snprintf(buff,128,"http://%s:%d/?wsdl",strOtherIP.c_str(),nPort);
 	string strURI = "/";//buff;
	UsherHttp(strURI,strOtherIP,xml,"",http);

	iResult = TcpOperator(strOtherIP,nPort, http, response_c, 5);
	if (iResult != 0)
	{
		return iResult;//SoftwareSTATE_ERROR_TCP
	}

	iResult = GetHttpContent( response_c, content_c);
	if (iResult != 0)
	{
		return iResult;//SoftwareSTATE_ERROR_HTTP
	}

	int nRet;
	iResult = Parser_SwitchSMS(content_c,nRet );
	return nRet;
}


// 解析切换结果
int C_Hall::Parser_SwitchSMS(std::string &content,int &nRet)
{
	XercesDOMParser *parser = new XercesDOMParser();
	ErrorHandler *errHandler = (ErrorHandler*) new HandlerBase();
	DOMElement *rootChild = NULL;

	int result = GetRootChild( content, parser, errHandler, &rootChild);
	if (result < 0 || rootChild == NULL)
		return -1;


	DOMElement *child = GetElementByName(rootChild->getFirstChild(), "ExeSwitchSMSToOtherResponse");
	if(child == NULL)
	{
		return ERROR_PLAYER_AQ_NEEDSOAPELEM;
	}

	char *p;
	DOMElement *root = GetElementByName(child->getFirstChild(), "ret");
	if ( child == NULL || child->getFirstChild() == NULL || child->getFirstChild()->getNodeValue() == NULL)
	{
		return ERROR_PLAYER_AQ_NEEDSOAPELEM;
	}
	p = (char *)XMLString::transcode(root->getFirstChild()->getNodeValue());
	nRet = atoi(p);
	XMLString::release( &p);


	delete errHandler;
	delete parser;
	return result;
}

// 填充http头
int C_Hall::UsherHttp(std::string& strURI,std::string& strIP,std::string &xml,std::string action,std::string &strRequest)
{
	HttpRequestParser request;
	request.SetMethod("POST");
	request.SetUri( strURI.c_str());
	request.SetVersion("HTTP/1.1");
	request.SetHost(strIP.c_str());
	request.SetContentType("text/xml; charset=utf-8");
	request.SetContent(xml);
	if(!action.empty())
	{
		request.SetSoapAction(action.c_str());
	}
	else
	{
		request.SetSoapAction("");
	}
	strRequest = request.GetHttpRequest();

	return 0;
}

// 提取返回的xml
int C_Hall::GetHttpContent(const string &http, string &content)
{
	//http protocol
	HttpResponseParser response;
	int result = response.SetHttpResponse(http);
	if(result < 0)
	{
		return ERROR_PLAYER_AQ_BADHTTPRESPONSE;//
	}

	content = response.GetContent();
	int status = response.GetStatus();

	return status;
}

//解析SMS返回的XML
int C_Hall::Parser_GetSMSWorkState( const string &content, int &state, string &info)
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
			
		DOMNodeList *ptrNodeList = ptrDoc->getElementsByTagName(XMLString::transcode ("state"));
		if ( ptrNodeList == NULL)
		{
			return ERROR_PLAYER_AQ_NEEDSOAPELEM;
		}
		else
		{
			DOMNode* ptrNode = ptrNodeList->item(0);
			char *pstate =  XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state = pstate;
			if(!str_state.empty())
			{
				state = atoi(str_state.c_str());
			}

			XMLString::release( &pstate);
		}

		DOMNodeList *ptrInfoNodeList = ptrDoc->getElementsByTagName(XMLString::transcode ("info"));
		if ( ptrNodeList == NULL)
		{
			return ERROR_PLAYER_AQ_NEEDSOAPELEM;
		}
		else
		{
			DOMNode* ptrNode = ptrInfoNodeList->item(0);
			char * pinfo =  XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string strInfo = pinfo;
			if(!strInfo.empty())
			{
				info = strInfo;
			}
			XMLString::release( &pinfo);
		}
	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOG(ERROR_PARSE_MONITORSTATE_XML,message);
		delete ptrParser;
		delete ptrInputsource;
		
	}

	delete ptrParser;
	delete ptrInputsource;
	return true;
}



//发送与接收 数据/////////////////////////////////////////////////////////////////
int C_Hall::TcpOperator(std::string strIp,int nPort ,const string &send, string &recv,int overtime)
{
	int result = m_tcp.TcpConnect(strIp.c_str(), nPort);
	if(result < 0)
	{
		//SetAq10Error(-ERROR_PLAYER_CTRL_DEVICECONNECTFAILED, "Player can not connect! 01");
		return ERROR_PLAYER_AQ_TCPCONNECT;
	}

	result = SendAndRecvInfo(send, recv, overtime);

	if(m_tcp.BeConnected())
		m_tcp.ReleaseConnect();
	return result;
}


//发送与接收 数据/////////////////////////////////////////////////////////////////
int C_Hall::SendAndRecvInfo(const string &send, string &recv, int overtime)
{
	int result = m_tcp.BlockSend(send.c_str(), send.size());
	if(result < 0)
	{
		//SetAq10Error(ERROR_PLAYER_AQ_TCPSEND, "Player can not connect! 03");
		return -1;//ERROR_PLAYER_AQ_TCPSEND;
	}

	string http;
	string content;
	result = ReceiveCommand(http, overtime);
	if(result < 0)
		return result;

	result = GetHttpContent(http, content);
	if(result < 0)
		return result;
	else if(result < HttpResponseParser::RecvAndContinue)
	{
		//SetAq10Error(ERROR_PLAYER_AQ_BADHTTPRESPONSE, "bad http response! 01");
		return -1;//ERROR_PLAYER_AQ_BADHTTPRESPONSE;
	}
	else if(result >= HttpResponseParser::Success)
	{//if status code is 200, return.
		recv = http;
		return 0;
	}
	else
		;//if status code is 100

	if(!content.empty())
	{//if one time operatoring receive two responses.
		http = content;
		content.clear();
		result = GetHttpContent(http, content);
		if(result >= HttpResponseParser::Success)
		{//Get second response, return.
			recv = http;
			return 0;
		}
	}

	http.clear();
	content.clear();
	//receive second response.
	int waitTime = ((overtime > 120) ? overtime : 60);
	result = ReceiveCommand(http, waitTime);//wait 60 second
	if(result < 0)
	{
		return result;
	}
	
	result = GetHttpContent(http, content);


	if(result < 0)
		return result;

	else if(result < HttpResponseParser::Success)
	{
		//SetAq10Error(ERROR_PLAYER_AQ_HTTPUNKNOW, "bad http response! 02");
		return -1;//ERROR_PLAYER_AQ_HTTPUNKNOW;
	}
	else
	{
		recv = http;
		return 0;
	}
}

// 接收webservice调用返回
int C_Hall::ReceiveCommand(string &recv, int waitTime)
{
	recv.clear();
	char buffer[BUFFLEN];
	timeval timeOut;
	timeOut.tv_sec = waitTime;
	timeOut.tv_usec = 0;

	int result = 0;
	while((result = m_tcp.SelectRecv(buffer, BUFFLEN-1, timeOut)) >= 0)
	{
		if(result == 0)
			break;
		buffer[result] = 0;
		recv += buffer;

		//find the terminator.
		if(recv.find("</soap:Envelope>") != string::npos || recv.find("</s:Envelope>") != string::npos ||
			recv.find("</soapenv:Envelope>") != string::npos)
			break;

		timeOut.tv_sec = 1;//1 second
		timeOut.tv_usec = 0;
	}
	if(result < 0)
	{
		return -1;//ERROR_PLAYER_AQ_TCPRECEIVE;
	}
	return 0;
}


int C_Hall::GetRootChild( const std::string &xml,xercesc::XercesDOMParser *parser, 
								  xercesc::ErrorHandler *errHandler, xercesc::DOMElement **rootChild)
{
	string compactXml = ExtractXml(xml);
	if(compactXml.empty())
		return -1;

	parser->setErrorHandler(errHandler);
	MemBufInputSource input((const XMLByte*)(compactXml.c_str()), compactXml.size(), "xml");
	try
	{
		parser->parse(input);
	}
	catch(...)
	{
		return -2;
	}

	DOMDocument *doc = parser->getDocument();

	if(doc == NULL)
	{
		return -3;
	}
	*rootChild = doc->getDocumentElement();
	return *rootChild == NULL ? -1 : 0;
}

//GET NODE NAME
xercesc::DOMElement *C_Hall::GetElementByName( const xercesc::DOMNode *elem, const std::string &name)
{
	if(name.empty() || elem == NULL)
		return NULL;

	string source = name;
	for_each(source.begin(), source.end(), ToLower);

	for(DOMElement *child = (DOMElement*)elem; child != NULL;)
	{
		char *p = (char *)XMLString::transcode(child->getNodeName());
		string dest = p;
		XMLString::release(&p);

		for_each(dest.begin(), dest.end(), ToLower);
		if(source == dest)
		{
			return child;
		}

		child = (DOMElement *)child->getNextSibling();
	}
	return NULL;
}

xercesc::DOMElement *C_Hall::FindElementByName( const xercesc::DOMNode *elem, const std::string &name)
{
	if(name.empty() || elem == NULL)
		return NULL;

	string source = name;
	for_each(source.begin(), source.end(), ToLower);

	for(DOMElement *child = (DOMElement*)elem; child != NULL;
		child = (DOMElement *)child->getNextSibling())
	{
		char *p = (char *)XMLString::transcode(child->getNodeName());
		string dest = p;
		XMLString::release(&p);

		for_each(dest.begin(), dest.end(), ToLower);
		if(dest.find(source) != string::npos)
			return child;
	}
	return NULL;
}

string C_Hall::ExtractXml(const string &response)
{
	string xml;
	std::string::size_type beg = response.find('<'), end;
	while(beg != string::npos && (end = response.find('>', beg)) != string::npos)
	{//if node value's first char is '\n' '\t' '\r' or blank, here will make a mistake 
		xml += response.substr(beg, end - beg + 1);
		beg = response.find('<', end);
		if(beg == string::npos)
			break;
		if(!(++end==beg || response[end]==' ' || response[end]=='\n' || response[end]=='\t' || response[end]=='\r'))
			xml += response.substr(end, beg - end);
	}

	return xml;
}
