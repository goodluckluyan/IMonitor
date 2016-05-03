#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <cstdlib>
#include "C_WriteDataThread.h"
#include "C_Log.h"
#include "ConstDef.h"

static const size_t QueueLen =1000;
pthread_mutex_t mutex ;
void* writeThread(void *ptr )
{
	CWriteDataThread *p =static_cast<CWriteDataThread*>(ptr );
	p->WriteToFile();
	return NULL;
}

CWriteDataThread::CWriteDataThread():m_pCircularQueue(NULL )
{
	m_pCircularQueue =new CircularQueue(QueueLen );
	m_syncSz =0;
	m_recDataSz =0;
	m_bFileClosed =false;
	m_bWriteWrong =false;
	pthread_mutex_init(&mutex, NULL ); 
}

CWriteDataThread::~CWriteDataThread()
{
	if (!m_bColseFile )
	{
		pthread_cancel(m_threadId );
		usleep(10000);
	}
	if (m_pCircularQueue )
	{
		delete m_pCircularQueue;
	}
	pthread_mutex_destroy(&mutex ); 
}

void CWriteDataThread::SetFilePath(const std::string& path )
{
	m_filePath =path;
}

void CWriteDataThread::SetSyncSz(const int& sz )
{
	m_syncSz =sz;
}

bool CWriteDataThread::SetWriteData(const element_Info& elemInfo ,const int& syncFlag )
{
	m_syncFlag =syncFlag;
	pthread_mutex_lock(&mutex);
	bool flag =m_pCircularQueue->en_queue(elemInfo );
	pthread_mutex_unlock(&mutex);
	return flag;
}

void CWriteDataThread::SetCloseFlag(const bool& flag )
{
	m_bColseFile =flag;
}

void CWriteDataThread::StartThread()
{
	int threadFlag =pthread_create(&m_threadId, NULL,writeThread,this );
	pthread_detach(m_threadId );
}

bool CWriteDataThread::IsFileClosed( )
{
	return m_bFileClosed;
}

void CWriteDataThread::WriteToFile()
{
	m_recDataSz =0;
	m_bColseFile =false;
	m_bFileClosed =false;
	element_Info element;
	element.data =(char*)malloc(QUEUEDATABSIZE );
	int fileHandle =open(m_filePath.c_str(), O_WRONLY|O_CREAT , S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH );
	if ( fileHandle == -1 )
	{
		free( element.data );
		m_bWriteWrong = true;
		CLog::Write(Error, "Error:open() Failed "+ m_filePath + ","+ FileCopyProtocol::CreateFileErrStr );
		return;
	}

	while(true )
	{
		if (m_syncFlag )
		{
			if (m_recDataSz>=m_syncSz )
			{
				fsync(fileHandle );
				m_recDataSz =0;
			}
		}
		if (m_pCircularQueue->IsEmpty()&&m_bColseFile )
		{
			close( fileHandle );
			m_bFileClosed =true;
			free(element.data );
			return;
		}
		if (m_pCircularQueue->IsEmpty() )
		{
			usleep(10000);
		}
		pthread_mutex_lock(&mutex);
		bool flag =m_pCircularQueue->out_queue(&element);
		pthread_mutex_unlock(&mutex);
		if (flag )
		{
			int sz =write(fileHandle,element.data,element.datalen );
			if (sz!=element.datalen )
			{
				m_bWriteWrong =true;
				//add zhangmiao
				close(fileHandle);
				m_bFileClosed =true;
				free(element.data );
				return;
			}
			m_recDataSz+=element.datalen;
		}
	}
}
