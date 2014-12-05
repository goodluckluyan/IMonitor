//@file:C_Para.cpp
//@brief: ������C_Para �ķ���ʵ��
//@author:wangzhongping@oristartech.com
//dade:2012-07-12

#include <stdlib.h>
#include <string.h>
#include "C_Para.h"
#include "ec_config.h"

extern int g_LogLevel;
C_Para *C_Para::m_pInstance = NULL;

C_Para::C_Para()
{
	m_nStartSMSType = 1;
	m_nTimeOutWaitOtherIMonitor = 300;
	m_nDiskCheckDelay = 300;
	m_nEthCheckDelay = 10;
	m_nHallListCheckDelay = 5;
	m_nTMSCheckDelay = 5;
	m_nOtherMonitorCheckDelay = 10;
	m_nOtherTMSCheckDelay = 10;
	m_nOtherSMSCheckDelay = 5;
	m_nOtherRAIDCheckDelay = 5;
	m_nOtherEthCheckDelay = 5;
	m_nOtherSwitchCheckDelay = 5;
	m_nOtherSpeedLmtCheckDelay = 5;
	m_nOtherEWCheckDelay = 5;

}
C_Para::~C_Para()
{

}
C_Para* C_Para::GetInstance()
{
	if(m_pInstance == NULL)
	{
		m_pInstance = new C_Para;
	}
	return m_pInstance;
}

