#include "mons.nsmap"
#include "soapH.h"
#include <iostream>
#include "para/C_Para.h"
#include "jobs/DataManager.h"
using namespace std;

int mons__GetMontorState(struct soap*, struct mons__MontorStateRes &ret)
{
	ret.bMain = C_Para::GetInstance()->m_bMain;
	ret.iState = 0xff;
	//在此添加代码
	return 0;
}

int mons__GetTMSState(struct soap*, struct mons__TmsStateRes &ret)
{
	
	ret.bRun = 1;
	ret.iState = 0x01;
	ret.iWorkState = 0x0f;
	return 0;
}

int mons__GetSMSState(struct soap*, struct mons__SMSState &ret)
{
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
	pDM->GetDevStat(df);
	int nLen = df.diskDrives.size();
	int nSpeed = 0;
	for(int i = 0 ;i < nLen ; i++)
	{
		// 取最小值
		if(nSpeed == 0 || nSpeed > df.diskDrives[i].driveSpeed)
		{
			nSpeed = df.diskDrives[i].driveSpeed;
		}
		ret.diskState.push_back(df.diskDrives[i].driveFirmwareState);
	}
	ret.ReadSpeed = nSpeed;
	ret.WriteSpeed = nSpeed;
	ret.state = 0x01;

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
		node.eth=Ethinfo.strName;
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
