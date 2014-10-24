#include "Dispatch.h"

CDispatch::CDispatch()
:m_ptrDM(NULL)
{

}

CDispatch::~CDispatch()
{

}

bool CDispatch::Init()
{
	m_ptrDM = CDataManager::GetInstance();
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