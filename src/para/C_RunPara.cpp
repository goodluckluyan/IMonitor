//@file:C_RunPara.cpp
//@brief: 包含类C_RunPara的功能实现。
//@author:wangzhongping@oristartech.com
//dade:2012-07-12

#include "../utility/C_Time.h"
#include <string>
#include <sys/vfs.h>    /* 或者 <sys/statfs.h> */
#include "C_RunPara.h"
#include <iconv.h>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <uuid/uuid.h>

C_RunPara *C_RunPara::m_pInstance = NULL;

C_RunPara::C_RunPara()
{
	m_iZeroTime = 0;
	m_bSettingTime = 0;
	m_strMountPath = "/media/";
	m_strSavePath = "/share/TMS/DCP/";
	m_strKdmPath = "/share/TMS/KDM/";
	m_iFieldLampChannel = 0;
  m_iSkirtingLampChannel = 0;
  m_iCleanLampChannel = 0;   
  m_strFtpUrl = "ftp://admin:admin@172.23.140.163/TMS/DCP"; 
  m_strFTpKdmUrl = "ftp://admin:admin@172.23.140.163/TMS/KDM"; 
  m_iDispatchCount = 0;
 //M0001 
  m_iAq10PlayOffset = 1000;
  m_iGdc3dPlayOffset = 1000;
  m_iDolbyPlayOffset = 1000; 
 //M0001
  //m_iDiskPercent = 70;
  m_iFinishDeleteMinute = 1;

//@author zhangmiao@oristartech.com
//@date [2012-11-30]
//@brief 添加 C++ 配置参数读取。
//@new
  m_nDebugLevel = 0;
  m_nHall_disk_alarm = 0;
  m_strAdver_ip = "172.23.22.5";
  m_nAdver_port = 0;
  m_nAdver_update_period = 0;
  m_nDcp_alarm = 0;
//@modify end;

  m_OsStartWaitTime = 300;//duheqing 2014-08-07

	m_strTmsCenterUrl = "http://172.23.140.33:80/tmsCenter/services/tmsServer.php"; //20140220 by xiaozhengxiu add
}

C_RunPara::~C_RunPara()
{
	
}

C_RunPara* C_RunPara::GetInstance()
{
	if(m_pInstance == NULL)
	{
		m_pInstance = new C_RunPara;
	}
	return m_pInstance;
}

void  C_RunPara::DestoryInstance()
{
	if(m_pInstance != NULL)
	{
		delete m_pInstance;
		m_pInstance = NULL;
	}
}



