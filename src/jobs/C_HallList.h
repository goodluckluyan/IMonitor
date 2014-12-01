//@file:C_HallList.h
//@brief: ������C_HallList��
//C_HallList:�ڲ���װ��ӰԺ��Ӱ���������ṩ��ӡ�ɾ�����޸�Ӱ���豸�Ȳ�����
//ͬʱ��װ��Ӱ�������豸�Ĳ�����
//@author:wangzhongping@oristartech.com
//dade:2012-07-12



 #ifndef HALL_LIST
 #define HALL_LIST
#include <string>
#include <list>
#include <pthread.h>
#include "threadManage/C_CS.h"
#include "C_constDef.h"
#include "DataManager.h"
#include "C_Hall.h"


//using namespace std;

struct stConditionSwitch
{
	std::string strHallID;
	std::string strCond;
	int nVal;
};

class C_HallList
{
public:
	C_HallList()
		:m_ptrDM(NULL)
	{
		pthread_cond_init(&cond,NULL);
	};
   
	
    ~C_HallList();

   	//��ʼ������Ӱ����
    int Init(bool bRunOther);
   
	// ��ȡSMS����״̬
	bool GetSMSWorkState();

	// �л�sms
	bool SwitchSMS(std::string strHallID,int &nState);

	// ��ȡhallid
	void GetAllHallID(std::vector<std::string>& vecHallID);

	// ��ȡ����������webservice�˿�
	bool GetSMSRunHost(std::string strHallID,std::string &strIP,int &nPort);

	// ��������ȴ��л�����
	bool AddCondSwitchTask(std::string strHallID,std::string strCond,int nVal);

	// ִ�������ȴ��л�����
	bool ProcessCondSwitchTask();

	// �ж��л�sms�����Ѿ������������д���
	bool IsHaveCondSwitchTask(std::string strHallID);

private:

	//����tomcat
	bool StartTOMCAT(std::string strPath);

	//�ر�tomcat
	bool ShutdownTOMCAT(std::string strPath);


	int GetPID(std::string strName,std::vector<int>& vecPID);

	bool GetPIDExeDir(int ,std::string &);

    C_CS m_CS;
	std::map<std::string,int> m_mapHallCurState;

	std::map<std::string,C_Hall *> m_mapHall;
    static C_HallList *m_pInstance; 
	std::string m_WebServiceLocalIP;
	std::string m_WebServiceOtherIP;
	CDataManager *m_ptrDM;

	std::list<stConditionSwitch>  m_lstCondSwitch;
	C_CS m_csCondTaskLst;
	pthread_cond_t cond; 
	C_CS m_csSwitching;

};
#endif //HALL_LIST;
