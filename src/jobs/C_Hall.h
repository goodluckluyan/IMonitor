//@file:C_Hall.h
//@brief: 包含类C_Hall。
//C_Hall：封装了影厅中sms相关操作。
//@author:luyan@oristartech.com
//dade:2012-07-12

#ifndef HALL_DEFINE
#define HALL_DEFINE


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
#include "C_constDef.h"
#include "utility/C_HttpParser.h"
#include "utility/C_TcpTransport.h"
enum enRUNTYPE{MAINRUNTYPE=1,STDBYRUNTYPE=2,TAKEOVERRUNTYPE=3};
class C_Hall
{

 public:   
	 C_Hall(SMSInfo &stSms);
	 ~C_Hall();

public:
	// 初始化
	SMSInfo & Init(bool bRun,int nPID = 0);

	 // 获取SMS工作状态
	int  GetSMSWorkState( int &state, std::string &info);

	 // 获取SMS工作状态,指定IP
	int GetSMSWorkState(std::string strIp,int nPort,int &state,std::string &info);
	
	// 启动SMS
	bool StartSMS(int &nPid,bool bLocalHost=false);

	// 关闭SMS
	int ShutDownSMS();

	// 改变SMS的运行主机信息	
	SMSInfo& ChangeSMSHost(std::string strIP,int nRunType,bool bLocalRun);

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

	// 是否运行
	bool IsRun()
	{
		return m_SMS.stStatus.nRun == 1||m_SMS.stStatus.nRun == 2;
	}

	// 获取运行角色分为1:为主机运行，2:为备机运行 3:接管运行
	int GetRunRole()
	{
		return m_SMS.nRole;
	}

	// 调用对端调度软件的切换接口
	int CallStandbySwitchSMS(bool bDelaySwitch,std::string strOtherIP,int nPort,std::string strHallID);

	// 获取运行主机及webservice端口
	void GetRunHost(std::string &strIP,int &nPort);

	// 调用从机的closesms接口
	int  CallStandbyStartOrCloseSMS(bool bSoC,std::string strOtherIP,int nPort,std::string strHallID);

	bool IsRouteReboot();
private:

	// 检测本进程是否在运行
	int ISSMSRun();

	// 获取针定进程的pid
	int GetPID(std::string strName,std::vector<int>& vecPID);

	// 获取进程执行的路径
	bool GetPIDExeDir(int nPID,std::string &strDir);

	// 在当前终端启动SMS
	bool StartSMS_CurTerminal(int &nPid,bool bLocalHost=false);

	// 打开新终端启动SMS
	bool StartSMS_NewTerminal(int &nPid,bool bLocalHost=false);

	// 打开新终端启动SMS,使用execl
	bool StartSMS_NewTerminalExe(int &nPid,bool bLocalHost=false);


	// webservice调用函数
	int UsherHttp(std::string &strURI,std::string& strIP,std::string &xml,std::string action,std::string &strRequest);
	int GetHttpContent(const std::string &http, std::string &content);
	int TcpOperator(std::string strIP,int nPort,const std::string &send, std::string &recv, int overtime);
	int SendAndRecvInfo(const std::string &send, std::string &recv, int overtime);
	int ReceiveCommand(std::string &recv, int waitTime);
	std::string ExtractXml(const std::string &response);
 	int GetRootChild( const std::string &xml,xercesc::XercesDOMParser *parser,
 		xercesc::ErrorHandler *errHandler, xercesc::DOMElement **rootChild);
 	xercesc::DOMElement *GetElementByName( const xercesc::DOMNode *elem, const std::string &name);
 	xercesc::DOMElement *FindElementByName( const xercesc::DOMNode *elem, const std::string &name);

	// 解析调用GetSMSWorkState接口的返回
	int Parser_GetSMSWorkState( const std::string &content, int &state, std::string &info);

	// 解析调用SwitchSMS接口的返回
	int Parser_SwitchSMS(std::string &content,int &nRet);

	// 解析调用closesms接口的返回
	int Parser_StartOrCloseSMS(bool bSoC,std::string &content,int &nRet);

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

	bool m_bUseLocalDB;

	time_t m_tmReboot;// 用于记录维护性重启的时间
};
#endif //HALL_DEFINE