void C_RunPara::Init()
{
	//计算当天的0点时间
	std::string str;
	C_Time t1,t2;
	t1.setCurTime();
	t1.getDateStr(m_strCurDate);
	str = m_strCurDate + " 00:00:00";
	t2.setTimeStr(str);
	m_iZeroTime = t2.getTimeInt(); 
	//获取当前时间。
	gettimeofday(&m_CurTV, NULL);

//@author zhangmiao@oristartech.com
//@date [2012-11-30]
//@brief 添加 C++ 配置参数读取。
//@new
// 	std::string mount_path;
// 	int ret = ReadTmsDBData4Str( "mount_path" , mount_path );
// 	if( ret == 0 ) 
// 		m_strMountPath = mount_path;
// 
// 	std::string strSavePath;
// 	ret = ReadTmsDBData4Str( "ext_path" , strSavePath );
// 	if( ret == 0 ) 
// 		m_strSavePath = strSavePath;
// 
// 	std::string strKdmPath;
// 	ret = ReadTmsDBData4Str( "kdm_path" , strKdmPath );
// 	if( ret == 0 ) 
// 		m_strKdmPath = strKdmPath;
// 
// 	std::string strLocal_kdm_path;
// 	ret = ReadTmsDBData4Str( "local_kdm_path" , strLocal_kdm_path );
// 	if( ret == 0 ) 
// 		m_local_kdm_path = strLocal_kdm_path;
// 
// 	int nDebugLevel = 0;
// 	ret = ReadTmsDBData4Int( "debug" , nDebugLevel );
// 	if( ret == 0 ) 
// 		m_nDebugLevel = nDebugLevel;
// 
// 	int nHall_disk_alarm = 0;
// 	ret = ReadTmsDBData4Int( "hall_disk_alarm" , nHall_disk_alarm );
// 	if( ret == 0 )
// 	{
// 		m_nHall_disk_alarm = nHall_disk_alarm;
// 		m_iDCSDiskSizeAlarm = m_nHall_disk_alarm;
// 	}
// 	ret = ReadTmsDBData4Int( "cs_disk_alarm" , nHall_disk_alarm );
// 	if( ret == 0 )
// 	{
// 		m_iTMSDiskSizeAlarm = nHall_disk_alarm;
// 	}
// 
// 	int nFinishDeleteMinute = 1;
// 	ret = ReadTmsDBData4Int( "task_delete_time" , nFinishDeleteMinute );
// 	if( ret == 0 )
// 		m_iFinishDeleteMinute = nFinishDeleteMinute;
//@modify end;

//@author zhangmiao@oristartech.com
//@date [2012-12-12]
//@brief 添加 C++ 配置参数读取。
//@new
// 	int nDispatchCount = 0;
// 	ret = ReadTmsDBData4Int( "dispatch_dcp_count" , nDispatchCount );
// 	if( ret == 0 ) 
// 		m_iDispatchCount = nDispatchCount;
// 
// 	std::string strFtp_ip;
// 	std::string strFtp_port = "21";
// 	ret = ReadTmsDBData4Str( "ftp_ip" , strFtp_ip );
// 
// 	std::string strFtp_usrname, strFtp_pwd, strFtp_dcp_path, strFtp_kdm_path;
// 	strFtp_kdm_path = "/KDM/";
// 	strFtp_dcp_path = "/DCP/";
// 	strFtp_usrname = "admin";
// 	strFtp_pwd = "admin";
// 	
// 	ret = ReadTmsDBData4Str( "ftp_usrname" , strFtp_usrname );
// 	ret = ReadTmsDBData4Str( "ftp_pwd" ,  strFtp_pwd );
// 	ret = ReadTmsDBData4Str( "ftp_dcp_path" , strFtp_dcp_path );
// 	ret = ReadTmsDBData4Str( "ftp_kdm_path" , strFtp_kdm_path );
// 
// 	ret = ReadTmsDBData4Int( "os_start_waittime" , m_OsStartWaitTime );//duheqing 2014-08-07
// 
// 	if( !strFtp_dcp_path.empty() && strFtp_dcp_path[strFtp_dcp_path.size() - 1] != '/' )
// 		strFtp_dcp_path += '/';
// 	if( !strFtp_dcp_path.empty() && strFtp_dcp_path[0] != '/' )
// 		strFtp_dcp_path = '/' + strFtp_dcp_path;
// 	if (strFtp_dcp_path.empty()) 
// 		strFtp_dcp_path += '/';
// 	
// 	if( !strFtp_kdm_path.empty() && strFtp_kdm_path[strFtp_kdm_path.size() - 1] != '/' )
// 		strFtp_kdm_path += '/';
// 	if( !strFtp_kdm_path.empty() && strFtp_kdm_path[0] != '/' )
// 		strFtp_kdm_path = '/' + strFtp_kdm_path;
// 	if (strFtp_kdm_path.empty())
// 		strFtp_kdm_path += '/';
// 	
// 	m_strFtpUrl    = "ftp://";
// 	m_strFTpKdmUrl = "ftp://";
// 	if ( !strFtp_usrname.empty() && !strFtp_pwd.empty() )
// 	{
// 		m_strFtpUrl  += strFtp_usrname + ":" + strFtp_pwd + "@";
// 		m_strFTpKdmUrl  += strFtp_usrname + ":" + strFtp_pwd + "@";
// 	}
// 	
// 	m_strFtpUrl    +=  strFtp_ip + strFtp_dcp_path;
// 	m_strFTpKdmUrl +=  strFtp_ip + strFtp_kdm_path;

//@modify end;

//20140220 by xiaozhengxiu add
// 	std::string strTmsCenter_ip;
// 	std::string strTmsCenter_port;
// 	ret = ReadTmsDBData4Str( "tmsCenter_ip" , strTmsCenter_ip );
// 	ret = ReadTmsDBData4Str( "tmsCenter_port" , strTmsCenter_port );
// 	m_strTmsCenterUrl = "http://"; 
// 	if ( !strTmsCenter_ip.empty() && !strTmsCenter_port.empty() )
// 	{
// 		m_strTmsCenterUrl  += strTmsCenter_ip + ":" + strTmsCenter_port + "/tmsCenter/services/tmsServer.php";
// 	}
//20140220 by xiaozhengxiu add end
}

