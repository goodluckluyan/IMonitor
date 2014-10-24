
#include "check_netcard.h"
//#include <linux/sockios.h>
//#include <sys/socket.h>
//#include <sys/ioctl.h>
//#include <net/if.h>
//#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <string>
#include <vector>

#ifndef WIN32
#define O_BINARY       0x8000  
#ifndef strcmpi
#define strcmpi strcasecmp
#endif
#ifndef stricmp
#define stricmp strcasecmp
#endif
#endif

#define ETHTOOL_GLINK        0x0000000a /* Get link status (ethtool_value) */

typedef enum { IFSTATUS_UP, IFSTATUS_DOWN, IFSTATUS_ERR } interface_status_t;

typedef signed int u32;

/* for passing single values */
struct ethtool_value
{
	u32    cmd;
	u32    data;
};

interface_status_t interface_detect_beat_ethtool(int fd, char *iface)
{
	struct ifreq ifr;
	struct ethtool_value edata;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, iface, sizeof(ifr.ifr_name)-1);

	edata.cmd = ETHTOOL_GLINK;
	ifr.ifr_data = (caddr_t) &edata;

	if (ioctl(fd, SIOCETHTOOL, &ifr) == -1)
	{
		perror("ETHTOOL_GLINK failed ");
		return IFSTATUS_ERR;
	}

	return edata.data ? IFSTATUS_UP : IFSTATUS_DOWN;
}

int GetIFInfo(std::vector<std::string> &vecEth)
{
	int fd;
	char buf[1024];
	struct ifconf ifc;
	ifc.ifc_len=sizeof(buf);
	ifc.ifc_buf = buf;
	
	fd = socket(AF_INET,SOCK_DGRAM,0);

	if(ioctl(fd,SIOCGIFCONF,&ifc) < 0)
	{
		printf("get if info error: %s",strerror(errno));
		return -1;
	}

	int nEth = ifc.ifc_len/sizeof(struct ifreq);
	struct ifreq *ifr;
	ifr = (struct ifreq*)buf;
	for(int i = 0 ;i < nEth ;i++)
	{
		if(strcmp(ifr->ifr_name,"lo")== 0)
		{
			ifr++;
			continue;
		}

		char cName[4]={0};
		strncpy(cName,ifr->ifr_name,3);
		if(strcmp(cName,"eth") != 0)
		{
			ifr++;
			continue;
		}

		printf("%s:%s\n",ifr->ifr_name,inet_ntoa(((struct sockaddr_in*)&(ifr->ifr_addr))->sin_addr));
		vecEth.push_back(ifr->ifr_name);
		ifr++;
	}
	return  nEth;

	
}


//检测网卡的连接状态，rLinkStatus = 1,连接正常，rLinkStatus = 0,连接不正常 
int Test_NetCard::Check_EthLinkStatus(const char* eth_name , int& rLinkStatus )
{
	FILE *fp = NULL;
	interface_status_t status;
	char buf[512] = {'\0'};
	char hw_name[10] = {'\0'};
	//char *token = NULL;

	if ( eth_name == NULL )
	{
		return -1;
	}
	
	strcpy( hw_name , eth_name );

	//方法一：查看一个文件文件，相对来说比较简单
#if 0
	char carrier_path[512] = {'\0'};

	memset(buf, 0, sizeof(buf)); 
	snprintf(carrier_path, sizeof(carrier_path), "/sys/class/net/%s/carrier", hw_name);
	if ((fp = fopen(carrier_path, "r")) != NULL)
	{
		while (fgets(buf, sizeof(buf), fp) != NULL)
		{
			if (buf[0] == '0')
			{
				status = IFSTATUS_DOWN;
			}
			else
			{
				status = IFSTATUS_UP;
			}
		}
	}
	else
	{
		perror("Open carrier ");
		return -1;
	}
	fclose(fp);
#endif

	//方法二：用函数吧！有点复杂，但是也是一种有效的办法
#if 1
	int fd = -1;

	if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket ");
		return -70;
		//exit(0);
	}
	status = interface_detect_beat_ethtool(fd, hw_name);
	close(fd);
#endif

	switch ( status )
	{
	case IFSTATUS_UP:
		rLinkStatus = 1;
		//printf("%s : link up\n", hw_name);
		break;

	case IFSTATUS_DOWN:
		rLinkStatus = 0;
		//printf("%s : link down\n", hw_name);
		break;

	default:
		rLinkStatus = 0;
		//printf("Detect Error\n");
		
		return -1;
		break;
	}

	return 0;
}


