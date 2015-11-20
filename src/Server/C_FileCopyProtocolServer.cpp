#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <memory.h>
#include <stdio.h>
#include <unistd.h>
#include "C_FileCopyProtocolServer.h"
#include "C_RecvDataObserverImp.h"
#include "C_FileOperationsTool.h"
#include "C_Log.h"

namespace FileCopyProtocol
{
	CFileCopyProtocolServer::CFileCopyProtocolServer():m_pTcpServer(NULL),
		m_pRecvDataObserver(NULL),
		m_serverPort(12319)
	{
		Init();
	}

	CFileCopyProtocolServer::~CFileCopyProtocolServer()
	{
		Close();
	}

	void CFileCopyProtocolServer::SetServerPort(const unsigned short& port )
	{
		m_serverPort =port;
	}

	void CFileCopyProtocolServer::SetLogPath(const std::string& path )
	{
		m_LogPath =path;
	}

	bool CFileCopyProtocolServer::Start()
	{
		m_pTcpServer =createTcpServer();
		m_pRecvDataObserver =new CRecvDataObserverImp(this );
		m_pTcpServer->SetRecvDataObserver(m_pRecvDataObserver );
		m_pTcpServer->SetServerPort(m_serverPort );
		CFileOperationsTool* pInst =CFileOperationsTool::Instance();
		if (pInst )
		{
			if(!pInst->FileExist(m_LogPath ) )
			{
				if (pInst->CreateDirectoryPath(m_LogPath ) )
				{
					CLog::SetFilePath(m_LogPath );
					CLog::Write(Waring,"The LogPath "+m_LogPath+" is  not exist,Create Sucess" );
				}
			}else
			{
				CLog::SetFilePath(m_LogPath );
			}
		}
		bool flag =m_pTcpServer->StartServer();
		return flag;
	}

	bool CFileCopyProtocolServer::Close()
	{
		bool flag =true;
		if (m_pTcpServer )
		{
			flag =m_pTcpServer->CloseServer();
			releaseTcpServer(m_pTcpServer );
			m_pTcpServer =NULL;
		}
		if (m_pRecvDataObserver )
		{
			delete m_pRecvDataObserver;
			m_pRecvDataObserver =NULL;
		}
		if (flag )
		{
			CLog::Write(Error, "Server close error!");
		}else
		{
			CLog::Write(Error, "Server close sucess!");
		}
		return flag;
	}

	void CFileCopyProtocolServer::Init()
	{
		m_errNoErrMsgMap.insert(std::make_pair(ReveiveDataFailed,ReveiveDataFailedStr) );
		m_errNoErrMsgMap.insert(std::make_pair(CreateFileErr,CreateFileErrStr) );
		m_errNoErrMsgMap.insert(std::make_pair(WriteFileFail,WriteFileFailStr) );
		m_errNoErrMsgMap.insert(std::make_pair(WriteFileIncompletion,WriteFileIncompletionStr) );
		//m_errNoErrMsgMap.insert(std::make_pair(FileNotExist,FileNotExistStr) );
		m_errNoErrMsgMap.insert(std::make_pair(DirectoryNotExist,DirectoryNotExistStr) );
		m_errNoErrMsgMap.insert(std::make_pair(TransferFileFinishErr,TransferFileFinishErrStr) );
		m_errNoErrMsgMap.insert(std::make_pair(TransferFileFinishNotOpen,TransferFileFinishNotOpenStr) );
		m_errNoErrMsgMap.insert(std::make_pair(RemoveDirectoryALLErr,RemoveDirectoryALLErrStr) );
		m_errNoErrMsgMap.insert(std::make_pair(RemoveDirectoryErr,RemoveDirectoryErrStr) );
		m_errNoErrMsgMap.insert(std::make_pair(CreateDirectoryNotRmEr,CreateDirectoryNotRmErStr) );
		m_errNoErrMsgMap.insert(std::make_pair(CreateDirectoryRmRmEr,CreateDirectoryRmRmErStr) );
		m_errNoErrMsgMap.insert(std::make_pair(CreateDirectoryRmCreateEr,CreateDirectoryRmCreateErStr) );
		m_errNoErrMsgMap.insert(std::make_pair(ChangeDirectoryOwnErr,ChangeDirectoryOwnErrStr) );
	}

