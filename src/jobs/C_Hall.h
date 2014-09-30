//@file:C_Hall.h
//@brief: ������C_Hall��
//C_Hall����װ��Ӱ����sms��ز�����
//@author:luyan@oristartech.com
//dade:2012-07-12

#ifndef HALL_DEFINE
#define HALL_DEFINE

#include "C_constDef.h"
//#include "timeTask/C_TaskList.h"
#include <string>
#include <pthread.h>

class C_Hall
{

 public:   
	 C_Hall();
	 ~C_Hall();

public:
	// sms webservice �ӿ�
	// ����Ӱ������
	bool SetPara(std::string strHallid,std::string strHallName,SMSInfo stSms,ProjectorInfo stPrjinfo);
 
	 // ��ȡSMS����״̬
	bool GetWorkState(int &iState,std::string &strInfo);

	// ���DCP
	bool Ingest(std::string strPath);
	
	// ���KDM
	bool IngestKeyForm(std::string strPathName);

	// ɾ��KDM
	bool DeleteKDM(std::string strKDMID);

public:
	// ���Ӱ����Ӧ�豸�Ķ�ʱ����
	int AddTask(); 

	// ����SMS
	bool StartSMS();

	// ����ӳ��
	bool CheckProjector();
private:
	
	// SMS��Ϣ
	SMSInfo m_SMS;

	// �����ڼ�ΪSMS����
	pthread_mutex_t m_mtxSMSInfo;

	// ��ӳ����Ϣ
	ProjectorInfo m_Projector;
 
	// Ӱ����ID
	std::string m_strHallId;

	// Ӱ������
	std::string m_strHallName;

};
#endif //HALL_DEFINE
