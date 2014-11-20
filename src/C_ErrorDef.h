//@file:C_ErrorDef.h
//@brief: 包含错误码信息信息定义。
//@author:wangzhongping@oristartech.com
//dade:2012-07-12

#ifndef _TMS20_ERROR_DEFINE
#define _TMS20_ERROR_DEFINE
typedef  const  int  c_u_int;

//CenterCtrl Module:16,subModule:0000

//ERROR
// web访问地址设置错误，非本机地址。
c_u_int ERROR_WEB_VISIT_ADDRESS = 0x0001;
//WARNING

//INFO
//M0001
c_u_int INFO_IMonitor_START_RUN = 0x0001;
//M0001
//CenterCtrl end 

//Thread Manage Module: 17, subModule: 0000

//ERROR

//线程对列已满，无空闲线程。
c_u_int ERROR_THREAD_LIST_FULL= 0x0001;
// 初试化线程属性数据错误。
c_u_int ERROR_INIT_THREAD_ATTRIB = 0x0003;
// 设置线程属性数据错误。
c_u_int ERROR_SET_THREAD_ATTRIB = 0x0004;
// 创建线程错误。
c_u_int ERROR_CREATE_TRREAD = 0x0005;
// 线程状态错误
c_u_int ERROR_THREAD_STATE = 0x0006;
//根据threadID没有找到对应的C_ThreadData。
c_u_int ERROR_NO_FIND_THREADDATA_OF_ID = 0x0007;
// 初试化webservice线程属性数据错误。
c_u_int ERROR_INIT_WEBSERVICE_THREAD_ATTRIB = 0x0008;
// 设置webservice线程属性数据错误。
c_u_int ERROR_SET_WEBSERVICE_THREAD_ATTRIB = 0x0009;
// 创建webservice线程错误。
c_u_int ERROR_WEBSERVICE_CREATE_TRREAD = 0x000a;
// 线程已经超时。
c_u_int ERROR_THREAD_TIMEOUT = 0x000b;
// 线程的任务编号没有找到位执行相关的处理函数。
c_u_int ERROR_THREAD_TASK_NUMBER_NO_FIND = 0x000d;
//线程的任务类型错误。
c_u_int ERROR_THREAD_TASK_TYPE = 0x000e;

// 没有符合条件的线程
c_u_int INFO_NO_CONDITION_THREAD = 02;


//任务对列已满，无空闲任务。
c_u_int ERROR_TASK_LIST_FULL= 0x0001;

//Log Manage Module:24 subModule 0
//ERROR
//内存访问错误 pLogManage 没有被初试化！
c_u_int ERROR_MEMORY_NOT_INIT = 0x0001;

//与模块编号和子模块编号对应的日志类没有找到！
c_u_int ERROR_NOT_FIND_LOG_TYPE = 0x0002;
// 获取日志根目录属性错误。
c_u_int ERROR_GET_LOG_BOOT_PATH = 0x0003;
//创建日志目录错误
c_u_int ERROR_CREATE_LOG_PATH = 0x0004;
//获取目录属性错误
c_u_int ERROR_GET_LOG_PATH = 0x0005;
//删除过期的日志错误
c_u_int ERROR_DELETE_PRE_LOG = 0x0006;
//日志级别设置错误。
c_u_int ERROR_DELETE_LEVEL = 0x0007;  //
//日志模块编号设置错误。
c_u_int ERROR_DELETE_MODULE = 0x0007;  //
//日志模块编号设置错误。
c_u_int ERROR_DELETE_SUB_MODULE = 0x0008;  //



//Database Manage Module: 20, subModule: 0000;
//error
//查询结果为空。
const int ERROR_QUERY_RESULT_EMPTY = 0x0001;
//更新记录失败。
const int ERROR_UPDATE_TABLE = 0x0002;
//删除数据记录错误
const int ERROR_DELETE_TABLE = 0x0003;
//插入数据记录错误 
const int ERROR_INSERT_TABLE = 0x0004;
// 打开数据库错误。
const int ERROR_OPEN_DATABASE = 0x0005;   //db log;
//查询数据库错误
const int ERROR_QUERY_TABLE = 0x0006;  //db log;
//执行sql语句错误
const int ERROR_EXEC_TABLE = 0x0007; //db log;
//开始事务错误。
const int ERROR_START_TRANSACTION = 0x0008; //db log;
//提交事务错误。
const int ERROR_COMMIT_TRANSACTION = 0x0009; //db log;
//回滚事务错误。
const int ERROR_ROLL_BACK_TRANSACTION = 0x000a; //db log;
//获取字段值错误。
const int ERROR_GET_FIELD = 0x000b;
const int ERROR_FIELD_VALUE_NULL_OR_ERROR = 0x000c;

//Log Manage Module:18 subModule 0
// MonitorSensor 
c_u_int ERROR_PARSE_MONITORSTATE_XML = 0x0001;




c_u_int ERROR_CREATE_HTTP = -1;
c_u_int ERROR_SENSOR_TCP_CONNECT = -2;
c_u_int ERROR_SENSOR_TCP_SEND = -3;
c_u_int ERROR_SENSOR_TCP_RECV = -4;


c_u_int ERROR_SIGCATCH_FUN = 0x0001;

c_u_int ERROR_PLAYER_AQ_TCPCONNECT = -5;


// 线程已经超时。
c_u_int ERROR_DEVSTATUS = 0x0011;

#endif  //_TMS20_ERROR_DEFINE
