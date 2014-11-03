//@file:C_Para.h
//@brief: 包含类C_Para。
//C_Para:读取系统配置文件中的各个参数。
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
    //读取配置参数。
    int ReadPara();
protected:
     C_Para();
public:    
//Property:  
	//数据库服务器的IP  
    string m_strDBServiceIP;

   	//数据库服务器的端口号
    unsigned short m_usDBServicePort;

    //数据库服务器登陆的用户名。
    string m_strDBUserName;

    //数据库服务器登陆的用户名。
    string m_strDBPWD;
    string m_strDBName;

    //系统线程池中线程的个数。
    unsigned int m_uiThreadCount;

	//日志存放路径
    string m_strLogPath;

	//是主、辅调度程序
	bool m_bMain ;

	//本机webservice服务打开的端口
	//std::string m_strWebServiceIP;
	int m_nWebServicePort;

	//对端的调度程序的IP
	std::string m_strOIP;

	//对端的调度程序的端口
	int m_nOPort ;

	//对端的调度程序wsdl的URI
	std::string m_strOURI;

	std::string m_strTMSPath;
private:
    static C_Para *m_pInstance;
};
#endif //_TMS20_PARA
