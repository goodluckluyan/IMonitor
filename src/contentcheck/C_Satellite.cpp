//@file:C_Satellite.cpp
//@brief: Satellite的接口实现。目前CRC32校验和，暂时没有采用，可能以后会使用，代码已经添加上了；
//@author: zhangmiao@oristartech.com
//date: 2013-05-28

#ifndef _SAT_XML_PARSER_H
#include "C_SatXmlParser.h"
#endif
#ifndef _SAT_DCP_H_
#include "C_Satellite.h"
#endif

#include "crc32.h"

//表示是否激活CRC32校验，目前不激活
#define _ENABLE_CRC32_

const int RECV_DELYTIME = 3; 
const int MINDELYTIME = 15;

using namespace std;
using namespace xercesc;

C_Satellite::C_Satellite( void )
{
	m_xmlHeader = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	m_responseBegin = "<response status=\"OK\"  version=\"2\" >";
	m_responseEnd = "</response>";

	m_tcp = new C_TcpSocket( BUFFERLENGTH );

	m_bConncetState = 0;
}

C_Satellite::~C_Satellite( void )
{
	if( ( m_tcp != NULL ) && m_tcp->BeConnected() )
		m_tcp->TcpDisConnect();

	delete m_tcp;
	m_tcp = NULL;

	m_bConncetState = 0;
}

//初始化建立与卫星接收设备tcp连接
int C_Satellite::Init( const std::string &strIp, unsigned short usPort/*=20080 */ )
{
	if( ( m_tcp != NULL ) && m_tcp->BeConnected() )
		m_tcp->TcpDisConnect();

	m_ip = strIp;
	m_port = usPort;
	
	m_tcp->SetTimeout( MINDELYTIME );
	int result = m_tcp->TcpConnect( m_ip.c_str() , m_port );

	return result != 0 ? -1 : 0;
}

//关闭连接认证
int C_Satellite::Close()
{
	if( ( m_tcp != NULL ) && m_tcp->BeConnected() )
		m_tcp->TcpDisConnect();

	m_bConncetState = 0;
	return 0;
}

//获得认证连接状态
int C_Satellite::GetCertConnect_Status()
{
	if( ( m_tcp != NULL ) && m_tcp->BeConnected() )
	{
		return m_bConncetState;
	}
	else
	{
		m_bConncetState = 0;
		return m_bConncetState;
	}
}

//建立连接认证请求
int C_Satellite::CreateCertConnect( )
{
	int result = 0;
	unsigned char sendBuffer[BUFFERLENGTH];
	unsigned char recvBuffer[BUFFERLENGTH];
	memset(sendBuffer, 0, BUFFERLENGTH);
	memset(recvBuffer, 0, BUFFERLENGTH);

	int lDatagramLen = 0;
	result = SetCtrlCmd_Device( sendBuffer , 0x11, 0 , NULL , lDatagramLen );
	if ( result != 0 )
	{
		printf("Error:SetCtrlCmd_Device()\n");
		return result;
	}

	int rHasSendDataBytes = 0;
	result = m_tcp->Send( (const char *)sendBuffer , lDatagramLen , rHasSendDataBytes );

#ifdef _DEBUG_
	printf("[---send cmd---] :\n");	
	int i = 0;
	for( i = 0; i < rHasSendDataBytes; i++ )
		printf( "%02X " , sendBuffer[i] );
	printf("\n");
#endif

	if( (0 != result) || (lDatagramLen != rHasSendDataBytes) )
	{
		printf("[ERROR] \" CreateCertConnect \" send failed(%d):Len=%d,sendlen=%d\n", result , lDatagramLen , rHasSendDataBytes );
		
		m_tcp->TcpDisConnect();
		return ERRPR_TCPTRANSPORTFAILED;
	}
	else
	{
		printf("[INFO]  \" CreateCertConnect \" send succeed(%d)!\n", result);

		int nRecvDataLen = 0;
		result = RecvInfo( (char*)recvBuffer , BUFFERLENGTH , nRecvDataLen );
		if( result != 0 )
		{
			printf( "RecvInfo() Error:%d\n" , result );
			return result;
		}

		if ( nRecvDataLen != ( CmdInfoHeaderLen + CheckSumLen ) )
		{

#ifdef _DEBUG_
			printf("ERROR_CONNECT_ACK_FORMAT\n");
#endif
			return ERROR_CONNECT_ACK_FORMAT;
		}

		CtrlCmdInfoHeader* pRequestCtrlCmdInfo = (CtrlCmdInfoHeader*)recvBuffer;
		if ( pRequestCtrlCmdInfo && (pRequestCtrlCmdInfo->Preamble == nPreamble) && ( pRequestCtrlCmdInfo->Cmd == 0x12 ) )
		{
			m_bConncetState = 1;
		}
		else
		{
			m_bConncetState = 0;

#ifdef _DEBUG_
			printf("ERROR_CONNECT_FAILED\n");
#endif
			return ERROR_CONNECT_FAILED;
		}
	}

	return 0;
}

