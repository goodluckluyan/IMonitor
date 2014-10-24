/***********************************************************
Copyright (C), 2010-2020, DADI MEDIA Co.,Ltd.
ModuleName: CheckDisk.h
FileName: CheckDisk.h
Author: chengyu
Date: 14/06/26
Version:
Description: ��ȡ������Ϣ
Others:
History:
		<Author>		<Date>		<Modification>
		chengyu			14/09/12
***********************************************************/

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "CheckDisk.h"

using namespace std;

const int CHECKDISK_ERROR_NO_INFOLOG = -1;
const int CHECKDISK_ERROR_RM_INFOLOG = -2;
const std::string INFOLOG = "MegaSAS.log";
const std::string DIR = "rm -rf ";
const int MAX_BUFF_LEN = 128;



CheckDisk :: CheckDisk()
{
	DiskSize = "";
	DiskState = "";
	DiskNumOfDrives = "";
	m_ptrDM = NULL;
}

CheckDisk ::~CheckDisk()
{
}

bool CheckDisk::InitAndCheck()
{
	m_ptrDM=CDataManager::GetInstance();
	if(ReadMegaSASInfo() != 0)
	{
		return false;
	}
	
	if(diskInfo.diskState != "Optimal")
	{
		return false;
	}
	else
	{
		return true;
	}
}



//��ȡMegaSAS.log �ļ���Ϣ
int CheckDisk::ReadMegaSASInfo()
{
	int iResult = 0;
	char a[64];
	memset(a, 0, 64);
	
	char tmp[256];
	char buf[256];
	memset(tmp, 0, 256);
	memset(buf, 0, 256);
	
	sprintf(tmp, "/proc/%d/exe", getpid());
	readlink(tmp,buf,256);
	string str = buf;
	size_t iPos = -1;
	if ((iPos = str.rfind('/')) == string::npos)
	{
		return CHECKDISK_ERROR_NO_INFOLOG;//-1
	}
	string strInipath = str.substr(0, iPos);
	strInipath += "/MegaSAS.log";

	//ɾ��֮ǰ��MegaSAS.log��־
	iResult = RemoveDir( strInipath);
//	if (iResult != 0)
//	{
//		return -1;
//	}

	//��ȡ�µ�MegaSAS.log��Ϣ
	iResult = GetDickInfoLog(strInipath);
	if (iResult != 0)
	{
		return -1;
	}
	
	//��ȡ��Ϣ����ṹ��
	DiskInfo diskinfo_temp;
	iResult = GetDiskInfo( strInipath.c_str(), diskinfo_temp);
	if (iResult != 0)
	{
		return iResult;
	}
	else if(diskinfo_temp.diskDrives.size() != atoi(diskinfo_temp.diskNumOfDrives.c_str()))
	{
		return -1;
	}

	diskInfo = diskinfo_temp;
	DiskSize = diskinfo_temp.diskSize;
	DiskState = diskinfo_temp.diskState;
	DiskNumOfDrives = diskinfo_temp.diskNumOfDrives;
	
	if(m_ptrDM != NULL)
	{	
		m_ptrDM->UpdateDevStat(diskInfo);
	}
	return 0;
}