void  C_Para::DestoryInstance()
{
	if(m_pInstance != NULL)
	{
		delete m_pInstance ;
		m_pInstance = NULL;
	}

}
int C_Para::ReadPara()
{
	int iResult = -1;
	ec_config config;
	char a[64];
	memset(a,0,64);

	char tmp[256];
	char buf[256];
	memset(tmp, 0, 256);
	memset(buf, 0, 256);

	sprintf(tmp,"/proc/%d/exe",getpid());
	readlink(tmp,buf,256);
	string str = buf;
	size_t iPos = -1;
	if((iPos =str.rfind('/')) == string::npos)
	{
		return -1;
	}
	m_strInipath = str.substr(0,iPos);
	string strInipath = m_strInipath;
	strInipath += "/para.ini";

	iResult = config.readvalue("PARA","DBServiceIP", a, strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_strDBServiceIP = a;

	memset(a,0,64);
	iResult = config.readvalue("PARA","DBServicePort",a, strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	int iTmp  =  atoi(a); 
	if(iTmp <= 0)
	{
		return -1;
	}
	m_usDBServicePort = (unsigned short) iTmp;

	memset(a,0,64);

	iResult = config.readvalue("PARA","DBName",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_strDBName = a;
	memset(a,0,64);    

	iResult = config.readvalue("PARA","DBUserName",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_strDBUserName = a;
	memset(a,0,64);
	iResult = config.readvalue("PARA","DBPWD",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_strDBPWD = a;    



// 	memset(a,0,64);
// 	iResult = config.readvalue("PARA","ThreadCount",a,strInipath.c_str());
// 	if(iResult != 0)
// 	{
// 		return iResult;
// 	}
// 	iTmp  =  atoi(a); 
// 	if(iTmp <= 0)
// 	{
// 		return -1;
// 	}
// 	m_uiThreadCount = (unsigned int)iTmp;  

	memset(a,0,64);
	iResult = config.readvalue("PARA","LogPath",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_strLogPath = a; 

	memset(a,0,64);
	iResult = config.readvalue("PARA","Main",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_bMain = atoi(a) == 1 ?true: false;

	// 	memset(a,0,64);
	// 	iResult = config.readvalue("PARA","WebServiceIP",a,strInipath.c_str());
	// 	if(iResult != 0)
	// 	{
	// 		return iResult;
	// 	}
	// 	m_strWebServiceIP = a ;

	memset(a,0,64);
	iResult = config.readvalue("PARA","WebServicePort",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_nWebServicePort = atoi(a) ;
	m_nWebServicePort = m_nWebServicePort <= 0 ?12316 :m_nWebServicePort;


	memset(a,0,64);
	iResult = config.readvalue("PARA","OtherMonitorIP",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_strOIP = a;

	memset(a,0,64);
	iResult = config.readvalue("PARA","OtherMonitorPort",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_nOPort = atoi(a);

// 	memset(a,0,64);
// 	iResult = config.readvalue("PARA","OtherMonitorURI",a,strInipath.c_str());
// 	if(iResult != 0)
// 	{
// 		return iResult;
// 	}
// 	m_strOURI = a;

	memset(a,0,64);
	iResult = config.readvalue("PARA","TMSPath",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_strTMSPath = a;

	memset(a,0,64);
	iResult = config.readvalue("PARA","TMSWSPort",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_nTMSWSPort = atoi(a);

	memset(a,0,64);
	iResult = config.readvalue("PARA","StartTMSSMSType",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_nStartSMSType = atoi(a);

	memset(a,0,64);
	iResult = config.readvalue("PARA","TimeOutWaitOtherIMonitor",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_nTimeOutWaitOtherIMonitor = atoi(a);

	memset(a,0,64);
	iResult = config.readvalue("PARA","TOMCATPath",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_strTOMCATPath = a;

	memset(a,0,64);
	iResult = config.readvalue("PARA","WriteLogLevel",a,strInipath.c_str());
	if(iResult != 0)
	{
		return iResult;
	}
	m_nWirteLogLevel = atoi(a);

	memset(a,0,64);
	iResult = config.readvalue("PARA","DiskCheckDelay",a,strInipath.c_str());
	if(iResult == 0)
	{
		m_nDiskCheckDelay = atoi(a);
	}

	memset(a,0,64);
	iResult = config.readvalue("PARA","EthCheckDelay",a,strInipath.c_str());
	if(iResult == 0)
	{
		m_nEthCheckDelay = atoi(a);
	}

	memset(a,0,64);
	iResult = config.readvalue("PARA","HallListCheckDelay",a,strInipath.c_str());
	if(iResult == 0)
	{
		m_nHallListCheckDelay = atoi(a);
	}

	memset(a,0,64);
	iResult = config.readvalue("PARA","TMSCheckDelay",a,strInipath.c_str());
	if(iResult == 0)
	{
		m_nTMSCheckDelay = atoi(a);
	}
	
	memset(a,0,64);
	iResult = config.readvalue("PARA","OtherMonitorCheckDelay",a,strInipath.c_str());
	if(iResult == 0)
	{
		m_nOtherMonitorCheckDelay = atoi(a);
	}
	
	memset(a,0,64);
	iResult = config.readvalue("PARA","OtherTMSCheckDelay",a,strInipath.c_str());
	if(iResult == 0)
	{
		m_nOtherTMSCheckDelay = atoi(a);
	}
	
	memset(a,0,64);
	iResult = config.readvalue("PARA","OtherSMSCheckDelay",a,strInipath.c_str());
	if(iResult == 0)
	{
		m_nOtherSMSCheckDelay = atoi(a);
	}

	memset(a,0,64);
	iResult = config.readvalue("PARA","OtherRAIDCheckDelay",a,strInipath.c_str());
	if(iResult == 0)
	{
		m_nOtherRAIDCheckDelay = atoi(a);
	}
	
	memset(a,0,64);
	iResult = config.readvalue("PARA","OtherEthCheckDelay",a,strInipath.c_str());
	if(iResult == 0)
	{
		m_nOtherEthCheckDelay = atoi(a);
	}
	
	memset(a,0,64);
	iResult = config.readvalue("PARA","OtherSwitchCheckDelay",a,strInipath.c_str());
	if(iResult == 0)
	{
		m_nOtherSwitchCheckDelay = atoi(a);
	}
	
	memset(a,0,64);
	iResult = config.readvalue("PARA","OtherSpeedLmtCheckDelay",a,strInipath.c_str());
	if(iResult == 0)
	{
		m_nOtherSpeedLmtCheckDelay = atoi(a);
	}

	memset(a,0,64);
	iResult = config.readvalue("PARA","OtherEWCheckDelay",a,strInipath.c_str());
	if(iResult == 0)
	{
		m_nOtherEWCheckDelay = atoi(a);
	}

	return 0;

}