//获得影片信息列表
int C_Satellite::GetDcpList( std::vector< Satellite::DCPInfo > &DcpList )
{
	int result = 0;
	unsigned char sendBuffer[BUFFERLENGTH];
	unsigned char recvBuffer[BUFFERLENGTH];
	memset(sendBuffer, 0, BUFFERLENGTH);
	memset(recvBuffer, 0, BUFFERLENGTH);

	int lDatagramLen = 0;
	result = SetCtrlCmd_Device( sendBuffer , 0x21, 0 , NULL , lDatagramLen );
	if ( result != 0 )
	{
		printf("Error:SetCtrlCmd_Device()\n");
		return result;
	}

	int rHasSendDataBytes = 0;
	result = m_tcp->Send( (const char*)sendBuffer , lDatagramLen , rHasSendDataBytes );
	
#ifdef _DEBUG_
	printf("[---send cmd---] :\n");	
	int i = 0;
	for(i = 0; i < rHasSendDataBytes; i++)
		printf("%02X ", sendBuffer[i]);
	printf("\n");
#endif

	if( (0 != result) || (lDatagramLen != rHasSendDataBytes) )
	{
		printf("[ERROR] \" GetDcpList \" send failed(%d):Len=%d,sendlen=%d\n", result , lDatagramLen , rHasSendDataBytes );

		m_tcp->TcpDisConnect();
		return ERRPR_TCPTRANSPORTFAILED;
	}
	else
	{
		printf("[INFO]  \" GetDcpList \" send succeed(%d)!\n", result);

		int nRecvDataLen = 0;
		result = RecvInfo( (char*)recvBuffer , BUFFERLENGTH , nRecvDataLen );
		if( result != 0 )
		{
			printf( "RecvInfo() Error:%d\n" , result );
			return result;
		}

		CtrlCmdInfoHeader* pRequestCtrlCmdInfo = (CtrlCmdInfoHeader*)recvBuffer;
		if ( pRequestCtrlCmdInfo && ( pRequestCtrlCmdInfo->Preamble == nPreamble ) && ( pRequestCtrlCmdInfo->Cmd == 0x22 ) )
		{
			if ( (u32)nRecvDataLen != ( CmdInfoHeaderLen + CheckSumLen + pRequestCtrlCmdInfo->PayloadLength ) )
			{

#ifdef _DEBUG_
				printf("ERRPR_GETDCPLIST_ACK_FORMAT\n");
#endif
				return ERRPR_GETDCPLIST_ACK_FORMAT;
			}
			//解析操作
			char PayloadBuffer[BUFFERLENGTH] = "";
			u32 uPayloadLength = pRequestCtrlCmdInfo->PayloadLength;
			memcpy( PayloadBuffer , recvBuffer + CmdInfoHeaderLen , uPayloadLength );

#ifdef _DEBUG_
			printf("Payload Buffer:%s\n\n",PayloadBuffer);
#endif
			C_SatXmlParser xml_parser( PayloadBuffer );
			result = xml_parser.GetDcpInfoList_parser( DcpList );
			if ( result != 0 )
			{
				return result;
			}
			
		}
		else
		{

#ifdef _DEBUG_
			printf("ERRPR_GET_DCPLIST_RESPONSE\n");
#endif

			return ERRPR_GET_DCPLIST_RESPONSE;
		}
	}

	return 0;
}