int CheckDisk :: GetDiskInfo( const char* ppath, DiskInfo &diskInfo)//MegaSAS.log
{
	int iResult;
	string strline;
	ifstream ifs;
	char cbuf[MAX_BUFF_LEN];
	
	if (NULL == ppath)
	{
		perror("path is NULL\n");
		return -1;
	}
	
	ifs.open( ppath, ios::in);
	
	if (NULL == ifs)
	{
		perror("file is not exist\n");
		return -1;
	}
	
	DiskDriveInfo disDriveInfo;
	while (!ifs.eof())
	{
		getline(ifs,strline);
		
		string cfg_key;
		string cfg_value;
		iResult = getkey(strline.c_str(), cfg_key, cfg_value);
		if (iResult)
		{
			if ( !strcmp( cfg_key.c_str(), "Size"))
			{
				diskInfo.diskSize = cfg_value;
			}
			else if( !strcmp( cfg_key.c_str(), "State"))
			{
				diskInfo.diskState = cfg_value;
			}
			else if( !strcmp( cfg_key.c_str(), "Number Of Drives"))
			{
				diskInfo.diskNumOfDrives = cfg_value;
			}
			else if( !strcmp( cfg_key.c_str(), "Enclosure Device ID"))
			{
				disDriveInfo.driveID = cfg_value;
			}
			else if( !strcmp( cfg_key.c_str(), "Slot Number"))
			{
				disDriveInfo.driveSlotNum = cfg_value;
			}
			else if( !strcmp( cfg_key.c_str(), "Media Error Count"))
			{
				disDriveInfo.driveErrorCount = cfg_value;
			}
			else if( !strcmp( cfg_key.c_str(), "Raw Size"))
			{
				disDriveInfo.driveSize = cfg_value;
			}
			else if( !strcmp( cfg_key.c_str(), "Firmware state"))
			{
				disDriveInfo.driveFirmwareState = cfg_value;
			}
			else if ( !strcmp( cfg_key.c_str(), "Port's Linkspeed"))
			{
				disDriveInfo.driveSpeed = cfg_value;
				bool bFind = false;
			        int nLen = diskInfo.diskDrives.size();
				for(int i = 0 ;i < nLen ;i++)
				{
				  if(!strcmp(diskInfo.diskDrives[i].driveSlotNum.c_str(),disDriveInfo.driveSlotNum.c_str()) )
				  {
				    bFind = true;
			            break;	
				   }
				}
				if(!bFind)
				   diskInfo.diskDrives.push_back( disDriveInfo);
			}
		}
	}

	ifs.close();
	return 0;   
}


int CheckDisk::GetDickInfoLog(std::string ppath)
{
	int iResult;
	char cmd[256]={'\0'};
	snprintf(cmd,256,"sudo /usr/local/MegaRAID\\ Storage\\ Manager/StorCLI/storcli64 -LDInfo -LALL -aAll > %s",ppath.c_str());
	iResult = system( cmd);
	sleep(3);
//	if (iResult != 0)
//	{
//		return CHECKDISK_ERROR_NO_INFOLOG;
//	}

	snprintf(cmd,256, "sudo /usr/local/MegaRAID\\ Storage\\ Manager/StorCLI/storcli64 -PDList -aAll >> %s",ppath.c_str());
	iResult = system(cmd);
//	if (iResult != 0)
//	{
//		return CHECKDISK_ERROR_NO_INFOLOG;
//	}

	return 0;
}

int CheckDisk::RemoveDir( const string &logpath)
{
	int iResult;

//	if( ( iResult = access( logpath.c_str() , F_OK ) ) == 0 )
//	{
//	iResult = system( (DIR+logpath).c_str());
	iResult = unlink(logpath.c_str());
	if (iResult != 0)
	{
		return CHECKDISK_ERROR_RM_INFOLOG;
	}

	return iResult;
}

