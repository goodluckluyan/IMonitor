#ifndef __CONTENT_H__
#include "content.h"
#endif
#ifdef WIN32
#include "socket_err.h"
#endif
#include "C_ErrorDef.h"
#include "FtpClient.h"
#include <list>
#include <errno.h>

//@author zhangmiao@oristartech.com
//@date [2012-07-10]
//@brief 定义、声明全局变量，用于支持DCP获取进度。
//@new
//extern std::string g_sCopyDcp_PklUuidName;//拷贝的pklUUID名
//extern std::string g_sCopyDcp_Copying_FileName;//正在拷贝的文件名
extern int g_flag_DcpCopyStop;//标志位，1=取消导入DCP
//extern unsigned long long g_Dcp_AllFile_Size;//Dcp所有文件的大小。
extern unsigned long long g_hasDcpCopied_Size;//已经拷贝的dcp包的大小。
extern std::map< std::string , Content::FileCopyInfo > g_map_FileCopyInfo;//映射表，用于生成文件拷贝状态信息【文件路径名，FileCopyInfo】

//声明互斥锁
#ifdef WIN32
#include <windows.h>
extern HANDLE hMutex;
#else
#include <pthread.h>
extern pthread_mutex_t work_mutex;
#endif
//@modify end;

using namespace Content;
using namespace std;

namespace cftp
{
	const int BUFFERSIZE = 4096;
	const int MAXCLIENT = 10;
	const int FILETYPELENGTH = 10;
	const int MAXWAITTIME = 3*60;

#define MAXLINELEN 2000
#define RECVBUFFERSIZE 4096

	
	FtpClient::FtpClient()
	{
		m_tcp  = new C_TcpSocket( BUFFERSIZE );

		m_tcpPasv  = new C_TcpSocket( BUFFERSIZE );

		m_iTimeout = MAXWAITTIME;
	}

	FtpClient::~FtpClient()
	{
		if( ( m_tcp != NULL ) && m_tcp->BeConnected() )
			m_tcp->TcpDisConnect();

		delete m_tcp;

		if( ( m_tcpPasv != NULL ) && m_tcpPasv->BeConnected() )
			m_tcpPasv->TcpDisConnect();

		delete m_tcpPasv;
	}

	void FtpClient::SetTimeOut( int iTimeout )
	{
		m_iTimeout = iTimeout;
		
		if ( m_tcp != NULL )
		{
			m_tcp->SetTimeout(m_iTimeout);
		}

		if ( m_tcpPasv != NULL )
		{
			m_tcpPasv->SetTimeout(m_iTimeout);
		}
	}

	int FtpClient::GetTimeout()
	{
		return m_iTimeout;
	}

	int FtpClient::ReFtpInit( int iTimeout )
	{
		m_iTimeout = iTimeout;

		if( ( m_tcp != NULL ) && m_tcp->BeConnected() )
			m_tcp->TcpDisConnect();
		
		if( ( m_tcpPasv != NULL ) && m_tcpPasv->BeConnected() )
			m_tcpPasv->TcpDisConnect();

		if ( m_tcp != NULL )
		{
			delete m_tcp;
			m_tcp = NULL;
		}

		if ( m_tcpPasv != NULL )
		{
			delete m_tcpPasv;
			m_tcpPasv = NULL;
		}

		if ( m_tcp == NULL )
		{
			m_tcp  = new C_TcpSocket( BUFFERSIZE );
		}
		
		if ( m_tcpPasv == NULL )
		{
			m_tcpPasv  = new C_TcpSocket( BUFFERSIZE );
		}

		return 0;
	}

	int FtpClient::ParseUrl(const string &url, string &ip, string &nu, string &pw, string &port, string &dir)
	{
		ip.clear();
		nu.clear();
		pw.clear();
		port.clear();
		dir.clear();

		string::size_type bgn = url.find("//");
		if(bgn == string::npos)
			return PARAMETERERROR;
		bgn += 2;
		if(bgn >= url.size())
			return PARAMETERERROR;

		string::size_type end = url.find("@", bgn);
		if(end != string::npos)
		{
			string::size_type colon = url.find(":", bgn);
			if(colon > end)
				return PARAMETERERROR;
			nu.assign(url, bgn, colon - bgn);
			pw.assign(url, colon + 1, end - (colon + 1));

			bgn = end + 1;
			if(bgn >= url.size())
				return PARAMETERERROR;
		}

		end = url.find("/", bgn);
		string::size_type colon = url.find(":", bgn);
		if(colon == string::npos)
		{
			port = "21";
			ip.assign(url, bgn, end - bgn);
		}
		else
		{
			if(colon > end)
				return PARAMETERERROR;
			ip.assign(url, bgn, colon - bgn);
			port.assign(url, colon + 1, end - (colon + 1));
		}
		if(end != string::npos)
			dir.assign(url, end, url.size());
		else
			dir = "/";

		return NOANYERROR;
	}

