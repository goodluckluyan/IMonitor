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

#ifndef _H_SOFTWARESTATE_
#define _H_SOFTWARESTATE_

#include <iostream>
#include <string>
#include "DataManager.h"

class CTMSSensor
{
public:
	
	CTMSSensor(CDataManager * ptr);
	
	~CTMSSensor();

	
	//获取TMS 工作状态
	int GetTMSWorkState();
private:
	CDataManager * m_ptrDM;

};


#endif //