//��ȡÿһ������ð��Ϊ�ָ�� ���ƺ�ֵ
int CheckDisk::getkey( const char *pbuf, string &temp_key, string &temp_value)
{
	int iret = 0;
	int ilenbuf = 0;
	int ipos_eq = -1;
	char ctmp[MAX_BUFF_LEN];///////

	if( NULL == pbuf )
	{
		cout<<"string point is NULL"<<endl;
		return -1;
	}

	ilenbuf = strlen(pbuf);

	/// ���ҹؼ�ֵ��־ ':',�Ե�һ���Ⱥų���λ��Ϊ׼,���ַ�����Ϊ������
	/// �Ⱥ�֮ǰΪ �ؼ�ֵ �����֣��ؼ�ֵ������֮ǰ����Ϊ�ջ�ո��ַ���
	/// �Ⱥ�֮��Ϊ �ؼ�ֵ ��ȡֵ, �ؼ�ֵ��ȡֵ֮����ַ�������Ϊ�ո��ע��

	for( int i = 0; i < ilenbuf; i++ )
	{
		if( KEYVALUE_FLAG == pbuf[i])
		{
			ipos_eq = i;
		}
	}

	if( -1 == ipos_eq ||
		0 == ipos_eq)
	{
		return 0;
	}

	/// �жϵȺ�֮ǰ���ַ����Ƿ����Ҫ��
	int ipos_start = -1; /// key�ַ���������ʼλ��
	int ipos_end = -1;	 /// key�ַ������ֽ���λ��
	int ipos_blank = -1; /// key�ַ��������г��ֿո�ı�־


	for( int i = 0; i < ipos_eq; i++)
	{
		//cout<<pbuf[i]<<endl;

		if( (pbuf[i] > 96 && pbuf[i] < 123 ) ||  // Сд��ĸ
			(pbuf[i] > 64 && pbuf[i] < 91 ) ||	 // ��д��ĸ
			(pbuf[i] > 47 && pbuf[i] < 58 ) ||	// ����	
				'.' == pbuf[i] ||				//��
				',' == pbuf[i])					//����
		{
			if( ipos_start == -1 )
			{
				ipos_start = i;
			}

			if( ipos_start != -1 )
			{
				ipos_end = i;
			}
		}

		if( (BLANK_FLAG1 == pbuf[i] && ( BLANK_FLAG1 == pbuf[i+1] || KEYVALUE_FLAG == pbuf[i+1]))||
			BLANK_FLAG2 == pbuf[i] ||
			BLANK_FLAG3 == pbuf[i])
		{
			if( -1 != ipos_start && 
				-1 == ipos_blank)
			{
				ipos_blank = i;
			}
		}
	}

	if( -1 == ipos_start )
	{
		return 0;
	}

//	if( -1 != ipos_blank && ipos_end > ipos_blank )
//	{
//		return 0;
//	}

	/// �ж�:��֮����ַ����Ƿ����Ҫ��

	/// �Ƿ�Ϊ�հ���
	iret = isblank(&pbuf[ipos_eq+1]);
	if( iret )
	{
		return 0;
	}

	/// �Ƿ�Ϊע��
//	iret = iscomment(&pbuf[ipos_eq+1]);
//	if( iret )
//	{
//		return 0;
//	}

	int ipos_start2 = -1; /// key�ַ���ȡֵ��ʼλ��
	int ipos_end2 = -1;	 /// key�ַ���ȡֵ����λ��
	int ipos_eof = -1;	///  key�ַ���ȡֵ������־

	for( int i = ipos_eq + 2; i < ilenbuf; i++)
	{
		//cout<<pbuf[i]<<endl;

		if( (pbuf[i] > 96 && pbuf[i] < 123 ) ||  // Сд��ĸ
			(pbuf[i] > 64 && pbuf[i] < 91 ) ||	 // ��д��ĸ
			(pbuf[i] > 47 && pbuf[i] < 58 ))// ||	// ����	
		//		',' == pbuf[i])	  				// ����
		{
			if( ipos_start2 == -1 )
			{
				ipos_start2 = i;
			}

			if( ipos_start2 != -1 )
			{
				ipos_end2 = i;
			}
		}

		if( BLANK_FLAG1 == pbuf[i] || 
			COMM_FLAG == pbuf[i] ||
			BLANK_FLAG2 == pbuf[i] ||
			BLANK_FLAG3 == pbuf[i])
		{
			if( -1 != ipos_start2 )
			{
				ipos_eof = i;
				break;
			}
		}
	}


/*	int icommentflag = 0;
	if( -1 != ipos_eof )
	{
		/// �����������Ƿ��ǿո��ע��
		if(!iscomment(&pbuf[ipos_eof]))
		{
			if(!isblank(&pbuf[ipos_eof]))
			{
				return 0;
			}
		}
		else
		{
			icommentflag = 1;
		}
	}
*/

	/// ���ؼ�ֵ��Ϣ
	/// ð��֮ǰΪ�ؼ�ֵ����
	memset(ctmp,0,MAX_BUFF_LEN);
	if (ipos_blank != -1)
	{
		memcpy(ctmp,&pbuf[0],ipos_blank);
	}
	else
		memcpy(ctmp,&pbuf[0],ipos_eq);

	temp_key = ctmp;

	/// ð��֮��Ϊ�ؼ�ֵȡֵ
	memset(ctmp,0,MAX_BUFF_LEN);
	memcpy(ctmp,&pbuf[ipos_eq + 2], ipos_end2 - ipos_eq-1);

	temp_value = ctmp;

	return 1;
}

int CheckDisk::isblank( const char *pbuf )
{
	int iret = 0;
	int ilenbuf = 0;
	int inum_blank = 0;

	if( NULL == pbuf )
	{
		cout<<"string point is NULL"<<endl;
		return -1;
	}

	ilenbuf = strlen(pbuf);
	if( 0 == ilenbuf )
	{
		return 1;
	}

	for(int i = 0; i < ilenbuf; i++ )
	{
		if( BLANK_FLAG1 == pbuf[i] || 
			BLANK_FLAG2 == pbuf[i] ||
			BLANK_FLAG3 == pbuf[i])
		{
			inum_blank++;
		}
	}

	if( inum_blank == ilenbuf )
	{
		return 1;
	}

	return iret;
}


