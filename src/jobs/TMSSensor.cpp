

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
#include "database/CppMySQL3DB.h"


#define  LOGFMT(level,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(level,LOG_MODEL_JOBS,0,0,fmt,##__VA_ARGS__)
#define  LOGIDFMT(level,errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(level,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)

using namespace std;
using namespace xercesc;

extern int g_nRunType;
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

	//StartTMS();
	return true;
}


//获取TMS 运行状态
int CTMSSensor::GetTMSPID()
{	
	int state = 0;
	char buf[256] = {0};
	char command[] = "ps -ef|grep Tms20_DeviceService|grep -v 'grep Tms20_DeviceService'|awk '{print $2}'";//"pidof  -s  Tms20_DeviceService";

	m_nPid = 0; //-1;
	
	FILE *fp = popen(command,"r");
	if(fp==NULL)
	{
		LOGFMT(ULOG_ERROR,"failed to popen %s:%s\n",command,strerror(errno));
		state = -1;
		return 0;//////////////return -1;
	}
	
	if(fgets(buf,sizeof(buf)-1,fp)!=NULL)
	{
		LOGFMT(ULOG_INFO,"[ Tms20_DeviceService ] Pid = %s",buf);
		int result(0);
		int pid = atoi(buf);
		if( 0 != pid && pid != m_nPid)
		{
			m_nPid = pid;
		}
		if(result == 0)//sscanf失败，则返回0
		{
			string error = "Error:pidof -s  Tms20_DeviceService\n";
			LOGFMT(ULOG_ERROR, "%s" , error.c_str() );
			state = -1;
		}
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

	if(C_Para::GetInstance()->IsMain())
	{
		LOGFMT(ULOG_INFO,"Call Standby SwitchTMS Interface !");
		if( CallStandbySwitchTMS())
		{
			bRet &= true;
			LOGFMT(ULOG_INFO,"Call Standby SwitchTMS Interface OK!");
		}
		else
		{
			bRet &= false;
			LOGFMT(ULOG_ERROR,"Call Standby SwitchTMS Interface Fail!");
		}
	}
	return bRet;
}


// 关闭tms
bool CTMSSensor::ShutDownTMS()
{
	bool bRet = false;
	if(m_nPid > 0)
	{
		int i=0;
		while(i<3)
		{
			kill(m_nPid,9);
			sleep(1);
			std::vector<int> vecPID;
			int nRet = Getpid("Tms20_DeviceService",vecPID);
			if(nRet == 0 && vecPID.size()==0)
			{
				LOGFMT(ULOG_INFO,"Kill Local TMS(%d) Done!\n",m_nPid);
				bRet = true;
				break;
			}
			i++;
		}
		
		return bRet;
	}
	else
	{
		return false;
	}
}

// 启动tms
bool CTMSSensor::StartTMS()
{
	if(!C_Para::GetInstance()->IsMain())
	{
		LOGFMT(ULOG_ERROR,"Standby Host can't start TMS!\n");
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
		int i = 0;
		while(i<3)
		{
			int nState = -1;
			std::string strInfo;
			if(GetTMSWorkState())
			{
				break;
			}
			LOGFMT(ULOG_ERROR,"StartTMS GetTMSWorkState Fail!");
			i++;
		}

		// 三次获取状态失败才开启TMS
		if(i<3)
		{
			LOGFMT(ULOG_ERROR,"StartTMS:TMS Run Normal On Other Host ,Can't Local Run!");
			return false;
		}
		
		 C_Para * ptr = C_Para::GetInstance();
		 int nRole=ptr->GetRole();
		 if(nRole==MAINROLE || nRole == ONLYMAINROLE)
		 {
			 UpdateDataBaseTMSPos(0);
		 }
		 else
		 {
			 UpdateDataBaseTMSPos(1);
		 }

		int nStartType =ptr->m_nStartSMSType;
		if(1 == nStartType || 1 == g_nRunType)
		{
			StartTMS_CurTerminal(strTMSPath);
		}
		else if(2 == nStartType &&0 == g_nRunType)
		{
			StartTMS_NewTerminal(strTMSPath);
		}

		// 启动后把通知切换信息发给tms
// 		if(m_nPid > 0 && m_vecSwitchInfo.size() > 0)
// 		{
// 			std::vector<stNotifySmsSwitchInfo>::iterator it = m_vecSwitchInfo.begin();
// 			for( ;it != m_vecSwitchInfo.end() ;)
// 			{
// 				stNotifySmsSwitchInfo &stNode = *it;
// 				int i = 0;
// 				while(i<3)
// 				{
// 					bool bRet = NotifyTMSSMSSwitch(stNode.strHallId,stNode.strNewIp,stNode.port);
// 					if(bRet)
// 					{
// 						break;
// 					}
// 					i++;
// 				}
// 				m_vecSwitchInfo.erase(it++);
// 			}
// 		}
	}
}

// 获取指定进程的pid
int CTMSSensor::Getpid(std::string strName,std::vector<int>& vecPID)
{	
	char acExe[256]={'\0'};
	snprintf(acExe,sizeof(acExe),"ps -ef|grep %s|grep -v 'grep %s'|awk '{print $2}'",strName.c_str(),strName.c_str());
	FILE *pp = popen(acExe,"r");
	if(!pp)
	{
		LOGFMT(ULOG_ERROR,"%s popen fail",acExe);
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
	snprintf(buf,sizeof(buf),"sudo gnome-terminal --title=\"%s\" --working-directory=%s -e \"%s\"",
		"TMS",strDir.c_str(),strTMSPath.c_str());
	LOGFMT(ULOG_INFO,"%s\n",buf);
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
			LOGFMT(LOG_LERROR,"StartTMS_NewTerminal Getpid Failed !");
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
				LOGFMT(ULOG_INFO,"Fork Process(%d) Start TMS ... \n",exepid);
				break;
			}
		}

		time_t tm2;
		time(&tm2);
		if( tm2-tm1 > 5)
		{
			LOGFMT(ULOG_INFO,"waiting 5 sec ,but TMS not run..\n");
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

	struct rlimit rl;
	if(getrlimit(RLIMIT_NOFILE,&rl)<0)
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

		// 关闭所有父进程打开的文件描述符，以免子进程继承父进程打开的端口。
		if(rl.rlim_max == RLIM_INFINITY)
		{
			rl.rlim_max = 1024;
		}
		for(int i = 3 ;i < rl.rlim_max;i++)
		{
			close(i);
		}

		// 为了防止TMS要获取它子进程的状态时失败，所以把SIGCHLD信号处理设成默认处理方式。
		// 因为TMS会继承调度软件的信号处理方式,调度软件的信号处理方法是忽略。
		struct sigaction sa;
		sa.sa_handler=SIG_DFL;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		if(sigaction(SIGCHLD,&sa,NULL)<0)
		{
		       LOGFMT(ULOG_ERROR,"cannot Set SIGCHLD Signal Catchfun! ");
		}

		LOGFMT(ULOG_INFO,"Fork Process(%d) Start TMS ... \n",getpid());
		if(execl(strTMSPath.c_str(),"Tms20_DeviceService&","",NULL) < 0)
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
		LOGFMT(ULOG_ERROR,"CallStandbySwitchTMS:Parse Fail! xml is empty!\n");
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

// 通过webservice获取tms的状态
bool CTMSSensor::GetTMSWorkState()
{
	std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	xml += "<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" ";
	xml += "xmlns:SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\" ";
	xml += "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
	xml += "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" ";
	xml += "xmlns:ns1=\"http://tempuri.org/ns1.xsd\" ";
	xml += "xmlns:ns2=\"http://tempuri.org/mons.xsd\"> <SOAP-ENV:Body> ";
	xml += "<ns1:GetTMSState></ns1:GetTMSState>";
	xml +="</SOAP-ENV:Body></SOAP-ENV:Envelope>";

	// 通过http方式调用另一个调度软件的WebService服务
	std::string strResponse;
	std::string strURI = "/";
	int nInvokeRes = InvokerWebServer(false,strURI,xml,strResponse);
	if( nInvokeRes == ERROR_SENSOR_TCP_RECV || nInvokeRes == ERROR_SENSOR_TCP_CONNECT 
		|| nInvokeRes == ERROR_SENSOR_TCP_SEND)
	{
		// 写错误日志
		LOGFMT(ULOG_ERROR,"GetTMSState InvokerWebServer Fail(%d)!",nInvokeRes);
		return false;
	}

	// 提取xml
	std::string retXml;
	int result = GetHttpContent(strResponse, retXml);

	if(retXml.empty())
	{
		LOGFMT(ULOG_ERROR,"GetTMSState:Parse Fail! xml is empty!\n");
		return false;
	}

	int nState = -1;
	if(ParseXmlFromTMSState(retXml,nState))
	{
		return nState == 0 ? true:false;
	}
	else
	{
		return false;
	}
}

// 通过webservice调用对端的切换tms接口
bool CTMSSensor::NotifyTMSSMSSwitch(std::string strHallId,std::string strNewIp,unsigned short port)
{
	if(m_nPid <= 0)
	{
		//m_vecSwitchInfo.push_back(stNotifySmsSwitchInfo(strHallId,strNewIp,port));
		return false;
	}

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
		LOGFMT(ULOG_ERROR,"NotifyTMSSMSSwitch InvokerWebServer Fail(%d)!",nInvokeRes);
		return false;
	}

	// 提取xml
	std::string retXml;
	int result = GetHttpContent(strResponse, retXml);

	if(retXml.empty())
	{
		LOGFMT(ULOG_ERROR,"NotifyTMSSMSSwitch:Parse Fail! xml is empty!");
		return false;
	}

	int nRet = -1 ;
	if(ParseXmlFromTMS(retXml,nRet))
	{
		return nRet == 0 ? true:false;
	}
	else
	{
		return false;
	}
}

// 解析TMS状态返回xml
bool  CTMSSensor::ParseXmlFromTMSState(std::string &retXml,int &nRet)
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
		DOMNodeList *ptrNodeList = ptrDoc->getElementsByTagName(C2X("tmsState"));
		if(ptrNodeList == NULL)
		{
			LOGIDFMT(ULOG_ERROR,ERROR_PARSE_MONITORSTATE_XML,"ParseXmlFromTMSState:tmsState");
			return false;
		}
		else 
		{
			if(ptrNodeList->getLength() == 0)
			{
				LOGIDFMT(ULOG_ERROR,ERROR_PARSE_MONITORSTATE_XML,"ParseXmlFromTMSState:tmsState");
				return false;
			}
			DOMNode* ptrNode = ptrNodeList->item(0);
			char* pstate =  XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state = pstate;
			if(!str_state.empty())
			{
				nRet = atoi(str_state.c_str());
			}
			XMLString::release(&pstate);
			//LOGINFFMT("%s,%s\n",str_name.c_str(),str_state.c_str());
		}
	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOGIDFMT(ULOG_ERROR,ERROR_PARSE_MONITORSTATE_XML,message);
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
			LOGIDFMT(ULOG_ERROR,ERROR_PARSE_MONITORSTATE_XML,"ParseXmlFromTMS:ret");
			return false;
		}
		else 
		{
			if(ptrNodeList->getLength() == 0)
			{
				LOGIDFMT(ULOG_ERROR,ERROR_PARSE_MONITORSTATE_XML,"ParseXmlFromTMS:ret");
				return false;
			}
			DOMNode* ptrNode = ptrNodeList->item(0);
			char* pstate =  XMLString::transcode(ptrNode->getFirstChild()->getNodeValue());
			std::string str_state = pstate;
			if(!str_state.empty())
			{
				nRet = atoi(str_state.c_str());
			}
			XMLString::release(&pstate);
			//LOGINFFMT("%s,%s\n",str_name.c_str(),str_state.c_str());
		}
	}
	catch(  XMLException& e )
	{
		char* message =  XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		LOGIDFMT(ULOG_ERROR,ERROR_PARSE_MONITORSTATE_XML,message);
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
			LOGIDFMT(ULOG_ERROR,ERROR_PARSE_MONITORSTATE_XML,"ParseOtherMonitorState:没有找到bMain节点");
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
		LOGIDFMT(ULOG_ERROR,ERROR_PARSE_MONITORSTATE_XML,message);
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
	std::string strIP;
	if(bTMSWS)
	{
		strIP="127.0.0.1";
	}
	else
	{
		strIP=m_strIP;
	}

	result = tcp.TcpConnect(strIP.c_str(), m_nTMSWBPort,delayTime);
	if(result < 0)
	{	
		LOGFMT(ULOG_ERROR,"CMonitorSensor::SendAndRecvResponse TcpConnect %s:%d Fail !\n",m_strIP.c_str(), m_nTMSWBPort);
		return  ERROR_SENSOR_TCP_CONNECT;
	}

	result = tcp.BlockSend(request.c_str(), request.size());
	if(result < 0)
	{
		LOGFMT(ULOG_ERROR,"CMonitorSensor::SendAndRecvResponse Tcp Send %s Fail(%s:%d) !\n",request.c_str(),
			m_strIP.c_str(),m_nTMSWBPort);
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
	request.SetHost(bTMSWS?"127.0.0.1":m_strIP);
	request.SetContentType("text/xml; charset=utf-8");
	request.SetContent(xml);
	request.SetSoapAction("");
	std::string strHttp = request.GetHttpRequest();

	int result = SendAndRecvResponse(bTMSWS,strHttp, strResponse);
	return result;

}


int CTMSSensor::ForkExeSh(std::string strExe)
{
	struct rlimit rl;
	if(getrlimit(RLIMIT_NOFILE,&rl)<0)
	{
		return false;
	}
	pid_t pid;

	if((pid = fork()) == 0)
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

		char buf[128]={'\0'};
		snprintf(buf,sizeof(buf),"sudo /bin/bash %s",strExe.c_str());
		LOGFMT(ULOG_INFO,"%s\n",buf);
		system(buf);
		exit(0);
	}

	return true;

}

