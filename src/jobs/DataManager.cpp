#include<algorithm>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include"para/C_Para.h"
#include"DataManager.h"
#include"log/C_LogManage.h"
extern time_t g_tmDBSynch;
//extern int g_RunState;
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
	m_bSwitching=false;
	m_nSSDNum=0;
	m_nSataNum=0;
	m_nDBSynchState=-1;

	m_nOtherRaidStatus = -1;
	m_nLocalRaidStatus = -1;
}
CDataManager::~CDataManager()
{
}

// 初始化
bool CDataManager::Init(void * vptr,int nSSDNum,int nSataNum)
{
	if(vptr != NULL)
	{
		m_ptrInvoker = vptr;
	}

	// ssd 硬盘数量
	m_nSSDNum=nSSDNum;

	// sata 硬盘数量
	m_nSataNum=nSataNum;

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
// 	if(m_mapdf.size()==0)
// 	{
// 		m_mapdf = mapdf;
// 	}
// 	else
// 	{
// 		std::map<int,DiskInfo>::iterator it = mapdf.begin();
// 		for(;it!=mapdf.end();it++)
// 		{
// 			int index = it->first;
// 			std::map<int,DiskInfo>::iterator fmit = m_mapdf.find(index);
// 			if(fmit!=m_mapdf.end())
// 			{
// 				DiskInfo &mdi = fmit->second;
// 				DiskInfo &di = it->second;
// 
// 				mdi.diskGroup=di.diskGroup;
// 				mdi.diskNumOfDrives=di.diskNumOfDrives;
// 				mdi.diskSize=di.diskSize;
// 				mdi.diskState=di.diskState;
// 
// 				// mapdf到m_mapdf中查找对应的更新，没有对应的测添加
// 				std::map<std::string ,struct DiskDriveInfo>::iterator fddit = di.diskDrives.begin();
// 				for(;fddit!=di.diskDrives.end();fddit++)
// 				{
// 					std::string strDSN=fddit->first;
// 					DiskDriveInfo &ddi=fddit->second;
// 					if(strDSN.empty())
// 					{
// 						continue;
// 					}
// 
// 					std::map<std::string ,struct DiskDriveInfo>::iterator fddmit = mdi.diskDrives.find(strDSN);
// 					if(fddmit!=mdi.diskDrives.end())
// 					{
// 						DiskDriveInfo &mddi = fddmit->second;
// 						mddi=ddi;
// 					}
// 					else
// 					{
// 						mdi.diskDrives[strDSN]=ddi;
// 					}
// 
// 				}
// 
// 				// m_mapdf到mapdf中查找对应的更新，没有对应的认为是offline
// 				std::map<std::string ,struct DiskDriveInfo>::iterator fddmit = mdi.diskDrives.begin();
// 				for(;fddmit!=mdi.diskDrives.end();fddmit++)
// 				{
// 					std::string strDSN=fddmit->first;
// 					DiskDriveInfo &mddi=fddmit->second;
// 					if(strDSN.empty())
// 					{
// 						continue;
// 					}
// 
// 					std::map<std::string ,struct DiskDriveInfo>::iterator fddit = di.diskDrives.find(strDSN);
// 					if(fddit!=di.diskDrives.end())
// 					{
// 						DiskDriveInfo &ddi = fddit->second;
// 						mddi=ddi;
// 					}
// 					else
// 					{
// 						mddi.driveFirmwareState="offline";
// 					}
// 
// 				}
// 
// 			}
// 		}
// 	}
	std::map<int,DiskInfo> df=m_mapdf;
	m_csDisk.LeaveCS();

	std::map<std::string ,struct DiskDriveInfo>mapSN;
	std::map<int,DiskInfo>::iterator it = df.begin();
	for(;it!=df.end();it++)
	{
		DiskInfo &di = it->second;
		std::map<std::string ,struct DiskDriveInfo>::iterator ddmit = di.diskDrives.begin();
		for(;ddmit!=di.diskDrives.end();ddmit++)
		{
			mapSN[ddmit->first] = ddmit->second;
		}	
	}
	int nSum=m_nSSDNum+m_nSataNum;
	for(int i=0 ;i < nSum ;i++)
	{
		char buf[16]={'\0'};
		snprintf(buf,16,"%d",i);
		std::map<std::string ,struct DiskDriveInfo>::iterator fit = mapSN.find(buf);
		if(fit == mapSN.end())
		{
			DiskDriveInfo node;
			node.driveSlotNum = buf;
			node.driveFirmwareState="bad";
			mapSN[buf]=node;
			LOGDEBFMT("----------------NOT FOUND(SN:%d)----------------",i);
		
		}
	}
	
	LOGDEBFMT("*****************Raid State************");
	std::map<int,DiskInfo>::iterator mit = df.begin();
	for(;mit != df.end();mit++)
	{
		DiskInfo &di = mit->second;
		
		LOGDEBFMT("diskGroup:%d",mit->first);
		LOGDEBFMT("diskSize:%s",di.diskSize.c_str());
		std::transform(di.diskState.begin(),di.diskState.end(),di.diskState.begin(),::tolower);
		LOGDEBFMT("diskState:%s",di.diskState.c_str());
		LOGDEBFMT("diskNumberOfDrives:%s",di.diskNumOfDrives.c_str());
		LOGDEBFMT("-------------------Detail--------------");
		std::map<std::string ,struct DiskDriveInfo>::iterator diit = di.diskDrives.begin();
		int i=0;
		for( ;diit != di.diskDrives.end();diit++,i++)
		{	
			DiskDriveInfo &ddi=diit->second;
			LOGDEBFMT("----------------%d----------------",i);
			LOGDEBFMT("dirveID:%s",ddi.driveID.c_str());
			LOGDEBFMT("dirveSlotNum:%s",ddi.driveSlotNum.c_str());
			LOGDEBFMT("dirveErrorCount:%s",ddi.driveErrorCount.c_str());
			LOGDEBFMT("dirveSize:%s",ddi.driveSize.c_str());
			std::transform(ddi.driveFirmwareState.begin(),
				ddi.driveFirmwareState.end(),ddi.driveFirmwareState.begin(),::tolower);
			LOGDEBFMT("dirveFirmwareState:%s",ddi.driveFirmwareState.c_str());
			LOGDEBFMT("dirveType:%s",ddi.driveType.c_str());
			LOGDEBFMT("dirveSpeed:%s",ddi.driveSpeed.c_str());
		}
		
	}
    LOGDEBFMT("---------------------------------------");
	
	
	std::vector<stError> vecRE;
	if(!CheckRaidError(vecRE,mapSN))
	{
		if(m_ptrDispatch)
		{
			m_ptrDispatch->TriggerDispatch(RAIDTask,vecRE);
		}
		m_nLocalRaidStatus = 1;
	}
	else
	{
		m_nLocalRaidStatus = 0;
	}
	
	return true;
}

