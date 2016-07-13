//@file:C_SatXmlParser.cpp
//@brief: 此类解析xml格式的数据
//@author: zhangmiao@oristartech.com
//date: 2013-05-28

#include <string>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <list>
#include <sys/types.h>
#include <sys/stat.h>
//#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#ifndef _SAT_XML_PARSER_H
#include "C_SatXmlParser.h"
#endif

typedef xercesc::DOMDocument DOMDOC;

using namespace std;

/*使用一段内存实例化一个C_SatXmlParser*/

C_SatXmlParser::C_SatXmlParser(const char* str)
{
// 	try
// 	{
// 		XMLPlatformUtils::Initialize();
// 	}
// 	catch( XMLException& e )
// 	{
// 		char* message = XMLString::transcode( e.getMessage() );
// 		XMLString::release( &message );
// 		m_xml_parser = NULL;
// 	}

	m_xml_parser = new XercesDOMParser;
	assert(m_xml_parser);
	m_xml_parser->setValidationScheme( XercesDOMParser::Val_Never );
	m_xml_parser->setDoNamespaces( true );
	m_xml_parser->setDoSchema( false );
	m_xml_parser->setLoadExternalDTD( false );
	m_inputsource = new MemBufInputSource((XMLByte*)str, strlen(str), "bufId");

	try
	{
		m_xml_parser->parse(*m_inputsource);
		m_xmlDoc = m_xml_parser->getDocument();		
	}
	catch( xercesc::XMLException& e )
	{
		char* message = xercesc::XMLString::transcode( e.getMessage() );
		XMLString::release( &message );
		m_xml_parser = NULL;
	}
}

C_SatXmlParser::~C_SatXmlParser()
{
	// Free memory
	if (m_xml_parser)
	{
		delete m_xml_parser;
		m_xml_parser = NULL;
	}

	if (m_inputsource)
	{
		delete m_inputsource;
		m_inputsource = NULL;
	}

	// Terminate Xerces

// 	try
// 	{
// 		XMLPlatformUtils::Terminate();  // Terminate after release of memory
// 	}
// 	catch( xercesc::XMLException& e )
// 	{
// 		char* message = xercesc::XMLString::transcode( e.getMessage() );
// 		//cerrout 
// 		XMLString::release( &message );
// 	}
}