	void CFileCopyProtocolServer::Process(const NetData& data )
	{
		ProtocolHeader proHeader;
		memcpy(&proHeader, data.buffer, HEADERLENGTH );
		if (data.sz!=proHeader.len )
		{
			RecToClient(proHeader.typeno+1000, ReveiveDataFailed );
			char recBuf[500];
			sprintf(recBuf, "The Reeived data is: %d, The Des sz is:%d ",data.sz, proHeader.len );
			CLog::Write(Error, std::string(recBuf) );
			return;
		}
		if (CREATEFILE==proHeader.typeno )
		{
			CreateFile(data );
		}else if (TRANSFERFILE==proHeader.typeno )
		{
			TransferFile(data );
		}else if (TRANSFERFILEFINISH==proHeader.typeno )
		{
			TransferFileFinish(data);
		}else if (REMOVEDIRECTORYALL==proHeader.typeno )
		{
			RemoveDirectoryAll(data);
		}else if (REMOVEDIRECTORY==proHeader.typeno )
		{
			RemoveDirectory(data );
		}else if (CREATEDIRECTORYNOTRM==proHeader.typeno )
		{
			CreateDirectoryNotRm(data );
		}else if (CREATEDIRECTORYRM==proHeader.typeno )
		{
			CreateDirectoryRm(data );
		}else if (CHANGEDIRECTORYOWN==proHeader.typeno )
		{
			ChangeDirectoryOwn(data );
		}
	}

	void CFileCopyProtocolServer::CreateFile(const NetData& data )
	{
		m_recDataSz =0; // 接收文件大小
		m_syncSz =0;//清除缓存阈值
		memcpy(&m_syncSz, data.buffer+HEADERLENGTH, 4);
		m_syncSz*=1048576;
		char path[500];
		memset(path, 0, 500 );
		int len =HEADERLENGTH+4;
		memcpy(path, data.buffer+len, data.sz-len );
		std::string filePath =path;
		filePath =filePath.substr(0, filePath.find_last_of("/")+1 );
		CFileOperationsTool* pInst =CFileOperationsTool::Instance();
		if (pInst )
		{
			if( !pInst->FileExist(filePath ) )
			{
				RecToClient(CREATEFILE_REC, DirectoryNotExist );
				CLog::Write(Error, filePath+","+DirectoryNotExistStr );
				return;
			}
		}
		if (pInst->FileExist(path ) )
		{
			pInst->RemoveFile(path );
		}
		m_fileHandle =open(path, O_WRONLY|O_CREAT , S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH );
		if (m_fileHandle>=0 )
		{
			m_curpath =path;
			RecToClient(CREATEFILE_REC,Success );
			CLog::Write(Normal, "Create the"+std::string(path)+"Sucess" );

		}else
		{
			RecToClient(CREATEFILE_REC, CreateFileErr );
			CLog::Write(Error, std::string(path)+","+CreateFileErrStr );
		}
		char strbuf[255];
		memset(strbuf,0 ,255 );
		sprintf(strbuf,"The write buffer is:%dMB", m_syncSz/1048576 );
		CLog::Write(Normal, filePath+","+strbuf );
	}

	void CFileCopyProtocolServer::TransferFile(const NetData& data )
	{
		int writeSize =data.sz-HEADERLENGTH;
		int retno =write(m_fileHandle, data.buffer+HEADERLENGTH, writeSize );
		if (retno==writeSize)
		{
			m_recDataSz+=writeSize;
			RecToClient(TRANSFERFILE_REC, Success );
		}else 
		{
			if (retno<0 )//写入失败
			{
				RecToClient(TRANSFERFILE_REC, WriteFileFail );
				CLog::Write(Error, m_curpath+","+WriteFileFailStr );
			}else//写入不完全
			{
				RecToClient(TRANSFERFILE_REC, WriteFileIncompletion );
				CLog::Write(Error, m_curpath+","+WriteFileIncompletionStr );
			}
		}
		if (m_recDataSz>=m_syncSz)
		{
			fsync(m_fileHandle );
			m_recDataSz =0;
		}
	}

