#include<algorithm>
#include<stdio.h>
#include<stdlib.h>
#include"para/C_Para.h"
#include"DataManager.h"
CDataManager *CDataManager::m_pinstance=NULL;
CDataManager::CDataManager()
{
	m_ptrInvoker = NULL;
	m_ptrDispatch = NULL;
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
	std::transform(m_df.diskState.begin(),m_df.diskState.end(),m_df.diskState.begin(),::tolower);
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
		std::transform(df.diskDrives[i].driveFirmwareState.begin(),
			df.diskDrives[i].driveFirmwareState.end(),df.diskDrives[i].driveFirmwareState.begin(),::tolower);
	    printf("dirveFirmwareState:%s\n",df.diskDrives[i].driveFirmwareState.c_str());
	    printf("dirveSpeed:%s\n",df.diskDrives[i].driveSpeed.c_str());
	}
	printf("---------------------------------------\n");
	m_csDisk.LeaveCS();
	
	std::vector<stError> vecRE;
	if(!CheckRaidError(vecRE))
	{
		if(m_ptrDispatch)
		{
			m_ptrDispatch->TriggerDispatch(RAIDTask,vecRE);
		}
	}

	return true;
}

// 检测磁盘陈列是否有错误
bool CDataManager::CheckRaidError(std::vector<stError> &vecErr)
{
	bool bRet = true;
	DiskInfo df;
	m_csDisk.EnterCS();
	df = m_df;
	m_csDisk.LeaveCS();
	
	if(df.diskState == "degraded")
	{
		stError re;
		re.ErrorName = "diskState";
		re.ErrorVal = "degraded";
		vecErr.push_back(re);
		bRet = false;
	}

	int nLen = df.diskDrives.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		DiskDriveInfo &di = df.diskDrives[i];
		if(di.driveFirmwareState.find("online") == std::string::npos)
		{
			stError re;
			re.ErrorName = "driveFirmwareState";
			re.ErrorVal = di.driveFirmwareState;
			re.nOrdinal = atoi(di.driveSlotNum.c_str());
			vecErr.push_back(re);
			bRet = false;
		}

		if(di.driveErrorCount != "0")
		{
			stError re;
			re.ErrorName = "driveErrorCount";
			re.ErrorVal = di.driveErrorCount;
			re.nOrdinal = atoi(di.driveSlotNum.c_str());
			vecErr.push_back(re);
			bRet = false;
		}
	}
	return bRet;
}

// 更新网卡状态监测数据
bool CDataManager::UpdateNetStat(std::vector<EthStatus> &vecEthStatus)
{
	m_csNet.EnterCS();
	int nLen = vecEthStatus.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		std::map<std::string,EthStatus>::iterator fit = m_mapEthStatus.find(vecEthStatus[i].strName);
		if(fit != m_mapEthStatus.end())
		{
			fit->second = vecEthStatus[i];
			printf("Eth:%s Status:%d RxSpeed:%llu, TxSpeed:%llu\n",vecEthStatus[i].strName.c_str(),
				vecEthStatus[i].nConnStatue,vecEthStatus[i].nRxSpeed,vecEthStatus[i].nTxSpeed);
		}
	}
	
	m_csNet.LeaveCS();

	return true;
}

