
//@file:TMSSensor.h
//@brief:TMS相关操作。
//@author:luyan@oristartech.com
//date:2014-09-17


#ifndef _H_SOFTWARESTATE_
#define _H_SOFTWARESTATE_

#include <iostream>
#include <string>
#include "DataManager.h"
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
#include "threadManage/C_CS.h"


struct stNotifySmsSwitchInfo
{
	std::string strHallId;
	std::string strNewIp;
	unsigned short port;

	stNotifySmsSwitchInfo():
	  port(0)
	{
	}

	  stNotifySmsSwitchInfo(std::string HallId,std::string NewIp,unsigned short Port)
	  {
		  strHallId = HallId;
		  strNewIp = NewIp;
		  port = Port;
	  }

	stNotifySmsSwitchInfo(const stNotifySmsSwitchInfo &obj)
	{
		strHallId = obj.strHallId;
		strNewIp = obj.strNewIp;
		port = obj.port;
	}

	stNotifySmsSwitchInfo& operator=(const stNotifySmsSwitchInfo &obj)
	{
		if(this != &obj)
		{
			strHallId = obj.strHallId;
			strNewIp = obj.strNewIp;
			port = obj.port;
		}
		return *this;
	}
};
class CTMSSensor
{
public:
	
	CTMSSensor();
	
	~CTMSSensor();

	// 开启tms
	bool StartTMS();

	// 切换tms
	bool SwitchTMS();
		
	// 获取TMS PID
	int GetTMSPID();

	// 获取对端主机的TMS工作状态
	bool GetTMSWorkState();

	// 初始化
	bool Init(std::string strIP,int nPort,int nTMSWSPort);

	// 通知tms sms切换到新的主机
	bool NotifyTMSSMSSwitch(std::string strHallId,std::string strNewIp,unsigned short port);

	// 询问tms是否可以重启服务器
	bool AskTMSReboot();

	// kill tms PID
	bool ShutDownTMS();

private:
	
	// 用不同的方式启动tms
	bool StartTMS_CurTerminal(std::string strTMSPath);
	bool StartTMS_NewTerminal(std::string strTMSPath);
	

	// 调用webservice接口，bTMSWS:是否调用本机的tms接口
	int InvokerWebServer(bool bTMSWS,std::string strURI,std::string &xml,std::string &strResponse);
	int GetHttpContent(const std::string &http, std::string &response);
	int SendAndRecvResponse(bool bTMSWS,const std::string &request, std::string &response, int delayTime = 3);
	bool ParseXmlFromOtherMonitor(std::string &retXml,int &nRet);
	bool ParseXmlFromTMS(std::string &retXml,int &nRet);
	bool ParseXmlFromTMSState(std::string &retXml,int &nRet);
	bool ParseIsRebootXml(std::string &retXml,int &nRet);

	// 调用备机上的切换tms接口
	bool CallStandbySwitchTMS();

	// 在新的进程中执行脚本
	int ForkExeSh(std::string strExe);

	// 更新数据库中tms的启动位置
	bool UpdateDataBaseTMSPos(int nPosition);

	// 获取指定进程的pid
	int Getpid(std::string strName,std::vector<int>& vecPID);

	

private:
	CDataManager * m_ptrDM;
	int m_nPid;

	std::vector<stNotifySmsSwitchInfo> m_vecSwitchInfo;

	//对端主机信息
	std::string m_strIP;
	int m_nPort;
	int m_nTMSWBPort;
	C_CS m_csPID;



};


#endif //