// 更新数据库中的sms运行主机位置
bool CTMSSensor::UpdateDataBaseTMSPos(int nPosition)
{
	// 打开数据库
	C_Para *ptrPara = C_Para::GetInstance();
	CppMySQL3DB mysql;
	if(mysql.open(ptrPara->m_strDBServiceIP.c_str(),ptrPara->m_strDBUserName.c_str(),
		ptrPara->m_strDBPWD.c_str(),ptrPara->m_strDBName.c_str()) == -1)
	{
		LOGIDFMT(ULOG_ERROR,0,"mysql open failed!");
		return false;
	}

	char sql[256]={'\0'};
	snprintf(sql,sizeof(sql),"update system_config  set conf_val=%d where conf_item=\"tms_boot_postion\"",nPosition);
	int i=0;
	while(i<3)
	{
		int nResult = mysql.execSQL(sql);
		if(nResult != -1)
		{
			LOGFMT(ULOG_INFO,"CTMSSensor:update system_config database OK<%s>",sql);
			break;
		}
		else
		{
			LOGFMT(ULOG_ERROR,"CTMSSensor:update system_config database FAILED<%s>",sql);
		}
		i++;
		sleep(1);
	}

	if(i == 3)
	{
		LOGFMT(ULOG_ERROR,"CTMSSensor Update TMS RUN Position failed!<%s>",sql);
		return false;
	}

	return true;
}


