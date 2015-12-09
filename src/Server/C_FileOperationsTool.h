/****************************************************************************
*@file: C_FileOperationsTool.h
*@brief: 常用文件操作工具类，对常用的文件操作进行了封装
*		 
*@author: zhangchao@oristartech.com
*@dade: 2015-11-10
****************************************************************************/
#ifndef FILEOPERATIONSTOOL_H
#define FILEOPERATIONSTOOL_H

#include <string>

class CFileOperationsTool
{
private:
	CFileOperationsTool();
	CFileOperationsTool(const CFileOperationsTool& obj);
	CFileOperationsTool& operator=(const CFileOperationsTool& obj );
public:
	static CFileOperationsTool* Instance();
public:
	/**
	*brief：判断路径是否存在
	*/
	bool FileExist(const std::string& path );

	/**
	*brief：删除文件路径,包括目标目录
	*/
	bool RemoveDirectoryPathIncFolder(const std::string& path );

	/**
	*brief：删除路径
	*/
	bool RemoveDirectoryPath(const std::string& path );

	/**
	*brief：删除文件
	*/
	bool RemoveFile(const std::string& path );

	/**
	*brief：创建路径
	*/
	bool CreateDirectoryPath(const std::string& path );

	/**
	*brief：修改文件权限
	*/
	bool ChangeFilePermissions(const std::string& path );
private:
	bool CheckOwn(const std::string& path );
	
};

#endif // FILEOPERATIONSTOOL_H