int Read_NetDataBytes(const char * eth_name  , char rwMode , unsigned long long& bytes )
{
	FILE *fp = NULL;
	char buf[1024] = {'\0'};
	char path[1024] = {'\0'};

	memset(buf, 0, sizeof(buf)); 

	if ( rwMode == 'r' )
	{
		snprintf( path , sizeof(path) , "/sys/class/net/%s/statistics/rx_bytes" , eth_name );
	} 
	else
	{
		snprintf( path , sizeof(path) , "/sys/class/net/%s/statistics/tx_bytes" , eth_name );
	}

	if ((fp = fopen(path, "r")) != NULL)
	{
		while ( fgets( buf , sizeof(buf) , fp ) != NULL)
		{
			int result(0);
			result = sscanf( buf,"%llu\n", &bytes );
			if(result == 0)//sscanf失败，则返回0
			{
				printf( "Error:sscanf %s\n" , path );
				//return -1;
			}
		}
	}
	else
	{
		perror("Open statistics ");
		return -1;
	}

	fclose(fp);

	return 0;
}


Test_NetCard::Test_NetCard()
{
	int nIndex = 0;
	m_bInit = false;
	m_ptrDM = NULL;
}

Test_NetCard::~Test_NetCard()
{

}

//初始化函数，必须先被调用
int Test_NetCard::Init()
{
	m_ptrDM = CDataManager::GetInstance();
	int ret = 0;
	int nEth = GetIFInfo(m_vecEth);
	if(nEth <= 1)
	{
		return -1;
	}

	for ( int nIndex = 0; nIndex < m_vecEth.size() ; nIndex++ )
	{
		//初始化 m_old_CheckTime，每个网卡1个时间值
		time_t curTime = time(NULL);
		m_old_CheckTime[m_vecEth[nIndex]] = curTime;

		unsigned long long nInitRx_bytes = 0;
		ret = Read_NetDataBytes(m_vecEth[nIndex].c_str() , 'r' ,  nInitRx_bytes );
		if ( ret == 0 )
		{
			//printf( "Read_NetDataBytes:Recv Operate nInitRx_bytes = %llu\n" , nInitRx_bytes );
			m_oldRx_bytes[m_vecEth[nIndex]] = nInitRx_bytes;
		}
		else
		{
			//printf( "Init Error:eth=%s ,Recv nInitRx_bytes\n" , m_vecEth[nIndex].c_str() );
			//return -10;
		}

		unsigned long long nInitTx_bytes = 0;
		ret = Read_NetDataBytes( m_vecEth[nIndex].c_str() , 't' ,  nInitTx_bytes );
		if ( ret == 0 )
		{
			//printf( "Read_NetDataBytes:Send Operate nInitTx_bytes = %llu\n" , nInitTx_bytes );
			m_oldTx_bytes[m_vecEth[nIndex]] = nInitTx_bytes;
		}
		else
		{
			printf( "Init Error:eth=%s ,Send nInitTx_bytes\n" , m_vecEth[nIndex].c_str() );
			//return -11;
		}

	}
	m_bInit = true;

	return 0;
}



