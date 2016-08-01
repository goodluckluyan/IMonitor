//@file:C_constDef.h
//@brief: 包含各种状态信息定义。
//@author:luyan@oristartech.com
//dade:2014-09-12

#ifndef IMONITOR_CONST_DEFINE
#define IMONITOR_CONST_DEFINE
#include "threadManage/C_CS.h"
#include <vector>
#include <string>
#include <map>


//const define

// 线程池中线程的个数。
const int THREAD_COUNT_IN_POOL = 20;

//任务ID
const int TASK_NUMBER_GET_DISK_STATUS = 0x0101;
const int TASK_NUMBER_GET_NET_STATUS = 0x0102;
const int TASK_NUMBER_DISPATCH_ROUTINE = 0x0201;
const int TASK_NUMBER_GET_HALL_STATUS = 0x0301;
const int TASK_NUMBER_GET_TMS_STATUS = 0x0401;

const int TASK_NUMBER_GET_OTHERMONITOR_STATUS = 0x0501;
const int TASK_NUMBER_GET_OTHERMONITOR_TMS_STATUS = 0x0502;
const int TASK_NUMBER_GET_OTHERMONITOR_SMS_STATUS = 0x0503;
const int TASK_NUMBER_GET_OTHERMONITOR_RAID_STATUS = 0x0504;
const int TASK_NUMBER_GET_OTHERMONITOR_ETH_STATUS = 0x0505;
const int TASK_NUMBER_GET_OTHERMONITOR_SWITCH_STATUS = 0x0506;
const int TASK_NUMBER_GET_OTHERMONITOR_SPEEDLIMIT_STATUS = 0x0507;
const int TASK_NUMBER_GET_OTHERMONITOR_SMSEW_STATUS = 0x0508;
const int TASK_NUMBER_GET_DB_SYNCN_STATUS = 0x0509;

const int TASK_NUMBER_PROCESS_USERINPUT = 0x0601;
const int TASK_NUMBER_CONDSWITCH_ROUTINE = 0x0701;
const int TASK_NUMBER_HASHCHECK_ROUTINE = 0x0801;
const int TASK_NUMBER_FILEOPERATION_ROUTINE = 0x0901;

const int TASK_NUMBER_REBOOT = 0x0902;
const int TASK_NUMBER_SHUTDOWN = 0x0903;
const int TASK_NUMBER_CHECKWATCHDOG = 0x0904;

// sms 状态
const int SMS_STATE_SUCCESS = 101 ;    // 连接正常
const int SMS_STATE_ERROR = 102;       // 连接异常 / 连接超时
const int SMS_STATE_SYSERROR =103 ;    // 系统异常

// sms 播放状态：
const int SMS_STATE_PLAYING = 201;     // 正在播放
const int SMS_STATE_PAUSING = 202;     // 暂停播放	
const int SMS_STATE_FINISHED = 203;    // 停止播放
const int SMS_STATE_FINISHED_ABORT = 204; // 手动停止
const int SMS_STATE_FINISHED_ERROR = 205; // 播放失败
const int SMS_STATE_NOT_EXECUTED = 206;   // 不再执行

// sms ingest job 状态
const int SMS_STATE_INGEST_SCHEULED = 301;    // 等待导入
const int SMS_STATE_INGEST_RUNNING = 302;     // 正在导入
const int SMS_STATE_INGEST_PAUSING = 303;     // 暂停导入
const int SMS_STATE_INGEST_CHECK = 304;       // 导入检查
const int SMS_STATE_INGEST_FINISHED = 305;    // 导入成功
const int SMS_STATE_INGEST_FINISHED_ERROR = 306;// 导入失败

// sms cpl check状态
const int SMS_STATE_CPL = 401; 			// 等待验证
const int SMS_STATE_CPL_RUNNING = 402;	// 正在验证
const int SMS_STATE_CPL_PAUSING = 403;	// 暂停验证
const int SMS_STATE_CPL_FINISHED = 404; // 验证成功
const int SMS_STATE_CPL_FINISHED_ERROR = 405; // 验证失败

//日志级别。
const int ULOG_DEBUG	= 0;
const int ULOG_INFO	= 1;
const int ULOG_ERROR	= 2;
const int ULOG_FATAL	= 3;
const int UDEFAULT_LOG_LEVEL = 0;

//日志所属模块
const int LOG_MODEL_THREADMGR = 1;
const int LOG_MODEL_TIMETASK = 2;
const int LOG_MODEL_DB = 3;
const int LOG_MODEL_WEBS = 4;
const int LOG_MODEL_JOBS = 5;
const int LOG_MODEL_OTHER = 6;
const int LOG_MODEL_LOGMGR = 7;

// 线程状态。
enum Thread_State
{
	INIT_STATE = 0, //初始
	RUN_STATE = 1,  //运行
	SUSPEND_IDL_STATE = 2, //空闲
	SUSPEND_FINISH_STATE = 3,//操作执行完成而暂停。
	RUN_LOCKED_STATE = 4,  // 运行时锁定。
	SUSPEND_LOCKED_STATE = 5 ,// 暂停时锁定。
	QUIT_STATE =6	// 线程结束。
};

