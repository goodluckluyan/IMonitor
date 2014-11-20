//@file:C_ErrorDef.h
//@brief: ������������Ϣ��Ϣ���塣
//@author:wangzhongping@oristartech.com
//dade:2012-07-12

#ifndef _TMS20_ERROR_DEFINE
#define _TMS20_ERROR_DEFINE
typedef  const  int  c_u_int;

//CenterCtrl Module:16,subModule:0000

//ERROR
// web���ʵ�ַ���ô��󣬷Ǳ�����ַ��
c_u_int ERROR_WEB_VISIT_ADDRESS = 0x0001;
//WARNING

//INFO
//M0001
c_u_int INFO_IMonitor_START_RUN = 0x0001;
//M0001
//CenterCtrl end 

//Thread Manage Module: 17, subModule: 0000

//ERROR

//�̶߳����������޿����̡߳�
c_u_int ERROR_THREAD_LIST_FULL= 0x0001;
// ���Ի��߳��������ݴ���
c_u_int ERROR_INIT_THREAD_ATTRIB = 0x0003;
// �����߳��������ݴ���
c_u_int ERROR_SET_THREAD_ATTRIB = 0x0004;
// �����̴߳���
c_u_int ERROR_CREATE_TRREAD = 0x0005;
// �߳�״̬����
c_u_int ERROR_THREAD_STATE = 0x0006;
//����threadIDû���ҵ���Ӧ��C_ThreadData��
c_u_int ERROR_NO_FIND_THREADDATA_OF_ID = 0x0007;
// ���Ի�webservice�߳��������ݴ���
c_u_int ERROR_INIT_WEBSERVICE_THREAD_ATTRIB = 0x0008;
// ����webservice�߳��������ݴ���
c_u_int ERROR_SET_WEBSERVICE_THREAD_ATTRIB = 0x0009;
// ����webservice�̴߳���
c_u_int ERROR_WEBSERVICE_CREATE_TRREAD = 0x000a;
// �߳��Ѿ���ʱ��
c_u_int ERROR_THREAD_TIMEOUT = 0x000b;
// �̵߳�������û���ҵ�λִ����صĴ�������
c_u_int ERROR_THREAD_TASK_NUMBER_NO_FIND = 0x000d;
//�̵߳��������ʹ���
c_u_int ERROR_THREAD_TASK_TYPE = 0x000e;

// û�з����������߳�
c_u_int INFO_NO_CONDITION_THREAD = 02;


//��������������޿�������
c_u_int ERROR_TASK_LIST_FULL= 0x0001;

//Log Manage Module:24 subModule 0
//ERROR
//�ڴ���ʴ��� pLogManage û�б����Ի���
c_u_int ERROR_MEMORY_NOT_INIT = 0x0001;

//��ģ���ź���ģ���Ŷ�Ӧ����־��û���ҵ���
c_u_int ERROR_NOT_FIND_LOG_TYPE = 0x0002;
// ��ȡ��־��Ŀ¼���Դ���
c_u_int ERROR_GET_LOG_BOOT_PATH = 0x0003;
//������־Ŀ¼����
c_u_int ERROR_CREATE_LOG_PATH = 0x0004;
//��ȡĿ¼���Դ���
c_u_int ERROR_GET_LOG_PATH = 0x0005;
//ɾ�����ڵ���־����
c_u_int ERROR_DELETE_PRE_LOG = 0x0006;
//��־�������ô���
c_u_int ERROR_DELETE_LEVEL = 0x0007;  //
//��־ģ�������ô���
c_u_int ERROR_DELETE_MODULE = 0x0007;  //
//��־ģ�������ô���
c_u_int ERROR_DELETE_SUB_MODULE = 0x0008;  //



//Database Manage Module: 20, subModule: 0000;
//error
//��ѯ���Ϊ�ա�
const int ERROR_QUERY_RESULT_EMPTY = 0x0001;
//���¼�¼ʧ�ܡ�
const int ERROR_UPDATE_TABLE = 0x0002;
//ɾ�����ݼ�¼����
const int ERROR_DELETE_TABLE = 0x0003;
//�������ݼ�¼���� 
const int ERROR_INSERT_TABLE = 0x0004;
// �����ݿ����
const int ERROR_OPEN_DATABASE = 0x0005;   //db log;
//��ѯ���ݿ����
const int ERROR_QUERY_TABLE = 0x0006;  //db log;
//ִ��sql������
const int ERROR_EXEC_TABLE = 0x0007; //db log;
//��ʼ�������
const int ERROR_START_TRANSACTION = 0x0008; //db log;
//�ύ�������
const int ERROR_COMMIT_TRANSACTION = 0x0009; //db log;
//�ع��������
const int ERROR_ROLL_BACK_TRANSACTION = 0x000a; //db log;
//��ȡ�ֶ�ֵ����
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


// �߳��Ѿ���ʱ��
c_u_int ERROR_DEVSTATUS = 0x0011;

#endif  //_TMS20_ERROR_DEFINE
