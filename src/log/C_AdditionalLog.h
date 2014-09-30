#ifndef TMS20_ADDITIONAL_LOG
#define TMS20_ADDITIONAL_LOG
#include "LogManage.h"
#include "../threadManage/C_CS.h"
#include "../C_ErrorDef.h"
#include <string>
class C_AdditionalLog
{
protected:
		 C_AdditionalLog();
public:
	   ~C_AdditionalLog();
	   static C_AdditionalLog* GetInstance();
	   int WriteLog(int iLevel, int iModule, int iSubModule,c_u_int errorCode, std::string strError);
	   int Init();
	   int CreateLogNumber(int iLevel, int iModule, int iSubModule,c_u_int errorCode)
		{
			int iResult = 0;
			iResult = iLevel << 28;
			iResult |= iModule << 20;
			iResult |= iSubModule << 16;
			iResult |= errorCode;
			return iResult;
		}
private:
	   static C_AdditionalLog* m_pInstance;
	   LogManage *m_pLog;
	   C_CS m_CS;
};
#endif //TMS20_ADDITIONAL_LOG
