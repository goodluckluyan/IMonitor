#include "C_Log.h"
#include <time.h>
#include <stdio.h>
#ifdef WINBUILD
#include <io.h>
#else
#include <unistd.h>
#endif
std::string CLog::m_path;
CLog::CLog()
{

}

CLog::~CLog()
{

}

void CLog::SetFilePath(const std::string& path )
{
	m_path =path;
}

void CLog::Write(const Level& lev, const std::string& text )
{
	time_t curTime =time(NULL );
	struct tm* curtime =gmtime(&curTime );
	char bufstr[255];
	sprintf(bufstr,"Log_%d-%d-%d",
		curtime->tm_year,curtime->tm_mon+1,curtime->tm_mday);
	std::string nameStr =bufstr;
	std::string path =m_path+nameStr;
	FILE* pf =NULL;
#ifdef WINBUILD
	if(access(path.c_str(),0 ) )
#else
	if(access(path.c_str(),F_OK ) )
#endif
	{
		pf =fopen(path.c_str(),"wt");
		fclose(pf );
	}
	pf =fopen(path.c_str(),"at" );
	if (pf )
	{
		std::string levStr;
		if (lev==Waring)
		{
			levStr="Waring";
		}else if (lev==Normal )
		{
			levStr ="Normal";
		}else if (lev==Error )
		{
			levStr ="Error";
		}
		fprintf(pf,"%s-%d:%d:%d\t%s\n",levStr.c_str(), 
			curtime->tm_hour+8, curtime->tm_min, curtime->tm_sec, text.c_str() );
		fclose( pf );
	}
}