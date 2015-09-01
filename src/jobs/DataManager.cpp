#include<algorithm>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include"para/C_Para.h"
#include"DataManager.h"
#include"log/C_LogManage.h"
extern time_t g_tmDBSynch;
#define  LOG(errid,msg)   C_LogManage::GetInstance()->WriteLog(ULOG_FATAL,LOG_MODEL_JOBS,0,errid,msg)
#define  LOGINF(msg)	  C_LogManage::GetInstance()->WriteLog(ULOG_INFO,LOG_MODEL_JOBS,0,0,msg)
#define  LOGINFFMT(fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_INFO,LOG_MODEL_JOBS,0,0,fmt,##__VA_ARGS__)
#define  LOGDEBFMT(fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_DEBUG,LOG_MODEL_JOBS,0,0,fmt,##__VA_ARGS__)
						

CDataManager *CDataManager::m_pinstance=NULL;
CDataManager::CDataManager()
{
	m_ptrInvoker = NULL;
	m_ptrDispatch = NULL;
	m_nOterHostFail = 0;
	time(&m_tmOtherHostFail);

	time(&m_tmCheckTMSNoRun);
	m_nCheckTMSNoRun = 0;

	m_nOtherMonitorState = -1;
	m_lSynch = 0;
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
bool CDataManager::UpdateDevStat(std::map<int,DiskInfo> &mapdf)
{
	m_csDisk.EnterCS();
	m_mapdf = mapdf;
	m_csDisk.LeaveCS();
	
	LOGDEBFMT("*****************Raid State************");
	std::map<int,DiskInfo>::iterator it = mapdf.begin();
	for(;it != mapdf.end();it++)
	{
		DiskInfo &df = it->second;
		
		LOGDEBFMT("diskGroup:%d",it->first);
		LOGDEBFMT("diskSize:%s",df.diskSize.c_str());
		std::transform(df.diskState.begin(),df.diskState.end(),df.diskState.begin(),::tolower);
		LOGDEBFMT("diskState:%s",df.diskState.c_str());
		LOGDEBFMT("diskNumberOfDrives:%s",df.diskNumOfDrives.c_str());
		LOGDEBFMT("-------------------Detail--------------");
		int nLen = df.diskDrives.size();
		for(int i = 0 ;i < nLen ;i ++)
		{	
			LOGDEBFMT("----------------%d----------------",i);
			LOGDEBFMT("dirveID:%s",df.diskDrives[i].driveID.c_str());
			LOGDEBFMT("dirveSlotNum:%s",df.diskDrives[i].driveSlotNum.c_str());
			LOGDEBFMT("dirveErrorCount:%s",df.diskDrives[i].driveErrorCount.c_str());
			LOGDEBFMT("dirveSize:%s",df.diskDrives[i].driveSize.c_str());
			std::transform(df.diskDrives[i].driveFirmwareState.begin(),
				df.diskDrives[i].driveFirmwareState.end(),df.diskDrives[i].driveFirmwareState.begin(),::tolower);
			LOGDEBFMT("dirveFirmwareState:%s",df.diskDrives[i].driveFirmwareState.c_str());
			LOGDEBFMT("dirveType:%s",df.diskDrives[i].driveType.c_str());
			LOGDEBFMT("dirveSpeed:%s",df.diskDrives[i].driveSpeed.c_str());
		}
		
	}
    LOGDEBFMT("---------------------------------------");
	
	
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
	std::map<int,DiskInfo> mapdf;
	m_csDisk.EnterCS();
	mapdf = m_mapdf;
	m_csDisk.LeaveCS();
	
	std::map<int,DiskInfo>::iterator it = mapdf.begin();
	for(;it != mapdf.end();it++)
	{
		DiskInfo &df = it->second;
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
			fit->second.nConnStatue = vecEthStatus[i].nConnStatue;
			fit->second.nRxSpeed = vecEthStatus[i].nRxSpeed;
			fit->second.nTxSpeed = vecEthStatus[i].nTxSpeed;
			LOGDEBFMT("Eth:%s Status:%d RxSpeed:%llu, TxSpeed:%llu",vecEthStatus[i].strName.c_str(),
				vecEthStatus[i].nConnStatue,vecEthStatus[i].nRxSpeed,vecEthStatus[i].nTxSpeed);
		}
	}
	m_csNet.LeaveCS();

	std::vector<stError> vecErr;
	CheckEthError(vecErr);
	if(vecErr.size()>0)
	{
		if(m_ptrDispatch)
		{
			m_ptrDispatch->TriggerDispatch(ETH,vecErr);
		}
	}

	return true;
}