//获得对应影片FTP信息
int C_Satellite::GetFTPDCPInfoById( const std::string &pklUuid , Satellite::FTP_DCPInfo& rFTP_DCPInfo )
{
	int result = 0;
	unsigned char sendBuffer[BUFFERLENGTH];
	unsigned char recvBuffer[BUFFERLENGTH];
	memset(sendBuffer, 0, BUFFERLENGTH);
	memset(recvBuffer, 0, BUFFERLENGTH);

	string xml = GetFTPDCPInfoByIdXml( pklUuid );

#ifdef _DEBUG_
	printf("Payload Buffer： %s\n\n",xml.c_str());
#endif

	int lDatagramLen = 0;
	result = SetCtrlCmd_Device( sendBuffer , 0x24 , xml.length() , (unsigned char*)(xml.c_str()) , lDatagramLen );
	if ( result != 0 )
	{
		printf("Error:SetCtrlCmd_Device()\n");
		return result;
	}
	
	int rHasSendDataBytes = 0;
	result = m_tcp->Send( (const char*)sendBuffer , lDatagramLen , rHasSendDataBytes );

#ifdef _DEBUG_
	printf("[---send cmd---] :\n");	
	int i = 0;
	for(i = 0; i < rHasSendDataBytes; i++)
		printf("%02X ", sendBuffer[i]);
	printf("\n");
#endif

	if( (0 != result) || (lDatagramLen != rHasSendDataBytes) )
	{
		printf("[ERROR] \" GetFTPDCPInfoById \" send failed(%d):Len=%d,sendlen=%d\n", result , lDatagramLen , rHasSendDataBytes );

		m_tcp->TcpDisConnect();
		return ERRPR_TCPTRANSPORTFAILED;
	}
	else
	{
		printf("[INFO]  \" GetFTPDCPInfoById \" send succeed(%d)!\n", result);
		
		int nRecvDataLen = 0;
		result = RecvInfo( (char*)recvBuffer , BUFFERLENGTH , nRecvDataLen );
		if( result != 0 )
		{
			printf( "RecvInfo() Error:%d\n" , result );
			return result;
		}

		CtrlCmdInfoHeader* pRequestCtrlCmdInfo = (CtrlCmdInfoHeader*)recvBuffer;
		if ( pRequestCtrlCmdInfo && ( pRequestCtrlCmdInfo->Preamble == nPreamble ) && ( pRequestCtrlCmdInfo->Cmd == 0x25 ) )
		{
			if ( (u32)nRecvDataLen != ( CmdInfoHeaderLen + CheckSumLen + pRequestCtrlCmdInfo->PayloadLength ) )
			{

#ifdef _DEBUG_
				printf("ERRPR_GET_FILM_FTPINFO_ACK_FORMAT\n");
#endif

				return ERRPR_GET_FILM_FTPINFO_ACK_FORMAT;
			}

			//解析操作
			char PayloadBuffer[BUFFERLENGTH] = "";
			u32 uPayloadLength = pRequestCtrlCmdInfo->PayloadLength;
			memcpy( PayloadBuffer , recvBuffer + CmdInfoHeaderLen , uPayloadLength );

#ifdef _DEBUG_
			printf("Payload Buffer:%s\n\n",PayloadBuffer);
#endif

			C_SatXmlParser xml_parser( PayloadBuffer );
			result = xml_parser.GetFilmFtpInfo_parser( rFTP_DCPInfo );
			if ( result != 0 )
			{
				return result;
			}

		}
		else
		{

#ifdef _DEBUG_
			printf("ERRPR_GET_FILM_FTPINFO_RESPONSE\n");
#endif

			return ERRPR_GET_FILM_FTPINFO_RESPONSE;
		}
	}
	
	return 0;
}

