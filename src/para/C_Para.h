//@file:C_Para.h
//@brief: ������C_Para��
//C_Para:��ȡϵͳ�����ļ��еĸ���������
//@author:wangzhongping@oristartech.com
//dade:2012-07-12


#ifndef _TMS20_PARA
#define _TMS20_PARA
#include <string>
using namespace std;
class C_Para
{
public:
 //method:
    static C_Para* GetInstance();
	static void DestoryInstance();
    ~C_Para();
    //��ȡ���ò�����
    int ReadPara();
protected:
     C_Para();
public:    
//Property:  
	//���ݿ��������IP  
    string m_strDBServiceIP;

   	//���ݿ�������Ķ˿ں�
    unsigned short m_usDBServicePort;

    //���ݿ��������½���û�����
    string m_strDBUserName;

    //���ݿ��������½���û�����
    string m_strDBPWD;
    string m_strDBName;

    //ϵͳ�̳߳����̵߳ĸ�����
    unsigned int m_uiThreadCount;

	//��־���·��
    string m_strLogPath;

	//�����������ȳ���
	bool m_bMain ;

	//����webservice����򿪵Ķ˿�
	//std::string m_strWebServiceIP;
	int m_nWebServicePort;

	//�Զ˵ĵ��ȳ����IP
	std::string m_strOIP;

	//�Զ˵ĵ��ȳ���Ķ˿�
	int m_nOPort ;

	//TMS��Webservice�˿�
	int m_nTMSWSPort;

	//�Զ˵ĵ��ȳ���wsdl��URI
	std::string m_strOURI;

	//tms����Ŀ¼
	std::string m_strTMSPath;

	//tomcat����Ŀ¼
	std::string m_strTOMCATPath;

	//��Ŀ¼
	std::string m_strInipath;

	//����sms�ķ�ʽ��1Ϊͬһ�ն�����  2Ϊ���ն�����
	int m_nStartSMSType ;

	//�ȴ��Զ˵������������ⳬʱʱ��,��λ��
	int m_nTimeOutWaitOtherIMonitor;

	//д��־����
	int m_nWirteLogLevel;

	// ������״̬ʱ����
	int m_nDiskCheckDelay;

	// �������״̬ʱ����
	int m_nEthCheckDelay;

	// �������״̬ʱ����
	int m_nHallListCheckDelay;

	// �������״̬ʱ����
	int m_nTMSCheckDelay;

	// ���Զ˵��ȳ���״̬ʱ����
	int m_nOtherMonitorCheckDelay;

	// ���Զ�TMS״̬ʱ����
	int m_nOtherTMSCheckDelay;

	// ���Զ�SMS״̬ʱ����
	int m_nOtherSMSCheckDelay;

	// ���Զ˴���״̬ʱ����
	int m_nOtherRAIDCheckDelay;

	// ���Զ�����״̬ʱ����
	int m_nOtherEthCheckDelay;

	// ���Զ˽�����ʱ����
	int m_nOtherSwitchCheckDelay;

	// ���Զ��ٶ�����ʱ����
	int m_nOtherSpeedLmtCheckDelay;

	// ���Զ��쳣״̬ʱ����
	int m_nOtherEWCheckDelay;
	
private:
    static C_Para *m_pInstance;
};
#endif //_TMS20_PARA
