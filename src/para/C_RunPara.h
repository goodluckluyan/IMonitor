//@file:C_RunPara.h
//@brief: 包含类C_RunPara。
//C_RunPara:包含系统运行时的重要参数的 读取/设置。
//目前包含系统时钟的封装、获取主程序所在的路径功能。其他参数根据后续的开发需要添加。
//@author:wangzhongping@oristartech.com
//dade:2012-07-12



#ifndef TMS20_RUN_PARA
#define TMS20_RUN_PARA
#include <sys/time.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "C_constDef.h"
class C_RunPara
{
public:
	~C_RunPara();
	static C_RunPara* GetInstance();
	static void  DestoryInstance();
	//初始化时间，程序开始运行时有主线程调用。
	void Init();
	//主线程通过调用此方法，获取系统时间，同时把系统时间暂存在m_CurTV中
	//主线程定时调用此方法更新系统时间。
	//同时返回以当天0点为基准的毫秒数。
	//bDateStart指示当前时间是否是0点。1：当前时间为0点，0：非0点。
	int GetCurMillisecond(int &bDateStart);
	//功能：获取系统的时间。
	//返回：一秒为单位的系统时间。
	int GetCurTime()
	{
		return (int) m_CurTV.tv_sec;
	}
	//功能：获取系统的当前时间。
	//返回：0:成功；其他为错误码。
	//参数：strCurTime  [out] 返回当前的系统时间字符串形式 如：2012-07-15 14:54:30
	int GetCurTime(std::string &strCurTime);
	//功能：获取系统的当前日期。
	//返回：0:成功；其他为错误码。
	//参数：strDate  [out] 返回当前的系统日期字符串形式 如：2012-07-15
	int GetCurDate(std::string &strDate);
	//M0001
	//转换当天的时间戳为字符串为格式，HH:MM:SS
	char *ConvertTime(int iTime, char *strTime);
	//M0001
	int GetZeroTime();
	//获取 主程序所在的路径。
	int GetExePath(std::string &strExePath);
	//获取存储所在的路径。
	int GetSavePath(std::string &strPath);
	//获取挂载路径。
	int GetMountPath(std::string &strPath);
	//M0002
	int GetSatIpPort(std::string &strStatIp, unsigned short &usStatPort);
	//M0002
	int GetFtpUrL(std::string &strFtpUrl);
	int GetFtpKdmUrl(std::string &strFtpUrl);
	int GetKdmPath(std::string &strKdmPath);
	int GetDispatchCount(int &iDispatchCount);

	int GetLocalKdmPath(std::string &strLocal_kdm_path);

	int GetDcsPlayOffset(std::string &strDeviceModel, int &iMillisecond);
	int GetTMSDiskSizeAlarm(int &iDiskSizeAlarm);
	int GetDCSDiskSizeAlarm(int &iDiskSizeAlarm);
	int GetFinishDeleteMinute(int &iMinute);
	int SumAllFileSize(const char *pathname, unsigned long long& rSize);
	//wzp add 2014-2-13
	//转换中文字符串格式。
	int ZhGbkToUtf(std::string &info);
	//wzp add end;


	//@author zhangmiao@oristartech.com
	//@date 2012-09-13 
	//@brief 获取磁盘空间等参数，包括总空间、空闲空间、空闲空间所占的百分比。
	//@new
	int DH_GetDiskSpace(const char *pDisk, unsigned int& rFreespace , unsigned int & rTotalspace , float& rFreeSpacePercent );
	//@modify end;

	//duheqing 2012-11-2
	//返回 -2 表示不接受票务排期。返回0 为接受票务系统排期。其他为错误码。
	int GetCinemaIpPort(std::string &ip, unsigned short &port, std::string &cinemaId);//tickets
	//返回 -2 表示不接受广告计划。返回0 为接受广告计划。其他为错误码。
	int GetShopIpPort(std::string &ip, unsigned short &port, std::string &cinemaId);//advertise a
	int GetPeerIpPort(std::string &ip, unsigned short &port);//advertise dcp

	//@author zhangmiao@oristartech.com
	//@date [2012-12-03]
	//@brief 添加 C++ 配置参数读取。
	//@new
	int GetDebugLevel( int& nDebugLevel );
	int GetHall_disk_alarm( int& nHall_disk_alarm );

	int GetAdver_ip( std::string& strAdver_ip );
	int GetAdver_port( unsigned short& nAdver_port );
	int GetAdver_update_period( int& nAdver_update_period );
	int GetDcp_alarm( int& nDcp_alarm );
	int GetDcp_fault_alarm( int& nDcp_fault_alarm );
	int GetKdm_alarm( int& nKdm_alarm );
	int GetKdm_fault_alarm( int& nKdm_fault_alarm );
	int GetKdm_end_alarm( int& nKdm_end_alarm );
	int GetShow_alarm( int& nShow_alarm );
	int GetShow_fault_alarm( int& nShow_fault_alarm );
	int GetCent_adver( int& nCent_adver );
	int GetTicket_show( int& nTicket_show );
	int GetShow_sync_way( int& nShow_sync_way );
	int GetTicket_show_ip( std::string& strTicket_show_ip );
	int GetTicket_show_port( unsigned short& nTicket_show_port );
	int TranslateGbkToUtf8(std::string &strGbk, std::string &strUtf8);
	//@modify end;

	//wzp add on 2014-1-6
	int GetCuePlayCheckDelay(int &iCuePlayCheckDelay);
	//wzp add
	//wzp add on 2014-4-8
	int GetOpenLampFlag(int &bOpenLampFlag);
	int GetOpenShutterFlag(int &bOpenShutterFlag);
	int GetCloseShutterFlag(int &bCloseShutterFlag);
	int GetDefaultAudioLevel(int &iAudioLevel);
	int GetSwichtAudioChannel(int &bSwitchAudioChannel);
	int GetSwitchAudioCDChannel(int &bSwitchAudioCDChannel);
	int GetCloseLampFlag(int &bCloseLamp);
	int GetColorSpace(int &iColorSpace);
	void CreateUuid(std::string &strUuid);
	//wzp end

