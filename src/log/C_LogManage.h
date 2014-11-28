#ifndef TMS20_LOG
#define TMS20_LOG
#include <string>
#include <list>
#include "C_ErrorDef.h"
#include "LogManage.h"
#include "../threadManage/C_CS.h"
typedef struct LogSct
{
	int iModule;
	int iSubModule;
	std::string strLogName;
	LogManage *pLogManage;	
	C_CS m_CS;
}LOG_STRUCT, *PLOG_STRUCT;
//临时的日志结构，用于设置日志目录时，暂存日志。
typedef struct TmpLog
{
	int iLevel;
	int iModule;
	int iSubModule;
	int errorCode;
	std::string strError;
}TMP_LOG, *PTMP_LOG;

class C_LogManage
{
public:
	~C_LogManage();
	static C_LogManage* GetInstance();
	static void  DestoryInstance();
	int WriteLog(int iLevel, int iModule, int iSubModule,c_u_int errorCode, std::string strError);
	int WriteLogFmt(int iLevel, int iModule, int iSubModule,c_u_int errorCode, const char *fmt, ...);
	int WriteLog(int iLogNumber, std::string strError);
	char* ft_Printf(const char *fmt, ...);
	
	//iLevel：日志级别；
	//iModule: 模块编号；
	//iSubModule:子模块编号；
	// errorCode：日志的错误码常量。
	int CreateLogNumber(int iLevel, int iModule, int iSubModule,c_u_int errorCode)
	{
		if(iLevel > 3)
		{
			printf("日志级别设置错误。\n");
			return -1;
		}
		if(iModule <16 || iModule >26)
		{
			printf("日志模块编号设置错误。\n");
			return -1;
		}
		if((iModule != 22 && iSubModule != 0) || iSubModule >4 || iSubModule <0)
		{
			printf("子模块设置错误\n");
			return-1;
		}
			
		int iResult = 0;
		iResult = iLevel << 28;
		iResult |= iModule << 20;
		iResult |= iSubModule << 16;
		iResult |= errorCode;
		return iResult;
	}
	int SeparateLogNumber(int iLogNumber,int &iLevel, int &iModule, int &iSubModule)
	{
		iLevel = iLogNumber >> 28;
		iModule = (iLogNumber & 0x0FF00000) >> 20;
		iSubModule = (iLogNumber & 0x00F0000) >> 16;
		return iLogNumber & 0x0000FFFF;
	}
	int SeparateLogNumber(int iLogNumber)
	{
		return iLogNumber & 0x0000FFFF;
	}
	int SetLogLevel(int iLevel, int iModele, int iSubModule);
	//初始化日志目录 首先调用CheckLogPath，然后调用
	int InitLogPath(std::string &strBootPath);
	int SetLogDate();
	//检查日志根目录strBootPath是否合法。同时检测“01”---“31”子目录是否存在，
	//如果不存在则创建子目录。
	//返回值：默认。
	//参数：strBootPath 所有日志文件的根目录。
	int CheckLogPath(std::string &strBootPath);
	//删除以前的日志文件。
	//返回值默认；
	//参数：strPath 日志文件的目录
	int DeletePreLog(std::string &strPath);
	// 创建日志结构,系统初始化时调用。
	//返回值默认。
	int CreateLogStruct(std::string &strPath);
	int ModifyLogStruct(std::string &strPath);
	//重新初始化日志 每天0点由主线程调用。
	int ReInitLog();

		
protected:
	C_LogManage();
private:
	static 	C_LogManage* m_pInstance;
	volatile int m_bSettingDate;
	std::string m_strBootPath;
	std::string m_strLogPath;
	std::list<PLOG_STRUCT> m_listLog;	
	std::list<PTMP_LOG> m_listTmpLog;
	C_CS m_TmpLogCS;
};
#endif //TMS20_LOG
