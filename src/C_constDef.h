//@file:C_constDef.h
//@brief: 包含各种状态信息定义。
//@author:wangzhongping@oristartech.com
//dade:2012-07-12

#ifndef TMS20_CONST_DEFINE
#define TMS20_CONST_DEFINE
#include "threadManage/C_CS.h"
#include <vector>
#include <string>
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

const int  TASK_NUMBER_GET_DISK_STATUS = 0x0101;
const int TASK_NUMBER_GET_NET_STATUS = 0x0102;
const int TASK_NUMBER_DISPATCH_ROUTINE = 0x0201;
const int TASK_NUMBER_GET_HALL_STATUS = 0x0301;
const int  TASK_NUMBER_GET_TMS_STATUS = 0x0401;


const int  TASK_NUMBER_GET_OTHERMONITOR_STATUS = 0x0501;
const int  TASK_NUMBER_GET_OTHERMONITOR_TMS_STATUS = 0x0502;
const int  TASK_NUMBER_GET_OTHERMONITOR_SMS_STATUS = 0x0503;
const int  TASK_NUMBER_GET_OTHERMONITOR_RAID_STATUS = 0x0504;
const int  TASK_NUMBER_GET_OTHERMONITOR_ETH_STATUS = 0x0505;
const int  TASK_NUMBER_GET_OTHERMONITOR_SWITCH_STATUS = 0x0506;
const int  TASK_NUMBER_GET_OTHERMONITOR_SPEEDLIMIT_STATUS = 0x0507;
const int  TASK_NUMBER_GET_OTHERMONITOR_SMSEW_STATUS = 0x0508;

const int TASK_NUMBER_PROCESS_USERINPUT = 0x0601;



//影厅状态；
enum HALL_STATE
{
	//影厅在禁用状态，允许启用、删除、更换设备。
	HALL_STATE_UNENABLE = 0x0,

	//影厅在启用状态，允许各种操作。
	HALL_STATE_ENABLE = 0x1,

	//影厅正在被删除，瞬间状态，不允许任何操作。
	HALL_STATE_DELETING = 0x4,

	//影厅正在正在初始化，瞬间状态，不允许任何操作。
	HALL_STATE_INITING = 0x8
};


enum TMS_WORK_STATE
{
	TMS_WORK_STATE_NO_WORKING = 0,
	//正在导入KDM
	TMS_WORK_STATE_IMPORTING_KDM = 1,
	//正在导入DCP
	TMS_WORK_STATE_IMPORTING_DCP = 2,
	//正在分发DCP
	TMS_WORK_STATE_DISPATCHING_DCP = 3,
	//播放影片cue正在执行，或者即将执行。
	TMS_WORK_STATE_RUN_CUE = 4
};

//const define
//默认的日志级别。
const int DEFAULT_LOG_LEVEL = 0;

 // 线程池中线程的个数。
 const int THREAD_COUNT_IN_POOL = 10;

// RAID状态
 typedef struct DiskInfo
 {
	 std::string diskSize;
	 std::string diskState;
	 std::string diskNumOfDrives;
	 std::vector<struct DiskDriveInfo> diskDrives;
 }DiskInfo;

 typedef struct DiskDriveInfo
 {
	 std::string driveID;
	 std::string driveSlotNum;
	 std::string driveErrorCount;
	 std::string driveSize;
	 std::string driveFirmwareState;
	 std::string driveSpeed;
 }DiskDriveInfo;

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

// SMS参数
 struct SMSInfo
 {
	 std::string strId;		// 标识id或序列号
	 std::string strIp;		// 通信的ip地址
	 int nPort ;			// 通信的端口
	 int nRunstatus;		// 状态  0:没有运行 1:本地运行  2:异地运行
	 int nRole;				// 角色，是主要运行角色还是候选角色 1:为主角，2:为候选 
	 std::string strExepath;// sms执行文件路径
	 int nPID;				// SMS进程PID

	 SMSInfo():
	 nPort(-1)
		 ,nRunstatus(-1)
		 ,nRole(-1)
		 ,nPID(0)
	 {}
 };

 struct ProjectorInfo
 {
	 std::string strId;
	 std::string strIp;
	 int nPort;

	 ProjectorInfo():
	 nPort(-1)
	 {

	 }
 };


typedef struct HallPara
{
	std::string hallId;
	std::string name;

	
}HALL_PARA, *PHALL_PARA;




#endif //TMS20_CONST_DEFINE
