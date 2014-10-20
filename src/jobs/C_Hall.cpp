//@file:C_Hall.cpp
//@brief: 实现C_Hall 所有的方法。
//@author:wangzhongping@oristartech.com
//dade:2012-07-12

#include "C_Hall.h"
#include <algorithm>
#include "database/CppMySQL3DB.h"
#include "para/C_RunPara.h"
const int ERROR_PLAYER_AQ_BADHTTPRESPONSE = -1;
const int ERROR_PLAYER_AQ_NEEDSOAPELEM = -2;
const int BufferLength = 2048;

using namespace std;
using namespace xercesc;
C_Hall::C_Hall(SMSInfo &stSms)
{
   m_SMS = stSms;
}

C_Hall::~C_Hall()
{
    
}

// 启动SMS
bool C_Hall::StartSMS()
{
	return true;
}

int C_Hall :: SmsInit( )
// const string &strUserName, const string &strPassword)
{
	m_xmlHeader = "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
	m_xmlnsXsi = "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" ";
	m_xmlnsXsd = "xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" ";
	m_xmlnsSoap = "xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\" ";
	m_envelopeBgn = "<soap:Envelope " + m_xmlnsXsi + m_xmlnsXsd + m_xmlnsSoap + ">";
	m_envelopeEnd = "</soap:Envelope>";
	m_bodyBgn = "<soap:Body>";
	m_bodyEnd = "</soap:Body>";

	//refer to wsdl file.
	m_usherLocation = "/oristar/services/Usher";

	m_usherNs = "http://webservices.oristar.com/XP/Usher/2009-09-29/";

	return 0;
}


//获取SMS 运行状态
int C_Hall :: GetSMSWorkState( int &state, string &info)
{
	//初始化获取AQ33的ip和port
	if (m_SMS.strIp.empty())
	{
		return -1;
	}
	SmsInit();

	int iResult;
	string response_c;
	string content_c;

	string cmd = GetSMSWorkState_Xml();
	string http = UsherHttp( cmd, "GetWorkState_CS");

	iResult = SendAndRecvInfo( http, response_c, 30);
	if (iResult != 0)
	{
		return iResult;//SoftwareSTATE_ERROR_TCP
	}

	iResult = GetHttpContent( response_c, content_c);
	if (iResult != 0)
	{
		return iResult;//SoftwareSTATE_ERROR_HTTP
	}

	iResult = Parser_GetSMSWorkState( content_c, state, info);
	if (iResult != 0)
	{
		return iResult;//SoftwareSTATE_ERROR_XMLPARSER
	}
	return 0;
}

string C_Hall :: GetSMSWorkState_Xml()
{
	string temp;
	temp = m_xmlHeader + m_envelopeBgn + m_bodyBgn
		+ "<GetWorkState_CS xmlns=\"" + m_usherNs + "\" /"
		+ m_bodyEnd + m_envelopeEnd;
	return temp;
}

string &C_Hall::UsherHttp(const string &xml, const string &action)
{
	m_request.ClearHttp();
	m_request.SetMethod("POST");
	m_request.SetUri( m_usherLocation.c_str());
	m_request.SetVersion("HTTP/1.0");
	m_request.SetHost(m_SMS.strIp.c_str());
	m_request.SetContentType("text/xml; charset=utf-8");
	m_request.SetContent(xml);
	m_request.SetSoapAction((m_usherNs + action).c_str());
	return m_request.GetHttpRequest();
}

int C_Hall::GetHttpContent(const string &http, string &content)
{
	m_response.ClearHttp();

	int result = m_response.SetHttpResponse(http);
	if(result < 0)
	{
		//SetAq10Error(ERROR_PLAYER_AQ_BADHTTPRESPONSE, "bad http response! 03");
		return ERROR_PLAYER_AQ_BADHTTPRESPONSE;//
	}

	content = m_response.GetContent();
	int status = m_response.GetStatus();
	return status;
}

