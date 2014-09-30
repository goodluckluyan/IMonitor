#include "Dispatch.h"

CDispatch::CDispatch()
:m_ptrDM(NULL)
{

}
CDispatch::CDispatch(CDataManager *ptr)
:m_ptrDM(ptr)
{

}

CDispatch::~CDispatch()
{

}

bool CDispatch::Init()
{
	return true;
}

bool CDispatch::AddTask()
{
	return true;
}

bool CDispatch::routine()
{
	return true;
}