#ifndef DISPATCH_INC
#define DISPATCH_INC

#include "DataManager.h"

// ����������
class CDispatch
{
public:
	CDispatch();
	~CDispatch();

	bool Init();
	bool AddTask();
	bool routine();
private:
	CDataManager * m_ptrDM;
};

#endif