//解析SMS返回的XML
int C_Hall :: Parser_GetSMSWorkState( const string &content, int &state, string &info)
{
	XercesDOMParser *parser = new XercesDOMParser();
	ErrorHandler *errHandler = (ErrorHandler*) new HandlerBase();
	DOMElement *rootChild = NULL;

	int result = GetRootChild( content, parser, errHandler, &rootChild);
	if (result < 0 || rootChild == NULL)
		;
	else
		result = Parser_GetSMSWorkState_Response( rootChild, state, info);

	delete errHandler;
	delete parser;
	return result;
}

int C_Hall :: Parser_GetSMSWorkState_Response( xercesc::DOMElement *rootChild, int &state, string &info)
{
	DOMElement *child = GetElementByName(rootChild->getFirstChild(), "GetWorkState_CSResponse");
	if(child == NULL)
	{
		return ERROR_PLAYER_AQ_NEEDSOAPELEM;
	}

	char *p;
	DOMElement *root = GetElementByName(child->getFirstChild(), "state");
	if ( child == NULL || child->getFirstChild() == NULL || child->getFirstChild()->getNodeValue() == NULL)
	{
		return ERROR_PLAYER_AQ_NEEDSOAPELEM;
	}
	p = (char *)XMLString::transcode(root->getFirstChild()->getNodeValue());
	state = atoi(p);
	XMLString::release( &p);

	root = GetElementByName( child->getFirstChild(), "info");
	if ( child == NULL || child->getFirstChild() == NULL || child->getFirstChild()->getNodeValue() == NULL)
	{
		return ERROR_PLAYER_AQ_NEEDSOAPELEM;
	}
	p = (char*)XMLString::transcode(root->getFirstChild()->getNodeValue());
	info = p;
	XMLString::release( &p);

	return 0;
}

//发送与接收 数据/////////////////////////////////////////////////////////////////
int C_Hall :: SendAndRecvInfo(const string &send, string &recv, int overtime)
{
	//send
	int result = m_tcp.TcpConnect(m_SMS.strIp.c_str(), m_SMS.nPort);
	if(result < 0)
	{
		printf("C_Hall::SendAndRecvInfo TcpConnect %s:%d Fail !\n",m_SMS.strIp.c_str(), m_SMS.nPort);
		return  -1;
	}
	result = m_tcp.BlockSend(send.c_str(), send.size());
	if(result < 0)
	{
		//SetAq10Error(ERROR_PLAYER_AQ_TCPSEND, "Player can not connect! 03");
		return -1;//ERROR_PLAYER_AQ_TCPSEND;
	}

	string http;
	string content;
	result = ReceiveCommand(http, overtime);
	if(result < 0)
		return result;

	result = GetHttpContent(http, content);
	if(result < 0)
		return result;
	else if(result < HttpResponseParser::RecvAndContinue)
	{
		//SetAq10Error(ERROR_PLAYER_AQ_BADHTTPRESPONSE, "bad http response! 01");
		return -1;//ERROR_PLAYER_AQ_BADHTTPRESPONSE;
	}
	else if(result >= HttpResponseParser::Success)
	{//if status code is 200, return.
		recv = http;
		return 0;
	}
	else
		;//if status code is 100

	if(!content.empty())
	{//if one time operatoring receive two responses.
		http = content;
		content.clear();
		result = GetHttpContent(http, content);
		if(result >= HttpResponseParser::Success)
		{//Get second response, return.
			recv = http;
			return 0;
		}
	}

	http.clear();
	content.clear();
	//receive second response.
	int waitTime = ((overtime > 120) ? overtime : 60);
	result = ReceiveCommand(http, waitTime);//wait 60 second
	if(result < 0)
		return result;

	result = GetHttpContent(http, content);
	if(result < 0)
		return result;
	else if(result < HttpResponseParser::Success)
	{
		//SetAq10Error(ERROR_PLAYER_AQ_HTTPUNKNOW, "bad http response! 02");
		return -1;//ERROR_PLAYER_AQ_HTTPUNKNOW;
	}
	else
	{
		recv = http;
		return 0;
	}
}