//对应影片下载完毕；如果status = 1，表示影片下载完毕。status = 0，表示下载失败。
int C_Satellite::SetFTPDownLoadFinished( const std::string &pklUuid , const int status )
{
	int result = 0;
	unsigned char sendBuffer[BUFFERLENGTH];
	unsigned char recvBuffer[BUFFERLENGTH];
	memset(sendBuffer, 0, BUFFERLENGTH);
	memset(recvBuffer, 0, BUFFERLENGTH);

	string xml = FTPDownLoadFinishedAckXml( pklUuid , status );

#ifdef _DEBUG_
	printf("Payload Buffer：%s\n\n",xml.c_str());
#endif

	int lDatagramLen = 0;
	result = SetCtrlCmd_Device( sendBuffer , 0x27 , xml.length() , (unsigned char*)(xml.c_str()) , lDatagramLen );
	if ( result != 0 )
	{
		printf("Error:SetCtrlCmd_Device()\n");
		return result;
	}

	int rHasSendDataBytes = 0;
	result = m_tcp->Send( (const char*)sendBuffer , lDatagramLen , rHasSendDataBytes );

#ifdef _DEBUG_
	printf("[---send cmd---] :\n");	
	int i = 0;
	for(i = 0; i < rHasSendDataBytes; i++)
		printf("%02X ", sendBuffer[i]);
	printf("\n");
#endif

	if( (0 != result) || (lDatagramLen != rHasSendDataBytes) )
	{
		printf("[ERROR] \" SetFTPDownLoadFinished \" send failed(%d):Len=%d,sendlen=%d\n", result , lDatagramLen , rHasSendDataBytes );

		m_tcp->TcpDisConnect();
		return ERRPR_TCPTRANSPORTFAILED;
	}
	else
	{
		printf("[INFO]  \" SetFTPDownLoadFinished \" send succeed(%d)!\n", result);

		int nRecvDataLen = 0;
		result = RecvInfo( (char*)recvBuffer , BUFFERLENGTH , nRecvDataLen );
		if( result != 0 )
		{

#ifdef _DEBUG_
			printf( "RecvInfo() Error:%d\n" , result );
#endif
			return result;
		}

		if ( nRecvDataLen != ( CmdInfoHeaderLen + CheckSumLen ) )
		{

#ifdef _DEBUG_
			printf("ERRPR_FTP_DOWNLOAD_FINISH_ACK_FORMAT\n");
#endif
			return ERRPR_FTP_DOWNLOAD_FINISH_ACK_FORMAT;
		}

		CtrlCmdInfoHeader* pRequestCtrlCmdInfo = (CtrlCmdInfoHeader*)recvBuffer;
		if ( pRequestCtrlCmdInfo && (pRequestCtrlCmdInfo->Preamble == nPreamble) && ( pRequestCtrlCmdInfo->Cmd == 0x28 ) )
		{
#ifdef _DEBUG_
			printf("SetFTPDownLoadFinished() OK!\n");
#endif
		}
		else
		{

#ifdef _DEBUG_
			printf("ERROR_FTP_DOWNLOAD_FINISH_FAILED\n");
#endif
			return ERROR_FTP_DOWNLOAD_FINISH_FAILED;
		}
	}

	return 0;
}

//生成设备的控制命令报文;rDatagramLen为生成的报文长度
int C_Satellite::SetCtrlCmd_Device( unsigned char* pBuffer , u16 nCmd, u32 PayloadLength, unsigned char* pPayload , int& rDatagramLen )
{
	CtrlCmdInfoHeader* pRequestCtrlCmdInfo = (CtrlCmdInfoHeader*)pBuffer;
	if (pRequestCtrlCmdInfo)
	{
		pRequestCtrlCmdInfo->Preamble = nPreamble;
		pRequestCtrlCmdInfo->Cmd = nCmd;
		pRequestCtrlCmdInfo->PayloadLength = PayloadLength;
		int nHeaderSize = sizeof( CtrlCmdInfoHeader );
		if ( PayloadLength != 0 )
		{
			memcpy( pBuffer + nHeaderSize, pPayload, PayloadLength );
		}

		rDatagramLen = nHeaderSize + PayloadLength + CheckSumLen;

#ifdef _ENABLE_CRC32_
		//uint32_t crc32_val = crc32( 0xffffffff , pBuffer  , nHeaderSize + PayloadLength );
		uint32_t crc32_val = crc32(  pBuffer  , nHeaderSize + PayloadLength );
		uint32_t* p = (uint32_t*)(pBuffer + nHeaderSize + PayloadLength) ;
		*p = crc32_val;
#endif

	}
	else
	{
		return -1;
	}

	return 0;
}

