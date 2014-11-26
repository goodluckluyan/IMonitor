//@file:C_HallList.h
//@brief: ������C_HallList��
//C_HallList:�ڲ���װ��ӰԺ��Ӱ���������ṩ��ӡ�ɾ�����޸�Ӱ���豸�Ȳ�����
//ͬʱ��װ��Ӱ�������豸�Ĳ�����
//@author:wangzhongping@oristartech.com
//dade:2012-07-12



 #ifndef HALL_LIST
 #define HALL_LIST

#include "threadManage/C_CS.h"
#include "C_constDef.h"
#include "DataManager.h"
#include "C_Hall.h"
#include <string>
#include <list>
//using namespace std;


class C_HallList
{
public:
	C_HallList()
		:m_ptrDM(NULL)
	{};
   
	
    ~C_HallList();

   	//��ʼ������Ӱ����
    int Init(bool bRunOther);
   
	// ��ȡSMS����״̬
	bool GetSMSWorkState();

	// �л�sms
	bool SwitchSMS(std::string strHallID);

	// �л����е�sms
	bool SwitchAllSMS();


	// ��ȡ����������webservice�˿�
	bool GetSMSRunHost(std::string strHallID,std::string &strIP,int &nPort);

private:

	//����tomcat
	bool StartTOMCAT(std::string strPath);

	//�ر�tomcat
	bool ShutdownTOMCAT(std::string strPath);
	 
    C_CS m_CS;
	std::map<std::string,C_Hall *> m_mapHall;
    static C_HallList *m_pInstance; 
	std::string m_WebServiceLocalIP;
	std::string m_WebServiceOtherIP;
	CDataManager *m_ptrDM;

};
#endif //HALL_LIST;