	int FtpClient::LogIn(const string &ip, const string &userName, const string &password, unsigned short port)
	{
		m_ip = ip;
		m_userName = userName;
		m_password = password;
		m_port = port;

		if( ( m_tcp != NULL ) && m_tcp->BeConnected() )
			m_tcp->TcpDisConnect();

		m_tcp->SetTimeout( m_iTimeout );    /* 60 */
		
		m_tcpPasv->SetTimeout( m_iTimeout );

		//int result = m_tcp.TcpConnect(ip.c_str(), port);
		int result = m_tcp->TcpConnect( m_ip.c_str() , m_port );
		if(result != 0)
			return TCPTRANSPORTFAILED;

		char buffer[BUFFERSIZE] = "";
		//---[5/21/2013 zhangmiao]--begin--
		//接收ftp服务器Welcome欢迎信息
		//result = RecvInfo(buffer, BUFFERSIZE);
		//result = RecvInfo_Welcome(buffer, BUFFERSIZE);
		result = RecvInfo_MulRes(buffer, BUFFERSIZE);
		//---[5/21/2013 zhangmiao]--end----
		if(result != NOANYERROR)
			return result;

		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";
		int iHasRecvDataLen=0;

		memset(sendBuffer,0,BUFFERSIZE);
		sprintf(sendBuffer, "user %s\r\n", userName.c_str());
		//result = SendAndRecv(buffer, BUFFERSIZE);
		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;

		memset(sendBuffer,0,BUFFERSIZE);
		memset(recvBuffer,0,BUFFERSIZE);
		sprintf(sendBuffer, "pass %s\r\n", password.c_str());
		iHasRecvDataLen=0;
		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		return result;
	}

	int FtpClient::LogIn(const string &url)
	{
		string ip, userName, passwd, dir;
		string port = "21";

		int result = ParseUrl(url, ip, userName, passwd, port, dir);
		if(result != NOANYERROR)
			return result;

		result = LogIn(ip, userName, passwd, atoi(port.c_str()));
		if(result != NOANYERROR)
			return result;

		return (dir.empty() || dir == "/") ? NOANYERROR : SetWorkDir(dir);
	}

	int FtpClient::LogIn(const string &ip, const string &userName, const string &password, string& dir , string port )
	{
		int result = 0;

		result = LogIn( ip, userName, password, atoi(port.c_str()) );
		if( result != NOANYERROR )
			return result;

		return (dir.empty() || dir == "/") ? NOANYERROR : SetWorkDir(dir);
	}

	int FtpClient::ReLogIn()
	{
		LogOut();
		return LogIn(m_ip, m_userName, m_password, m_port);
	}

	int FtpClient::LogOut()
	{
		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";
		int iHasRecvDataLen=0;
		sprintf(sendBuffer, "quit\r\n");

		int result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		
		if( ( m_tcp != NULL ) && m_tcp->BeConnected() )
			m_tcp->TcpDisConnect();
		return result;
	}
	//---zhangmiao:begin---20120710----
	//@author zhangmiao@oristartech.com
	//@date [2012-07-10]
	//@brief 增加ftp断点续传功能
	//@new
	int FtpClient::CancelTransFile()
	{
		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";
		int iHasRecvDataLen=0;

		sprintf(sendBuffer, "abor\r\n");
		//int result = SendAndRecv(buffer, BUFFERSIZE);
		int result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );

		if( ( m_tcpPasv != NULL ) && m_tcpPasv->BeConnected() )
			m_tcpPasv->TcpDisConnect();
		