int C_Hall :: ReceiveCommand(string &recv, int waitTime)
{
	recv.clear();
	char buffer[BufferLength];
	timeval timeOut;
	timeOut.tv_sec = waitTime;
	timeOut.tv_usec = 0;

	int result = 0;
	while((result = m_tcp.SelectRecv(buffer, BufferLength-1, timeOut)) >= 0)
	{
		if(result == 0)
			break;
		buffer[result] = 0;
		recv += buffer;

		//find the terminator.
		if(recv.find("</soap:Envelope>") != string::npos || recv.find("</s:Envelope>") != string::npos ||
			recv.find("</soapenv:Envelope>") != string::npos)
			break;

		timeOut.tv_sec = 1;//1 second
		timeOut.tv_usec = 0;
	}
	if(result < 0)
	{
		//SetAq10Error(ERROR_PLAYER_AQ_TCPRECEIVE, "Player can not connect! 04");
		return -1;//ERROR_PLAYER_AQ_TCPRECEIVE;
	}
	return 0;
}//////////////////////////////////////


int C_Hall :: GetRootChild( const std::string &xml,xercesc::XercesDOMParser *parser, 
								  xercesc::ErrorHandler *errHandler, xercesc::DOMElement **rootChild)
{
	string compactXml = ExtractXml(xml);
	if(compactXml.empty())
		return -1;

	parser->setErrorHandler(errHandler);
	MemBufInputSource input((const XMLByte*)(compactXml.c_str()), compactXml.size(), "xml");
	try
	{
		parser->parse(input);
	}
	catch(...)
	{
		return -2;
	}

	DOMDocument *doc = parser->getDocument();

	if(doc == NULL)
	{
		return -3;
	}
	*rootChild = doc->getDocumentElement();
	return *rootChild == NULL ? -1 : 0;
}

//GET NODE NAME
xercesc::DOMElement *C_Hall :: GetElementByName( const xercesc::DOMNode *elem, const std::string &name)
{
	if(name.empty() || elem == NULL)
		return NULL;

	string source = name;
	for_each(source.begin(), source.end(), ToLower);

	for(DOMElement *child = (DOMElement*)elem; child != NULL;
		child = (DOMElement *)child->getNextSibling())
	{
		char *p = (char *)XMLString::transcode(child->getNodeName());
		string dest = p;
		XMLString::release(&p);

		for_each(dest.begin(), dest.end(), ToLower);
		if(source == dest)
			return child;
	}
	return NULL;
}

xercesc::DOMElement *C_Hall :: FindElementByName( const xercesc::DOMNode *elem, const std::string &name)
{
	if(name.empty() || elem == NULL)
		return NULL;

	string source = name;
	for_each(source.begin(), source.end(), ToLower);

	for(DOMElement *child = (DOMElement*)elem; child != NULL;
		child = (DOMElement *)child->getNextSibling())
	{
		char *p = (char *)XMLString::transcode(child->getNodeName());
		string dest = p;
		XMLString::release(&p);

		for_each(dest.begin(), dest.end(), ToLower);
		if(dest.find(source) != string::npos)
			return child;
	}
	return NULL;
}

string C_Hall::ExtractXml(const string &response)
{
	string xml;
	std::string::size_type beg = response.find('<'), end;
	while(beg != string::npos && (end = response.find('>', beg)) != string::npos)
	{//if node value's first char is '\n' '\t' '\r' or blank, here will make a mistake 
		xml += response.substr(beg, end - beg + 1);
		beg = response.find('<', end);
		if(beg == string::npos)
			break;
		if(!(++end==beg || response[end]==' ' || response[end]=='\n' || response[end]=='\t' || response[end]=='\r'))
			xml += response.substr(end, beg - end);
	}

	return xml;
}