//@author zhangmiao@oristartech.com
//@date [2012-11-30]
//@brief 添加 C++ 配置参数读取。
//@new
int C_RunPara::GetAdver_ip( std::string& strAdver_ip )
{
	int ret = ReadTmsDBData4Str( "adver_ip" , strAdver_ip );
	if( ret == 0 ) 
		m_strAdver_ip = strAdver_ip;
	return ret;
}

int C_RunPara::GetAdver_port( unsigned short& nAdver_port )
{
	int ret = ReadTmsDBData4Int( "adver_port" , (int&)nAdver_port );
	if( ret == 0 ) 
		m_nAdver_port = nAdver_port;
	return ret;
}

int C_RunPara::GetAdver_update_period( int& nAdver_update_period )
{
	int ret = ReadTmsDBData4Int( "adver_update_period" , nAdver_update_period );
	if( ret == 0 ) 
		m_nAdver_update_period = nAdver_update_period;
	return ret;
	
}

int C_RunPara::GetDcp_alarm( int& nDcp_alarm )
{
	int ret = ReadTmsDBData4Int( "dcp_alarm" , nDcp_alarm );
	if( ret == 0 ) 
		m_nDcp_alarm = nDcp_alarm;
	return ret;
}

int C_RunPara::GetDcp_fault_alarm( int& nDcp_fault_alarm )
{
	int ret = ReadTmsDBData4Int( "dcp_fault_alarm" , nDcp_fault_alarm );
	if( ret == 0 ) 
		m_nDcp_fault_alarm = nDcp_fault_alarm;
	return ret;
}

int C_RunPara::GetKdm_alarm( int& nKdm_alarm )
{
	int ret = ReadTmsDBData4Int( "kdm_alarm" , nKdm_alarm );
	if( ret == 0 ) 
		m_nKdm_alarm = nKdm_alarm;
	return ret;
}

int C_RunPara::GetKdm_fault_alarm( int& nKdm_fault_alarm )
{
	int ret = ReadTmsDBData4Int( "kdm_fault_alarm" , nKdm_fault_alarm );
	if( ret == 0 ) 
		m_nKdm_fault_alarm = nKdm_fault_alarm;
	return ret;
}

int C_RunPara::GetKdm_end_alarm( int& nKdm_end_alarm )
{
	int ret = ReadTmsDBData4Int( "kdm_end_alarm" , nKdm_end_alarm );
	if( ret == 0 ) 
		m_nKdm_end_alarm = nKdm_end_alarm;
	return ret;
}

int C_RunPara::GetShow_alarm( int& nShow_alarm )
{
	int ret = ReadTmsDBData4Int( "show_alarm" , nShow_alarm );
	if( ret == 0 ) 
		m_nShow_alarm = nShow_alarm;
	return ret;
}


int C_RunPara::GetShow_fault_alarm( int& nShow_fault_alarm )
{
	int ret = ReadTmsDBData4Int( "show_fault_alarm" , nShow_fault_alarm );
	if( ret == 0 ) 
		m_nShow_fault_alarm = nShow_fault_alarm;
	return ret;
}

int C_RunPara::GetCent_adver(int& nCent_adver)
{
	int ret = ReadTmsDBData4Int( "cent_adver" , nCent_adver );
	if( ret == 0 ) 
		m_nCent_adver = nCent_adver;
	return ret;
}

int C_RunPara::GetTicket_show(int& nTicket_show)
{
	int ret = ReadTmsDBData4Int( "ticket_show" , nTicket_show );
	if( ret == 0 ) 
		m_nTicket_show = nTicket_show;
	return ret;
}

int C_RunPara::GetShow_sync_way(int& nShow_sync_way)
{
	int ret = ReadTmsDBData4Int( "show_sync_way" , nShow_sync_way );
	if( ret == 0 ) 
		m_nShow_sync_way = nShow_sync_way;
	return ret;
}

int C_RunPara::GetTicket_show_ip( std::string& strTicket_show_ip )
{
	int ret = ReadTmsDBData4Str( "ticket_show_ip" , strTicket_show_ip );
	if( ret == 0 ) 
		m_strTicket_show_ip = strTicket_show_ip;
	return ret;
}

