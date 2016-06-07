#include "mons.nsmap"
#include "soapH.h"
#include <iostream>
#include <cstdio>
#include "para/C_Para.h"
#include "jobs/DataManager.h"
#include "jobs/Invoke.h"
#include "log/C_LogManage.h"
using namespace std;
extern int g_RunState;
extern time_t g_tmDBSynch;
#define  LOGINFFMT(errid,fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_INFO,LOG_MODEL_JOBS,0,errid,fmt,##__VA_ARGS__)
int mons__GetMontorState(struct soap* cSoap, struct mons__MontorStateRes &ret)
{
	int nRole = C_Para::GetInstance()->GetRole();
	ret.bMain = C_Para::GetInstance()->IsMain();

	if((nRole == 4 || nRole == 2) /*&& 0 == g_tmDBSynch*/)// TMPMAINROLE
	{
		CDataManager *pDM = CDataManager::GetInstance();
		time_t tm;
		time(&tm);
		ret.lSynch=tm;
	}
	else
	{
		ret.lSynch = 0;
	}

	// 如果正在启动时状态为0，启动完成后状态为本机现在的角色
	ret.iState = (0 == g_RunState || 2 == g_RunState || 3 == g_RunState) ? 0 :nRole;
	
	return 0;
}

int mons__GetTMSState(struct soap* cSoap, struct mons__TmsStateRes &ret)
{
	CDataManager *pDM = CDataManager::GetInstance();
	ret.bRun = pDM->GetTMSStat() == 0 ? 1 : 0;
	ret.iState = 0x01;
	ret.iWorkState = 0x0f;
	return 0;
}

int mons__GetSMSState(struct soap* cSoap , std::vector<struct mons__SMSState> &vecret)
{
	CDataManager *pDM = CDataManager::GetInstance();
	std::vector<SMSStatus> vecSMSState;
	// 处于恢复状态不进行状态获取
	if(g_RunState==2)
	{
		return 0;
	}
	pDM->GetSMSStat(vecSMSState);
	int nLen = vecSMSState.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		mons__SMSState node;
		node.HallId = vecSMSState[i].hallid;
		node.bRun = vecSMSState[i].nRun;
		node.state = vecSMSState[i].nStatus<<8;
		switch(vecSMSState[i].nStatus)
		{
		case 101:
		case 201:
		case 202:
		case 203:
		case 204:
		case 206:
		case 301:
		case 302:
		case 303:
		case 304:
		case 305:
		case 401:
		case 402:
		case 403:
		case 404:
			node.state |= 0;
			break;
		case 102:
		case 103:
			node.state |= 1;
			break;
		case 205:
		case 306:
		case 405:
			node.state |= 2;
			break;
		}
	
		node.position = 25;//vecSMSState[i].nPosition;
		node.strSplUuid = "C80CF8FC-D6F5-26F8-3927-E30B0AE06C56";//vecSMSState[i].strSPLUuid;
		vecret.push_back(node);
	}
	
	return 0;
}

int mons__GetRaidtate(struct soap* cSoap, struct mons__RaidStateRes &ret)
{
	CDataManager *pDM = CDataManager::GetInstance();
	std::map<int,DiskDriveInfo> mapdf;
	pDM->GetDevStat(mapdf);

	if(mapdf.size() == 0)
	{
		ret.ReadSpeed = 0;
		ret.WriteSpeed = 0;
		ret.state = 1;
		return 0;
	}

	int nSpeed = 0;
	bool bError = false;
	int nSum=mapdf.size();
	for(int i=0;i<nSum;i++)
	{
		// 取最小值.
		std::map<int,DiskDriveInfo>::iterator it = mapdf.find(i);
		if(it!=mapdf.end())
		{
			struct DiskDriveInfo &ddi = it->second;
			int nDriveSpeed = ddi.driveSpeed.empty() ? 0:atoi(ddi.driveSpeed.c_str());
			if(nSpeed == 0 || nSpeed > nDriveSpeed )
			{
				nSpeed = nDriveSpeed;
			}
			int nPos = ddi.driveFirmwareState.find("Online");
			if(nPos != std::string::npos)
			{
				ret.diskState.push_back(0);
				bError = false||bError;
			}
			else
			{
				ret.diskState.push_back(1);
				bError = true||bError;
			}
		}
		else
		{
			ret.diskState.push_back(0);
			bError = false||bError;
		}
		
	}
	

	ret.ReadSpeed = nSpeed;
	ret.WriteSpeed = nSpeed;
	ret.state = bError ? 1 : 0;

	return 0;
}