void CDataManager::CheckEthError(std::vector<stError> &vecErr)
{
	m_csNet.EnterCS();
	std::map<std::string,EthStatus>::iterator it = m_mapEthStatus.begin();
	for(;it!=m_mapEthStatus.end();it++)
	{
		EthStatus &estate = it->second;
		if(estate.nConnStatue == 0)
		{
			stError re;
			switch(estate.nTaskType)
			{
			case 1:
				re.ErrorName = "type1noconn";
				break;
			case 2:
				re.ErrorName = "type2noconn";
				break;
			case 3:
				re.ErrorName = "type3noconn";
				break;
			}
			
			re.ErrorVal = estate.strName;
			re.nOrdinal = 0;
			vecErr.push_back(re);
		}
	}
	m_csNet.LeaveCS();

}

//更新SMS的状态
/*
可能出现的角色和运行状态
        角色（info.nRole）               运行状态（info.stStatus.nRun）
		主机（MAINROLE）                     本机运行（1）
		主机（STDBYROLE）                    异机运行（2）
		从机（MAINROLE）                     异机运行（2）
		从机（STDBYROLE）                    本机运行（1）
		主机（TAKEOVERROLE）                 本机运行（1）
		从机（TAKEOVERROLE）                 本机运行（1）
*/
bool CDataManager::UpdateSMSStat(std::string strHallID,int nState,std::string strInfo)
{
	m_csSMS.EnterCS();
	std::map<std::string,SMSInfo>::iterator it = m_mapSmsStatus.find(strHallID);
	if(it != m_mapSmsStatus.end())
	{
		SMSInfo &info = it->second;
		info.stStatus.nStatus = nState;
		LOGDEBFMT("SMS:%s(%d:%d) Status:%d  (%s)",strHallID.c_str(),info.nRole,info.stStatus.nRun,nState,strInfo.c_str());

// 		int nRole = C_Para::GetInstance()->GetRole();
// 		if(info.nRole == TAKEOVERRUNTYPE &&  (nRole == MAINROLE || nRole == STDBYROLE ))
// 		{
// 			
// 		}
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
	if(state == -1 && C_Para::GetInstance()->IsMain())
	{
		time_t tm;
		time(&tm);
		int nSec = tm-m_tmOtherHostFail;
		if(nSec > 5)
		{
			stError er;
			er.ErrorName = "state";
			er.ErrorVal = "norun";
			vecRE.push_back(er);

			if(m_ptrDispatch)
			{
				m_ptrDispatch->TriggerDispatch(TMSTask,vecRE);
			}
			time(&m_tmOtherHostFail);
		}
	}

	return true;
}

// 读取disk监测参数
bool CDataManager::GetDevStat(std::map<int,DiskInfo> &mapdf)
{
	m_csDisk.EnterCS();
	mapdf = m_mapdf; 
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

// 获取SMS状态
bool CDataManager::GetSMSStat(std::string strHallID,SMSInfo& smsinfo)
{
	m_csSMS.EnterCS();
	std::map<std::string,SMSInfo>::iterator it = m_mapSmsStatus.find(strHallID);
	if(it != m_mapSmsStatus.end())
	{
		smsinfo = it->second;
	}
	m_csSMS.LeaveCS();

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
bool CDataManager::UpdateOtherMonitorState(bool bMain,int nState,long lSynch)
{
	LOGDEBFMT("Other Monitor State:bMain:%d,nState:%d,lSynch%lld",bMain,nState,lSynch);
	if(lSynch!=0 && nState == TMPMAINROLE)
	{
		m_lSynch = lSynch;
		return true;
	}
	
	// 对端机正在启动不作处理
	if(0 == nState)
	{
       m_nOtherMonitorState = 0;
	   return true;
	}

	// 获取状态失败
	if(-1 == nState)
	{
		time_t tm;
		time(&tm);
		int nSec = tm-m_tmOtherHostFail;
		if(nSec > 100)
		{
			m_nOterHostFail = 1;
			time(&m_tmOtherHostFail);
		}
		else
		{
			m_nOterHostFail++;
		}

		int nRole = C_Para::GetInstance()->GetRole();
		
		int nCnt=0;
		int nSmsSize=m_mapSmsStatus.size();
		std::map<std::string,SMSInfo>::iterator it=m_mapSmsStatus.begin();
		for(;it!=m_mapSmsStatus.end();it++)
		{
			if(it->second.stStatus.nRun==1)
			{
				nCnt++;
			}
		}

		bool bTrigger = (nCnt!=nSmsSize || (nRole != ONLYMAINROLE && nRole != TMPMAINROLE));
		if(m_nOterHostFail > 2 && bTrigger)
		{
			int nPerSec = nSec/m_nOterHostFail;
			char buf[16]={'\0'};
			snprintf(buf,16,"%d",nPerSec);
			
			LOGDEBFMT("Other Host Connection Fail(Interval Sec/PerCnt:%s=%d/%d)",buf,nSec,m_nOterHostFail);
			stError er;
			std::vector<stError> vecRE;
			er.ErrorName="connfailinterval";
			er.ErrorVal=buf;
			vecRE.push_back(er);
			if(m_ptrDispatch)
			{
				m_ptrDispatch->TriggerDispatch(IMonitorTask,vecRE);
			}
		}

		return true;
	}   

	m_nOtherMonitorState = nState;

	// 启动正常后，nState的状态为对端机所处的角色
	// 两端都是主
	if(C_Para::GetInstance()->IsMain() == bMain && bMain )
	{
		//本机为备机时 发现主机出现时，把临时主改回备
		if(C_Para::GetInstance()->GetRole()==(int)TMPMAINROLE )
		{	
			stError er;
			std::vector<stError> vecRE;
			er.ErrorName="mainback";
			er.ErrorVal="mainback";
			vecRE.push_back(er);
			if(m_ptrDispatch)
			{
				m_ptrDispatch->TriggerDispatch(IMonitorTask,vecRE);
			}
		}
		// 真正存在两个真正的主时做如下处理
		else if(nState == MAINROLE)
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
		
	}
	// 两端都是备
	else if(C_Para::GetInstance()->IsMain() == bMain && !bMain )
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
	else if(C_Para::GetInstance()->GetRole()==(int)ONLYMAINROLE && !bMain )
	{
		stError er;
		std::vector<stError> vecRE;
		er.ErrorName="stdbyback";
		er.ErrorVal="stdbyback";
		vecRE.push_back(er);
		if(m_ptrDispatch)
		{
			m_ptrDispatch->TriggerDispatch(IMonitorTask,vecRE);
		}
	}

	// 由于一些原因，可能会出现正常启动后，g_tmDBSynch没有恢复0的情况，所以在这判断一下。
	if(bMain && !C_Para::GetInstance()->IsMain() && g_tmDBSynch != 0)
	{
		g_tmDBSynch=0;
	}
	return true;
}


//更新对端tms状态
bool CDataManager::UpdateOtherTMSState(bool bRun,int nWorkState,int nState)
{
	LOGDEBFMT("Other TMS State:bRun:%d,nWorkState:%d,nState:%d",bRun,nWorkState,nState);
	return true;
}


//更新对端sms状态
bool CDataManager::UpdateOtherSMSState(std::vector<SMSStatus> &vecSMSStatus)
{
	int nLen = vecSMSStatus.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		LOGDEBFMT("Other SMS State:strHallId:%s,bRun:%d,nState:%d",vecSMSStatus[i].hallid.c_str(),
			vecSMSStatus[i].nRun,vecSMSStatus[i].nStatus);
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
		else
		{
			m_mapOtherSMSStatus[strID].strId=vecSMSStatus[i].hallid;
			m_mapOtherSMSStatus[strID].stStatus=vecSMSStatus[i];
		}
	}

	//只有主机才能判断和解决冲突
	if(C_Para::GetInstance()->GetRole()!=(int)MAINROLE)
	{
		return true;
	}

	// 判断冲突
	m_csSMS.EnterCS();
	std::map<std::string,SMSInfo>maptmp = m_mapSmsStatus;
	m_csSMS.LeaveCS();

	int nStdbyRun =0;
	std::vector<ConflictInfo> vecConflict;
	std::map<std::string,SMSInfo>::iterator it = m_mapOtherSMSStatus.begin();
	for(;it != m_mapOtherSMSStatus.end();it++)
	{
		SMSInfo &Other = it->second;
		std::map<std::string,SMSInfo>::iterator fit= maptmp.find(Other.strId);
		if(fit == maptmp.end())
		{
			continue;	
		}
		
		if(Other.stStatus.nRun == 1)
		{
			nStdbyRun++;
		}
	
		// 都在运行这个sms
		if(Other.stStatus.nRun == 1 && fit->second.stStatus.nRun == 1)
		{
			ConflictInfo ci;
			ci.nMainState=fit->second.stStatus.nStatus;
			ci.nStdbyState=Other.stStatus.nStatus;
			ci.nType = 1;
			ci.strHallID=Other.strId;
			time(&ci.tmTime);
			vecConflict.push_back(ci);
		}

		// 都没有运行这个sms, 暂不对两边都不启动进行处理。在切换时会出现这种情况。
		// 要处理两边都没有启动的情况，要过滤掉切换时出现的情况。比较繁琐，要多次上报才会处理(已做)并且要在切换时通知本函数
		// 不要进行冲突检测。（未做）
// 		if(Other.stStatus.nRun == 2 && fit->second.stStatus.nRun == 2)
// 		{
// 			ConflictInfo ci;
// 			ci.nMainState=fit->second.stStatus.nStatus;
// 			ci.nStdbyState=Other.stStatus.nStatus;
// 			ci.nType = 2;
// 			ci.strHallID=Other.strId;
// 			ci.tmTime=time();
// 			vecConflict.push_back(ci);
// 		}
	}

	int nMainRun=0;
	std::map<std::string,SMSInfo>::iterator mit = maptmp.begin();
	for(;mit!=maptmp.end();mit++)
	{
		if(mit->second.stStatus.nRun==1)
		{
			nMainRun++;
		}
	}

	nLen = vecConflict.size();
	for(int i=0;i<nLen;i++)
	{
		vecConflict[i].nStdbySMSSum=nStdbyRun;
		vecConflict[i].nMainSMSSum=nMainRun;
//		m_maplstConfilict[vecConflict[i].strHallID].push_back(vecConflict[i]);
	}

// 	vecConflict.clear();
// 	CheckConfile(vecConflict);
	if(vecConflict.size()>0 && m_ptrDispatch!=NULL)
	{
		m_ptrDispatch->AddConflictInfo(vecConflict);
		stError er;
		std::vector<stError> vecRE;
		er.ErrorName="smsconflict";
		er.ErrorVal="smsconflict";
		vecRE.push_back(er);
		m_ptrDispatch->TriggerDispatch(SMSTask,vecRE);

	}

	return true;
}

// 过滤检测到的冲突，防止误报
bool CDataManager::CheckConfile(std::vector<ConflictInfo> &vecConflict)
{
	std::map<std::string,std::list<ConflictInfo> >::iterator it = m_maplstConfilict.begin();
	while(it!=m_maplstConfilict.end())
	{
		int nLen = it->second.size();
		if(nLen<3)
		{
			it++;
			continue;
		}
		
		
		int nInterval = it->second.back().tmTime-it->second.front().tmTime;
		int nSmsCheckSec = C_Para::GetInstance()->m_nOtherSMSCheckDelay;
		if(nInterval < nSmsCheckSec*(nLen+1))
		{
			vecConflict.push_back(it->second.front());
			m_maplstConfilict.erase(it++);
			continue;
		}
		else
		{
			it->second.pop_front();
		}
		it++;
	}
}

//更新对端raid状态
bool CDataManager::UpdateOtherRaidState(int nState,int nReadSpeed,
										int nWriteSpeed,std::vector<int> &vecDiskState)
{
	LOGDEBFMT("Other Raid State:State:%d,RS:%d,WS:%d",nState,nReadSpeed,nWriteSpeed);
	for(int i=0;i<vecDiskState.size();i++)
	{
		LOGDEBFMT("Raid%d:%d",i,vecDiskState[i]);
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
		LOGDEBFMT("Other %s State:nConnectState:%d,nSpeed:%d",node.strName.c_str(),
			node.nConnStatue,node.nRxSpeed);
	}
	return true;
}


//帮助信息打印tms状态
void CDataManager::PrintTMSState()
{
	LOGDEBFMT("TMS Current State:");
	LOGDEBFMT("bRun:%d",m_nTMSState);
}


//帮助信息打印raid状态
void CDataManager::PrintDiskState()
{
	m_csDisk.EnterCS();
	std::map<int,DiskInfo> &mapdf = m_mapdf;
	m_csDisk.LeaveCS();

	LOGDEBFMT("*****************Raid State************");
	std::map<int,DiskInfo>::iterator it = mapdf.begin();
	for(;it != mapdf.end();it++)
	{
		DiskInfo &df = it->second;

		LOGDEBFMT("diskGroup:%d",it->first);
		LOGDEBFMT("diskSize:%s",df.diskSize.c_str());
		std::transform(df.diskState.begin(),df.diskState.end(),df.diskState.begin(),::tolower);
		LOGDEBFMT("diskState:%s",df.diskState.c_str());
		LOGDEBFMT("diskNumberOfDrives:%s",df.diskNumOfDrives.c_str());
		LOGDEBFMT("-------------------Detail--------------");
		int nLen = df.diskDrives.size();
		for(int i = 0 ;i < nLen ;i ++)
		{	
			LOGDEBFMT("----------------%d----------------",i);
			LOGDEBFMT("dirveID:%s",df.diskDrives[i].driveID.c_str());
			LOGDEBFMT("dirveSlotNum:%s",df.diskDrives[i].driveSlotNum.c_str());
			LOGDEBFMT("dirveErrorCount:%s",df.diskDrives[i].driveErrorCount.c_str());
			LOGDEBFMT("dirveSize:%s",df.diskDrives[i].driveSize.c_str());
			std::transform(df.diskDrives[i].driveFirmwareState.begin(),
				df.diskDrives[i].driveFirmwareState.end(),df.diskDrives[i].driveFirmwareState.begin(),::tolower);
			LOGDEBFMT("dirveFirmwareState:%s",df.diskDrives[i].driveFirmwareState.c_str());
			LOGDEBFMT("dirveType:%s",df.diskDrives[i].driveType.c_str());
			LOGDEBFMT("dirveSpeed:%s",df.diskDrives[i].driveSpeed.c_str());
		}

	}
	LOGDEBFMT("---------------------------------------");
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
		LOGDEBFMT("hallid:%s",info.strId.c_str());
		LOGDEBFMT("SMS state:%d",info.stStatus.nStatus);
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
		LOGDEBFMT("EthName:%s",node.strName.c_str());
		LOGDEBFMT("TaskType:%d",node.nTaskType);
		LOGDEBFMT("ConnState:%d",node.nConnStatue);
		LOGDEBFMT("Speed:%d",node.nRxSpeed);
	}
}


bool  CDataManager::UpdateOtherSwitchState(int nSwitch1State,int nSwitch2State)
{
	//LOGDEBFMT("Other Switch nSwitch1State:%d,nSwitch2State:%d",nSwitch1State,nSwitch2State);
	return true;
}

bool  CDataManager::UpdateOtherSpeedLmtState(bool bEnableIngest,int nSpeedLimit)
{
	//LOGDEBFMT("Other SpeedLmt bEnableIngest:%d,nSpeedLimit:%d",bEnableIngest,nSpeedLimit);
	return true;
}

bool  CDataManager::UpdateOtherSMSEWState(int nState,std::string  strInfo,std::string  strHall)
{
	//LOGDEBFMT("Other SMSEW nState:%d,strInfo:%s,strHall:%s",nState,strInfo.c_str(),strHall.c_str());
	return true;
}



