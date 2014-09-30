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
protected:
	C_HallList();
    
public:
	
    ~C_HallList();
    static C_HallList *GetInstance();
	static void DestoryInstance();

   	//��ʼ������Ӱ����
    int Init(CDataManager *);
   
	//���Ի�����Ӱ���ļƻ�����
    int InitHallsTask();

	//���Ӱ��
	int AddHall(HALL_PARA &hallPara, std::string &strError);

	// ��ȡӰ��״̬
	bool GetHallStat(std::string &strHallID);

	// ��ȡSMS����״̬
	bool GetSMSWorkState(std::string &strHallID,int &iState,std::string &strInfo);

	// ���DCP
	bool SMSIngest(std::string &strHallID,std::string strPath);

	// ���KDM
	bool SMSIngestKeyForm(std::string &strHallID,std::string strPathName);

	// ɾ��KDM
	bool SMSDeleteKDM(std::string &strHallID,std::string strKDMID);

public:
	// ����Ӱ��
	int CreateHall(HALL_PARA &hallPara, std::string &strError);

private:
	 
    C_CS m_CS;
	std::list<C_Hall *> m_Halllist;
    static C_HallList *m_pInstance; 
	CDataManager *m_ptrDM;

};
#endif //HALL_LIST;