int mons__GetEthState(struct soap* cSoap, std::vector<struct mons__ethstate> &vecret)
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

int mons__GetSwitchState(struct soap* cSoap, struct mons__SwitchStateRes &ret)
{
	ret.Switch1State = 1;
	ret.Switch2State = 2;
	return 0;
}

int mons__GetIngestSpeedLimit(struct soap* cSoap, struct mons__IngestSpeedLimitRes &ret)
{
	CDataManager *pDM = CDataManager::GetInstance();
	std::vector<SMSStatus> vecSMSState;

	int nRun = 0 ;
	pDM->GetSMSStat(vecSMSState);
	int nLen = vecSMSState.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		if(vecSMSState[i].nRun == 1 && vecSMSState[i].nStatus == 201)
		{
			nRun++;
		}
	}

	if(nRun <= 1)
	{
		ret.bEnableIngest =1 ;
		ret.speedLimit = -1;
	}
	else if(nRun == 2)
	{
		ret.bEnableIngest =1 ;
		ret.speedLimit = 20;
	}
	else if(nRun == 3)
	{
		ret.bEnableIngest =1 ;
		ret.speedLimit = 15;
	}
	else if(nRun == 5)
	{
		ret.bEnableIngest =1 ;
		ret.speedLimit = 10;
	}
	else if(nRun >= 6)
	{
		ret.bEnableIngest =1 ;
		ret.speedLimit = 0;
	}

	return 0;
}

int mons__GetWorkState_USCORECS(struct soap* cSoap, struct mons__WorkStateRes &ret)
{
	ret.hall = "tms01";
	ret.state = 1;
	ret.info = "error 0081";
	return 0;
}

int mons__ExeSwitchTMSToOther(struct soap* cSoap,int &ret)
{
// 	CDataManager *pDM = CDataManager::GetInstance();
// 	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
// 	if(ptr->SwitchTMS())
// 	{	
// 		ret = 0;
// 		return 0;
// 	}
// 	else
// 	{
// 		soap_sender_fault_subcode(cSoap, "mons:1", "Switch Fail", "SwitchTMS");
// 		ret = 1;
// 		return 1;
// 	}
	return 0;
}


int mons__ExeSwitchSMSToOther(struct soap* cSoap,std::string strHallID,int &ret)
{
	LOGINFFMT(0,"WS:ExeSwitchSMSToOther(%s)",strHallID.c_str());
	CDataManager *pDM = CDataManager::GetInstance();
	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
	int nState;
	if(ptr->SwitchSMS(strHallID,false,nState))
	{	
		ret = nState;
	}
	else
	{
		ret = nState;
	}
	return 0;
}

int mons__ExeSwitchSMSToOtherDelay(struct soap* cSoap,std::string strHallID,int &ret)
{
	LOGINFFMT(0,"WS:ExeSwitchSMSToOtherDelay(%s)",strHallID.c_str());
	CDataManager *pDM = CDataManager::GetInstance();
	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
	int nState;
	if(ptr->SwitchSMS(strHallID,true,nState))
	{	
		ret = nState;
	}
	else
	{
		ret = nState;
	}
	
	return 0;
	
}

int mons__ExeCloseSMS(struct soap* cSoap,std::string strHallID,int &ret)
{
	LOGINFFMT(0,"WS:ExeCloseSMS(%s)",strHallID.c_str());
	CDataManager *pDM = CDataManager::GetInstance();
	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
	if(ptr->CloseSMS(strHallID))
	{	
		ret = 0;
	}
	else
	{
		ret = 1;
	}
	return 0;
}

int mons__ExeStartSMS(struct soap* cSoap,std::string strHallID,int &ret)
{
	LOGINFFMT(0,"WS:ExeStartSMS(%s)",strHallID.c_str());
	CDataManager *pDM = CDataManager::GetInstance();
	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
	if(ptr->StartSMS(strHallID))
	{	
		ret = 0;		
	}
	else
	{
		ret = 1;
	}
	return 0;
}

int mons__ExeDcpHashCheck(struct soap* cSoap,std::string path,std::string PklUuid,std::string& errorInfo)
{
	LOGINFFMT(0,"WS:ExeDcpHashCheck(path:%s,uuid:%s)",path.c_str(),PklUuid.c_str());
	CDataManager *pDM = CDataManager::GetInstance();
	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
	ptr->DcpHashCheck(path,PklUuid,errorInfo);
	return 0;
}