// 检测磁盘陈列是否有错误
bool CDataManager::CheckRaidError(std::vector<stError> &vecErr,
	std::map<std::string ,struct DiskDriveInfo> &mapdf)
{
	bool bRet = true;
	
	std::map<std::string,DiskDriveInfo>::iterator it = mapdf.begin();
	for(;it != mapdf.end();it++)
	{
		DiskDriveInfo &di = it->second;
		if(di.driveFirmwareState.find("Online") == std::string::npos)
		{
			stError re;
			re.ErrorName = "driveFirmwareState";
			re.ErrorVal = "bad";
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
	std::string strTmp;
	for(int i = 0 ;i < nLen ;i++)
	{
		std::map<std::string,EthStatus>::iterator fit = m_mapEthStatus.find(vecEthStatus[i].strName);
		if(fit != m_mapEthStatus.end())
		{
			fit->second.nConnStatue = vecEthStatus[i].nConnStatue;
			fit->second.nRxSpeed = vecEthStatus[i].nRxSpeed;
			fit->second.nTxSpeed = vecEthStatus[i].nTxSpeed;
			char buf[128]={'\0'};
			snprintf(buf,128," %s[st:%d,RxSp:%8llu,TxSp:%8llu]",vecEthStatus[i].strName.c_str(),
				vecEthStatus[i].nConnStatue,vecEthStatus[i].nRxSpeed,vecEthStatus[i].nTxSpeed);
			strTmp+=buf;
			if((i+1)%4==0||(i+1)==nLen)
			{
				LOGDEBFMT("Local Eth%s",strTmp.c_str());
				strTmp="";
			}
			
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
		std::string strRole = info.nRole==1?" main":info.nRole==3?"takeover":"slave";
		std::string strPos = info.stStatus.nRun == 1 ? "   local":"nonlocal";
		LOGDEBFMT("SMS:%s(%s:%s) Status:%3d  (%s)",strHallID.c_str(),
			strRole.c_str(),strPos.c_str(),nState,strInfo.c_str());
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
bool CDataManager::GetDevStat(std::map<int,DiskDriveInfo> &df)
{
	m_csDisk.EnterCS();
	std::map<int,DiskInfo> mapdf = m_mapdf; 
	m_csDisk.LeaveCS();   

	std::map<int,DiskInfo>::iterator it = mapdf.begin();
	for(;it!=mapdf.end();it++)
	{
		DiskInfo &di = it->second;
		std::map<std::string ,struct DiskDriveInfo>::iterator ddmit = di.diskDrives.begin();
		for(;ddmit!=di.diskDrives.end();ddmit++)
		{
			int index = atoi(ddmit->first.c_str());
			df[index] = ddmit->second;
		}	
	}
	int nSum=m_nSSDNum+m_nSataNum;
	for(int i=0 ;i < nSum ;i++)
	{
		std::map<int ,struct DiskDriveInfo>::iterator fit = df.find(i);
		if(fit == df.end())
		{
			DiskDriveInfo node;
			char buf[16]={'\0'};
			snprintf(buf,16,"%d",i);
			node.driveSlotNum = buf;
			node.driveFirmwareState="bad";
			df[i]=node;
		}
	}
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

// 获取对端调度的状态
int CDataManager::GetOtherIMonitor()
{
	return m_nOtherMonitorState;
}

// 获取对端Raid的状态
int CDataManager::GetOtherRaidStatus()
{
	return m_nOtherRaidStatus;
}

// 获取对端Raid的状态
int CDataManager::GetLocalRaidStatus()
{
	return m_nLocalRaidStatus;
}


// 获取Invoker指针
void * CDataManager::GetInvokerPtr()
{
	return m_ptrInvoker;
}

//更新对端tms状态
bool CDataManager::UpdateOtherTMSState(bool bRun,int nWorkState,int nState)
{
	LOGDEBFMT("Other TMS State:bRun:%d,nWorkState:%d,nState:%d",bRun,nWorkState,nState);
	return true;
}

//更新对端调度软件状态
bool CDataManager::UpdateOtherMonitorState(bool bMain,int nState,long lSynch)
{
	LOGDEBFMT("Other Monitor State:bMain:%d,nState:%d,lSynch:%lld",bMain,nState,lSynch);
	m_nOtherMonitorState = nState;

	int nRunState = GlobalStatus::GetInstinct()->GetStatus();
	// 只有在刚启动时才能赋值并退出，否则进行冲突判断
	//if(0 == g_RunState && lSynch!=0 && (nState == TMPMAINROLE || nState == ONLYMAINROLE))
	if(0 == nRunState && lSynch!=0 && (nState == TMPMAINROLE || nState == ONLYMAINROLE))
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

	

	// 启动正常后，nState的状态为对端机所处的角色
	// 两端都是主
	if(C_Para::GetInstance()->IsMain() == bMain && bMain )
	{
		int nRunState = GlobalStatus::GetInstinct()->GetStatus();
		//本机为备机时 发现主机出现时并且主机角色为1时，把临时主改回备
		//if(C_Para::GetInstance()->GetRole()==(int)TMPMAINROLE && g_RunState == 1)
		if(C_Para::GetInstance()->GetRole()==(int)TMPMAINROLE && nRunState == 1 && nState == MAINROLE)
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
		else if(nState == MAINROLE && nRunState == 1)
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
		else if(C_Para::GetInstance()->GetRole()==(int)ONLYMAINROLE &&  nRunState == 1 && nState == TMPMAINROLE)
		{
			//脑裂恢复
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
		
	}
	// 两端都是备
	else if(C_Para::GetInstance()->IsMain() == bMain && !bMain && nRunState == 1)
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
	else if(C_Para::GetInstance()->GetRole()==(int)ONLYMAINROLE && !bMain && nRunState == 1 && nState == STDBYROLE)
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


// 	// 由于一些原因，可能会出现正常启动后，g_tmDBSynch没有恢复0的情况，所以在这判断一下。
// 	if(bMain && !C_Para::GetInstance()->IsMain() && g_tmDBSynch != 0)
// 	{
// 		g_tmDBSynch=0;
// 	}
	return true;
}

//更新对端sms状态
bool CDataManager::UpdateOtherSMSState(std::vector<SMSStatus> &vecSMSStatus)
{
	m_csOtherSMS.EnterCS();
	int nLen = vecSMSStatus.size();
	std::string strTmp;
	for(int i = 0 ;i < nLen ;i++)
	{
		std::string pos ;
		int bMain = C_Para::GetInstance()->IsMain();
		if(bMain)
		{
			pos = vecSMSStatus[i].nRun==1 ? "slave":"main";
		}
		else
		{
			pos = vecSMSStatus[i].nRun==1 ? "main":"slave";
		}
		char buf[64]={'\0'};
		snprintf(buf,64," %s:[po:%5s,st:%3d]",vecSMSStatus[i].hallid.c_str(),
			pos.c_str(),vecSMSStatus[i].nStatus);
		strTmp+=buf;
// 		if(vecSMSStatus[i].nRun == 0)
// 		{
// 			continue;
// 		}

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
		
		if((i+1)%4==0 || (i+1) == nLen)
		{
			LOGDEBFMT("Other Host SMS%s",strTmp.c_str());
			strTmp="";
		}
	}
	m_csOtherSMS.LeaveCS();
	

	if(nLen > 0)
	{
		time(&m_tmUpdateOSMS);
	}

	int nRunState = GlobalStatus::GetInstinct()->GetStatus();

	//在接管和恢复接管状态及正在处理冲突时不进行判断和解决冲突
	//if(g_RunState==2 || g_RunState==3 )
	if(nRunState==2 || nRunState==3 )
	{
		LOGDEBFMT("localhost run status: %d, not conflict check",nRunState);
		return true;
	}

	//只有主机才能判断和解决冲突
	if(C_Para::GetInstance()->GetRole()!=(int)MAINROLE)
	{
		return true;
	}

	// 当对方角色不为正常备时（包括未知和处于恢复和冲突状态）不能进行冲突判断
	if(m_nOtherMonitorState != 3 )
	{
		LOGDEBFMT("slave host run status: %d(not eq 3), not conflict check",m_nOtherMonitorState);
		return true;
	}


	// 判断冲突
	m_csSMS.EnterCS();
	std::map<std::string,SMSInfo>maptmp = m_mapSmsStatus;
	m_csSMS.LeaveCS();

	int nStdbyRun =0;
	std::vector<ConflictInfo> vecConflict;
	m_csSwitch.EnterCS();
	int bSwitching = m_bSwitching;
	m_csSwitch.LeaveCS();

	m_csOtherSMS.EnterCS();
	std::map<std::string,SMSInfo> mapOtherSMSStatus = m_mapOtherSMSStatus;
	m_csOtherSMS.LeaveCS();

	if(!bSwitching)//在切换期间不进行冲突判断
	{
		std::map<std::string,SMSInfo>::iterator it = mapOtherSMSStatus.begin();
		for(;it != mapOtherSMSStatus.end();it++)
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
			if(Other.stStatus.nRun == 1 && fit->second.stStatus.nRun == 1 )
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
// 			if(Other.stStatus.nRun == 2 && fit->second.stStatus.nRun == 2)
// 			{
// 				ConflictInfo ci;
// 				ci.nMainState=fit->second.stStatus.nStatus;
// 				ci.nStdbyState=Other.stStatus.nStatus;
// 				ci.nType = 2;
// 				ci.strHallID=Other.strId;
// 				ci.tmTime=time();
// 				vecConflict.push_back(ci);
// 			}
		}
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

		GlobalStatus::GetInstinct()->SetStatus(3);// 设置全局状态为处理冲突状态
		//g_RunState=3; // 设置全局状态为处理冲突状态

	}

	return true;
}


time_t CDataManager::GetOtherSMSstatus(std::vector<SMSStatus> &vecSMSStatus )
{
	m_csOtherSMS.EnterCS();
	std::map<std::string,SMSInfo>::iterator it = m_mapOtherSMSStatus.begin();
	for(;it != m_mapOtherSMSStatus.end();it++)
	{
		SMSStatus status;
		status=it->second.stStatus;
		vecSMSStatus.push_back(status);
	}
	m_csOtherSMS.LeaveCS();

	return m_tmUpdateOSMS;
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
	std::string raidinfo;
	for(int i=0;i<vecDiskState.size();i++)
	{
		char buf[8]={'\0'};
		snprintf(buf,sizeof(buf)," %d:%d ",i,vecDiskState[i]);
		raidinfo+=buf;
	}
	LOGDEBFMT("Other Raid:State:%d,RS:%d,WS:%d Raid[%s]",nState,nReadSpeed,nWriteSpeed,raidinfo.c_str());
	m_nOtherRaidStatus = nState;
	return true;
}

//更新对端eth状态
bool  CDataManager::UpdateOtherEthState(std::vector<EthStatus> &vecEthStatus)
{
	std::string strTmp;
	int nlen = vecEthStatus.size();
	for(int i = 0 ;i < nlen ;i++)
	{
		char buf[36]={'\0'};
		EthStatus &node = vecEthStatus[i];
		snprintf(buf,36," %s:[st:%2d,sp:%8d]",node.strName.c_str(),node.nConnStatue,node.nRxSpeed);
		strTmp += buf;
		if((i+1)%4==0 || (i+1) == nlen)
		{
			LOGDEBFMT("Other NetCard:%s",strTmp.c_str());
			strTmp="";
		}
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
		std::map<std::string ,struct DiskDriveInfo>::iterator it = df.diskDrives.begin();
		int i=0;
		for(;it != df.diskDrives.end();it++,i++)
		{	
			struct DiskDriveInfo &ddi=it->second;
			LOGDEBFMT("----------------%d----------------",i);
			LOGDEBFMT("dirveID:%s",ddi.driveID.c_str());
			LOGDEBFMT("dirveSlotNum:%s",ddi.driveSlotNum.c_str());
			LOGDEBFMT("dirveErrorCount:%s",ddi.driveErrorCount.c_str());
			LOGDEBFMT("dirveSize:%s",ddi.driveSize.c_str());
			std::transform(ddi.driveFirmwareState.begin(),
				ddi.driveFirmwareState.end(),ddi.driveFirmwareState.begin(),::tolower);
			LOGDEBFMT("dirveFirmwareState:%s",ddi.driveFirmwareState.c_str());
			LOGDEBFMT("dirveType:%s",ddi.driveType.c_str());
			LOGDEBFMT("dirveSpeed:%s",ddi.driveSpeed.c_str());
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

bool CDataManager::GetDBSynchStatus(int &state)
{
	state = m_nDBSynchState;
	return true;
}

