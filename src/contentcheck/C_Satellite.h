//@file:C_Satellite.h
//@brief: 定义Satellite的接口方法。
//@author: zhangmiao@oristartech.com
//date: 2013-05-28
#ifndef _SAT_DCP_H_
#define _SAT_DCP_H_
#include <cstring>
#include <string>
#include <cstdio>
#include <cstdlib>

typedef unsigned char   u8;
typedef unsigned short  u16;
typedef unsigned int    u32;

//采用TCP通讯模式，本地端口固定为20080。
const unsigned short tcp_port = 20080 ;
const char device_ip[] = "192.168.0.7" ;
const int BUFFERLENGTH  = 65536 ;
const u8 nPreamble  = 0x55 ;         //0x55:包头，起始标志
const u16 nCmd_Connect = 0x11 ;		     //0x11:建立连接
const int CheckSumLen = 4;	//校验和4个字节
const int CmdInfoHeaderLen = 7;		//头长不包括payload、校验和
#pragma pack(push) //保存对齐状态
#pragma pack(1)//设定为1字节对齐
//dcp接收接口命令报文格式的头部数据结构
typedef struct CtrlCmdInfoHeader 
{
	unsigned char Preamble;	//0x55:包头，起始标志
	u16 Cmd;
	u32 PayloadLength;	//Payload Length 4个字节
	//u32 ChkSum;	//校验和crc32
} CtrlCmdInfoHeader;
#pragma pack(pop)//恢复对齐状态

//#include "FtpClient.h"
#include "utility/C_TcpSocket.h"

namespace Satellite
{
	using namespace std;
	typedef struct DCPInfo
	{
		string PklUuid;
		string Name;
		string issuer;
		string issueDate;
		string Creater;
		DCPInfo(){}
	}DCPInfo;

	typedef struct FTP_DCPInfo
	{
		string asset_type;
		string source;
		string path;
		string username;
		string password;
		string ftp_url;
		string ftp_url_path;
		FTP_DCPInfo(){}
	}FTP_DCPInfo;
	
};

class C_Satellite 
{

	C_TcpSocket* m_tcp;

	std::string m_ip;		//IP地址
	unsigned short m_port;	//端口号:20080

	std::string m_userName;
	std::string m_passwd;

	bool m_bConncetState;	//连接状态  0----无法连接；	1----可以连接

public:
	 C_Satellite(void);
	 ~C_Satellite(void);

	 //初始化建立与卫星接收设备tcp连接
	 int Init(const std::string &strIp, unsigned short usPort = 20080	);

	 //关闭连接认证
	 int Close();

	 //建立连接认证请求
	 int CreateCertConnect( );

	 //获得影片信息列表
	 //int GetDcpList( std::vector< std::string > &PklIdList );
	 int GetDcpList( std::vector< Satellite::DCPInfo > &DcpList );

	 //获得对应影片FTP信息
	 int GetFTPDCPInfoById( const std::string &pklUuid , Satellite::FTP_DCPInfo& rFTP_DCPInfo );

	 //对应影片下载完毕；如果status = 1，表示影片下载完毕。status = 0，表示下载失败。
	 int SetFTPDownLoadFinished(const std::string &pklUuid , const int status);

	 //获得认证连接状态
	 int GetCertConnect_Status();

private:
	//生成设备的控制命令报文;rDatagramLen为生成的报文长度
	int SetCtrlCmd_Device( unsigned char* pBuffer , u16 nCmd, u32 PayloadLength, unsigned char* pPayload , int& rDatagramLen );

	int RecvInfo( char *buffer, int len , int& rHasRecvDataLen );

	std::string GetFTPDCPInfoByIdXml( const std::string &pklUuid );
	std::string FTPDownLoadFinishedAckXml( const std::string &pklUuid , const int status );

private:
	std::string m_xmlHeader;
	std::string m_responseBegin;
	std::string m_responseEnd;
};

#define ERROR_CONNECT_FAILED					-100	//连接认证请求失败	
#define ERROR_CONNECT_ACK_FORMAT				-101	//连接确认格式错误
#define ERRPR_GET_DCPLIST_RESPONSE				-102	//影片信息反馈报文，响应错误
#define ERRPR_GETDCPLIST_ACK_FORMAT				-103	//影片信息反馈报文，请求失败
#define ERRPR_GET_FILM_FTPINFO_RESPONSE			-104	//对应影片FTP信息反馈报文，响应错误
#define ERRPR_GET_FILM_FTPINFO_ACK_FORMAT		-105	//对应影片FTP信息反馈报文，请求失败
#define ERROR_FTP_DOWNLOAD_FINISH_FAILED		-107	//影片下载完毕报文，请求失败
#define ERRPR_FTP_DOWNLOAD_FINISH_ACK_FORMAT	-108	//影片下载完毕报文，响应格式错误

#define ERRPR_TCPTRANSPORTFAILED                -110    //TCP传输失败

#endif	//_SAT_DCP_H_