	int GetOsStartWaitTime(){ return m_OsStartWaitTime; };//duheqing 2014-08-07

	//@author zhangmiao@oristartech.com
	//@date [2013-09-02]
	//@brief 添加 C++ 配置参数读取。
	//@new
	int GetTms_version( std::string& strTms_version );
	int GetDB_version( std::string& strDB_version );
	int GetPhp_version( std::string& strPhp_version );
	//@modify end;

	//@author zhangmiao@oristartech.com
	//@date [2013-11-13]
	//@brief 添加 C++ 配置参数读取。
	//@new
	//获得Satellite卫星设备的 IP and Port
	int GetSateDeviceIpPort(std::string &ip, unsigned short &port);
	//@modify end;

	//20140220 by xiaozhengxiu add
	int GetTmsCenterUrl(std::string &strTmsCenterUrl);

private:
	//单件实例指针。
	static C_RunPara *m_pInstance;
	//每天0点对应的时间，以秒计。
	int m_iZeroTime;
	//当前时间结构
	struct timeval m_CurTV; 
	//当前日期  如：2012-07-15
	std::string m_strCurDate;
	// 是否正在设置0点时间值。
	int m_bSettingTime;
	//C++ 主程序所在的路径。
	std::string m_strExePath;


	//片库路径。
	std::string m_strSavePath;		//*
	//移动硬盘的挂载路径。
	std::string m_strMountPath;		//*
	// 密钥路径 kdm的片库路径。
	std::string m_strKdmPath;		//*
	//kdm系统上传路径local_kdm_path
	std::string m_local_kdm_path;	//*

	//磁盘空间报警上限 
	int m_iTMSDiskSizeAlarm;
	int m_iDCSDiskSizeAlarm;
	//int m_iDiskPercent; 
	//导入、分发任务完成后删除时间（分） 
	int m_iFinishDeleteMinute;		//*
	//dcp 从tms的ftp下载dcp时用到的ftp url。        
	std::string m_strFtpUrl; 
	//dcp 从tms的ftp下载kdm时用到的ftp url。
	std::string m_strFTpKdmUrl; 
	// TMS 同时分发DCP的个数。
	int m_iDispatchCount;

	int m_iFieldLampChannel;
	int m_iSkirtingLampChannel;
	int m_iCleanLampChannel;
	int m_iAq10PlayOffset;
	int m_iGdc3dPlayOffset;
	int m_iDolbyPlayOffset; 

	//@author zhangmiao@oristartech.com
	//@date [2012-12-03]
	//@brief 添加 C++ 配置参数读取。
	//@new
	//debug 级别(0:调试1:提示2:警告3:错误)  启动时读一次 
	int m_nDebugLevel;
	//hall_disk_alarm	190	影厅磁盘空间报警  启动时读一次 
	int m_nHall_disk_alarm;
	//中心广告ip地址每次调用时读取 
	std::string m_strAdver_ip;
	//adver_port	809	中心广告地址端口号每次调用时读取 unsigned short
	unsigned short m_nAdver_port;
	//adver_update_period	49	中心广告更新周期每次调用时读取 
	int m_nAdver_update_period;
	//dcp_alarm	 dcp未准备好预警时间   每次调用时读取 int
	int m_nDcp_alarm;
	//dcp_fault_alarm	dcp未准备好故障时间  每次调用时读取 int
	int m_nDcp_fault_alarm;
	//kdm_alarm	1	kdm未准备好预警时间  每次调用时读取 int
	int m_nKdm_alarm;
	//kdm_fault_alarm	1	dcp未准备好故障时间 每次调用时读取 int
	int m_nKdm_fault_alarm;
	//kdm_end_alarm	1	KDM即将到期预警时间每次调用时读取 int
	int m_nKdm_end_alarm;
	//show_alarm	2	排期未准备好预警时间 每次调用时读取 int
	int m_nShow_alarm;
	//show_fault_alarm	1	dcp未准备好故障时间 每次调用时读取 int
	int m_nShow_fault_alarm;
	//cent_adver	1	是否接受中心广告（0：不接受1：接受）每次调用时读取 int
	int m_nCent_adver;
	//ticket_show	0	是否接受票务排期（0：不接受1：接受）每次调用时读取 int
	int m_nTicket_show;
	//show_sync_way	2	排期同步方式   每次调用时读取 int
	int m_nShow_sync_way;
	//ticket_show_ip		票务系统排期ip每次调用时读取 string
	std::string m_strTicket_show_ip;
	//ticket_show_port		票务系统排期地址端口每次调用时读取 unsigned short
	unsigned short m_nTicket_show_port;
	//wzp add on 2014-1-6
	//在排期预计开始的时间点后的m_iCuePlayCheckDelay 秒对放映服务器是否播放
	//进行检测。
	int m_iCuePlayCheckDelay;
	//wzp add

	int m_OsStartWaitTime;//duheqing 2014-08-07
	//@modify end;

	//tmsCenter中心端url。        
	std::string m_strTmsCenterUrl; //20140220 by xiaozhengxiu add

protected:
	C_RunPara();
};

//@author zhangmiao@oristartech.com
//@date [2012-12-03]
//@brief 添加 C++ 配置参数读取。
//@new
int ReadTmsDBData4Str( const std::string& sFieldName , std::string& retValue );
int ReadTmsDBData4Int( const std::string& sFieldName , int& retValue );
//@modify end;

#endif //TMS20_RUN_PARA
