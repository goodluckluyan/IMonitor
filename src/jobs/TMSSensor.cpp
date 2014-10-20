/***********************************************************
Copyright (C), 2010-2020, DADI MEDIA Co.,Ltd.
ModuleName: SoftwareState.h
FileName: SoftwareState.h
Author: chengyu
Date: 14/09/19
Version:
Description: 获取SMS,TMS 状态信息
Others:
History:
		<Author>		<Date>		<Modification>
		chengyu			14/09/19
***********************************************************/
#include "TMSSensor.h"

#include <iostream>
#include <string>
#include <cstdlib>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/wait.h>

using namespace std;
CTMSSensor :: CTMSSensor(CDataManager * ptr):
m_ptrDM(ptr)
{};

CTMSSensor :: ~CTMSSensor()
{
}



//获取TMS 运行状态
int CTMSSensor :: GetTMSWorkState()
{	
	int state = 0;
	char buf[20] = {0};
	char command[] = "pidof  -s  Tms20_DeviceService";

	pid_t pid = 0; //-1;
	
	FILE *fp = popen(command,"r");
	if(fp==NULL)
	{
		printf("failed to popen %s:%s\n",command,strerror(errno));
		state = -1;
		return 0;//////////////return -1;
	}
	
	if(fgets(buf,19,fp)==NULL)
	{
		state = -1;
	}
	printf("[ Tms20_DeviceService ] Pid = %s\n",buf);
	pclose(fp);

	int result(0);
	result = sscanf( buf,"%d\n", &pid );
	if(result == 0)//sscanf失败，则返回0
	{
		string error = "Error:pidof -s  Tms20_DeviceService\n";
		printf( "%s" , error.c_str() );
		state = -1;
	}
	if ( pid == -1 )
	{
		fprintf( stderr, "[ %s ] is: <%u>\n", "Tms20_DeviceService", ( unsigned int )pid );  
		printf( "[ %s ] is: <%u>\n", "Tms20_DeviceService", ( unsigned int )pid  );
	}


	m_ptrDM->UpdateTMSStat(state);
	
	return 0;/////////return 0;
}

