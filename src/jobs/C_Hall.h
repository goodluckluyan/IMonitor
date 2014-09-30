//@file:C_Hall.h
//@brief: 包含类C_Hall。
//C_Hall：封装了影厅中sms相关操作。
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
	// sms webservice 接口
	// 设置影厅参数
	bool SetPara(std::string strHallid,std::string strHallName,SMSInfo stSms,ProjectorInfo stPrjinfo);
 
	 // 获取SMS工作状态
	bool GetWorkState(int &iState,std::string &strInfo);

	// 添加DCP
	bool Ingest(std::string strPath);
	
	// 添加KDM
	bool IngestKeyForm(std::string strPathName);

	// 删除KDM
	bool DeleteKDM(std::string strKDMID);

public:
	// 添加影厅对应设备的定时任务；
	int AddTask(); 

	// 启动SMS
	bool StartSMS();

	// 检测放映机
	bool CheckProjector();
private:
	
	// SMS信息
	SMSInfo m_SMS;

	// 谨慎期间为SMS加锁
	pthread_mutex_t m_mtxSMSInfo;

	// 放映机信息
	ProjectorInfo m_Projector;
 
	// 影厅的ID
	std::string m_strHallId;

	// 影厅名称
	std::string m_strHallName;

};
#endif //HALL_DEFINE