	void CFileCopyProtocolServer::TransferFileFinish(const NetData& data )
	{
		char path[500];
		memset(path, 0, 500 );
		int len =HEADERLENGTH;
		memcpy(path, data.buffer+len, data.sz-len );
		std::string filePath =path;
		if (m_curpath==path )
		{
			int retNo =close(m_fileHandle );
			if (retNo )//返回关闭文件错误
			{
				RecToClient(TRANSFERFILEFINISH_REC, TransferFileFinishErr );
				CLog::Write(Error, filePath+","+TransferFileFinishErrStr );
			}else//关闭文件成功,写入正确
			{
				RecToClient(TRANSFERFILEFINISH_REC, Success );
				CLog::Write(Normal, "The "+filePath+" Write finished, Success" );
			}

		}else//要关闭的文件不是当前打开的文件
		{
			RecToClient(TRANSFERFILEFINISH_REC, TransferFileFinishNotOpen );
			CLog::Write(Error, filePath+","+TransferFileFinishNotOpenStr );
		}
	}

	void CFileCopyProtocolServer::RemoveDirectoryAll(const NetData& data )
	{
		char path[500];
		memset(path, 0, 500 );
		int len =HEADERLENGTH;
		memcpy(path, data.buffer+len, data.sz-len );
		std::string filePath =path;
		CFileOperationsTool* pInst =CFileOperationsTool::Instance();
		if (pInst )
		{
			bool flag =pInst->FileExist(filePath );
			if (!flag)
			{
				RecToClient(REMOVEDIRECTORYALL_REC, DirectoryNotExist );
				CLog::Write(Error, filePath+","+DirectoryNotExistStr );
				return;
			}
			flag =pInst->RemoveDirectoryPathIncFolder(filePath );
			if (flag )
			{
				RecToClient(REMOVEDIRECTORYALL_REC, Success );
				CLog::Write(Normal, "The "+filePath+" remove Sucess, include itself" );
			}else
			{
				RecToClient(REMOVEDIRECTORYALL_REC, RemoveDirectoryALLErr );
				CLog::Write(Error, filePath+","+RemoveDirectoryALLErrStr );
			}
		}
	}

	void CFileCopyProtocolServer::RemoveDirectory(const NetData& data )
	{
		char path[500];
		memset(path, 0, 500 );
		int len =HEADERLENGTH;
		memcpy(path, data.buffer+len, data.sz-len );
		std::string filePath =path;
		CFileOperationsTool* pInst =CFileOperationsTool::Instance();
		if (pInst )
		{
			bool flag =pInst->FileExist(filePath );
			if (!flag)
			{
				RecToClient(REMOVEDIRECTORY_REC, DirectoryNotExist );
				CLog::Write(Error, filePath+","+DirectoryNotExistStr );
				return;
			}
			flag =pInst->RemoveDirectoryPath(filePath );
			if (flag )
			{
				RecToClient(REMOVEDIRECTORY_REC, Success );
				CLog::Write(Normal, "The "+filePath+" remove Sucess, not include itself" );
			}else
			{
				RecToClient(REMOVEDIRECTORY_REC, RemoveDirectoryErr );
				CLog::Write(Error, filePath+","+RemoveDirectoryErrStr );
			}
		}
	}