int C_RunPara::GetTicket_show_port( unsigned short& nTicket_show_port )
{
	//int ret = ReadTmsDBData4Int( "ticket_show_port" , (int&)nTicket_show_port );
	int retValue(0);
	int ret = ReadTmsDBData4Int( "ticket_show_port" , retValue );
	if( ret == 0 ) 
	{
		nTicket_show_port = (unsigned short)retValue;
		m_nTicket_show_port = nTicket_show_port;
	}
	return ret;
}

int C_RunPara::GetDebugLevel( int& nDebugLevel )
{
	nDebugLevel = m_nDebugLevel;
	return 0;
}
int C_RunPara::GetHall_disk_alarm( int& nHall_disk_alarm )
{
	nHall_disk_alarm = m_nHall_disk_alarm;
	return 0;
}
//@modify end;

//@author zhangmiao@oristartech.com
//@date [2013-09-02]
//@brief 添加 C++ 配置参数读取。
//@new
int C_RunPara::GetTms_version( std::string& strTms_version )
{
	int ret = ReadTmsDBData4Str( "tms_version" , strTms_version );

	return ret;
}

int C_RunPara::GetDB_version( std::string& strDB_version )
{
	int ret = ReadTmsDBData4Str( "db_version" , strDB_version );

	return ret;
}

int C_RunPara::GetPhp_version( std::string& strPhp_version )
{
	int ret = ReadTmsDBData4Str( "php_version" , strPhp_version );

	return ret;
}
//@modify end;

//@author zhangmiao@oristartech.com
//@date [2013-11-13]
//@brief 添加 C++ 配置参数读取。
//@new
//获得Satellite卫星设备的 IP and Port
int C_RunPara::GetSateDeviceIpPort(std::string &ip, unsigned short &port)
{
	int ret = ReadTmsDBData4Str( "Satellite_ip" , ip );
	if(ret != 0)
	{
		return ret;
	}
	string str;
	ret = ReadTmsDBData4Str( "Satellite_port" , str);
	if(ret != 0)
	{
		return ret;
	}
	int iTmp = atoi(str.c_str());
	port = (unsigned short)iTmp;
	return 0;
}
//@modify end;

int C_RunPara::GetCurMillisecond(int &bDateStart)
{
    bDateStart = 0;
    gettimeofday(&m_CurTV, NULL);
    if(m_CurTV.tv_sec - m_iZeroTime >= 86400)
    {
    		m_bSettingTime = 1;
        m_iZeroTime += 86400;
        C_Time t1;
				t1.setCurTime();
				t1.getDateStr(m_strCurDate);
				m_bSettingTime = 0;
        bDateStart = 1;
    }
    return (m_CurTV.tv_sec - m_iZeroTime)*1000 + m_CurTV.tv_usec/1000;
}

int C_RunPara::GetCurTime(std::string &strCurTime)
{
	if(m_bSettingTime == 1)
	{
		usleep(200000);
	}
	char strTime[32];
	memset(strTime, 0, 32);
	int iTmp = m_CurTV.tv_sec;
	sprintf(strTime," %02d:%02d:%02d", (iTmp - m_iZeroTime)/3600, 
	       ((iTmp - m_iZeroTime)%3600)/60, (iTmp - m_iZeroTime)%60);
  strCurTime = m_strCurDate + strTime;
	return 0;
}

int C_RunPara::GetCurDate(std::string &strDate)
{
	if(m_bSettingTime == 1)
	{
		usleep(200000);
	}
	strDate = m_strCurDate;
	return 0;
}

int C_RunPara::GetZeroTime()
{
	if(m_bSettingTime == 1)
	{
		usleep(200000);
	}
	return m_iZeroTime;
}
int C_RunPara::GetExePath(std::string &strExePath)
{
	if(m_strExePath != "")
	{
		strExePath  = m_strExePath;
		return 0;
	}
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
  m_strExePath = str.substr(0,iPos);
  strExePath = m_strExePath;
  return 0;
}

int C_RunPara::GetSavePath(std::string &strPath)
{
	if(m_strSavePath == "")
	{
		return -1;//After Add Log and ErrorCtrl;
	}
	strPath = m_strSavePath;
	return 0;
}	
int C_RunPara::GetMountPath(std::string &strPath)
{
	if(m_strMountPath == "")
	{
		return -1;//After Add Log and ErrorCtrl;
	}
	strPath = m_strMountPath;
	return 0;	
}
int C_RunPara::GetFtpUrL(std::string &strFtpUrl)
{
	if(m_strFtpUrl == "")
	{
		return -1;//After Add Log and ErrorCtrl;
	}
	strFtpUrl = m_strFtpUrl;
	return 0;	
}