int C_SatXmlParser::GetDcpInfoList_parser( std::vector< Satellite::DCPInfo > &DcpList )
{
	bool IsFind_dcplistNode = false;
	DOMDOC* xmlDoc = NULL;
	if ( m_xml_parser == NULL )
	{
		return XML_PARSER_ERROR;
	}
	else
	{
		xmlDoc = m_xml_parser->getDocument();
		if ( xmlDoc == NULL )
		{
			return XML_PARSER_ERROR;
		}
	}
	
	DOMNodeList* children = xmlDoc->getElementsByTagNameNS( C2X("*") , C2X("response") );
	if ( children == NULL )
	{
		return RESPONSE_FORMAT_ERROR;
	}
	else	//find "response" node
	{
		XMLSize_t count_of_node = children->getLength();
		if ( count_of_node <= 0 )
		{
			return RESPONSE_FORMAT_ERROR;
		}
		else 
		{
			//如果 "response" node存在，则取出"status"属性；如果"status"属性不存在，则表示格式错误
			DOMNode* xNode = children->item( 0 );
			if ( xNode == NULL ) 
				return RESPONSE_FORMAT_ERROR;
			
			XMLCh2Char tmpXmlch2Char( ((DOMElement*)xNode)->getAttribute( C2X("status") ) );
			std::string tmp = tmpXmlch2Char.get_char_str();
			if ( tmp.length() == 0 )
			{
				return RESPONSE_FORMAT_ERROR;
			}
			if ( strstr( tmpXmlch2Char.get_char_str() , "OK" ) == NULL )	//如果"status"属性不等于"OK"，则表示状态错误
			{
				//如果"status"属性不等于"OK"，则表示状态错误
				return RESPONSE_STATUS_ERROR;
			}
			else
			{
				//检查是否存在节点"dcp_list",如果不存在，则表示格式错误
				DOMNodeList* children = xNode->getChildNodes();
				count_of_node = children->getLength();
				if ( count_of_node <= 0 )
				{
					return RESPONSE_FORMAT_ERROR;
				}
				//
				DOMNode* current_Node = children->item(0);
				if ( current_Node == NULL ) 
					return RESPONSE_FORMAT_ERROR;
				XMLCh2Char Xmlch2Char(current_Node->getNodeName());
				if ( strstr( Xmlch2Char.get_char_str() , "dcp_list" ) )	//判断节点名字是否为"dcp_list",相同则表示节点找到设置IsFind_dcplistNode=true;
				{
					IsFind_dcplistNode = true;	//"dcp_list"节点已经找到
				}
				else
				{
					IsFind_dcplistNode = false;	//"dcp_list"节点没找到
				}
			}
		}
	}
	
	//如果<dcp_list>节点找到，则继续解析每个<dcp>节点。
	if ( IsFind_dcplistNode )
	{
		//搜索所有的节点<dcp>，然后进行逐个解析
		DOMNodeList* dcp_children = xmlDoc->getElementsByTagNameNS( C2X("*") , C2X("dcp") );
		if ( dcp_children == NULL )
		{
			return RESPONSE_FORMAT_ERROR;
		}
		else
		{
			XMLSize_t dcp_count_of_node = dcp_children->getLength();
			if ( dcp_count_of_node <= 0 )
			{
				return RESPONSE_FORMAT_ERROR;
			}
			else
			{
				XMLSize_t count_of_node = 0;
				for ( XMLSize_t i = 0; i < dcp_count_of_node; i++ )
				{
					//取出每个<dcp>节点
					DOMNode* xNode = dcp_children->item( i );
					if ( xNode == NULL )
					{
						return XML_NODE_NOEXIST_ERROR;
					}
					DOMNodeList* children = xNode->getChildNodes();
					if ( children == NULL )
					{
						return RESPONSE_FORMAT_ERROR;
					}
					count_of_node = children->getLength();
					if ( count_of_node <= 0 )
					{
						return RESPONSE_FORMAT_ERROR;
					}

					Satellite::DCPInfo info;
					
					for ( XMLSize_t xx = 0; xx < count_of_node; xx++ )
					{
						DOMNode* current_Node = children->item( xx );
						if ( current_Node == NULL )
						{
							return XML_NODE_NOEXIST_ERROR;
						}
						XMLCh2Char Xmlch2Char(current_Node->getNodeName());
						if ( strstr( Xmlch2Char.get_char_str() , "uuid" ) )	//DCP的uuid
						{
							XMLCh2Char tmpXmlch2Char(((DOMElement*)current_Node)->getFirstChild()->getNodeValue());
							info.PklUuid = tmpXmlch2Char.get_char_str() + strlen("urn:uuid:");
						}
						else if ( strstr( Xmlch2Char.get_char_str() , "issuer" ) )	//DCP的发行者
						{
							XMLCh2Char tmpXmlch2Char(((DOMElement*)current_Node)->getFirstChild()->getNodeValue());
							info.issuer = tmpXmlch2Char.get_char_str();
						}
						else if ( strstr( Xmlch2Char.get_char_str() , "creator" ) )	//DCP的创建者
						{
							XMLCh2Char tmpXmlch2Char(((DOMElement*)current_Node)->getFirstChild()->getNodeValue());
							info.Creater = tmpXmlch2Char.get_char_str();
						}
						else if ( strstr( Xmlch2Char.get_char_str() , "issueDate" ) )	//DCP的发行时间
						{
							XMLCh2Char tmpXmlch2Char(((DOMElement*)current_Node)->getFirstChild()->getNodeValue());
							info.issueDate = tmpXmlch2Char.get_char_str();
						}
						else if ( strstr( Xmlch2Char.get_char_str() , "name" ) )
						{
							XMLCh2Char tmpXmlch2Char(((DOMElement*)current_Node)->getFirstChild()->getNodeValue());
							info.Name = tmpXmlch2Char.get_char_str();
						}
					}

					if( info.Creater.size() && info.issueDate.size() && info.PklUuid.size() && info.issuer.size() )
						DcpList.push_back( info );
				}
			}
		}
	}
	else
	{
		return RESPONSE_FORMAT_ERROR;
	}

	return 0;
}