// 线程运行类型，用于区分线程执行操作的类型。
enum Thread_Run_Type
{
	RUN_WEBSERVICE_TYPE = 0,	//线程正在执行Webservice调用。
	RUN_TIMETASK_TYPE = 1,		// 线程正在执行定时任务。
	RUN_UNKNOWN_TYPE = 2		// 线程未执行操作。
};

// 任务类型
enum TASK_TYPE
{
	NULL_TASK,// 空
	TIME_TASK,// 定时任务
	ONCE_TASK,// 一次任务
	ALWAYS_TASK// 固定任务
};

// 任务状态；
enum TASK_STATE
{
	//空闲
	TASK_IDLE_STATE = 0,

	// 任务未开始执行
	TASK_NO_STATE =1,

	// 任务正在执行。
	TASK_RUNNING_STATE = 2,

	// 任务执行完成。
	TASK_FINISH_STATE  = 3,

	// 任务需要被删除
	TASK_DELETE_STATE  = 4,

	//空闲时被锁定
	TASK_LOCKED_STATE  = 5
};


// RAID状态
typedef struct DiskInfo
{
	std::string diskGroup;
	std::string diskSize;
	std::string diskState;
	std::string diskNumOfDrives;
	std::map<std::string ,struct DiskDriveInfo> diskDrives;
}DiskInfo;

typedef struct DiskDriveInfo
{
	int group ;
	std::string driveID;
	std::string drivePosition;
	std::string driveSlotNum;
	std::string driveErrorCount;
	std::string driveSize;
	std::string driveFirmwareState;
	std::string driveSpeed;
	std::string driveType;
	DiskDriveInfo():group(-1)
	{}

	DiskDriveInfo(const DiskDriveInfo &obj)
	{
		group=obj.group;
		driveID=obj.driveID;
		drivePosition=obj.drivePosition;
		driveSlotNum=obj.driveSlotNum;
		driveErrorCount=obj.driveErrorCount;
		driveSize=obj.driveSize;
		driveFirmwareState=obj.driveFirmwareState;
		driveSpeed=obj.driveSpeed;
		driveType=obj.driveType;
	}

	DiskDriveInfo & operator =(const DiskDriveInfo &obj)
	{
		if(this != &obj)
		{
			group=obj.group;
			driveID=obj.driveID;
			drivePosition=obj.drivePosition;
			driveSlotNum=obj.driveSlotNum;
			driveErrorCount=obj.driveErrorCount;
			driveSize=obj.driveSize;
			driveFirmwareState=obj.driveFirmwareState;
			driveSpeed=obj.driveSpeed;
			driveType=obj.driveType;
		}
		return *this;
	}
}DiskDriveInfo;


// 故障
struct stError
{
	stError():
		nOrdinal(-1)
	{}
	int nOrdinal;
	std::string ErrorName;
	std::string ErrorVal;
};


// 网卡状态
struct EthStatus
{
	std::string strName;
	int nTaskType;
	int nConnStatue;
	unsigned long long  nRxSpeed;
	unsigned long long nTxSpeed;

	EthStatus():
	nConnStatue(0),
		nRxSpeed(0),
		nTxSpeed(0),
		nTaskType(-1)
	{
	}
};



// SMS状态
struct SMSStatus
{
	std::string hallid;
	int nStatus;
	int nRun;				//0：本机没有运行 1：本机运行，2：另一台运行 3:另一台也没有运行
	int nPosition;
	std::string strSPLUuid;

	SMSStatus():
	nStatus(-1)
		,nRun(0)
		,nPosition(-1)
	{

	}

	SMSStatus(const SMSStatus &obj)
	{
		hallid = obj.hallid;
		nStatus = obj.nStatus;
		nRun = obj.nRun;
		nPosition = obj.nPosition;
		strSPLUuid = obj.strSPLUuid;
	}

	SMSStatus& operator=(const SMSStatus &obj)
	{
		if(this != &obj)
		{   
			hallid = obj.hallid;
			nStatus = obj.nStatus;
			nRun = obj.nRun;
			nPosition = obj.nPosition;
			strSPLUuid = obj.strSPLUuid;
		}
		return *this;
	}

};


// SMS参数
struct SMSInfo
{
	std::string strId;		// 标识id或序列号
	std::string strIp;		// 通信的ip地址
	int nPort ;				// 通信的端口
	int nRole;				// 角色，是主要运行角色还是候选角色 1:为主机运行，2:为备机运行 3:接管运行 
	std::string strExepath;// sms执行文件路径
	std::string strConfpath;// 配置文件路径
	SMSStatus stStatus;


	SMSInfo():
	nPort(-1)
		,nRole(-1)
	{}

	SMSInfo(const SMSInfo &obj)
	{
		strId = obj.strId;
		strIp = obj.strIp;
		nPort = obj.nPort;
		nRole = obj.nRole;
		strExepath = obj.strExepath;
		strConfpath = obj.strConfpath;
		stStatus = obj.stStatus;
	}

	SMSInfo& operator =(const SMSInfo &obj)
	{
		if(this != &obj)
		{
			strId = obj.strId;
			strIp = obj.strIp;
			nPort = obj.nPort;
			nRole = obj.nRole;
			strExepath = obj.strExepath;
			strConfpath = obj.strConfpath;
			stStatus = obj.stStatus;
		}
		return *this;
	}
};



#endif 