//20140220 by xiaozhengxiu add
int C_RunPara::GetTmsCenterUrl(std::string &strTmsCenterUrl)
{
	if(m_strTmsCenterUrl == "")
	{
		return -1;//After Add Log and ErrorCtrl;
	}
	strTmsCenterUrl = m_strTmsCenterUrl;
	return 0;	
}
//20140220 by xiaozhengxiu add end

//@author zhangmiao@oristartech.com
//@date 2012-09-13 
//@brief 获取磁盘空间等参数，包括总空间、空闲空间、空闲空间所占的百分比。
//@new
int C_RunPara::DH_GetDiskSpace(const char *pDisk, unsigned int& rFreespace , unsigned int & rTotalspace , float& rFreeSpacePercent )
{
	int ret(0);
	struct statfs disk_statfs;
	if( (ret = (statfs(pDisk, &disk_statfs))) >= 0 ) 
	{
		rFreespace =  ( ((long long)disk_statfs.f_bsize  * (long long)disk_statfs.f_bavail)/(long long)(1024*1024) ); 
		rTotalspace = ( ((long long)disk_statfs.f_bsize * (long long)disk_statfs.f_blocks) /(long long)(1024*1024) ); 
		rFreeSpacePercent = ((float)rFreespace/(float)rTotalspace)*100.0 ; 
	}
	else
	{
		return -1;
	}
	
	unsigned long long uSize = 0;
	int iAdSize = 0;
// 	广告平台的虚拟机所在的路径
// 		if(SumAllFileSize("/share/virtualOS/",uSize) == 0)
// 		{
// 			iAdSize = (int)(uSize/(1024*1024));
// 			rTotalspace -= DISK_SIZE_FOR_AD_RESERVE;
// 			rFreespace = rFreespace - DISK_SIZE_FOR_AD_RESERVE + iAdSize;
// 			rFreeSpacePercent = ((float)rFreespace/(float)rTotalspace)*100.0 ; 
// 		}
	return ret ; 
}
//@modify end;

int C_RunPara::GetKdmPath(std::string &strKdmPath)
{
	if(m_strKdmPath == "")
	{
		return -1;//After Add Log and ErrorCtrl;
	}
	strKdmPath = m_strKdmPath;
	return 0;	
}

//@author zhangmiao@oristartech.com
//@date [2012-12-03]
//@brief 添加 C++ 配置参数读取。
//@new
int C_RunPara::GetLocalKdmPath(std::string &strLocal_kdm_path)
{
	if(m_local_kdm_path == "")
	{
		return -1;
	}
	strLocal_kdm_path = m_local_kdm_path;
	return 0;
}
//@modify end;

int C_RunPara::GetDcsPlayOffset(std::string &strDeviceModel, int &iMillisecond)
{
	if(strDeviceModel == "AQ10")
	{
		iMillisecond =  m_iAq10PlayOffset;
		return 0;
	}
	else if(strDeviceModel == "DSS100")
	{
		iMillisecond = m_iDolbyPlayOffset;
		return 0;
	}
	else if(strDeviceModel == "SC2100-3D")
	{
		iMillisecond = m_iGdc3dPlayOffset;
		return 0;
	}
	//M0001
	else
	{
		iMillisecond = 1000;
	}
	//M0001
	return -1;
}

int C_RunPara::GetFtpKdmUrl(std::string &strFtpUrl)
{
	if(m_strFTpKdmUrl == "")
	{
		return -1;//After Add Log and ErrorCtrl;
	}
	strFtpUrl = m_strFTpKdmUrl;
	return 0;		
}