int C_SatXmlParser::GetFilmFtpInfo_parser( Satellite::FTP_DCPInfo& rFTP_DCPInfo )
{
	DOMDOC* xmlDoc = NULL;
	if ( m_xml_parser == NULL )
	{
		return XML_PARSER_ERROR;
	}
	else
	{
		xmlDoc = m_xml_parser->getDocument();
		if ( xmlDoc == NULL )
		{
			return XML_PARSER_ERROR;
		}
	}

	DOMNodeList* children = xmlDoc->getElementsByTagNameNS( C2X("*") , C2X("response") );
	if ( children == NULL )
	{
		return RESPONSE_FORMAT_ERROR;
	}
	else	//find "response" node
	{
		XMLSize_t count_of_node = children->getLength();
		if ( count_of_node <= 0 )
		{
			return RESPONSE_FORMAT_ERROR;
		}
		else 
		{
			//如果 "response" node存在，则取出"status"属性；如果"status"属性不存在，则表示格式错误
			DOMNode* xNode = children->item( 0 );
			if ( xNode == NULL ) 
				return RESPONSE_FORMAT_ERROR;

			XMLCh2Char tmpXmlch2Char( ((DOMElement*)xNode)->getAttribute( C2X("status") ) );
			std::string tmp = tmpXmlch2Char.get_char_str();
			if ( tmp.length() == 0 )
			{
				return RESPONSE_FORMAT_ERROR;
			}
			if ( strstr( tmpXmlch2Char.get_char_str() , "OK" ) == NULL )	//如果"status"属性不等于"OK"，则表示状态错误
			{
				//如果"status"属性不等于"OK"，则表示状态错误
				return RESPONSE_STATUS_ERROR;
			}
			else
			{
				DOMNodeList* children = xNode->getChildNodes();
				count_of_node = children->getLength();
				if ( count_of_node <= 0 )
				{
					return RESPONSE_FORMAT_ERROR;
				}
				for ( XMLSize_t xx = 0; xx < count_of_node; xx++ )
				{
					DOMNode* current_Node = children->item( xx );
					if ( current_Node == NULL )
					{
						return XML_NODE_NOEXIST_ERROR;
					}

					XMLCh2Char Xmlch2Char(current_Node->getNodeName());
					if ( strstr( Xmlch2Char.get_char_str() , "asset_type" ) )		//DCP下载类型（PKL或者CPL）
					{
						XMLCh2Char tmpXmlch2Char(((DOMElement*)current_Node)->getFirstChild()->getNodeValue());
						rFTP_DCPInfo.asset_type = tmpXmlch2Char.get_char_str();
					}
					else if ( strstr( Xmlch2Char.get_char_str() , "source" ) )	//DCP在ftp存放的路径
					{
						XMLCh2Char tmpXmlch2Char(((DOMElement*)current_Node)->getFirstChild()->getNodeValue());
						rFTP_DCPInfo.source = tmpXmlch2Char.get_char_str();
					}
					else if ( strstr( Xmlch2Char.get_char_str() , "path" ) )		//ftp服务器的位置
					{
						XMLCh2Char tmpXmlch2Char(((DOMElement*)current_Node)->getFirstChild()->getNodeValue());
						rFTP_DCPInfo.path = tmpXmlch2Char.get_char_str();
					}
					else if ( strstr( Xmlch2Char.get_char_str() , "username" ) )	//登陆ftp的用户名
					{
						XMLCh2Char tmpXmlch2Char(((DOMElement*)current_Node)->getFirstChild()->getNodeValue());
						rFTP_DCPInfo.username = tmpXmlch2Char.get_char_str();
					}
					else if ( strstr( Xmlch2Char.get_char_str() , "password" ) )	//登陆ftp的密码
					{
						XMLCh2Char tmpXmlch2Char(((DOMElement*)current_Node)->getFirstChild()->getNodeValue());
						rFTP_DCPInfo.password = tmpXmlch2Char.get_char_str();
					}
				}

				std::string::size_type bgn = rFTP_DCPInfo.source.find( "//" );
				if( bgn == string::npos )
					return -1;
				bgn += 2;
				if( bgn >= rFTP_DCPInfo.source.size() )
					return -1;
				/*
				 ftp_url格式如下：
				 FTP://账号:密码@主机/子目录或文件
				*/
				rFTP_DCPInfo.ftp_url = rFTP_DCPInfo.source + rFTP_DCPInfo.path;
				rFTP_DCPInfo.ftp_url.insert( bgn , rFTP_DCPInfo.username + ":" + rFTP_DCPInfo.password + "@" );
				rFTP_DCPInfo.ftp_url_path = rFTP_DCPInfo.ftp_url.substr( 0 , rFTP_DCPInfo.ftp_url.rfind("/") + 1 );
			}
		}
	}
	return 0;
}


