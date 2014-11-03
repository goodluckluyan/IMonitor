#include "DataManager.h"
#include<stdio.h>
#include<stdlib.h>
#include"database/CppMySQL3DB.h"
#include"para/C_Para.h"

CDataManager *CDataManager::m_pinstance=NULL;
CDataManager::CDataManager()
{
	m_ptrInvoker = NULL;
}
CDataManager::~CDataManager()
{
}

// 初始化
bool CDataManager::Init(void * vptr)
{
	if(vptr != NULL)
	{
		m_ptrInvoker = vptr;
	}

	C_Para *ptrPara = C_Para::GetInstance();
	CppMySQL3DB mysql;
	if(mysql.open(ptrPara->m_strDBServiceIP.c_str(),ptrPara->m_strDBUserName.c_str(),
		ptrPara->m_strDBPWD.c_str(),ptrPara->m_strDBName.c_str()) == -1)
	{
		printf("mysql open failed!\n");
		return false;
	}

	// 读取ethinfo ,初始化网卡信息
	int nResult;
	CppMySQLQuery query = mysql.querySQL("select * from EthInfo",nResult);
	int nRows = 0 ;
	if((nRows = query.numRow()) == 0)
	{
		printf("CDataManager Initial failed,ethinfo talbe no rows!\n");
		return false;
	}

	std::map<std::string,int> mapEthBaseInfo;
	query.seekRow(0);
	for(int i = 0 ;i < nRows ; i++)
	{
		std::string strName = query.getStringField("eth");
		int nType = atoi(query.getStringField("type"));
		mapEthBaseInfo[strName] = nType;
		query.nextRow();
	}
	SetEthBaseInfo(mapEthBaseInfo);

	return true;
}

// 设置网卡基本信息
void CDataManager::SetEthBaseInfo(std::map<std::string,int> &mapEthBaseInfo)
{	
	m_csNet.EnterCS();
	std::map<std::string,int>::iterator it = mapEthBaseInfo.begin();
	for(;it != mapEthBaseInfo.end();it++)
	{
		EthStatus node;
		node.strName = it->first;
		node.nTaskType = it->second;
		m_mapEthStatus[it->first] = node;
	}
	m_csNet.LeaveCS();
}

// 设置SMS基本信息
void CDataManager::SetSMSInfo(std::vector<SMSInfo> vecHall)
{
	m_csSMS.EnterCS();
	int nLen = vecHall.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		SMSInfo & node = vecHall[i];
		m_mapSmsStatus[node.strId] = node;
	}
	m_csSMS.LeaveCS();
}

// 更新各个模块的监测数据
bool CDataManager::UpdateDevStat(DiskInfo &df)
{
	m_csDisk.EnterCS();
	m_df = df;
    printf("*****************Raid State************\n");
	printf("diskSize:%s\n",df.diskSize.c_str());
	printf("diskState:%s\n",df.diskState.c_str());
	printf("diskNumberOfDrives:%s\n",df.diskNumOfDrives.c_str());
	printf("-------------------Detail--------------\n");
	int nLen = df.diskDrives.size();
	for(int i = 0 ;i < nLen ;i ++)
	{	
	    printf("----------------%d----------------\n",i);
	    printf("dirveID:%s\n",df.diskDrives[i].driveID.c_str());
	    printf("dirveSlotNum:%s\n",df.diskDrives[i].driveSlotNum.c_str());
	    printf("dirveErrorCount:%s\n",df.diskDrives[i].driveErrorCount.c_str());
	    printf("dirveSize:%s\n",df.diskDrives[i].driveSize.c_str());
	    printf("dirveFirmwareState:%s\n",df.diskDrives[i].driveFirmwareState.c_str());
	    printf("dirveSpeed:%s\n",df.diskDrives[i].driveSpeed.c_str());
	}
	printf("---------------------------------------\n");
	m_csDisk.LeaveCS();

	return true;
}