		return result;
	}

	int FtpClient::RecvFileOnBreakPoint(const string &destFile, unsigned long long& rlTransferredSize , unsigned long long from )
	{
		int out,n=0,nW=0;
		unsigned long long nTransferredSize = 0ull;

		//------------------
		if((out=open(destFile.c_str(),O_WRONLY|O_CREAT|O_BINARY,0644))==-1)
		{
#ifdef _TEST_
			printf("cp_f:fail to open %s:%s\n",destFile.c_str(),strerror(errno));
#endif
			return FILECANNOTOPEN;
		}

		//------------------------
		if (from<0)
		{
#ifdef _TEST_
			printf( "Error:File start position is %lld\n",from );
#endif
			return -1;
		}

		unsigned long long nlPos = 0;
		nlPos = lseek(out,(long)from,SEEK_SET);
		if (nlPos<0)
		{
#ifdef _TEST_
			printf( "Error:File start position is %lld\n",nlPos );
#endif
			return -1;
		}
		nTransferredSize += from;
		//--------------------
		int result = 0;
		char buffer[BUFFERSIZE]="";
		while(1)
		{
			memset(buffer,0,BUFFERSIZE);
			/*result = m_tcpPasv.TcpRecv(buffer, BUFFERSIZE);
			if(result <= 0)
				break;*/
			int recvlen = 0;
			result = m_tcpPasv->OneRecv( buffer, BUFFERSIZE, recvlen );
			if( result != 0 )    /*if(result < 0)*/
				break;

			//n = result;
			n = recvlen;
		
			nW = 0;
			if( ( nW = write(out,buffer,n) ) != n )
			{
				close(out);
#ifdef _TEST_
				printf("file handle %d is closed\n",out);
#endif
				return -1;
			}
			memset( buffer, 0, BUFFERSIZE );

#ifdef WIN32
			WaitForSingleObject(hMutex,INFINITE);
#else
			pthread_mutex_lock(&work_mutex);
#endif
			g_hasDcpCopied_Size += nW;
			nTransferredSize += nW;

			FileCopyInfo& copyInfo   = g_map_FileCopyInfo[string(destFile)]; 
			copyInfo.transferredSize = static_cast<int>(nTransferredSize/ONEMB);

			if( g_flag_DcpCopyStop == 1 )
			{
				close(out);
				CancelTransFile();

				rlTransferredSize = nTransferredSize;

#ifdef WIN32
				ReleaseMutex(hMutex);
#else
				pthread_mutex_unlock(&work_mutex);
#endif

#ifdef _TEST_
				printf("copy DCP received the stop signal! Code is %d\n",CODE_DCPCOPYCANCEL);
#endif
				errno = CODE_DCPCOPYCANCEL;
				return CODE_DCPCOPYCANCEL;
			}

#ifdef WIN32
			ReleaseMutex(hMutex);
#else
			pthread_mutex_unlock(&work_mutex);
#endif
			
		}

		rlTransferredSize = nTransferredSize;

		close(out);
#ifdef _TEST_
		printf("file handle %d is closed\n",out);
#endif
		if(n<0) return -1;
		
		//m_tcpPasv.ReleaseConnect();
		if( ( m_tcpPasv != NULL ) && m_tcpPasv->BeConnected() )
			m_tcpPasv->TcpDisConnect();

		//return result < 0 ? TCPTRANSPORTFAILED : NOANYERROR;
		if ( result != 0 && ( TECS(result) != CONN_PEER_TERMINATOR ) )  //if ( result < 0 && (result != CONN_PEER_TERMINATOR) )
		{
			return TCPTRANSPORTFAILED;
		}
		else
		{
			return NOANYERROR;
		}
	}
	int FtpClient::TransportFileOnBreakPoint(const std::string &source, const std::string &dest, const std::string &fileName, long long from )
	{
		string sourceFile = source;
		string destFile = dest;
		if(!sourceFile.empty() && sourceFile[sourceFile.size() - 1] != '/')
			sourceFile += '/';
		
		//@author zhangmiao@oristartech.com
		//@date [2013-07-18]
		//@brief 主要增加了FTP下载相关字幕文件的处理代码
		//@new
		/*if(!destFile.empty() && destFile[destFile.size() - 1] != '/')
			destFile += '/';*/
		if( !destFile.empty() && destFile[destFile.size() - 1] != DIR_SEPCHAR )
			destFile += DIR_SEPCHAR;
		//@modify end;

		sourceFile += fileName;
		destFile += fileName;

		int result = 0;
		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";

		int iHasRecvDataLen=0;
		sprintf(sendBuffer, "pasv\r\n");
		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;

		string ftpIp;
		unsigned short ftpPort;
		result = GetIpAndPort(recvBuffer, ftpIp, ftpPort);
		if(result != NOANYERROR)
			return result;
		result = ConnectPasv(ftpIp, ftpPort);
		if(result != NOANYERROR)
			return result;

		iHasRecvDataLen = 0;
		memset(recvBuffer,0,BUFFERSIZE);
		sprintf(sendBuffer, "type I\r\n" );
		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;
		
		iHasRecvDataLen = 0;
		memset(recvBuffer,0,BUFFERSIZE);
		sprintf(sendBuffer, "rest %lld\r\n", from);
		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;

		iHasRecvDataLen = 0;
		memset(recvBuffer,0,BUFFERSIZE);
		sprintf(sendBuffer, "retr %s\r\n", sourceFile.c_str());
		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;

		result = RecvFileOnBreakPoint(destFile,(unsigned long long&)fileWasTransported,from);
		if(result != NOANYERROR)
			return result;

		iHasRecvDataLen=0;
		memset(recvBuffer,0,BUFFERSIZE);
		result = RecvInfo( recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		return result;
	}
	//@modify end;
	//---zhangmiao:end--[7/10/2012]--

	int FtpClient::TransportFile(const string &source, const string &dest, const string &fileName, long long int point)
	{
		string sourceFile = source;
		string destFile = dest;
		if(!sourceFile.empty() && sourceFile[sourceFile.size() - 1] != '/')
			sourceFile += '/';
		
		//@author zhangmiao@oristartech.com
		//@date [2013-07-18]
		//@brief 主要增加了FTP下载相关字幕文件的处理代码
		//@new
		/*if(!destFile.empty() && destFile[destFile.size() - 1] != '/')
			destFile += '/';*/
		if( !destFile.empty() && destFile[destFile.size() - 1] != DIR_SEPCHAR )
			destFile += DIR_SEPCHAR;
		//@modify end;

		sourceFile += fileName;
		destFile += fileName;

		int result = 0;
		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";

		//char buffer[BUFFERSIZE];
		sprintf(sendBuffer, "pasv\r\n");
		//int result = SendAndRecv(buffer, BUFFERSIZE);
		int iHasRecvDataLen=0;
		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;

		string ftpIp;
		unsigned short ftpPort;
		result = GetIpAndPort(recvBuffer, ftpIp, ftpPort);
		if(result != NOANYERROR)
			return result;
		result = ConnectPasv(ftpIp, ftpPort);
		if(result != NOANYERROR)
			return result;

		iHasRecvDataLen = 0;
		memset(recvBuffer,0,BUFFERSIZE);
		sprintf(sendBuffer, "retr %s\r\n", sourceFile.c_str());
		//result = SendAndRecv(buffer, BUFFERSIZE);
		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;
		
		result = RecvFile(destFile);
		if(result != NOANYERROR)
			return result;

		//return RecvInfo(buffer, BUFFERSIZE);
		iHasRecvDataLen=0;
		memset(recvBuffer,0,BUFFERSIZE);
		result = RecvInfo( recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		return result;
	}

	

	int FtpClient::GetWorkDir(string &dir)
	{
		int result = 0;

		int iHasRecvDataLen=0;
		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";

		//char buffer[BUFFERSIZE];
		sprintf(sendBuffer, "pwd\r\n");
		//int result = SendAndRecv(buffer, BUFFERSIZE);
		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );

		if(result != NOANYERROR)
			return result;

		char *dirBegin = strstr(recvBuffer, "\"");
		dirBegin += 1;
		char *dirEnd = strstr(dirBegin, "\"");
		*dirEnd = 0;
		dir = dirBegin;
		return NOANYERROR;
	}

	int FtpClient::SetWorkDir(const string &dir)
	{
		int result = 0;

		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";
		int iHasRecvDataLen=0;

		//char buffer[BUFFERSIZE];
		sprintf(sendBuffer, "cwd %s\r\n", dir.c_str());

		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );

		return result;
	}

	
	int FtpClient::ListFile(const string &sourceFile, string &fileList)
	{
		int result = 0;
		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";

		sprintf(sendBuffer, "pasv\r\n");

		//int result = SendAndRecv(buffer, BUFFERSIZE);
		int iHasRecvDataLen=0;
		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;

		string ftpIp;
		unsigned short ftpPort;
		result = GetIpAndPort(recvBuffer, ftpIp, ftpPort);
		if(result != NOANYERROR)
			return result;

		result = ConnectPasv(ftpIp, ftpPort);
		if(result != NOANYERROR)
			return result;

		sprintf(sendBuffer, "list %s\r\n", sourceFile.c_str());
		iHasRecvDataLen=0;
		memset(recvBuffer,0,BUFFERSIZE);
		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;

		result = RecvFileInfo(fileList);
		if(result != NOANYERROR)
			return result;

		//return RecvInfo(buffer, BUFFERSIZE);
		iHasRecvDataLen=0;
		memset(recvBuffer,0,BUFFERSIZE);
		result = RecvInfo( recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		return result;
	}

	int FtpClient::ListFile(const string &sourceFile, vector<FileInformation> &fileList)
	{
		string fileListStr;
		int result = ListFile(sourceFile, fileListStr);
		if(result != NOANYERROR)
			return result;

		return FileListStringToStruct(fileListStr, fileList);
	}

	int FtpClient::SendAndRecv( const char *sendBuffer, int iLen , char *recvBuffer, int recvBufferLength , int& rHasRecvDataLen )
	{
		int result = -1;
		int iSendedLen = 0;
		result = m_tcp->Send( sendBuffer, iLen , iSendedLen );

#ifdef _DEBUG
		printf("[---send cmd---] :\n");	
		int i = 0;
		for(i = 0; i < iSendedLen; i++)
			printf("%02X ", sendBuffer[i]);
		printf("\n");
#endif // _DEBUG

		printf("send cmd:%s\n",sendBuffer);
		
		if( ( result != 0 ) || ( iLen != iSendedLen ) )
		{
			printf("[ERROR] \" SendAndRecv \" send failed(%d):Len=%d,sendlen=%d\n", result , iLen, iSendedLen);

			return TCPTRANSPORTFAILED;
		}
		else
		{
			printf("[INFO]  \" SendAndRecv \" send succeed(%d)!\n", result);

			result = RecvInfo( recvBuffer, recvBufferLength , rHasRecvDataLen );
			if(result == 0)    //SOCKET_SUCCEES			
			{
				printf("[INFO]  \" SendAndRecv \" recv succeed(%d)!\n", result);
			}
			else
			{
				if(result < 0)
					m_tcp->TcpDisConnect();
			}
			return result;
		}
		
	}

	int FtpClient::RecvInfo(char *recvBuffer, int bufferLength , int& rHasRecvDataLen )
	{
		int recvlen = 0;
		int result = -1;
		
		recvlen = 0;
		result = m_tcp->Recv( recvBuffer , bufferLength , "\r\n" , 2 , recvlen );
		if(result == 0)    //SOCKET_SUCCEES			
		{
#ifdef _DEBUG
			printf("[---recv data---]:\n");
			for(int i = 0; i < recvlen; i++)
				printf("%02X ", recvBuffer[i]);
			printf("\n");
#endif // _DEBUG
			
			printf("[INFO]  \" RecvInfo \" recv data succeed(%d)!\n", result);

			rHasRecvDataLen = recvlen;
		}
		else
		{
			if( (result < 0) || result != 0 )  //if(result < 0)
				return TCPTRANSPORTFAILED;
		}
		
		recvBuffer[recvlen] = '\0';
		
		if(recvBuffer[0] != '1' && recvBuffer[0] != '2' && recvBuffer[0] != '3')
		{
			printf("recv:%s\n",recvBuffer);
			m_errorCode = recvBuffer;
			return FTPSERVICEERROR;
		}

#ifdef _DEBUG
		printf("recv:%s\n",recvBuffer);
#endif // _DEBUG

		return NOANYERROR;
	}

	int FtpClient::RecvInfo_MulRes( char *receiveBuffer, int size )
	{
		std::list<std::string> MultilineResponseLines;
		std::string	MultilineResponseCode;
		std::string	sResponse;

		int BufferLen = 0;

		while (true)
		{
			int rHasRecvDataLen = 0;

			int result = RecvInfo( receiveBuffer + BufferLen ,  size - BufferLen , rHasRecvDataLen );

			if(result < 0)
				return TCPTRANSPORTFAILED;

			printf("recv:%s\n",receiveBuffer + BufferLen);

			int numread = rHasRecvDataLen;

			char* start = receiveBuffer + BufferLen;
			BufferLen += numread;

			for (int i = start - receiveBuffer; i < BufferLen; i++)
			{
				char& p = receiveBuffer[i];
				if (p == '\r' ||
					p == '\n' ||
					p == 0)
				{
					int len = i - (start - receiveBuffer);
					if (!len)
					{
						start++;
						continue;
					}

					if (len > MAXLINELEN)
						len = MAXLINELEN;

					p = 0;


					std::string line = start;
					start = receiveBuffer + i + 1;

					p = '\r';

					
					//Check for multi-line responses
					if (line.length() > 3)
					{
						if ( MultilineResponseCode != "" )
						{
							if ( line.substr( 0 , 4 ) == MultilineResponseCode )
							{
								// end of multi-line found
								MultilineResponseCode.clear();
								sResponse = line;
								MultilineResponseLines.push_back(line);
								return NOANYERROR;
							}
							else
								MultilineResponseLines.push_back(line);
						}
						// start of new multi-line
						else if (line.at(3) == '-')
						{
							// DDD<SP> is the end of a multi-line response
							MultilineResponseCode = line.substr( 0 , 3 ) + " ";
							MultilineResponseLines.push_back(line);
						}
						else
						{
							sResponse = line;
							return NOANYERROR;
						}
					}	
					

					// Abort if connection got closed
					if (!BeConnected())
						return TCPTRANSPORTFAILED;
				}
			}

			if (BufferLen > MAXLINELEN)
				BufferLen = MAXLINELEN;
		}

		return NOANYERROR;
	}

	

	int FtpClient::GetIpAndPort(char *buffer, string &ip, unsigned short &port)
	{
		char *ipbegin = strpbrk(buffer + 4, "123456789");
		if(ipbegin == NULL)
		{
			m_errorCode = buffer;
			return FTPSERVICEERROR;
		}

		char *doc = ipbegin;
		for(int i=0; i<4; i++)
		{
			doc = strstr(doc, ",");
			if(ipbegin == NULL)
			{
				m_errorCode = buffer;
				return FTPSERVICEERROR;
			}
			*doc = '.';
		}
		*doc = 0;
		ip = ipbegin;

		char *portHigh = doc+1;
		char *portLow = strstr(portHigh, ",");
		*portLow = 0;
		portLow += 1;
		port = (short)(atoi(portHigh) * 256 + atoi(portLow));
		return NOANYERROR;
	}

	int FtpClient::ConnectPasv(const string &ip, unsigned short port)
	{
		int result = 0;
		if( ( m_tcpPasv != NULL ) && m_tcpPasv->BeConnected() )
			m_tcpPasv->TcpDisConnect();

		m_tcpPasv->SetTimeout( m_iTimeout );

		result = m_tcpPasv->TcpConnect( ip.c_str() , port );

		return result != 0 ? TCPTRANSPORTFAILED : NOANYERROR;   /*return result < 0 ? TCPTRANSPORTFAILED : NOANYERROR;*/
	}

	int FtpClient::RecvFile(const string &destFile)
	{
		fileWasTransported = 0;
		ofstream fout(destFile.c_str(), ofstream::binary);
		if(!fout.is_open())
			return FILECANNOTOPEN;

		int result = 0;
		char buffer[BUFFERSIZE]="";
		while(1)
		{
			memset(buffer,0,BUFFERSIZE);
			
			int recvlen = 0;
			result = m_tcpPasv->OneRecv( buffer, BUFFERSIZE, recvlen );
			if(result != 0)  //if(result < 0)
				break;

			fout.write(buffer, recvlen);
			fileWasTransported += recvlen;
		}
		fout.close();

		
		if( ( m_tcpPasv != NULL ) && m_tcpPasv->BeConnected() )
			m_tcpPasv->TcpDisConnect();

		
		if ( result != 0 && ( TECS(result) != CONN_PEER_TERMINATOR ) )  //if ( result < 0 && (result != CONN_PEER_TERMINATOR) )
		{
			return TCPTRANSPORTFAILED;
		}
		else
		{
			return NOANYERROR;
		}
	}

	

	int FtpClient::RecvFileInfo(string &info)
	{
		info.clear();
		int result = 0;
		char buffer[BUFFERSIZE] = "";
		while(1)
		{
			memset(buffer,0,BUFFERSIZE);
			
			int recvlen = 0;
			result = m_tcpPasv->Recv( buffer , BUFFERSIZE , "\r\n" , 2 , recvlen );
			//result = m_tcpPasv->OneRecv( buffer, BUFFERSIZE, recvlen );

			if(result != 0)  //if(result < 0)
				break;

			buffer[recvlen] = 0;
			info += buffer;
		}
		
		if( ( m_tcpPasv != NULL ) && m_tcpPasv->BeConnected() )
			m_tcpPasv->TcpDisConnect();

		if ( result != 0 && ( TECS(result) != CONN_PEER_TERMINATOR ) )  //if ( result < 0 && (result != CONN_PEER_TERMINATOR) )
		{
			return TCPTRANSPORTFAILED;
		}
		else
		{
			return NOANYERROR;
		}
	}

	int FtpClient::FileListStringToStruct(const string &fileStr, vector<FileInformation> &fileList)
	{
		string::size_type begPos = 0;
		string::size_type endPos = fileStr.find("\r\n", begPos);
		while(begPos != string::npos && endPos != string::npos)
		{
			string aFileStr(fileStr, begPos, endPos-begPos);

			FileInformation fileInfo;
			string::size_type fileBegPos = 0;
			fileInfo.type.assign(aFileStr, fileBegPos, FILETYPELENGTH);

			fileBegPos = aFileStr.find_first_not_of("\t ", FILETYPELENGTH);//find '\t' and ' '
			string::size_type fileEndPos = aFileStr.find_first_of("\t ", fileBegPos);
			fileInfo.linkCount.assign(aFileStr, fileBegPos, fileEndPos - fileBegPos);

			fileBegPos = aFileStr.find_first_not_of("\t ", fileEndPos);
			fileEndPos = aFileStr.find_first_of("\t ", fileBegPos);
			fileInfo.user.assign(aFileStr, fileBegPos, fileEndPos - fileBegPos);

			fileBegPos = aFileStr.find_first_not_of("\t ", fileEndPos);
			fileEndPos = aFileStr.find_first_of("\t ", fileBegPos);
			fileInfo.group.assign(aFileStr, fileBegPos, fileEndPos - fileBegPos);

			fileBegPos = aFileStr.find_first_not_of("\t ", fileEndPos);
			fileEndPos = aFileStr.find_first_of("\t ", fileBegPos);
			fileInfo.size.assign(aFileStr, fileBegPos, fileEndPos - fileBegPos);

			//ftp服务器list命令返回返回的目录列表，解析存在问题，当文件名有空格时解析会出错
			//eg: drwxr-xr-x 1 ftp ftp              0 Jan 21 11:08 Assets of 4K_UnEnc_24_fps_3Min
			/*fileBegPos = aFileStr.find_first_not_of("\t ", fileEndPos);
			fileEndPos = aFileStr.find_last_of("\t ");
			fileInfo.name.assign(aFileStr, ++fileEndPos, string::npos);

			fileEndPos = aFileStr.find_last_not_of("\t ", fileEndPos-1);
			fileInfo.time.assign(aFileStr, fileBegPos, ++fileEndPos - fileBegPos);*/

			fileBegPos = aFileStr.find_first_not_of("\t ", fileEndPos);
			fileEndPos = aFileStr.find_first_of("\t ", fileBegPos);
			/*fileEndPos = aFileStr.find_first_of("\t ", fileEndPos+1);
			fileEndPos = aFileStr.find_first_of("\t ", fileEndPos+1);
			fileInfo.time.assign(aFileStr, fileBegPos, fileEndPos - fileBegPos);*/

			//list命令返回的目录列表，解析出错存在问题，日期、时间格式复杂
			//eg 1: -rw-r--r-- 1 ftp ftp           1735 Dec 18  2013 ASSETMAP
			//eg 2: -rw-r--r-- 1 ftp ftp       94464252 May 12 15:45 NISSAN_PICTURE.mxf
			string sMonth , sDay , sTimeOrYear;
			sMonth.assign( aFileStr, fileBegPos, fileEndPos - fileBegPos );

			string::size_type nTmpBegPos = 0;
			nTmpBegPos = aFileStr.find_first_not_of("\t ", fileEndPos);
			fileEndPos = aFileStr.find_first_of("\t ", nTmpBegPos);
			
			sDay.assign( aFileStr, nTmpBegPos, fileEndPos - nTmpBegPos );
			
			nTmpBegPos = aFileStr.find_first_not_of("\t ", fileEndPos);
			fileEndPos = aFileStr.find_first_of("\t ", nTmpBegPos);

			sTimeOrYear.assign(aFileStr, nTmpBegPos, fileEndPos - nTmpBegPos);

			fileInfo.time.assign(aFileStr, fileBegPos, fileEndPos - fileBegPos);
			//add end

			fileBegPos = aFileStr.find_first_not_of("\t ", fileEndPos);
			fileInfo.name.assign(aFileStr, fileBegPos, string::npos - fileBegPos);

			fileList.push_back(fileInfo);

			begPos = endPos + strlen("\r\n");
			endPos = fileStr.find("\r\n", begPos);
		}
		return NOANYERROR;
	}

	//create dir in current work directory
	int FtpClient::CreateDir( const std::string &dir )
	{
		int result = 0;

		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";
		int iHasRecvDataLen=0;

		sprintf(sendBuffer, "mkd %s\r\n", dir.c_str());

		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );

		return result;
	}

	//delete dir from current work directory
	int FtpClient::DeleteDir( const std::string &dir )
	{
		int result = 0;

		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";
		int iHasRecvDataLen = 0;

		sprintf(sendBuffer, "rmd %s\r\n", dir.c_str());

		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );

		return result;
	}
	
	//delete file parameter is file path with name
	int FtpClient::DeleteFile( const std::string &filePath )
	{
		int result = 0;

		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";
		int iHasRecvDataLen = 0;

		if(filePath.empty())
			return PARAMETERERROR;

		sprintf(sendBuffer, "dele %s\r\n", filePath.c_str());

		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );

		return result;
	}

	//find file name from current work directory, and rename it.
	int FtpClient::FileRename( const std::string &fileName, const std::string &newName )
	{
		int result = 0;

		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";
		int iHasRecvDataLen = 0;

		sprintf(sendBuffer, "rnfr %s\r\n", fileName.c_str());
		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;


		iHasRecvDataLen = 0;
		memset(sendBuffer,0,BUFFERSIZE);
		memset(recvBuffer,0,BUFFERSIZE);

		sprintf(sendBuffer, "rnto %s\r\n", newName.c_str());

		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );

		return result;
	}

	int FtpClient::UploadFile_Part_Start( const std::string &dest , const std::string &fileName , int IsAppend )
	{
		int result = 0;

		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";
		int iHasRecvDataLen = 0;

		string destFile = dest;
		if(!destFile.empty() && destFile[destFile.size() - 1] != '/')
			destFile += '/';
		destFile += fileName;

		sprintf(sendBuffer, "pasv\r\n");

		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;

		string ftpIp;
		unsigned short ftpPort;
		result = GetIpAndPort(recvBuffer, ftpIp, ftpPort);
		if(result != NOANYERROR)
			return result;
		
		result = ConnectPasv(ftpIp, ftpPort);
		if(result != NOANYERROR)
			return result;


		iHasRecvDataLen = 0;
		memset(sendBuffer,0,BUFFERSIZE);
		memset(recvBuffer,0,BUFFERSIZE);

		if ( IsAppend != 0 )
		{
			sprintf(sendBuffer, "appe %s\r\n", destFile.c_str());
		} 
		else
		{
			sprintf(sendBuffer, "stor %s\r\n", destFile.c_str());
		}

		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;

		/*result = SendFileInfo(fileInfo);
		if(result != NOANYERROR)
			return result;

		iHasRecvDataLen = 0;
		memset(recvBuffer,0,BUFFERSIZE);
		result = RecvInfo( recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		*/

		return result;
	}

	int FtpClient::UploadFile_Part_End()
	{
		if( ( m_tcpPasv != NULL ) && m_tcpPasv->BeConnected() )
			m_tcpPasv->TcpDisConnect();

		int result = 0;

		char recvBuffer[BUFFERSIZE]="";
		int iHasRecvDataLen = 0;
		
		iHasRecvDataLen = 0;
		memset(recvBuffer,0,BUFFERSIZE);

		result = RecvInfo( recvBuffer , BUFFERSIZE , iHasRecvDataLen );

		return result;
	}

	int FtpClient::UploadFile_Part_SendFileInfo( const char *sendBuffer , int bufferLength , int &rSendedLen )
	{
		int result = 0;
		int iSendedLen = 0;
		int iLen = 0;
		iLen = bufferLength;

		result = m_tcpPasv->Send( sendBuffer , iLen , iSendedLen );

		if( ( result != 0 ) || ( iLen != iSendedLen ) )
		{
			printf("[ERROR] \" UploadFile_Part_SendFileInfo \" send failed(%d):Len=%d,sendlen=%d\n", result , iLen, iSendedLen);

			if( ( m_tcpPasv != NULL ) && m_tcpPasv->BeConnected() )
				m_tcpPasv->TcpDisConnect();

			return TCPTRANSPORTFAILED;
		}
		else
		{
			rSendedLen = iSendedLen;
			return NOANYERROR;
		}

		//return result;
	}

	int FtpClient::UploadFile( const std::string &source , const std::string &dest , const std::string &fileName )
	{
		string sourceFile = source;

		string destFile = dest;
		if(!destFile.empty() && destFile[destFile.size() - 1] != '/')
			destFile += '/';
		destFile += fileName;

		int result = 0;

		char sendBuffer[BUFFERSIZE]="";
		char recvBuffer[BUFFERSIZE]="";
		int iHasRecvDataLen = 0;
		sprintf(sendBuffer, "pasv\r\n");

		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;

		string ftpIp;
		unsigned short ftpPort;
		result = GetIpAndPort(recvBuffer, ftpIp, ftpPort);
		if(result != NOANYERROR)
			return result;

		result = ConnectPasv(ftpIp, ftpPort);
		if(result != NOANYERROR)
			return result;

		iHasRecvDataLen = 0;
		memset(sendBuffer,0,BUFFERSIZE);
		memset(recvBuffer,0,BUFFERSIZE);
		sprintf(sendBuffer, "stor %s\r\n", destFile.c_str());

		result = SendAndRecv( sendBuffer , strlen(sendBuffer) , recvBuffer , BUFFERSIZE , iHasRecvDataLen );
		if(result != NOANYERROR)
			return result;

		result = SendFile( sourceFile );
		if(result != NOANYERROR)
			return result;

		iHasRecvDataLen = 0;
		memset(recvBuffer,0,BUFFERSIZE);
		result = RecvInfo( recvBuffer , BUFFERSIZE , iHasRecvDataLen );

		return result;
	}

	
	int FtpClient::SendFile( const std::string &sourceFile )
	{
		int result = 0;
		fileWasTransported = 0;
		char sendBuffer[BUFFERSIZE]="";
		int read_num = 0;
		int iSendedLen = 0;

		ifstream fin(sourceFile.c_str(), ofstream::binary);
		if(!fin.is_open())
			return FILECANNOTOPEN;

		while(1)
		{
			iSendedLen = 0;
			memset(sendBuffer,0,BUFFERSIZE);
			read_num = 0;
			
			if( fin.bad() )
			{
				fin.close();
#ifdef _TEST_
				printf("Error:file %s is closed\n" , sourceFile.c_str() );
#endif

				if( ( m_tcpPasv != NULL ) && m_tcpPasv->BeConnected() )
					m_tcpPasv->TcpDisConnect();

				return UNKNOWERROR;
			}

			fin.read(sendBuffer, BUFFERSIZE);
			read_num = fin.gcount();

			if( read_num >= 0 )
			{
				if( fin.good() )
				{
					printf( "read Data =%d\n" , read_num );
				}
				else if ( fin.eof() && ( read_num == 0 ) )
				{
					break;
				}
			}
			else
			{
				fin.close();
#ifdef _TEST_
				printf("Error:file %s is closed\n" , sourceFile.c_str() );
#endif

				if( ( m_tcpPasv != NULL ) && m_tcpPasv->BeConnected() )
					m_tcpPasv->TcpDisConnect();

				return UNKNOWERROR;
			}

			result = m_tcpPasv->Send( sendBuffer , read_num , iSendedLen );
			if( result != 0 )
				break;
			
			fileWasTransported += iSendedLen;
		}

		fin.close();

		if( ( m_tcpPasv != NULL ) && m_tcpPasv->BeConnected() )
			m_tcpPasv->TcpDisConnect();

		if( ( result != 0 ) || ( read_num != iSendedLen ) )
		{
			printf("[ERROR] \" SendFile \" send failed(%d):Len=%d,sendlen=%d\n" , result , read_num , iSendedLen );

			return TCPTRANSPORTFAILED;
		}
		else
		{
			return NOANYERROR;
		}
	}

};
