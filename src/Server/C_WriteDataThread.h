/****************************************************************************
*@file: C_WriteDataThread.h
*@brief: 写入文件线程类
*		 
*@author: zhangchao@oristartech.com
*@dade: 2015-11-11
****************************************************************************/
#ifndef CWRITEDATATHREAD_H
#define CWRITEDATATHREAD_H

#include <pthread.h>
#include <string>
#include "CircularQueue.h"
class CWriteDataThread
{
public:
    CWriteDataThread();
    ~CWriteDataThread();

public:
	/**
	*brief：设置写入文件路径
	*/
	void SetFilePath(const std::string& path );

	/**
	*brief：设置刷新磁盘阈值
	*/
	void SetSyncSz(const int& sz );

	/**
	*brief：设置写入数据
	*/
	bool SetWriteData(const element_Info& elemInfo ,const int& syncFlag =0 );

	/**
	*brief：设置关闭文件标识
	*/
	void SetCloseFlag(const bool& flag );

	/**
	*brief：开启线程
	*/
	void StartThread();

	/**
	*brief：文件是否处于关闭状态
	*/
	bool IsFileClosed( );

	/**
	*brief：写入文件线程函数
	*/
	void WriteToFile();

public:
	bool m_bWriteWrong;
private:
	unsigned char m_syncFlag;//刷新磁盘标识
	std::string m_filePath;//文件路径
	bool m_bColseFile;//关闭文件标识
	pthread_t  m_threadId;//线程Id
	CircularQueue* m_pCircularQueue;//循环队列缓存
	int m_syncSz;//清除缓存阈值
	int m_recDataSz; // 接收文件大小
	bool m_bFileClosed; //文件是否已经关闭标识
};

#endif // CWRITEDATATHREAD_H
