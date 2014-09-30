//@file:C_constDef.h
//@brief: ��������״̬��Ϣ���塣
//@author:wangzhongping@oristartech.com
//dade:2012-07-12

#ifndef TMS20_CONST_DEFINE
#define TMS20_CONST_DEFINE
#include "threadManage/C_CS.h"
#include <vector>
#include <string>
// �߳�״̬��
enum Thread_State
{
	INIT_STATE = 0, //��ʼ
	RUN_STATE = 1,  //����
	SUSPEND_IDL_STATE = 2, //����
	SUSPEND_FINISH_STATE = 3,//����ִ����ɶ���ͣ��
	RUN_LOCKED_STATE = 4,  // ����ʱ������
	SUSPEND_LOCKED_STATE = 5 ,// ��ͣʱ������
	QUIT_STATE =6	// �߳̽�����
};
// �߳��������ͣ����������߳�ִ�в��������͡�
enum Thread_Run_Type
{
	RUN_WEBSERVICE_TYPE = 0,	//�߳�����ִ��Webservice���á�
	RUN_TIMETASK_TYPE = 1,		// �߳�����ִ�ж�ʱ����
	RUN_UNKNOWN_TYPE = 2		// �߳�δִ�в�����
};

// ��������
enum TASK_TYPE
{
	NULL_TASK,// ��
	TIME_TASK,// ��ʱ����
	ONCE_TASK,// һ������
	ALWAYS_TASK// �̶�����
};

 // ����״̬��
enum TASK_STATE
{
	//����
	TASK_IDLE_STATE = 0,

	// ����δ��ʼִ��
	TASK_NO_STATE =1,

	// ��������ִ�С�
	TASK_RUNNING_STATE = 2,

	// ����ִ����ɡ�
	TASK_FINISH_STATE  = 3,

	// ������Ҫ��ɾ��
	TASK_DELETE_STATE  = 4,

	//����ʱ������
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



//Ӱ��״̬��
enum HALL_STATE
{
	//Ӱ���ڽ���״̬���������á�ɾ���������豸��
	HALL_STATE_UNENABLE = 0x0,

	//Ӱ��������״̬��������ֲ�����
	HALL_STATE_ENABLE = 0x1,

	//Ӱ�����ڱ�ɾ����˲��״̬���������κβ�����
	HALL_STATE_DELETING = 0x4,

	//Ӱ���������ڳ�ʼ����˲��״̬���������κβ�����
	HALL_STATE_INITING = 0x8
};


enum TMS_WORK_STATE
{
	TMS_WORK_STATE_NO_WORKING = 0,
	//���ڵ���KDM
	TMS_WORK_STATE_IMPORTING_KDM = 1,
	//���ڵ���DCP
	TMS_WORK_STATE_IMPORTING_DCP = 2,
	//���ڷַ�DCP
	TMS_WORK_STATE_DISPATCHING_DCP = 3,
	//����ӰƬcue����ִ�У����߼���ִ�С�
	TMS_WORK_STATE_RUN_CUE = 4
};

//const define
//Ĭ�ϵ���־����
const int DEFAULT_LOG_LEVEL = 0;

 // �̳߳����̵߳ĸ�����
 const int THREAD_COUNT_IN_POOL = 10;

// RAID״̬
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

 // ����״̬
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

// SMS����
 struct SMSInfo
 {
	 std::string strId;		// ��ʶid�����к�
	 std::string strIp;		// ͨ�ŵ�ip��ַ
	 int nPort ;			// ͨ�ŵĶ˿�
	 int nRunstatus;		// ״̬  0:û������ 1:��������  2:�������
	 int nRole;				// ��ɫ������Ҫ���н�ɫ���Ǻ�ѡ��ɫ 1:Ϊ���ǣ�2:Ϊ��ѡ 
	 std::string strExepath;// smsִ���ļ�·��
	 int nPID;				// SMS����PID

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