//网络带宽测量 eth_name=网卡名称必须为"eth0,eth1,eth2,eth3,eth4,eth5"中1个;TranBPS=表示每秒发送字节数，RecvBPS=表示每秒接收字节数
int Test_NetCard::Check_NewWork_Flow( std::string eth_name , unsigned long long& TranBPS ,
									 unsigned long long& RecvBPS )
{
	//printf("Enter Check_NewWork_Flow()!\n\n");

	int ret = 0;
	int nEthNameIndex = 0;
	char hw_name[20] = {'\0'};
	int nIntervalTime = 0;
	
	if ( eth_name.empty() )
	{
		return -1;
	}

	time_t curTime = time(NULL);
	nIntervalTime = curTime - m_old_CheckTime[eth_name];
	if ( nIntervalTime <= 0 )
	{
		nIntervalTime = 1;
	}

// 	printf( "\n curTime=%d\n" , curTime );
// 	printf( "m_old_CheckTime=%d\n" , m_old_CheckTime[nEthNameIndex] );
// 	printf( "IntervalTime=%d\n\n" , nIntervalTime );

	m_old_CheckTime[eth_name] = curTime;


	//初始化static数组 oldTx_bytes[],oldRx_bytes[] 
	if ( m_oldRx_bytes[eth_name] == 0 )
	{
		unsigned long long nInitRx_bytes = 0;
		ret = Read_NetDataBytes( eth_name.c_str() , 'r' ,  nInitRx_bytes );
		if ( ret == 0 )
		{
			//printf( "Read_NetDataBytes:Recv Operate nInitRx_bytes = %llu\n" , nInitRx_bytes );
			m_oldRx_bytes[eth_name] = nInitRx_bytes;
		}
		else
		{
			return -10;
		}
	} 
	
	if ( m_oldTx_bytes[eth_name] == 0 )
	{
		unsigned long long nInitTx_bytes = 0;
		ret = Read_NetDataBytes( eth_name.c_str() , 't' ,  nInitTx_bytes );
		if ( ret == 0 )
		{
			//printf( "Read_NetDataBytes:Send Operate nInitTx_bytes = %llu\n" , nInitTx_bytes );
			m_oldTx_bytes[eth_name] = nInitTx_bytes;
		}
		else
		{
			return -11;
		}
	}
	//初始化static数组 end 

	//首先，读取总的发送/接收字节数
	unsigned long long newRx_bytes = 0;
	ret = Read_NetDataBytes( eth_name.c_str() , 'r' ,  newRx_bytes );
	if ( ret != 0 )
	{
		return -3;
	}
	else
	{
		//printf( "Read_NetDataBytes:Recv Operate newRx_bytes = %llu\n" , newRx_bytes );
	}
	
	unsigned long long newTx_bytes = 0;
	ret = Read_NetDataBytes(  eth_name.c_str() , 't' ,  newTx_bytes );
	if ( ret != 0 )
	{
		return -4;
	}
	else
	{
		//printf( "Read_NetDataBytes:Send Operate newTx_bytes = %llu\n" , newTx_bytes );
	}

	unsigned long long nRealRecv_bytes = 0 , nRealSend_bytes = 0;

	//计算接收流量
	if ( newRx_bytes != 0 )
	{
		nRealRecv_bytes  = newRx_bytes - m_oldRx_bytes[eth_name];
		if( nRealRecv_bytes < 0 )
		{
			nRealRecv_bytes = newRx_bytes;
		}
	}

	RecvBPS = nRealRecv_bytes/(unsigned long long)nIntervalTime;

	//printf( "Real Recv bytes %llu B \n" , nRealRecv_bytes );
	//printf( "Net Speed RecvBPS: %llu B/S \n" , RecvBPS );
	//printf( "Net Speed Recv: %llu KB/S \n" , RecvBPS/1024 );

	if ( nRealRecv_bytes != 0 )
	{
		m_oldRx_bytes[eth_name] = newRx_bytes;
	}


	//计算发送流量
	if ( newTx_bytes != 0 )
	{
		nRealSend_bytes  = newTx_bytes - m_oldTx_bytes[eth_name];
		if( nRealSend_bytes < 0 )
		{
			nRealSend_bytes = newTx_bytes;
		}
	}

	TranBPS = nRealSend_bytes/(unsigned long long)nIntervalTime;

	//printf( "Real Send bytes %llu B \n" , nRealSend_bytes );
	//printf( "Net Speed TranBPS: %llu B/S \n" , TranBPS );
	//printf( "Net Speed Send: %llu KB/S \n" , TranBPS/1024 );

	if ( nRealSend_bytes != 0 )
	{
		m_oldTx_bytes[eth_name] = newTx_bytes;
	}

	//printf("Finished Check_NewWork_Flow()!\n\n");

	return 0;
}

bool Test_NetCard::GetAllEthStatus()
{
	if(!m_bInit)
	{
		Init();
	}

	std::vector<EthStatus> vecEthStatus;
	int nLen = m_vecEth.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		EthStatus node;
		node.strName = m_vecEth[i];
	
		if(Check_EthLinkStatus(m_vecEth[i].c_str(),node.nConnStatue) == -1)
		{
			vecEthStatus.push_back(node);
			continue;
		}
		if(node.nConnStatue == 1)
		{
			Check_NewWork_Flow(m_vecEth[i],node.nTxSpeed,node.nRxSpeed);
		}
		vecEthStatus.push_back(node);
	}

	if(m_ptrDM != NULL)
	{
		m_ptrDM->UpdateNetStat(vecEthStatus);
	}
	
}



bool Test_NetCard::InitAndCheck()
{
	bool bRet = true;
	if(!m_bInit)
	{
		Init();
	}
	
	int nLen = m_vecEth.size();
	for(int i = 0 ;i < nLen ;i++)
	{
		int nConnStatus = 0;
		if(Check_EthLinkStatus(m_vecEth[i].c_str(),nConnStatus) == -1)
		{
			printf("Get EthLink Status Fail!\n");
			bRet = false;
			break;
		}
		else
		{
			if(nConnStatus != 1)
			{
				bRet = false;
				printf("%s Connect Status Down!\n",m_vecEth[i].c_str());
				break;
			}
		}
		
		
	}
	return bRet;

}