//duheqing 2012-11-2
int C_RunPara::GetCinemaIpPort(std::string &ip, unsigned short &port, std::string &cinemaId)
{
	
	int ret = ReadTmsDBData4Str( "ticket_show_ip" , ip );
	if(ret != 0)
	{
		return ret;
	}
	ret = ReadTmsDBData4Str( "theater_id" , cinemaId );
	if(ret != 0)
	{
		return ret;
	}
	string str;
	ret = ReadTmsDBData4Str( "ticket_show_port" , str);
	if(ret != 0)
	{
		return ret;
	}
	if(ip == "" && str == "")
	{
		return -2;
	}
	if(str == "")
	{
		str = "0";
	}
	int iTmp = atoi(str.c_str());
	port = (unsigned short)iTmp;
	return 0;
}
int C_RunPara::GetShopIpPort(std::string &ip, unsigned short &port, std::string &cinemaId)
{
	int ret = ReadTmsDBData4Str( "adver_ip" , ip );
	if(ret != 0)
	{
		return ret;
	}
	ret = ReadTmsDBData4Str( "theater_id" , cinemaId );
	if(ret != 0)
	{
		return ret;
	}
	string str;
	ret = ReadTmsDBData4Str( "adver_port" , str);
	if(ret != 0)
	{
		return ret;
	}
	if(ip == "" && str == "")
	{
		return -2;
	}
	if(str == "")
	{
		str = "0";
	}
	int iTmp = atoi(str.c_str());
	port = (unsigned short)iTmp;
	return 0;
}
int C_RunPara::GetPeerIpPort(std::string &ip, unsigned short &port)
{
	int ret = ReadTmsDBData4Str( "adver_ip" , ip );
	if(ret != 0)
	{
		return ret;
	}
	string str;
	ret = ReadTmsDBData4Str( "adver_port" , str);
	if(ret != 0)
	{
		return ret;
	}
	int iTmp = atoi(str.c_str());
	port = (unsigned short)iTmp;
	return 0;
}
int C_RunPara::GetTMSDiskSizeAlarm(int &iDiskSizeAlarm)
{
	if(m_iTMSDiskSizeAlarm == 0)
	{
		return -1;
	}
	iDiskSizeAlarm = m_iTMSDiskSizeAlarm;
	return 0;
}

int C_RunPara::GetDCSDiskSizeAlarm(int &iDiskSizeAlarm)
{
	if(m_iDCSDiskSizeAlarm == 0)
	{
		return -1;
	}
	iDiskSizeAlarm = m_iDCSDiskSizeAlarm;
	return 0;	
}
int C_RunPara::GetFinishDeleteMinute(int &iMinute)
{
	if(m_iFinishDeleteMinute <0)
	{
		return -1;
	}
	iMinute = m_iFinishDeleteMinute;
	return 0;
}

//@author zhangmiao@oristartech.com
//@date [2012-12-03]
//@brief 添加 C++ 配置参数读取。
//@new
#define DATALEN 4096
#include "../database/CppMySQL3DB.h"

int ReadTmsDBData4Str( const std::string& sFieldName , std::string& retValue )
{
	int iResult(0);
	std::string strError;
	CppMySQL3DB mySql;
	iResult = mySql.openTMS();
	if(iResult != 0)
	{
		//strError = "打开数据库错误。";
		strError = "Open DataBase Error.";
		//printf( "Error:%s\n" , strError.c_str() );
		return iResult;
	}
	char tmp[DATALEN]="";
	memset(tmp,0,DATALEN);
	sprintf( tmp , "select * from system_config  where conf_item = '%s'" , sFieldName.c_str() );
	CppMySQLQuery recordset = mySql.querySQL(tmp,iResult);
	if(iResult != 0)
	{
		mySql.close();	
		//strError = "查询数据库错误";
		strError = "Query DataBase Error.";
		//printf( "Error:%s\n" , strError.c_str() );
		return iResult;
	}
	recordset.seekRow(0);
	if(recordset.numRow() != 0)
	{
		std::string strVal = recordset.getStringField("conf_val");
		if ( strVal.length() == 0 )
		{
			//strError = "查询数据为NULL";
			strError = "Query Data Is NULL.";
			//printf( "Error:%s\n" , strError.c_str() );
			iResult = -1;
		}
		else
		{
			retValue = strVal;
		}
	}
	else
	{
		//strError = "查询数据不存在";
		strError = "Query Data Not Exist.";
		//printf( "Error:%s\n" , strError.c_str() );
		mySql.close();	
		iResult = -1;
	}

	return iResult;
}
int ReadTmsDBData4Int( const std::string& sFieldName , int& retValue )
{
	int iResult(0);
	std::string strError;
	CppMySQL3DB mySql;
	iResult = mySql.openTMS();
	if(iResult != 0)
	{
		//strError = "打开数据库错误。";
		strError = "Open DataBase Error.";
		//printf( "Error:%s\n" , strError.c_str() );
		return iResult;
	}
	char tmp[DATALEN]="";
	memset(tmp,0,DATALEN);
	sprintf( tmp , "select * from system_config  where conf_item = '%s'" , sFieldName.c_str() );
	CppMySQLQuery recordset = mySql.querySQL(tmp,iResult);
	if(iResult != 0)
	{
		mySql.close();	
		//strError = "查询数据库错误";
		strError = "Query DataBase Error.";
		//printf( "Error:%s\n" , strError.c_str() );
		return iResult;
	}
	recordset.seekRow(0);
	if(recordset.numRow() != 0)
	{
		std::string strVal = recordset.getStringField("conf_val");
		if ( strVal.length() == 0 )
		{
			//strError = "查询数据为NULL";
			strError = "Query Data Is NULL.";
			//printf( "Error:%s\n" , strError.c_str() );
			iResult = -1;
		}
		else
		{
			retValue = atoi(strVal.c_str());
		}
	}
	else
	{
		//strError = "查询数据不存在";
		strError = "Query Data Not Exist.";
		//printf( "Error:%s\n" , strError.c_str() );
		mySql.close();	
		iResult = -1;
	}

	return iResult;
}
//@modify end;