int mons__ExeGetHashCheckPercent(struct soap* cSoap,std::string PklUuid,struct mons__HashCheckPercent& stResult)
{
	LOGINFFMT(0,"WS:ExeGetHashCheckPercent(uuid:%s)",PklUuid.c_str());
	CDataManager *pDM = CDataManager::GetInstance();
	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
	ptr->GetHashCheckPercent(PklUuid,stResult.Result,stResult.nPercent,stResult.errorInfo);
	return 0;
}

int mons__CopyDcp(struct soap* cSoap,std::string PklUuid,std::string srcPath,std::string desPath,
				  struct mons__FileOperationResult &result)
{
	LOGINFFMT(0,"WS:CopyDcp(src:%s,dest:%s,uuid:%s)",srcPath.c_str(),desPath.c_str(),PklUuid.c_str());
	CDataManager *pDM = CDataManager::GetInstance();
	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
	ptr->CopyDcp(PklUuid,srcPath,desPath,result.Result,result.errorInfo);
	return 0;
}

int mons__GetCopyDcpProgress(struct soap* cSoap,std::string PklUuid,struct mons__FileOperationResult &result)
{
	LOGINFFMT(0,"WS:GetCopyDcpProgress(uuid:%s)",PklUuid.c_str());
	CDataManager *pDM = CDataManager::GetInstance();
	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
	ptr->GetCopyDcpProgress(PklUuid,result.Result,result.errorInfo);
	return 0;
}

int mons__DeleteDcp(struct soap* cSoap,std::string PklUuid,std::string Path,struct mons__FileOperationResult &result)
{
	LOGINFFMT(0,"WS:DeleteDcp(path:%s,uuid:%s)",Path.c_str(),PklUuid.c_str());
	CDataManager *pDM = CDataManager::GetInstance();
	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
	ptr->DeleteDcp(PklUuid,Path,result.Result,result.errorInfo);
	return 0;
}

int mons__GetDeleteDcpProgress(struct soap* cSoap,std::string PklUuid,struct mons__FileOperationResult &result)
{
	LOGINFFMT(0,"WS:GetDeleteDcpProgress(uuid:%s)",PklUuid.c_str());
	CDataManager *pDM = CDataManager::GetInstance();
	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
	ptr->GetDeleteDcpProgress(PklUuid,result.Result,result.errorInfo);
	return 0;
}

int mons__SetDBSynchSign(struct soap* cSoap,std::string dbsynch,int &ret)
{
	LOGINFFMT(0,"WS:SetDBSynchSign(%s)",dbsynch.c_str());
	CDataManager *pDM = CDataManager::GetInstance();
	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
	if(ptr->UpdateDBSynch(dbsynch))
	{
		return 0;
	}
	else
	{
		return 0;
//		soap_sender_fault_subcode(cSoap, "mons:1", "SetDBSynchSign Fail", "SetDBSynchSign");
	}
	
}

int mons__GetSMSPosition(struct soap* cSoap,std::string strHallId,struct mons__SMSPositionInfo &PosInfo)
{
	LOGINFFMT(0,"WS:GetSMSPosition(%s)",strHallId.c_str());
	CDataManager *pDM = CDataManager::GetInstance();
	CInvoke *ptr = (CInvoke * )pDM->GetInvokerPtr();
	std::string strPos;
	int nPort;
	if(ptr->GetSMSPosition(strHallId,strPos,nPort))
	{	
		PosInfo.strPos=strPos;
		PosInfo.nPort=nPort;
		return 0;
	}
	else
	{
		soap_sender_fault_subcode(cSoap, "mons:1", "GetSMSPosition Fail", "GetSMSPosition");
		return 1;
	}
}

int mons__GetDBSynchStatus(struct soap* cSoap,int &state)
{
	LOGINFFMT(0,"WS:GetDBSynchStatus(%s)");
	CDataManager *pDM = CDataManager::GetInstance();
	int nDBSynch;
	if(pDM->GetDBSynchStatus(nDBSynch))
	{
		state=nDBSynch==0?0:1;
		return 0;
	}
	else
	{
		soap_sender_fault_subcode(cSoap, "mons:1", "GetDBSynchStatus Fail", "GetDBSynchStatus");
		return 1;
	}
}