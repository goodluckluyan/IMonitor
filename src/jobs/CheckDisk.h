/***********************************************************
Copyright (C), 2010-2020, DADI MEDIA Co.,Ltd.
ModuleName: CheckDisk.h
FileName: CheckDisk.h
Author: chengyu
Date: 14/06/26
Version:
Description: 获取磁盘信息
Others:
History:
		<Author>		<Date>		<Modification>
		chengyu			14/09/12
***********************************************************/


#ifndef _H_CHECKDISK_
#define _H_CHECKDISK_


#include <iostream>
#include <string.h>
#include <vector>
#include <cstdlib>
#include "DataManager.h"

#define COMM_FLAG		35		// '#'
#define	KEYVALUE_FLAG	58		// ':'
#define BLANK_FLAG1		32		// ' '
#define BLANK_FLAG2		9		// '	'
#define BLANK_FLAG3		13		// 换行


class CheckDisk
{
public:
	//cus
	CheckDisk();
	//dis
	~CheckDisk();

	// 初始化并检测
	bool InitAndCheck();

	//获取所有磁盘总信息
	int ReadMegaSASInfo();

	//获取总磁盘容量
	std::string GetDiskSize(){return DiskSize;};

	//获取总磁盘状态
	std::string GetDiskState(){return DiskState;};

	//获取总磁盘数量
	std::string GetDiskNumberOfDrives(){ return DiskNumOfDrives;};

	//打印磁盘状态信息
	void PrintState();

private:
	
	int GetDiskInfo();

	//生成包含磁盘信息的MegaSAS.log文件
	int GetDickInfoLog(std::string ppath);

	int GetRaidInfoFromPip();

	//删除存储磁盘信息的MegaSAS.log文件
	int RemoveDir( const std::string &dir);

	//是否为空白行
	int isblank( const char *pbuf);

	//获取每行名称和值
	int getkey( const char *pbuf, std::string &temp_key, std::string &temp_value);

	std::string DiskSize;//总磁盘容量
	std::string DiskState;//总磁盘状态
	std::string DiskNumOfDrives;//总磁盘数量

	std::map<int,DiskInfo> mapDiskInfo;//磁盘总信息

	CDataManager * m_ptrDM;

	char m_buf[20480];// Raid日志缓存
};

#endif //_H_CHECKDISK


