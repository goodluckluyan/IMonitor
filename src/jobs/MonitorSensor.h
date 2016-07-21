//@file:MonitorSensor.h
//@brief: 另一台调度程序的状态监测。
//@author:luyan@oristartech.com
//date:2014-09-17

#ifndef MONITORSENSOR_INC
#define MONITORSENSOR_INC

#include "DataManager.h"
#include "threadManage/C_CS.h"
#include <map>
#include "parser_xml.h"


class CMonitorSensor
{
public:
	CMonitorSensor();

	~CMonitorSensor();

	// 初始化
	bool Init(std::string strIP,int nPort);
	
	// 获取另一台主机的调度程序的状态
	bool GetOtherMonitorState(int nStateType,bool bNoticeDM = true);

	// 调用另一端接口设置数据同步标记
	bool SetOtherDBSynch(std::string dbsynch);

	// 解析调用SetOtherDBSynch返回
	int Parser_SetOtherDBSynch(std::string &content,int &nRet);

	// 调用备机的重启接口
	bool SlaveReboot(int nType,int &nState,std::string strDesc);

	// 调用备机的定时重启接口，只是为了在备机记录timer.xml文件
	bool InvokeSlaveTimingReboot(int nDay,int nWeek,int nHour,int nMinute,int nRepeatType,
		int nRepeatCnt,int &nState,std::string &strDesc);


	bool FixRebootXmlAttribute(std::string strAttr ,int nValue);

	int ReadRebootTask(int &nEnable,int& nDay,int& nWeek,int& nHour,int& nMinute,int &nType,int &nRepeatCnt,int &nExecnt);

	// 保存至xml文件
    bool WriteTimerFile(int nDay,int nWeek,int nHour,int nMinute,int nRepeatType,int nRepeatCnt,int &nState,std::string &strDesc);

	int WriteXmlFile(xercesc::DOMDocument * ptrDoc,std::string xmlFile);

	bool ParseAskAboutSlaveRestoreSwitch(std::string &retXml,int &nState);

	bool AskAboutSlaveRestoreSwitch(int &nIsRestoreSwitch);


private:

	// 获取http中的xml 
	int GetHttpContent(const std::string &http, std::string &response);

	// 把调用xml串以http方式发送到服务端并接收返回xml
	int SendAndRecvResponse(const std::string &request, std::string &response, int delayTime=2);

	// 调用webservice接口
	int InvokerWebServer(std::string &xml,std::string &strResponse);

	// 解析Monitor状态
	bool ParseOtherMonitorState(std::string &retXml,bool &bMain,int &nState ,long &lSynch);

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

	// 解析重启返回状态
	bool ParseSlaveRebootState(std::string &retXml,int &nState,std::string &strDesc);
	
private:
	std::string m_strURI;
	std::string m_strIP;
	int m_nPort;
	CDataManager * m_ptrDM;

	std::map<int,std::string> m_mapStateType;
	C_CS m_csMap;
};

#endif