	void CFileCopyProtocolServer::CreateDirectoryNotRm(const NetData& data )
	{
		char path[500];
		memset(path, 0, 500 );
		int len =HEADERLENGTH;
		memcpy(path, data.buffer+len, data.sz-len );
		std::string filePath =path;
		CFileOperationsTool* pInst =CFileOperationsTool::Instance();
		if (pInst )
		{
			bool flag =pInst->CreateDirectoryPath(filePath );
			if (flag )//创建成功
			{
				RecToClient(CREATEDIRECTORYNOTRM_REC, Success );
				CLog::Write(Normal, "The "+filePath+" create sucess " );
			}else //创建失败
			{
				RecToClient(CREATEDIRECTORYNOTRM_REC, CreateDirectoryNotRmEr );
				CLog::Write(Error, filePath+","+CreateDirectoryNotRmErStr );
			}
		}
	}

	void CFileCopyProtocolServer::CreateDirectoryRm(const NetData& data )
	{
		char path[500];
		memset(path, 0, 500 );
		int len =HEADERLENGTH;
		memcpy(path, data.buffer+len, data.sz-len );
		std::string filePath =path;
		CFileOperationsTool* pInst =CFileOperationsTool::Instance();
		if (pInst )
		{
			bool flag =pInst->FileExist(filePath );
			if (flag )
			{
				flag =pInst->RemoveDirectoryPathIncFolder(filePath );
				if ( !flag)//删除已经存在的文件失败
				{
					RecToClient(CREATEDIRECTORYRM_REC, CreateDirectoryRmRmEr );
					CLog::Write(Error, filePath+","+CreateDirectoryRmRmErStr );
					return;
				}
			}
			flag =pInst->CreateDirectoryPath(filePath );
			if (flag )//创建成功
			{
				RecToClient(CREATEDIRECTORYRM_REC, Success );
				CLog::Write(Normal, "The "+filePath+" create sucess " );
			}else //创建失败
			{
				RecToClient(CREATEDIRECTORYRM_REC, CreateDirectoryRmCreateEr );
				CLog::Write(Error, filePath+","+CreateDirectoryRmCreateErStr );
			}
		}
	}

	void CFileCopyProtocolServer::ChangeDirectoryOwn(const NetData& data )
	{
		char path[500];
		memset(path, 0, 500 );
		int len =HEADERLENGTH;
		memcpy(path, data.buffer+len, data.sz-len );
		std::string filePath =path;
		CFileOperationsTool* pInst =CFileOperationsTool::Instance();
		if (pInst )
		{
			bool flag =pInst->FileExist(filePath );
			if (!flag )//目录不存在
			{
				RecToClient(CHANGEDIRECTORYOWN_REC, DirectoryNotExist );
				CLog::Write(Error, filePath+","+DirectoryNotExistStr );
				return;
			}
			flag =pInst->ChangeFilePermissions(filePath );
			if (flag )//修改成功
			{
				RecToClient(CHANGEDIRECTORYOWN_REC, Success );
				CLog::Write(Normal,"Change "+filePath+" Sucess" );
			}else //创建失败
			{
				RecToClient(CHANGEDIRECTORYOWN_REC, ChangeDirectoryOwnErr );
				CLog::Write(Error, filePath+","+ChangeDirectoryOwnErrStr );
			}
		}
	}

	void CFileCopyProtocolServer::RecToClient(const unsigned int& typeNo, const Errorno& erno )
	{
		NetData data;
		ProtocolHeader proHeader;
		proHeader.typeno =typeNo;
		proHeader.len =HEADERLENGTH;
		memcpy(data.buffer+proHeader.len,&erno,4);
		proHeader.len +=4;
		if (Success!=erno )
		{
			std::map<int, std::string>::iterator iter =m_errNoErrMsgMap.find(erno);
			if (iter!=m_errNoErrMsgMap.end() )
			{
				int sz =iter->second.length();
				memcpy(data.buffer+proHeader.len,iter->second.c_str(), sz );
				proHeader.len +=sz;
			}
		}
		memcpy(data.buffer, &proHeader,HEADERLENGTH );
		data.sz =proHeader.len;
		m_pTcpServer->WriteData(data );
	}
}