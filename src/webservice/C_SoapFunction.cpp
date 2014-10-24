#include "mons.nsmap"
#include "soapH.h"
#include <iostream>
#include <cstdio>
#include "para/C_Para.h"
#include "jobs/DataManager.h"
#include "jobs/Invoke.h"
using namespace std;

int mons__GetMontorState(struct soap*, struct mons__MontorStateRes &ret)
{
	ret.bMain = C_Para::GetInstance()->m_bMain;
	ret.iState = 0;
	return 0;
}

int mons__GetTMSState(struct soap*, struct mons__TmsStateRes &ret)
{
	CDataManager *pDM = CDataManager::GetInstance();
	ret.bRun = pDM->GetTMSStat() == 0 ? 1 : 0;
	ret.iState = 0x01;
	ret.iWorkState = 0x0f;
	return 0;
}

int mons__GetSMSState(struct soap*, struct mons__SMSState &ret)
{
	CDataManager *pDM = CDataManager::GetInstance();
	ret.bRun = 1;
	ret.HallId = "tms01";
	ret.position = 0x25;
	ret.state = 0x01;
	ret.strSplUuid = "asdfasdfasdfasdfasdfasdf";
	return 0;
}

int mons__GetRaidtate(struct soap*, struct mons__RaidStateRes &ret)
{
	CDataManager *pDM = CDataManager::GetInstance();
	DiskInfo df;
	//pDM->GetDevStat(df);
	df.diskState = "1" ;
	df.diskSize = "8000000000";
	df.diskNumOfDrives = "8" ;
	for(int i = 0 ;i < 8 ;i++)
	{
		DiskDriveInfo node;
		node.driveErrorCount = "0";
		node.driveFirmwareState = "1";
		char buf[16]={'\0'};
		snprintf(buf,16,"%d",i);
		node.driveID = buf;
		node.driveSize = "1000000000";
		node.driveSlotNum = buf;
		node.driveSpeed = "1000000";
		df.diskDrives.push_back(node);
	}

	int nLen = df.diskDrives.size();
	int nSpeed = 0;
	for(int i = 0 ;i < nLen ; i++)
	{
		// 取最小值.
// 		int nDriveSpeed = atoi(df.diskDrives[i].driveSpeed.c_str());
// 		if(nSpeed == 0 || nSpeed > nDriveSpeed )
// 		{
// 			nSpeed = nDriveSpeed;
// 		}
		ret.diskState.push_back(atoi(df.diskDrives[i].driveFirmwareState.c_str()));
 	}

	ret.ReadSpeed = 1000000;
	ret.WriteSpeed = 1000000;
	ret.state =atoi( df.diskState.c_str());

	return 0;
}

int mons__GetEthState(struct soap*, std::vector<struct mons__ethstate> &vecret)
{
	CDataManager *pDM = CDataManager::GetInstance();
	std::map<std::string,EthStatus> mapEthStatus;
	pDM->GetNetStat(mapEthStatus);
	std::map<std::string,EthStatus>::iterator it = mapEthStatus.begin();
	for(;it != mapEthStatus.end();it++)
	{
		mons__ethstate node;
		EthStatus &Ethinfo = it->second;
		node.eth=atoi(Ethinfo.strName.substr(3).c_str());
		node.type = Ethinfo.nTaskType;
		node.ConnectState = Ethinfo.nConnStatue;
		node.speed = (Ethinfo.nRxSpeed + Ethinfo.nTxSpeed)/2;
		vecret.push_back(node);
	}
	 

	return 0;
}

int mons__GetSwitchState(struct soap*, struct mons__SwitchStateRes &ret)
{
	ret.Switch1State = 1;
	ret.Switch2State = 2;
	return 0;
}

int mons__GetIngestSpeedLimit(struct soap*, struct mons__IngestSpeedLimitRes &ret)
{
	ret.bEnableIngest =1 ;
	ret.speedLimit = 1000;
	return 0;
}

int mons__GetWorkState_USCORECS(struct soap*, struct mons__WorkStateRes &ret)
{
	ret.hall = "tms01";
	ret.state = 1;
	ret.info = "error 0081";
	return 0;
}

int mons__ExeSwitchTMSToOther(struct soap* cSoap,int &ret)
{
	CDataManager *pDM = CDataManager::GetInstance();
	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
	if(ptr->SwitchTMS())
	{	
		ret = 0;
		return 0;
	}
	else
	{
		soap_sender_fault_subcode(cSoap, "mons:1", "Switch Fail", "SwitchTMS");
		ret = 1;
		return 1;
	}
}