int C_Satellite::RecvInfo( char *buffer, int bufferLength , int& rHasRecvDataLen )
{
	int result = -1;
	int recvlen = 0;
	int i=0;

	m_tcp->SetTimeout( 10 );

	memset(buffer, 0, bufferLength);

	while( buffer[0] != 0x55 )
	{
		recvlen = 0;
		result = m_tcp->Recv( &buffer[0] , 1 , recvlen );
		if( result == 0 && recvlen == 1 )    //SOCKET_SUCCEES		
		{
#ifdef _DEBUG_
			printf("[---recv Preamble---] :\n");
			printf("%02X ", buffer[0]);
			printf("\n");
#endif
			printf("[INFO]  \" RecvInfo \" Recv Preamble succeed(%d)!\n", result);
		}
		else
		{
			m_tcp->TcpDisConnect();
			return ERRPR_TCPTRANSPORTFAILED;
		}
	}
	rHasRecvDataLen++;

	result = -1;
	recvlen = 0;
	result = m_tcp->Recv( &buffer[1] , 6 , recvlen );
	if( result == 0 && (recvlen == 6 ) )    //SOCKET_SUCCEES			
	{
		rHasRecvDataLen += 6;

#ifdef _DEBUG_
		printf("[---recv Other Header 6 bytes ---] :\n");
		for(i = 0; i < recvlen; i++)
			printf("%02X ", buffer[1+i]);
		printf("\n");
#endif

		printf("[INFO]  \" RecvInfo \" Recv Other Header 6 bytes succeed(%d)!\n", result);
	}
	else
	{
		if( result < 0 || result != 0 )
			m_tcp->TcpDisConnect();
		return ERRPR_TCPTRANSPORTFAILED;
	}

	u32 uPayloadLength = 0;
	CtrlCmdInfoHeader* pRequestCtrlCmdInfo = (CtrlCmdInfoHeader*)buffer;
	if ( pRequestCtrlCmdInfo && ( pRequestCtrlCmdInfo->Preamble == nPreamble ) && 
		( pRequestCtrlCmdInfo->Cmd == 0x12 || pRequestCtrlCmdInfo->Cmd == 0x25 || pRequestCtrlCmdInfo->Cmd == 0x26 
		 || pRequestCtrlCmdInfo->Cmd == 0x28 || pRequestCtrlCmdInfo->Cmd == 0x22 || pRequestCtrlCmdInfo->Cmd == 0x23 ) )
	{
		uPayloadLength = pRequestCtrlCmdInfo->PayloadLength;
	}
	else
	{
		m_tcp->TcpDisConnect();
		return ERRPR_TCPTRANSPORTFAILED;
	}

	//如果PayloadLength不等于0，则必须先接受载荷内容。
	if ( uPayloadLength != 0 )
	{
		//u32 recvLen = 0;
		int recvLen = 0;
		char* pPayloadBuf = buffer + CmdInfoHeaderLen ;

		m_tcp->SetTimeout( 90 );
		result = m_tcp->Recv( pPayloadBuf , uPayloadLength , recvLen );
		if( result == 0 && ((u32)recvLen == uPayloadLength ) )    //SOCKET_SUCCEES			
		{
#ifdef _DEBUG_
			printf("[---recv Payload Data---] :\n");
			for(i = 0; i < recvLen; i++)
				printf("%02X ", *(pPayloadBuf+i) );
#endif
			printf("\n");
			
			printf("[INFO]  \" RecvInfo \" Recv Payload succeed(%d)!\n", result);
		}
		else
		{
			if( result < 0 || result != 0 )
				m_tcp->TcpDisConnect();
			return ERRPR_TCPTRANSPORTFAILED;
		}

		rHasRecvDataLen += recvLen;

#ifdef _DEBUG_
		printf( "---Payload Data:%s---\n" , pPayloadBuf );
		printf( "---Recv Payload Data length :%d---\n" , recvLen );
		printf( "---PayloadLength:%d---\n", uPayloadLength );
#endif

	}

	//最后接收校验和
	char* pCheckSumBuf = buffer + CmdInfoHeaderLen + uPayloadLength ;
	m_tcp->SetTimeout( 60 );
	result = -1;
	recvlen = 0;
	result = m_tcp->Recv( pCheckSumBuf , CheckSumLen , recvlen );
	if( result == 0 && (recvlen == CheckSumLen) )    //SOCKET_SUCCEES			
	{
#ifdef _DEBUG_
		printf("[---recv CheckSum---] :\n");
		for(i = 0; i < recvlen; i++)
			printf("%02X ", pCheckSumBuf[i]);
		printf("\n");
		printf("[INFO]  \" RecvInfo \" Recv CheckSum succeed(%d)!\n", result);
#endif

		rHasRecvDataLen += recvlen;
	}
	else
	{
		if( result < 0 || result != 0 )
			m_tcp->TcpDisConnect();
		return ERRPR_TCPTRANSPORTFAILED;
	}
	
	return 0;
}

string C_Satellite::GetFTPDCPInfoByIdXml(const string &pklUuid)
{
	return m_xmlHeader 
		+ m_responseBegin 
		+ "<uuid>" + "urn:uuid:" + pklUuid + "</uuid>"
		+ m_responseEnd;
}

string C_Satellite::FTPDownLoadFinishedAckXml( const string &pklUuid, const int status )
{
	if( status == 1 )
	{
		return m_xmlHeader 
			+ m_responseBegin 
			+ "<uuid>" + "urn:uuid:" + pklUuid + "</uuid>"
			+ m_responseEnd;
	}
	else
	{
		return m_xmlHeader 
			+ "<response status=\"ERROR\"  version=\"2\" >" 
			+ "<uuid>" + "urn:uuid:" + pklUuid + "</uuid>"
			+ "<error>Asset is missing</error>"
			+ m_responseEnd;
	}
}

