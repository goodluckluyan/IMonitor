#ifndef _CS_MUTEX
#define _CS_MUTEX

//@file:C_CS.h
//@brief: °üº¬C_CS£º·â×°»¥³â¡£
//@author: wangzhongping@oristartech.com
//date: 2012-05-23
#include <pthread.h>

class C_CS
{
	
public:
    pthread_mutex_t m_CS;
    
	C_CS(void)
	{
		pthread_mutex_init(&m_CS,NULL);
	}
	~C_CS(void)
	{
		pthread_mutex_destroy(&m_CS);
	}
	void EnterCS(void)
	{
		pthread_mutex_lock(&m_CS);
	}
	void LeaveCS(void)
	{
		pthread_mutex_unlock(&m_CS);
	}
};
#endif //_CS_MUTEX