//更新SMS的状态
bool CDataManager::UpdateSMSStat(std::string strHallID,int nState,std::string strInfo)
{
	printf("SMS:%s Status:%d  (%s)\n",strHallID.c_str(),nState,strInfo.c_str());
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

	std::vector<stError> vecRE;
	if(state == -1 && C_Para::GetInstance()->m_bMain)
	{
		stError er;
		er.ErrorName = "state";
		er.ErrorVal = "norun";
		vecRE.push_back(er);

		if(m_ptrDispatch)
		{
			m_ptrDispatch->TriggerDispatch(TMSTask,vecRE);
		}
	}

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
		//if(info.stStatus.nRun ==1)
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

// 获取Invoker指针
void * CDataManager::GetInvokerPtr()
{
	return m_ptrInvoker;
}


//更新对端调度软件状态
bool CDataManager::UpdateOtherMonitorState(bool bMain,int nState)
{
	printf("Other Monitor State:bMain:%d,nState:%d\n",bMain,nState);
	if(C_Para::GetInstance()->m_bMain == bMain && bMain )
	{
		stError er;
		std::vector<stError> vecRE;
		er.ErrorName="brotherhood";
		er.ErrorVal="nostandby";
		vecRE.push_back(er);
		if(m_ptrDispatch)
		{
			m_ptrDispatch->TriggerDispatch(IMonitorTask,vecRE);
		}
	}
	else if(C_Para::GetInstance()->m_bMain == bMain && !bMain )
	{
		stError er;
		std::vector<stError> vecRE;
		er.ErrorName="brotherhood";
		er.ErrorVal="nomain";
		vecRE.push_back(er);
		if(m_ptrDispatch)
		{
			m_ptrDispatch->TriggerDispatch(IMonitorTask,vecRE);
		}
	}
	
	return true;
}


//更新对端tms状态
bool CDataManager::UpdateOtherTMSState(bool bRun,int nWorkState,int nState)
{
	printf("Other TMS State:bRun:%d,nWorkState:%d,nState:%d\n",bRun,nWorkState,nState);
	return true;
}


//更新对端sms状态
bool CDataManager::UpdateOtherSMSState(std::vector<SMSStatus> &vecSMSStatus)
{
// 	printf("Other SMS State:strHallId:%s,bRun:%d,nState:%d,nPosition:%d,spluuid:%s\n",strHallId.c_str(),
// 		bRun,nState,nPosition,strSplUuid.c_str());

	
	int nLen = vecSMSStatus.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		if(vecSMSStatus[i].nRun == 0)
		{
			continue;
		}

		std::string strID = vecSMSStatus[i].hallid;
		std::map<std::string,SMSInfo>::iterator it = m_mapOtherSMSStatus.find(strID);
		if(it != m_mapOtherSMSStatus.end())
		{
			SMSInfo& node = it->second;
			node.stStatus = vecSMSStatus[i];
		}
	}

	// 更新主结构
	std::map<std::string,SMSInfo>::iterator it = m_mapOtherSMSStatus.begin();
	for(;it != m_mapOtherSMSStatus.end();it++)
	{
		SMSInfo &info = it->second;
		if(info.nRole == 1 && info.stStatus.nRun == 1)
		{
			m_csSMS.EnterCS();
			std::map<std::string,SMSInfo>::iterator fit= m_mapSmsStatus.find(info.strId);
			if(fit != m_mapSmsStatus.end())
			{
				it->second.stStatus.nRun = 2;
			}
			m_csSMS.LeaveCS();
		}
	}
	return true;
}

//更新对端raid状态
bool CDataManager::UpdateOtherRaidState(int nState,int nReadSpeed,
										int nWriteSpeed,std::vector<int> &vecDiskState)
{
	printf("Other Raid State:State:%d,RS:%d,WS:%d\n",nState,nReadSpeed,nWriteSpeed);
	for(int i=0;i<vecDiskState.size();i++)
	{
		printf("Raid%d:%d\n",i,vecDiskState[i]);
	}


	return true;
}

//更新对端eth状态
bool  CDataManager::UpdateOtherEthState(std::vector<EthStatus> &vecEthStatus)
{
	int nlen = vecEthStatus.size();
	for(int i = 0 ;i < nlen ;i++)
	{
		EthStatus &node = vecEthStatus[i];
		printf("Other %s State:nConnectState:%d,nSpeed:%d\n",node.strName.c_str(),
			node.nConnStatue,node.nRxSpeed);
	}
	return true;
}


//帮助信息打印tms状态
void CDataManager::PrintTMSState()
{
	printf("TMS Current State:\n");
	printf("bRun:%d\n",m_nTMSState);
}


//帮助信息打印raid状态
void CDataManager::PrintDiskState()
{
	printf("Number of RAID Disk :%s\n",m_df.diskNumOfDrives.c_str());
	printf("RAID Disk State :%s\n",m_df.diskState.c_str());
	printf("RAID Disk State: %s\n",m_df.diskSize.c_str());
}

//帮助信息打印sms状态
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

//帮助信息打印eth状态
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