// 更新网卡状态监测数据
bool CDataManager::UpdateNetStat(std::vector<EthStatus> &vecEthStatus)
{
	m_csNet.EnterCS();
	int nLen = vecEthStatus.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		m_mapEthStatus[vecEthStatus[i].strName] = vecEthStatus[i];
		printf("Eth:%s Status:%d RxSpeed:%llu, TxSpeed:%llu\n",vecEthStatus[i].strName.c_str(),
			vecEthStatus[i].nConnStatue,vecEthStatus[i].nRxSpeed,vecEthStatus[i].nTxSpeed);
	}
	
	m_csNet.LeaveCS();

	return true;
}

//更新SMS的状态
bool CDataManager::UpdateSMSStat(std::string strHallID,int nState,std::string strInfo)
{
	m_csSMS.EnterCS();
	std::map<std::string,SMSInfo>::iterator it = m_mapSmsStatus.find(strHallID);
	if(it != m_mapSmsStatus.end())
	{
		SMSInfo &info = it->second;
		info.stStatus.nStatus = nState;
		//info.stStatus.bRun = 1;
	}
	m_csSMS.LeaveCS();
	return true;
}

//更新SMS的状态
bool CDataManager::UpdateSMSStat(std::string strHallID,SMSInfo &stSMSInfo)
{
	m_csSMS.EnterCS();
	std::map<std::string,SMSInfo>::iterator it = m_mapSmsStatus.find(strHallID);
	if(it != m_mapSmsStatus.end())
	{
		SMSInfo &info = it->second;
		info = stSMSInfo;
	}
	m_csSMS.LeaveCS();
	return true;
}


// 更新TMS的状态
bool CDataManager::UpdateTMSStat(int state)
{
	m_csTMS.EnterCS();
	m_nTMSState = state;
	m_csTMS.LeaveCS();
	return true;
}

// 读取disk监测参数
bool CDataManager::GetDevStat(DiskInfo &df)
{
	m_csDisk.EnterCS();
	df = m_df; 
	m_csDisk.LeaveCS();   
}

// 获取网卡状态
bool CDataManager::GetNetStat(std::map<std::string,EthStatus> &mapEthStatus)
{
	m_csNet.EnterCS();
	mapEthStatus = m_mapEthStatus;
	m_csNet.LeaveCS();
	return true;
}

// 获取SMS状态
bool CDataManager::GetSMSStat(std::vector<SMSStatus>& vecSMSState)
{
	m_csSMS.EnterCS();
	std::map<std::string,SMSInfo> mapTmp = m_mapSmsStatus;
	m_csSMS.LeaveCS();

	std::map<std::string,SMSInfo>::iterator it = mapTmp.begin();
	for(;it != mapTmp.end();it++)
	{
		SMSStatus state;
		SMSInfo &info = it->second;
		if(info.stStatus.nRun ==1)
		{
			state.nStatus = info.stStatus.nStatus;
			state.hallid = info.strId;
			state.nPosition = info.stStatus.nPosition;
			state.nRun = info.stStatus.nRun;
			state.strSPLUuid = info.stStatus.strSPLUuid;
			vecSMSState.push_back(state);
		}
	}
	
	return true;
}

// 获取TMS的状态
int CDataManager::GetTMSStat()
{
	return m_nTMSState;
}

void * CDataManager::GetInvokerPtr()
{
	return m_ptrInvoker;
}

bool CDataManager::UpdateOtherMonitorState(bool bMain,int nState)
{
	printf("Other Monitor State:bMain:%d,nState:%d\n",bMain,nState);
	return true;
}

bool CDataManager::UpdateOtherTMSState(bool bRun,int nWorkState,int nState)
{
	printf("Other TMS State:bRun:%d,nWorkState:%d,nState:%d\n",bRun,nWorkState,nState);
	return true;
}

