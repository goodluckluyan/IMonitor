/****************************************************************************
*@file: C_Log.h
*@brief: 日志打印类
*		 
*@author: zhangchao@oristartech.com
*@dade: 2015-11-09
****************************************************************************/
#ifndef C_LOG_H
#define C_LOG_H
#include <string>
enum Level
{
	Normal =0,
	Waring,
	Error
};
class CLog
{
public:
	CLog();
	~CLog();
	static void SetFilePath(const std::string& path );
	static void Write(const Level& lev, const std::string& text );
private:
	static std::string m_path;//日子文件所在文件夹路径
};
#endif // C_LOG_H
