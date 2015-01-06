#ifndef _CS_MUTEX
#define _CS_MUTEX

//@file:C_CS.h
//@brief: °üº¬C_CS£º·â×°»¥³â¡£
//@author: wangzhongping@oristartech.com
//date: 2012-05-23
#include <pthread.h>
#include <error.h>
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

	bool TryLock(void)
	{
		int nRet = pthread_mutex_trylock(&m_CS);
		if(nRet == 0)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
};

class C_GuardCS
{

public:
	C_CS *m_pCS;

	C_GuardCS(C_CS *pCS)
	{
		if(pCS)
		{
			m_pCS = pCS;
			m_pCS->EnterCS();
		}
	}
	~C_GuardCS(void)
	{
		if(m_pCS)
		{
			m_pCS->LeaveCS();
		}
	}
};
#endif //_CS_MUTEX