bool CDataManager::UpdateOtherSMSState(std::vector<SMSStatus> &vecSMSStatus)
{
// 	printf("Other SMS State:strHallId:%s,bRun:%d,nState:%d,nPosition:%d,spluuid:%s\n",strHallId.c_str(),
// 		bRun,nState,nPosition,strSplUuid.c_str());

	int nLen = vecSMSStatus.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		std::string strID = vecSMSStatus[i].hallid;
		std::map<std::string,SMSInfo>::iterator it = m_mapSmsStatus.find(strID);
		if(it != m_mapSmsStatus.end())
		{
			SMSInfo& node = it->second;
			node.stStatus.nPosition = vecSMSStatus[i].nPosition;
			node.stStatus.nRun = vecSMSStatus[i].nRun == 1 ? 2:vecSMSStatus[i].nRun;
			node.stStatus.nStatus = vecSMSStatus[i].nStatus;
			node.stStatus.strSPLUuid = vecSMSStatus[i].strSPLUuid;
		}
	}
	return true;
}

bool CDataManager::UpdateOtherRaidState(int nState,int nReadSpeed,
										int nWriteSpeed,std::vector<int> &vecDiskState)
{
// 	printf("Other Raid State:State:%d,RS:%d,WS:%d\n",nState,nReadSpeed,nWriteSpeed);
// 	for(int i=0;i<vecDiskState.size();i++)
// 	{
// 		printf("Raid%d:%d\n",i,vecDiskState[i]);
// 	}
	return true;
}

bool  CDataManager::UpdateOtherEthState(std::vector<EthStatus> &vecEthStatus)
{
// 	int nlen = vecEthStatus.size();
// 	for(int i = 0 ;i < nlen ;i++)
// 	{
// 		EthStatus &node = vecEthStatus[i];
// 		printf("Other %s State:nConnectState:%d,nSpeed:%d\n",node.strName.c_str(),
// 			node.nConnStatue,node.nRxSpeed);
// 	}
	return true;
}

bool  CDataManager::UpdateOtherSwitchState(int nSwitch1State,int nSwitch2State)
{
	//printf("Other Switch nSwitch1State:%d,nSwitch2State:%d\n",nSwitch1State,nSwitch2State);
	return true;
}

bool  CDataManager::UpdateOtherSpeedLmtState(bool bEnableIngest,int nSpeedLimit)
{
	//printf("Other SpeedLmt bEnableIngest:%d,nSpeedLimit:%d\n",bEnableIngest,nSpeedLimit);
	return true;
}

bool  CDataManager::UpdateOtherSMSEWState(int nState,std::string  strInfo,std::string  strHall)
{
	//printf("Other SMSEW nState:%d,strInfo:%s,strHall:%s\n",nState,strInfo.c_str(),strHall.c_str());
	return true;
}

void CDataManager::PrintTMSState()
{
	printf("TMS Current State:\n");
	printf("bRun:%d\n",m_nTMSState);
}

void CDataManager::PrintDiskState()
{
	printf("Number of RAID Disk :%s\n",m_df.diskNumOfDrives.c_str());
	printf("RAID Disk State :%s\n",m_df.diskState.c_str());
	printf("RAID Disk State: %s\n",m_df.diskSize.c_str());
}

void CDataManager::PrintSMSState()
{
	m_csSMS.EnterCS();
	std::map<std::string,SMSInfo>maptmp = m_mapSmsStatus;
	m_csSMS.LeaveCS();

	std::map<std::string,SMSInfo>::iterator it = maptmp.begin();
	for(;it != maptmp.end(); it++)
	{
		SMSInfo &info = it->second;
		printf("hallid:%s\n",info.strId.c_str());
		printf("SMS state:%d\n",info.stStatus.nStatus);
	}
}

void CDataManager::PrintEthState()
{
	m_csNet.EnterCS();
	std::map<std::string,EthStatus> mapTmp = m_mapEthStatus;
	m_csNet.LeaveCS();

	std::map<std::string,EthStatus>::iterator it = mapTmp.begin();
	for(;it != mapTmp.end(); it++)
	{
		EthStatus &node = it->second;
		printf("EthName:%s\n",node.strName.c_str());
		printf("TaskType:%d\n",node.nTaskType);
		printf("ConnState:%d\n",node.nConnStatue);
		printf("Speed:%d\n",node.nRxSpeed);
	}
}

