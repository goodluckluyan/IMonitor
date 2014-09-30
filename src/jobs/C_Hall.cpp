//@file:C_Hall.cpp
//@brief: 实现C_Hall 所有的方法。
//@author:wangzhongping@oristartech.com
//dade:2012-07-12

#include "C_Hall.h"
#include "database/CppMySQL3DB.h"
#include "para/C_RunPara.h"

using namespace std;
C_Hall::C_Hall()
{
   
}

C_Hall::~C_Hall()
{
    
}


// 设置影厅参数
bool C_Hall::SetPara(std::string strHallid,std::string strHallName,SMSInfo stSms,ProjectorInfo stPrjinfo)
{
	return true;
}

// 获取SMS工作状态
bool C_Hall::GetWorkState(int &iState,std::string &strInfo)
{
	return true;
}

// 添加DCP
bool C_Hall::Ingest(std::string strPath)
{
	return true;
}

// 添加KDM
bool C_Hall::IngestKeyForm(std::string strPathName)
{
	return true;
}

// 删除KDM
bool C_Hall::DeleteKDM(std::string strKDMID)
{
	return true;
}

// 添加影厅对应设备的定时任务；
int C_Hall::AddTask()
{
	return 0;
}

// 启动SMS
bool C_Hall::StartSMS()
{
	return true;
}

// 检测放映机
bool C_Hall::CheckProjector()
{
	return true;
}