int C_RunPara::TranslateGbkToUtf8(string &strGbk, string &strUtf8)
{
	iconv_t gbkToUtf = iconv_open("utf-8", "gb2312");
	char *gbkBuffer = new char[1024];
	char *utfBuffer = new char[1024];
	char *gbk = gbkBuffer;
	char *utf = utfBuffer;
	bzero(gbkBuffer, 1024);
	bzero(utfBuffer, 1024);
	strcpy(gbkBuffer, strGbk.c_str());
	int gbkLength = strlen(gbkBuffer);
	int bufferLength = 1024;
	int count = iconv(gbkToUtf, (char**)(&gbk), (size_t*)(&gbkLength), (char**)(&utf), (size_t*)(&bufferLength));
	if(count <0)
	{
		return count;
	}
	strUtf8 = utfBuffer;
	delete []gbkBuffer;
	delete []utfBuffer;
	iconv_close(gbkToUtf);
	return 0;
}

int C_RunPara::GetDispatchCount(int &iDispatchCount)
{
	if(m_iDispatchCount <= 0)
	{
		return -1;
	}
	iDispatchCount = m_iDispatchCount;
	return 0;
}

int C_RunPara::SumAllFileSize(const char *pathname, unsigned long long& rSize)
{
	DIR *dir;
 struct dirent *dirt;
 char tmppath[1024] = {0}; 
 int ret = 0;
 
 if(pathname==NULL)
 {
  //printf("para is NULL!\n"); 
  return -1;
 } 
 
 char srcPath[1024] = {0};
 strcpy(srcPath,pathname);
 if(srcPath[strlen(srcPath)-1]=='/')
  srcPath[strlen(srcPath)-1] = '\0';
 
 dir = opendir(srcPath);
 if(dir==NULL)
 {
      return errno;
 }
 while((dirt=readdir(dir))!=NULL)
 {
  //printf("%s\n",dirt->d_name);
  if(strcmp(dirt->d_name,".")==0 ||strcmp(dirt->d_name,"..")==0)
   continue;
 
  if(dirt->d_type==DT_DIR)
  {
   sprintf(tmppath,"%s/%s",srcPath,dirt->d_name);
   ret = SumAllFileSize( tmppath , rSize );
   if(ret==-1) 
   {
    closedir(dir);
    return -1;
   }
 
  }
  else
  {
   //printf("%s\n",dirt->d_name);
   sprintf( tmppath ,"%s/%s",srcPath,dirt->d_name);
   //获得文件大小,fileLength
   unsigned long long fileLength = 0;
   struct stat stsrc;
   if( lstat( tmppath , &stsrc ) == 0 )
   {
    fileLength = stsrc.st_size;
    rSize += fileLength;
   }
   else
   {
    //printf("lstat %s is err:%s!\n", tmppath ,strerror(errno));
    closedir(dir);
    return -1;
   }
  }
 }
 closedir(dir);
 return 0;

}
	

int C_RunPara::GetCuePlayCheckDelay(int &iCuePlayCheckDelay)
{
	int ret = ReadTmsDBData4Int( "cue_play_check_delay" , (int&)iCuePlayCheckDelay );
	if( ret == 0 ) 
	{
		m_iCuePlayCheckDelay = iCuePlayCheckDelay;
	}
	return ret;
}

