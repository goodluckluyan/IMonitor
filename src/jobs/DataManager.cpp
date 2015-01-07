#include<algorithm>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include"para/C_Para.h"
#include"DataManager.h"
#include"log/C_LogManage.h"

#define  LOG(errid,msg)   C_LogManage::GetInstance()->WriteLog(LOG_FATAL,LOG_MODEL_JOBS,0,errid,msg)
#define  LOGINF(msg)	  C_LogManage::GetInstance()->WriteLog(LOG_INFO,LOG_MODEL_JOBS,0,0,msg)
#define  LOGINFFMT(fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(LOG_INFO,LOG_MODEL_JOBS,0,0,fmt,##__VA_ARGS__)
#define  LOGDEBFMT(fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(LOG_DEBUG,LOG_MODEL_JOBS,0,0,fmt,##__VA_ARGS__)
						

CDataManager *CDataManager::m_pinstance=NULL;
CDataManager::CDataManager()
{
	m_ptrInvoker = NULL;
	m_ptrDispatch = NULL;
	m_nOterHostFail = 0;
	time(&m_tmOtherHostFail);

	time(&m_tmCheckTMSNoRun);
	m_nCheckTMSNoRun = 0;
}
CDataManager::~CDataManager()
{
}

// ��ʼ��
bool CDataManager::Init(void * vptr)
{
	if(vptr != NULL)
	{
		m_ptrInvoker = vptr;
	}

	return true;
}

// ��������������Ϣ
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

// ����SMS������Ϣ
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

// ���¸���ģ��ļ������
bool CDataManager::UpdateDevStat(DiskInfo &df)
{
	m_csDisk.EnterCS();
	m_df = df;
	
    LOGDEBFMT("*****************Raid State************");
	LOGDEBFMT("diskSize:%s",df.diskSize.c_str());
	std::transform(m_df.diskState.begin(),m_df.diskState.end(),m_df.diskState.begin(),::tolower);
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
	    LOGDEBFMT("dirveSpeed:%s",df.diskDrives[i].driveSpeed.c_str());
	}
	LOGDEBFMT("---------------------------------------");
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

// �����̳����Ƿ��д���
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

// ��������״̬�������
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

//����SMS��״̬
bool CDataManager::UpdateSMSStat(std::string strHallID,int nState,std::string strInfo)
{
	m_csSMS.EnterCS();
	std::map<std::string,SMSInfo>::iterator it = m_mapSmsStatus.find(strHallID);
	if(it != m_mapSmsStatus.end())
	{
		SMSInfo &info = it->second;
		info.stStatus.nStatus = nState;
		
		//std::string strLocation;
		//strLocation = info.stStatus.nRun == 1 ? "Local" :"Other";
		LOGDEBFMT("SMS:%s(%d:%d) Status:%d  (%s)",strHallID.c_str(),info.nRole,info.stStatus.nRun,nState,strInfo.c_str());
	}
	m_csSMS.LeaveCS();

	
	return true;
}

//����SMS��״̬
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


// ����TMS��״̬
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

// ��ȡdisk������
bool CDataManager::GetDevStat(DiskInfo &df)
{
	m_csDisk.EnterCS();
	df = m_df; 
	m_csDisk.LeaveCS();   
}

// ��ȡ����״̬
bool CDataManager::GetNetStat(std::map<std::string,EthStatus> &mapEthStatus)
{
	m_csNet.EnterCS();
	mapEthStatus = m_mapEthStatus;
	m_csNet.LeaveCS();
	return true;
}

// ��ȡSMS״̬
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

// ��ȡSMS״̬
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

// ��ȡTMS��״̬
int CDataManager::GetTMSStat()
{
	return m_nTMSState;
}

// ��ȡInvokerָ��
void * CDataManager::GetInvokerPtr()
{
	return m_ptrInvoker;
}


//���¶Զ˵������״̬
bool CDataManager::UpdateOtherMonitorState(bool bMain,int nState)
{
	LOGDEBFMT("Other Monitor State:bMain:%d,nState:%d",bMain,nState);
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

		if(m_nOterHostFail > 3 && nCnt!=nSmsSize)
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

	// ���˶�����
	if(C_Para::GetInstance()->IsMain() == bMain && bMain )
	{
		// ������������ʱ������ʱ���Ļر�
		if(C_Para::GetInstance()->GetRole()==(int)TMPMAINROLE)
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
		else
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
	// ���˶��Ǳ�
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


	
	return true;
}


//���¶Զ�tms״̬
bool CDataManager::UpdateOtherTMSState(bool bRun,int nWorkState,int nState)
{
	LOGDEBFMT("Other TMS State:bRun:%d,nWorkState:%d,nState:%d",bRun,nWorkState,nState);
	return true;
}


//���¶Զ�sms״̬
bool CDataManager::UpdateOtherSMSState(std::vector<SMSStatus> &vecSMSStatus)
{
	int nLen = vecSMSStatus.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		LOGDEBFMT("Other SMS State:strHallId:%s,bRun:%d,nState:%d,nPosition:%d,spluuid:%s",vecSMSStatus[i].hallid.c_str(),
			vecSMSStatus[i].nRun,vecSMSStatus[i].nStatus,vecSMSStatus[i].nPosition,vecSMSStatus[i].strSPLUuid.c_str());
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

	// �������ṹ
// 	std::map<std::string,SMSInfo>::iterator it = m_mapOtherSMSStatus.begin();
// 	for(;it != m_mapOtherSMSStatus.end();it++)
// 	{
// 		SMSInfo &info = it->second;
// 		if(info.nRole == 1 && info.stStatus.nRun == 1)
// 		{
// 			m_csSMS.EnterCS();
// 			std::map<std::string,SMSInfo>::iterator fit= m_mapSmsStatus.find(info.strId);
// 			if(fit != m_mapSmsStatus.end())
// 			{
// 				it->second.stStatus.nRun = 2;
// 			}
// 			m_csSMS.LeaveCS();
// 		}
// 	}
	return true;
}

//���¶Զ�raid״̬
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

//���¶Զ�eth״̬
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


//������Ϣ��ӡtms״̬
void CDataManager::PrintTMSState()
{
	LOGDEBFMT("TMS Current State:");
	LOGDEBFMT("bRun:%d",m_nTMSState);
}


//������Ϣ��ӡraid״̬
void CDataManager::PrintDiskState()
{
	LOGDEBFMT("Number of RAID Disk :%s",m_df.diskNumOfDrives.c_str());
	LOGDEBFMT("RAID Disk State :%s",m_df.diskState.c_str());
	LOGDEBFMT("RAID Disk State: %s",m_df.diskSize.c_str());
}

//������Ϣ��ӡsms״̬
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

//������Ϣ��ӡeth״̬
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




