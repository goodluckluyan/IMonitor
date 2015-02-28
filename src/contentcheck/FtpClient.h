/******************************************************************************
fileName;
Description:FTP客户端
compile/link:g++
History:     
      <author>               <time>   <version>

how to use:
******************************************************************************/
#ifndef _H_FTPCLIENT_
#define _H_FTPCLIENT_

#if defined(_WIN32)
#include <io.h>
#include <winsock2.h>
#define net_close closesocket
#define MSG_NOSIGNAL 0
#define EINPROGRESS             WSAEINPROGRESS
#define ECONNREFUSED            WSAECONNREFUSED
#define ETIMEDOUT               WSAETIMEDOUT
typedef int socklen_t;

#define TECS(n) (n)
#else
#define net_close close
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define TECS(n) (n & 0xffff)
#endif
#if defined(_WIN32)
#define SETSOCKOPT_OPTVAL_TYPE (const char *)
#define GETSOCKOPT_OPTVAL_TYPE (char *)
#define sleep(s) Sleep(s*1000)
#else
#define SETSOCKOPT_OPTVAL_TYPE (void *)
#define GETSOCKOPT_OPTVAL_TYPE (void *)
#endif

#include <assert.h>
#include <string>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <fcntl.h>

#include "utility/C_TcpSocket.h"

namespace cftp
{

	struct FileInformation
	{//file information from ftp server
		std::string type;
		std::string linkCount;
		std::string user;
		std::string group;
		std::string size;
		std::string time;
		std::string name;
	};

	//first call LogIn();
	//then call TransportFile() to transfer file or ListFile() to get DCP file information.
	//when finishing,call LogOut() to release the connect with FTP server.
	//if function return FTPSERVICEERROR,call GetErrorCode() to get specific information
	class FtpClient
	{
	public:
		FtpClient();
		~FtpClient();

		int ReFtpInit( int iTimeout = 3*60 );

		void SetTimeOut( int iTimeout = 3*60 );
		int GetTimeout();

		int ParseUrl(const std::string &url, std::string &ip, std::string &nu, std::string &pw, std::string &port, std::string &dir);

		int LogIn(const std::string &ip, const std::string &userName, const std::string &password, std::string& dir , std::string port = "21" );

		//after calling LogIn(),having connecting FTP server
		int LogIn(const std::string &ip, const std::string &userName, const std::string &password, unsigned short port = 21);
		//url format:ftp://username:password@172.16.9.100:21/directory/
		//if the port is 21,ommit is ok;if being able to login root directory,ommiting path is ok.
		int LogIn(const std::string &url);
		//log out and log in
		int ReLogIn();
		int LogOut();

		//to get current work directory
		int GetWorkDir(std::string &dir);
		//set working path,the path can be either absolute path or relative path
		int SetWorkDir(const std::string &dir);

		//@author zhangmiao@oristartech.com
		//@date [2014-12-08]
		//@brief 增加FTP上传文件的代码
		//@new
		//create dir in current work directory
		int CreateDir(const std::string &dir);
		
		//delete dir from current work directory
		int DeleteDir(const std::string &dir);

		//delete file parameter is file path with name
		int DeleteFile(const std::string &filePath);
		
		//find file name from current work directory, and rename it.
		int FileRename(const std::string &fileName, const std::string &newName);

		//Upload file to ftp server
		int UploadFile_Part_Start( const std::string &dest ,  const std::string &fileName , int IsAppend = 0 );
		int UploadFile_Part_SendFileInfo( const char *sendBuffer , int bufferLength , int &rSendedLen );
		int UploadFile_Part_End();

		//上传单个文件，从本地磁盘。
		int UploadFile( const std::string &source , const std::string &dest ,  const std::string &fileName );

		int SendFile( const std::string &sourceFile );
		//@modify [2014-12-08] end;

		//---zhangmiao:begin---[7/10/2012]----
		//@author zhangmiao@oristartech.com
		//@date [2012-07-10]
		//@brief 增加ftp断点续传功能
		//@new
		int RecvFileOnBreakPoint(const std::string &destFile, unsigned long long& rlTransferredSize , unsigned long long from = 0 );
		int CancelTransFile();
		int TransportFileOnBreakPoint(const std::string &source, const std::string &dest, const std::string &fileName, long long from = 0 );
        //@modify end;
		//---zhangmiao:end--[7/10/2012]--
		//parameter:source path;destination path;file name
		int TransportFile(const std::string &source, const std::string &dest, const std::string &fileName, long long int point = 0);
		

		//get file information;if 'sourceFile' is file,fileList return file information.
		//if 'sourceFile' is path,fileList return informarion about all of the files under path
		//if 'sourceFile'is null,fileList return informarion about all of the files under the current path
		int ListFile(const std::string &sourceFile, std::string &fileList);
		int ListFile(const std::string &sourceFile, std::vector<FileInformation> &fileList);
		

		//to get error information,if return value type is 'FTPSERVICEERROR'
		std::string GetErrorCode(){ return m_errorCode; }
		//test function weather the ftp server was connected.
		bool BeConnected()const
		{ 
			return m_tcp->BeConnected(); 
		};
		//file size which was transported.
		long long FileTransportSize(){ return fileWasTransported; }

	private:
		
		int SendAndRecv( const char *sendBuffer, int iLen , char *recvBuffer, int recvBufferLength , int& rHasRecvDataLen );
		
		
		int RecvInfo(char *recvBuffer, int bufferLength , int& rHasRecvDataLen );

		int GetIpAndPort(char *buffer, std::string &ip, unsigned short &port);
		int ConnectPasv(const std::string &ip, unsigned short port);
		
		int RecvFile(const std::string &destFile);
		
		int RecvFileInfo(std::string &info);

		//parse the file info string to a struct
		int FileListStringToStruct(const std::string &fileStr, std::vector<FileInformation> &fileList);

		

		int RecvInfo_MulRes( char *receiveBuffer, int BufferLen );

	private:
		std::string m_ip;
		std::string m_userName;
		std::string m_password;
		unsigned short m_port;

		C_TcpSocket* m_tcp;
		C_TcpSocket* m_tcpPasv;

	
		long long fileWasTransported;
		std::string m_errorCode;

		FtpClient(FtpClient &);
		FtpClient &operator=(FtpClient &);

		//通信的超时时间
		int m_iTimeout;
	
	};

	enum
	{
		NOANYERROR = 0,
		TCPTRANSPORTFAILED = -1,
		FILECANNOTOPEN = -2,
		PARAMETERERROR = -3,
		FTPSERVICEERROR = -10,
		UNKNOWERROR = -100,
	};
};

#endif//_H_FTPCLIENT_
