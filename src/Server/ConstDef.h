/****************************************************************************
*@file: ConstDef.h
*@brief: 对协议所需的数据结构和常量进行了定义
*		 
*@author: zhangchao@oristartech.com
*@dade: 2015-11-09
****************************************************************************/
#ifndef CONSTDEF_H
#define CONSTDEF_H
#include <string>

namespace FileCopyProtocol
{
	//协议头定义
	typedef struct  ProtocolHeader{
		int typeno;//协议编号
		int len;//协议长度
	}ProtocolHeader;
	const int HEADERLENGTH =8;

	//协议编号
	const unsigned int CREATEFILE =1001;//在对等机器上创建文件
	const unsigned int CREATEFILE_REC =2001;//Server返回创建文件结果
	const unsigned int TRANSFERFILE =1002;//传输文件
	const unsigned int TRANSFERFILE_REC =2002;//Server返回传输结果
	const unsigned int TRANSFERFILEFINISH =1003;//文件传输完成关闭文件
	const unsigned int TRANSFERFILEFINISH_REC =2003;//Server返回操作结果
	const unsigned int REMOVEDIRECTORYALL =1004;//文件传输错误时删除整个路径
	const unsigned int REMOVEDIRECTORYALL_REC =2004;//Server返回删除整个路径结果
	const unsigned int REMOVEDIRECTORY =1005;//删除整个目录下的所有文件
	const unsigned int REMOVEDIRECTORY_REC =2005;//Server返回删除整个目录下的所有文件操作
	const unsigned int CREATEDIRECTORYNOTRM =1006;//建立目录，如果目标目录存在则不再创建
	const unsigned int CREATEDIRECTORYNOTRM_REC =2006;//Server返回建立目录操作
	const unsigned int CREATEDIRECTORYRM =1007;//建立目录,存在先删除，再创建
	const unsigned int CREATEDIRECTORYRM_REC =2007;//Server返回建立目录操作
	const unsigned int CHANGEDIRECTORYOWN =1009;//修改目录权限
	const unsigned int CHANGEDIRECTORYOWN_REC =2009;//Server返回建立目录操作

	//错误码定义
	enum Errorno
	{
		Success =0,	//操作成功返回值
		ReveiveDataFailed=7000001, //网络数据接收失败
		CreateFileErr,	//创建文件失败
		WriteFileFail,//文件写入失败
		WriteFileIncompletion,//文件写入不完全
		//FileNotExist,//文件不存在
		DirectoryNotExist,//路径不存在
		TransferFileFinishErr,//关闭文件失败
		TransferFileFinishNotOpen,//所要关闭的文件未打开
		RemoveDirectoryALLErr,//删除整个路径失败
		RemoveDirectoryErr,//删除整个目录下的所有文件失败
		CreateDirectoryNotRmEr,//建立目录,存在不删除，失败
		CreateDirectoryRmRmEr,//建立目录，删除已经存在的文件失败
		CreateDirectoryRmCreateEr,//建立目录，创建文件失败
		ChangeDirectoryOwnErr,//修改目录权限失败
		
	};

	//错误信息定义
	const std::string ReveiveDataFailedStr ="Reveived the net data failed";//接收网络数据失败
	const std::string CreateFileErrStr ="Create the file error";//创建文件失败
	const std::string WriteFileFailStr ="Write data error";//文件数据写入失败
	const std::string WriteFileIncompletionStr ="Write data incompleted";//文件写入不完全
	//const std::string FileNotExistStr ="The file is not Exist";//文件不存在
	const std::string DirectoryNotExistStr ="The directory is not Exist";//路径不存在
	const std::string TransferFileFinishErrStr ="Close file failed";//关闭文件失败
	const std::string TransferFileFinishNotOpenStr ="The file being closed is not open";//所要关闭的文件未打开
	const std::string RemoveDirectoryALLErrStr ="Remove the directory which include the Folder Failed";//删除整个路径失败
	const std::string RemoveDirectoryErrStr ="Remove the directory which doesn't include the Folder Failed";//删除整个目录下的所有文件失败
	const std::string CreateDirectoryNotRmErStr ="Create the directory which exist not remove Failed";//建立目录,存在不删除，失败
	const std::string CreateDirectoryRmRmErStr ="Delete the Exist directory Failed";//建立目录，删除已经存在的文件失败
	const std::string CreateDirectoryRmCreateErStr ="Create the directory Failed";//建立目录，创建文件失败
	const std::string ChangeDirectoryOwnErrStr ="Change the directory Permissions Failed"; //修改目录权限失败
}
#endif//CONSTDEF_H
