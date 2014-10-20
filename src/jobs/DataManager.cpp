#include "DataManager.h"
#include<stdio.h>
#include<stdlib.h>
#include"database/CppMySQL3DB.h"
#include"para/C_Para.h"

CDataManager *CDataManager::m_pinstance=NULL;
CDataManager::CDataManager()
{

}
CDataManager::~CDataManager()
{

}

// ��ʼ��
bool CDataManager::Init()
{
	C_Para *ptrPara = C_Para::GetInstance();
	CppMySQL3DB mysql;
	if(mysql.open(ptrPara->m_strDBServiceIP.c_str(),ptrPara->m_strDBUserName.c_str(),
		ptrPara->m_strDBPWD.c_str(),ptrPara->m_strDBName.c_str()) == -1)
	{
		printf("mysql open failed!\n");
		return false;
	}

	// ��ȡethinfo ,��ʼ��������Ϣ
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
	m_csDisk.LeaveCS();
	return true;
}

// ��������״̬�������
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

//����SMS��״̬
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


// ����TMS��״̬
bool CDataManager::UpdateTMSStat(int state)
{
	m_csTMS.EnterCS();
	m_nTMSState = state;
	m_csTMS.LeaveCS();
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
bool CDataManager::GetSMSStat()
{
	return true;
}

// ��ȡTMS��״̬
int CDataManager::GetTMSStat()
{
	return m_nTMSState;
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

bool CDataManager::UpdateOtherSMSState(std::string strHallId,bool bRun,int nState,
						 int nPosition,std::string strSplUuid)
{
	printf("Other SMS State:strHallId:%s,bRun:%d,nState:%d,nPosition:%d,spluuid:%s\n",strHallId.c_str(),
		bRun,nState,nPosition,strSplUuid.c_str());
	return true;
}

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

bool  CDataManager::UpdateOtherSwitchState(int nSwitch1State,int nSwitch2State)
{
	printf("Other Switch nSwitch1State:%d,nSwitch2State:%d\n",nSwitch1State,nSwitch2State);
	return true;
}

bool  CDataManager::UpdateOtherSpeedLmtState(bool bEnableIngest,int nSpeedLimit)
{
	printf("Other SpeedLmt bEnableIngest:%d,nSpeedLimit:%d\n",bEnableIngest,nSpeedLimit);
	return true;
}

bool  CDataManager::UpdateOtherSMSEWState(int nState,std::string  strInfo,std::string  strHall)
{
	printf("Other SMSEW nState:%d,strInfo:%s,strHall:%s\n",nState,strInfo.c_str(),strHall.c_str());
	return true;
}