int C_RunPara::ZhGbkToUtf(std::string &info)
{
	int utfBufferLength = 1024;
	char bufferGbk[1024], bufferUtf[1024];
	bzero(bufferGbk, 1024);
	strcpy(bufferGbk, info.c_str());
	bzero(bufferUtf, 1024);
	char *gbk = bufferGbk;
	char *utf = bufferUtf;

	iconv_t gbkToUtf = iconv_open("utf-8", "gb2312");
	int gbkLength = strlen(gbk);
	int count = iconv(gbkToUtf, (char**)(&gbk), (size_t*)(&gbkLength), (char**)(&utf), (size_t*)(&utfBufferLength));
	if(count != 0)
		;//do nothing
	info = bufferUtf;
	return 0;
}

int C_RunPara::GetOpenLampFlag(int &bOpenLampFlag)
{
	int ret = ReadTmsDBData4Int("is_turnonLamp" , bOpenLampFlag);
	if(ret != 0)
	{
		bOpenLampFlag = 0;
	}
	return 0;
}

int C_RunPara::GetOpenShutterFlag(int &bOpenShutterFlag)
{
	int ret = ReadTmsDBData4Int("is_turnonshutter" , bOpenShutterFlag);
	if(ret != 0)
	{
		bOpenShutterFlag = 0;
	}
	return 0;
}

int C_RunPara::GetCloseShutterFlag(int &bCloseShutterFlag)
{
		int ret = ReadTmsDBData4Int("is_turnoffshutter" , bCloseShutterFlag);
	if(ret != 0)
	{
		bCloseShutterFlag = 0;
	}
	return 0;
}

int C_RunPara::GetDefaultAudioLevel(int &iAudioLevel)
{	
	int ret = ReadTmsDBData4Int("ad_defaultVolume" , iAudioLevel);
	if(ret != 0)
	{
		iAudioLevel = 30;
	}
	return 0;
}

int C_RunPara::GetSwichtAudioChannel(int &bSwitchAudioChannel)
{
	int ret = ReadTmsDBData4Int("is_film" , bSwitchAudioChannel);
	if(ret != 0)
	{
		bSwitchAudioChannel = 0;
	}
	return 0;
}

int C_RunPara::GetSwitchAudioCDChannel(int &bSwitchAudioCDChannel)
{
	int ret = ReadTmsDBData4Int("is_music" , bSwitchAudioCDChannel);
	if(ret != 0)
	{
		bSwitchAudioCDChannel = 0;
	}
	return 0;
}

int C_RunPara::GetCloseLampFlag(int &bCloseLamp)
{
	int ret = ReadTmsDBData4Int("is_turnoffLamp" , bCloseLamp);
	if(ret != 0)
	{
		bCloseLamp = 0;
	}	
	return 0;
}

int C_RunPara::GetColorSpace(int &iColorSpace)
{
	int ret = ReadTmsDBData4Int("ad_default_macro" , iColorSpace);
	if(ret != 0)
	{
		iColorSpace = 1;
	}	
	return 0;	
}

void C_RunPara::CreateUuid(std::string &strUuid)
{
//   uuid_t uuid;
//   uuid_generate_random(uuid);
//   char tmp[40];
//   sprintf(tmp,"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
//          uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5],
//          uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11],
//          uuid[12], uuid[13], uuid[14], uuid[15]);
//   strUuid = tmp;
}
//M0001
char* C_RunPara::ConvertTime(int iTime, char *strTime)
{
	int iTmp = iTime - m_iZeroTime;
	if(iTmp<0)
	{
		sprintf(strTime,"errorTime:%d", iTime);
	}
	sprintf(strTime,"%02d:%02d:%02d",iTmp/3600,(iTmp%3600)/60, iTmp%60);
	return strTime;
}
//M0001
//M0002
int C_RunPara::GetSatIpPort(std::string &strStatIp, unsigned short &usStatPort)
{
	int iResult = ReadTmsDBData4Str("sat_ip",	strStatIp);
	if(iResult != 0)
	{
		return iResult;
	}
	int iTmp;
	iResult = ReadTmsDBData4Int("sat_port", iTmp);
	if(iResult != 0)
	{
		return iResult;
	}
	usStatPort = (unsigned short) iTmp;
	return 0;
}	
//M0002
	
	
