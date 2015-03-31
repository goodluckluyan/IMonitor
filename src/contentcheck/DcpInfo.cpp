/*
*@file:DcpInfo.cpp
*@brief:DCP解析工具函数库
*@author: zhangmiao@oristartech.com
*@date: 2012/06/05
*/
#ifndef __DCPINFO_H__
#include "DcpInfo.h"
#endif
#include <map>
//#define DIR_SEPCHAR '/'
#ifdef WIN32
#define DIR_SEPCHAR '\\'
#undef  _POSIX_
#include <io.h>
#include <sys/stat.h>
//#define atoll atol
#define atoll atof
//#define lstat stat
#define lstat _stat64
#define stat  _stat64
#endif

#ifndef WIN32
#include "para/C_RunPara.h"
#endif
#include "log/C_LogManage.h"

#define  LOGERRFMT(fmt,...)  C_LogManage::GetInstance()->WriteLogFmt(ULOG_ERROR,LOG_MODEL_JOBS,0,0,fmt,##__VA_ARGS__)

using namespace Content;
using namespace Content::Dcp;

DcpInfo::DcpInfo()
{
    memset(pklList,0,sizeof(pklList));
    pklN = 0;
    docRet = NULL;
    parser = NULL;
    output = NULL;
    serializer = NULL;
	impl = NULL;
    dcpSize = 0ll;
   
    assetmapIdVt.clear();
 
    assetmapFile.clear();
    
}
DcpInfo::~DcpInfo()
{
    for(int i=0;PKLInfo::num!=0;i++) 
	{ 
		delete pklList[i];
		pklList[i] = NULL;
	}
    //if(parser!=NULL)parser->release();
    //if(docRet!=NULL)docRet->release();
    //if(output!=NULL)output->release();
    //if(serializer!=NULL)serializer->release();
    
    //XMLPlatformUtils::Terminate();
}

//DcpInfo初始化。
int DcpInfo::init()
{
//#ifdef WIN32
    /*try
    {
       XMLPlatformUtils::Initialize();
    }
    catch (const XMLException& toCatch)
    {
       XERCES_STD_QUALIFIER cerr << "Error during initialization! :\n"
                              << StrX(toCatch.getMessage()) << XERCES_STD_QUALIFIER endl;
       return 1;
    }*/
//#endif

    XMLCh tempStr[100];
    XMLString::transcode("LS",tempStr,99);
    impl = DOMImplementationRegistry::getDOMImplementation(tempStr);

    parser = ((DOMImplementationLS *)impl)->createLSParser(DOMImplementationLS::MODE_SYNCHRONOUS,0);
    if(parser->getDomConfig()->canSetParameter(XMLUni::fgDOMElementContentWhitespace,true))
       parser->getDomConfig()->setParameter(XMLUni::fgDOMElementContentWhitespace,bool(false));
    parser->getDomConfig()->setParameter(XMLUni::fgDOMErrorHandler, (const void*)(&errorHandler));
   
    output = ((DOMImplementationLS*)impl)->createLSOutput();
    //output->setEncoding(gOutputEncoding);
    output->setEncoding(NULL);

    serializer = ((DOMImplementationLS*)impl)->createLSSerializer();
    
    return 0;
}
int DcpInfo::getAssetmapList( const char *path,char **mapList, int *mapN, int max , string& error )
{
    int ret=0;
    
    ret = searchFile(path,"ASSETMAP",mapList,mapN,max);
    if(ret==-2 || ret==0 || ret==ENOENT);
    else {
        //LOGERRFMT("%s\n",strerror(ret));
		char buffer[BUF_SIZE]="";
		sprintf( buffer,"%s\n",strerror(ret) );
		error = buffer;
        return ret;
    }

	//---搜索file assetmap、ASSETMAP.xml 和 assetmap.xml 
	ret = searchFile(path,"assetmap",mapList,mapN,max);
	if(ret==-2 || ret==0 || ret==ENOENT);
	else {
		//LOGERRFMT("%s\n",strerror(ret));
		char buffer[BUF_SIZE]="";
		sprintf( buffer,"%s\n",strerror(ret) );
		error = buffer;
		return ret;
	}

	ret = searchFile(path,"assetmap.xml",mapList,mapN,max);
	if(ret==-2 || ret==0 || ret==ENOENT);
	else {
		//LOGERRFMT("%s\n",strerror(ret));
		char buffer[BUF_SIZE]="";
		sprintf( buffer,"%s\n",strerror(ret) );
		error = buffer;
		return ret;
	}

	ret = searchFile(path,"ASSETMAP.xml",mapList,mapN,max);
	if(ret==-2 || ret==0 || ret==ENOENT);
	else {
		//LOGERRFMT("%s\n",strerror(ret));
		char buffer[BUF_SIZE]="";
		sprintf( buffer,"%s\n",strerror(ret) );
		error = buffer;
		return ret;
	}

    if(*mapN<=0) 
	{
		error = "can not find ASSETMAP!\n";
		LOGERRFMT("can not find ASSETMAP!\n");
		return -1;
	}
    return 0;
}
int DcpInfo::getKdmList(const char *path,char **kdmList, int *kdmN, int max , string& error)
{
    int ret=0;
  
    ret = searchFile2(path,".xml",kdmList,kdmN,max);
    if(ret==-2 || ret==0 || ret==ENOENT);
	else {
		//LOGERRFMT("%s\n",strerror(ret));
		char buffer[BUF_SIZE]="";
		sprintf( buffer,"%s\n",strerror(ret) );
		error = buffer;
		return ret;
	}
    if(*kdmN<=0) 
	{
		error = "can not find kdm!\n";
		LOGERRFMT("can not find kdm!\n");
		return 1;
	}
    //LOGERRFMT("kdmN=%d\n",kdmN);
    return 0;
}



DOMNode *DcpInfo::getNodeByXpath(DOMNode *root,std::vector<std::string> path)
{
    if(root==NULL) return NULL;
    for(size_t i=0;i<path.size();i++)
    {
        //LOGERRFMT("path:%s\n",(*path)[i].c_str());
        //XMLString::transcode((*path)[i].c_str(),temp,256);
        DOMNodeList *childList = root->getChildNodes();
        size_t j;
        for(j=0;j<childList->getLength();j++)
        {
            //std::cout << StrX(temp) << "--" << StrX(childList->item(j)->getNodeName()) <<std::endl;
            if(childList->item(j)->getNodeType()==DOMNode::ELEMENT_NODE 
               && XMLString::compareString(childList->item(j)->getNodeName(),X(path[i].c_str()))==0)
            {
                root = childList->item(j);
                //LOGERRFMT("%s\n",StrX(root->getNodeName()).localForm());
                break;
            }
        }
        if(j==childList->getLength()) return NULL;
    }

    return root;
}

int DcpInfo::ParseInit(int f_schechk,const std::vector<std::string> &schePath)
{
    bool DoNamespaces          = true;
    bool DoSchema              = false;
    bool SchemaFullChecking    = false;
    bool DoValidate            = false; 
    bool UseCacheGrammar       = false; 

    if(f_schechk!=0)
    {
        DoSchema              = true;
        SchemaFullChecking    = true;
        DoValidate            = true; 
        UseCacheGrammar       = true; 
    }

    DOMConfiguration  *config = parser->getDomConfig();
    config->setParameter(XMLUni::fgDOMValidate,DoValidate);
    config->setParameter(XMLUni::fgXercesSchema,DoSchema);
    config->setParameter(XMLUni::fgDOMNamespaces,DoNamespaces);
    config->setParameter(XMLUni::fgXercesSchemaFullChecking,SchemaFullChecking);
    config->setParameter(XMLUni::fgXercesUseCachedGrammarInParse,UseCacheGrammar);

    for(size_t i = 0; (f_schechk!=0)&&(i<schePath.size()); i++)
    {
		//@author zhangmiao@oristartech.com
		//@date [2013-03-08]
		//@brief 增加了异常处理，捕捉xml解析器可能抛出了所有异常。
		//@new
		try
		{
			if(0 == parser->loadGrammar(schePath[i].c_str(),Grammar::SchemaGrammarType, true))
			{
				XERCES_STD_QUALIFIER cout << "Loading grammar error:"
					<< schePath[i].c_str()
					<< XERCES_STD_QUALIFIER endl;
				
				//@author zhangmiao@oristartech.com
				//@date [2014-04-04]
				//@brief 主要增加写错误日志的代码
				//@new
				string errstr = "Loading grammar error:" + schePath[i] + "!\n";
				WDELOG(errstr);
				//@modify end;

				return -1;
			}
		}
		catch (...)
		{
			LOGERRFMT( "Loading grammar error:Throw Exception!\n" );

			//@author zhangmiao@oristartech.com
			//@date [2014-04-04]
			//@brief 主要增加写错误日志的代码
			//@new
			string errstr = "Loading grammar error:Throw Exception!\n";
			WDELOG(errstr);
			//@modify end;

			return -1;
		}
		//@modify end;
    }
    return 0;
}
DOMDocument *DcpInfo::ParseXML(const std::string &xmlFile)
{
    DOMDocument *pDomRet = NULL;

    if(xmlFile.empty()) {LOGERRFMT("papa is NULL!\n");return NULL;}
    
    bool errorOccurred = false;
    try{
        errorHandler.resetErrors();
        pDomRet = parser->parseURI(xmlFile.c_str());
    }
    catch (const XMLException& toCatch)
    {
        XERCES_STD_QUALIFIER cerr << "\nError during parsing: '" << xmlFile << "'\n"
         << "Exception message is:  \n"
         << StrX(toCatch.getMessage()) << "\n" << XERCES_STD_QUALIFIER endl;
         errorOccurred = true;

		 //@author zhangmiao@oristartech.com
		 //@date [2014-04-04]
		 //@brief 主要增加写错误日志的代码
		 //@new
		 string errstr = "\nErrors during parsing: '" + xmlFile + "'\n" + "XMLException message is:  \n" + StrX(toCatch.getMessage()).localForm() + "\n";
		 WDELOG(errstr);
		 //@modify end;
    }
    catch (const DOMException& toCatch)
    {
        const unsigned int maxChars = 2047;
        XMLCh errText[maxChars + 1];
        XERCES_STD_QUALIFIER cerr << "\nDOM Error during parsing: '" << xmlFile << "'\n"
        << "DOMException code is:  " << toCatch.code << XERCES_STD_QUALIFIER endl;
        if (DOMImplementation::loadDOMExceptionMsg(toCatch.code, errText, maxChars))
        XERCES_STD_QUALIFIER cerr << "Message is: " << StrX(errText) << XERCES_STD_QUALIFIER endl;
        errorOccurred = true;

		//@author zhangmiao@oristartech.com
		//@date [2014-04-04]
		//@brief 主要增加写错误日志的代码
		//@new
		string errstr = "\nDOM during parsing: '" + xmlFile + "'\n" + "Exception message is:  \n" + StrX(errText).localForm() + "\n";
		WDELOG(errstr);
		//@modify end;
     }
    catch (...)
    {
        XERCES_STD_QUALIFIER cerr << "\nUnexpected exception during parsing: '" << xmlFile << "'\n";
        errorOccurred = true;

		//@author zhangmiao@oristartech.com
		//@date [2014-04-04]
		//@brief 主要增加写错误日志的代码
		//@new
		string errstr = "\nUnexpected exception during parsing: '" + xmlFile + "'\n";
		WDELOG(errstr);
		//@modify end;
    }
    if(errorHandler.getSawErrors())
    {
        XERCES_STD_QUALIFIER cout << "\nErrors occurred:" << xmlFile << XERCES_STD_QUALIFIER endl;
        errorOccurred = true;

		//@author zhangmiao@oristartech.com
		//@date [2014-04-04]
		//@brief 主要增加写错误日志的代码
		//@new
		string errstr = "\nErrors occurred:" + xmlFile + "\n";
		WDELOG(errstr);
		//@modify end;
    }
    if(errorOccurred) return NULL;

    return pDomRet;
}

//----zhangmiao:begin--20120605---
int DcpInfo::ReBuild_Assetmap(  std::string xmlFile, const string &pklUuid, pkl_Info_t& aPkl , string& error )
{
	int ret=0;

	if(ret!=init()) 
	{
		error = "DcpInfo::init() fail.\n";
#ifdef _TEST_
		LOGERRFMT("fail to init()!\n");
#endif
		return -1;
	}

	XMLSize_t nodeNum;
	//bool errorOccurred = false;
	DOMDocument *docAsset = NULL;
	DOMNode *tmpN = NULL;
	std::vector<std::string> path;

	path.clear();
	if( ParseInit(0,path) == -1 ) 
	{
		error = "ReBuild_Assetmap fail to ParseInit()!\n";
		return -1;
	}
	docAsset = ParseXML(xmlFile);
	if(docAsset==NULL) 
	{
		char buffer[BUF_SIZE]="";
		sprintf( buffer,"ReBuild_Assetmap fail to ParseXML()!\n parse(%s) is error!\n",xmlFile.c_str() );
		error = buffer;
		LOGERRFMT("parse(%s) is error!\n",xmlFile.c_str());
		return -1;
	}

	DOMElement *rootAsset = docAsset->getDocumentElement();
	//@author zhangmiao@oristartech.com
	//@date [2013-07-15]
	//@brief 生成新的AssetMap Uuid
	//主要增加了相关字幕文件的处理代码
	//@new
	//首先解析出AssetMap Uuid
	path.clear();path.push_back("Id");
	tmpN = getNodeByXpath(rootAsset,path);
	if(tmpN==NULL)
	{
		error += "ReBuild_Assetmap(AssetMap Id) is error!\n";
		LOGERRFMT("ReBuild_Assetmap(AssetMap Id) is error!\n");
		return -1;
	}
	string AssetMapUuid = "urn:uuid:";
	uuid_t uuid;
	uuid_generate_random(uuid);
	char tmp[3] = "";
	for(int i=0; i<16; i++)
	{
		sprintf(tmp, "%02x", uuid[i]);
		AssetMapUuid += tmp;
		if(i==3||i==5||i==7||i==9)
			AssetMapUuid += "-";
	}
	tmpN->setTextContent(X(AssetMapUuid.c_str()));
	//@modify end;

	//@author zhangmiao@oristartech.com
	//@date [2014-09-15]
	//@brief 生成新的AssetMap文件的<AnnotationText>内容格式：AssetMapUuid _+原始AnnotationText内容，最长255个字符
	//@new
	string new_AssetmapId;
	new_AssetmapId = AssetMapUuid;
	Remove_PrefixOfUuid( new_AssetmapId );

	DOMNode *tmpN_AnnotationText = NULL;
	path.clear();
	path.push_back("AnnotationText");
	tmpN_AnnotationText = getNodeByXpath(rootAsset,path);
	if( tmpN_AnnotationText == NULL )
	{
		error += "ReBuild_Assetmap(AssetMap AnnotationText) is Null!\n";
		LOGERRFMT("ReBuild_Assetmap(AssetMap AnnotationText) is Null!\n");
		//return -1;
	}
	else
	{
		string AnnotationText = StrX(tmpN_AnnotationText->getTextContent()).localForm();
		AnnotationText = new_AssetmapId + "_" + AnnotationText;
		if ( AnnotationText.length() > 255 )
		{
			AnnotationText = AnnotationText.substr( 0 , 255 );
		}

		tmpN_AnnotationText->setTextContent(X(AnnotationText.c_str()));
	}
	//@modify [2014-09-15] end;

	DOMNodeList *nlist = docAsset->getElementsByTagName(X("AssetList"));
	nodeNum = nlist->getLength(); 
	//LOGERRFMT("nodeNum=%d\n",nodeNum);
	if(nodeNum==0)
	{
		error = "Parse Assetmap error:not find AssetList!\n";
#ifdef _TEST_
		LOGERRFMT("Parse Assetmap error:not find AssetList!\n");
#endif
		return -1;
	}

	DOMNode* cloneNode = NULL;
	DOMNode* tmpCloneNode = NULL;
	//copy a empty AssetMap's AssetList 
	DOMNode* nAssetList = nlist->item(0);
	if (nAssetList)
	{
		cloneNode = nAssetList->cloneNode(false);
	}

    nlist = docAsset->getElementsByTagName(X("Asset"));
	nodeNum = nlist->getLength(); 
	XMLSize_t i=0;
	//insert pkl file info.
	while( i < nlist->getLength() )
	{
		//DOMNode *nParent = nlist->item(i)->getParentNode();
		//读取每个<Asset>的uuid
		DOMNode *nAssetChild = nlist->item(i);
		path.clear();path.push_back("Id");
		tmpN = getNodeByXpath(nAssetChild,path);
		if(tmpN==NULL) 
		{
			error = "parseAssetmap(pklId) is error!\n";
			LOGERRFMT("parseAssetmap(pklId) is error!\n");
			return -1;
		}
		std::string pklId = StrX(tmpN->getTextContent()).localForm();
		
		Remove_PrefixOfUuid( pklId );
		
		if (pklId == pklUuid)	//如果找到pkl文件所在的节点，则复制该节点，并把它插入到新的AssetMap's AssetList节点表中
		{
			tmpCloneNode = nAssetChild->cloneNode(true);	//复制该节点
			cloneNode->appendChild(tmpCloneNode);	//插入到新的AssetMap's AssetList节点表中
			break;
		}
		else
		{
			i++;
		}
	}

	//@author zhangmiao@oristartech.com
	//@date [2015-01-14]
	//@new
	//主要改进重建AssetMap文件处理代码的性能
	std::map< string , XMLSize_t > aMap_AssetMap_AssetList;
	for ( XMLSize_t m = 0; m < nlist->getLength(); m++ )
	{
		DOMNode *tmpNode = NULL;
		DOMNode *TmpAsset = nlist->item( m );
		path.clear();
		path.push_back("Id");
		tmpNode = getNodeByXpath( TmpAsset , path );
		if(tmpNode==NULL) 
		{
			error = "parse AssetMap(Asset Id) is error!\n";
			LOGERRFMT("parse AssetMap(Asset Id) is error!\n");
			return -1;
		}

		string sAssetId = StrX(tmpNode->getTextContent()).localForm();

		Remove_PrefixOfUuid( sAssetId );

		aMap_AssetMap_AssetList[ sAssetId ] = m;
	}
    //@modify end;

	//insert other asset file info.
	vector<asset_pkl_t> vAsset_pkl = aPkl.asset;
	vector<asset_pkl_t>::size_type nSize_Asset_Pkl = vAsset_pkl.size();
	for ( vector<asset_pkl_t>::size_type k=0; k< nSize_Asset_Pkl; k++ )
	{
		asset_pkl_t aAsset_Pkl = vAsset_pkl[k];
		string fileUuid = aAsset_Pkl.uuid;

#if 1
		//主要改进重建AssetMap文件处理代码的性能
		map< string , XMLSize_t >::iterator it = aMap_AssetMap_AssetList.find( fileUuid );
		if( it != aMap_AssetMap_AssetList.end() ) 
		{
			XMLSize_t nIndex = ((XMLSize_t)~((XMLSize_t)0));   //-1 =>#define MAXSIZE_T   ((SIZE_T)~((SIZE_T)0))
			nIndex = it->second;

			if( nIndex == ((XMLSize_t)~((XMLSize_t)0)) ) /*-1*/
				continue;

			//读取每个找到的<Asset>的uuid
			DOMNode *pAssetChild = nlist->item( nIndex );
			tmpCloneNode = pAssetChild->cloneNode(true);	//复制该节点
			cloneNode->appendChild(tmpCloneNode);	//插入到新的AssetMap's AssetList节点表中
		}
#else
		i=0;
		while( i < nlist->getLength() )
		{
			//DOMNode *nParent = nlist->item(i)->getParentNode();
			//读取每个<Asset>的uuid
			DOMNode *nAssetChild = nlist->item(i);
			path.clear();path.push_back("Id");
			tmpN = getNodeByXpath(nAssetChild,path);
			if(tmpN==NULL) 
			{
				error = "parseAssetmap(pklId) is error!\n";
				LOGERRFMT("parseAssetmap(pklId) is error!\n");
				return -1;
			}
			std::string Id = StrX(tmpN->getTextContent()).localForm();

			Remove_PrefixOfUuid( Id );

			if ( Id == fileUuid )	//如果找到vAsset_pkl中匹配的节点，则复制该节点，并把它插入到新的AssetMap's AssetList节点表中
			{
				tmpCloneNode = nAssetChild->cloneNode(true);	//复制该节点
				cloneNode->appendChild(tmpCloneNode);	//插入到新的AssetMap's AssetList节点表中
				break;
			}
			else
			{
				i++;
			}
		}
#endif
	}

	//@author zhangmiao@oristartech.com
	//@date [2012-12-21]
	//@new
	//判断重新生成的AssetMap的AssetList节点表，是否为空
	if ( cloneNode == NULL )
	{
		error = "ReBuild_Assetmap error: cloneNode is Null !\n";
		LOGERRFMT("ReBuild_Assetmap error: cloneNode is Null !\n");
		return -1;
	}
	else
	{
		DOMNodeList* nChildNodesList = cloneNode->getChildNodes();
		if ( nChildNodesList == NULL )
		{
			error = "ReBuild_Assetmap error: ChildNodesList Of cloneNode is Null!\n";
			LOGERRFMT("ReBuild_Assetmap error: ChildNodesList Of cloneNode is Null!\n");
			return -1;
		}
	}
	//@modify end;

	nodeNum = cloneNode->getChildNodes()->getLength();
#ifdef _TEST_
	LOGERRFMT("nodeNum=%d\n",nodeNum);
#endif
	if(nodeNum!=0)
	{
		rootAsset->replaceChild( cloneNode, nAssetList ) ;	//用新的AssetMap的AssetList节点表，替换原有的AssetList
		//写入文件操作
		XMLFormatTarget *formatTarget;	
#if 1
		formatTarget = new LocalFileFormatTarget(xmlFile.c_str());
		output->setByteStream(formatTarget);
		serializer->getDomConfig()->setParameter(X("format-pretty-print"), true);
		serializer->write(docAsset, output);
		delete formatTarget;
#endif
	}
	
	return 0;
}

int DcpInfo::Parse_Assetmap_New( std::string xmlFile ,AMInfo& aAMInfo , string& error )
{
	if( 0!= init() )
	{
		error += "DcpInfo::init() fail.\n";
		LOGERRFMT("fail to init()!\n");
		return -1;
	}

	assetmap_Info assetmap_info;

	XMLSize_t nodeNum = 0;
	bool errorOccurred = false;
	DOMDocument *docAsset = NULL;
	std::vector<std::string> path;

	path.clear();
	if( ParseInit(0,path) == -1 ) 
	{
		error += "fail to ParseInit()!\n";
		return -1;
	}
	docAsset = ParseXML(xmlFile);
	if(docAsset==NULL) 
	{
		char buffer[BUF_SIZE]="";
		sprintf( buffer,"Parse_Assetmap_New fail to ParseXML()!\n parse(%s) is error!\n",xmlFile.c_str() );
		error += buffer;
		LOGERRFMT("parse(%s) is error!\n",xmlFile.c_str());
		return -1;
	}

	DOMElement *rootAsset = docAsset->getDocumentElement();

	//首先解析出AssetMap Uuid
	path.clear();path.push_back("Id");
	DOMNode *tmpN = getNodeByXpath(rootAsset,path);
	if(tmpN==NULL)
	{
		error += "parseAssetmap(AssetId) is error!\n";
		LOGERRFMT("parseAssetmap(AssetId) is error!\n");
		return -1;
	}
	std::string assetmapId = StrX(tmpN->getTextContent()).localForm();
	
	Remove_PrefixOfUuid( assetmapId );

	//Assetmap文件 如果重复出现就不在解析
	for(size_t i=0;i<assetmapIdVt.size();i++) //remember the assetmapId,not send the same id;
	{
		if(assetmapIdVt[i]==assetmapId)
		{
			char buffer[BUF_SIZE]="";
			sprintf( buffer,"Assetmap is repeat!%s\n",xmlFile.c_str() );
			error += buffer;
			LOGERRFMT("Assetmap is repeat!%s\n",xmlFile.c_str());
			return -1;
		}
	}

	assetmap_info.assetmapUuid = assetmapId;

	//对AssetList中的每个Asset进行逐个解析
	assetmapFile.clear();//assetmapFile在解析pkl时会用到，在Parse_Pkl_New()中会调用，避免重复解析
	//找出所有的<Path>节点，进行解析
	DOMNodeList *pathList = docAsset->getElementsByTagName(X("Path"));
	nodeNum = pathList->getLength(); 
	//LOGERRFMT("nodeNum=%d\n",nodeNum);
	if(nodeNum==0)
	{
		error += "parseAssetmap(path) is error!\n";
		LOGERRFMT("parseAssetmap(path) is error!\n");
		return -1;
	}
	for(size_t i=0;i<nodeNum;i++)
	{
		DOMNode *tmpPath = pathList->item(i);

		DOMNode *tmpN = pathList->item(i);
		if(tmpN==NULL)
		{
			error += "parseAssetmap(path) is error!\n";
			LOGERRFMT("parseAssetmap(path) is error!\n");
			return -1;
		}
		std::string fileName = StrX(tmpN->getTextContent()).localForm();
		//@author zhangmiao@oristartech.com
		//@date [2013-07-15]
		//主要增加了相关字幕文件的处理代码
		//@new
		std::string PathfileName = StrX(tmpN->getTextContent()).localForm();
#ifdef WIN32
		//如果带目录'/',会将最后的'/'替换为'\'
		string::size_type pos = 0;
		pos = fileName.rfind('/');
		if(pos != string::npos)
			fileName.replace(pos,1,"\\");
#endif //WIN32
		//@modify end;
		

		//@author zhangmiao@oristartech.com
		//@date [2012-12-21]
		//@new
		//判定<Path>的父节点<Asset>是否存在
		if ( tmpN->getParentNode() != NULL )
		{
			DOMNode *tmpNODE = tmpN->getParentNode();
			if ( tmpNODE->getParentNode() != NULL )
			{
				if (tmpNODE->getParentNode()->getParentNode() == NULL )
				{
					error += "parseAssetmap( path: getParentNode() ) is error!\n";
					LOGERRFMT("parseAssetmap( path: getParentNode() ) is error!\n");
					return -1;
				}
			}
			else
			{
				error += "parseAssetmap( path: getParentNode() ) is error!\n";
				LOGERRFMT("parseAssetmap( path: getParentNode() ) is error!\n");
				return -1;
			}
		}
		else
		{
			error += "parseAssetmap( path: getParentNode() ) is error!\n";
			LOGERRFMT("parseAssetmap( path: getParentNode() ) is error!\n");
			return -1;
		}
		//@modify end;

		//将<Asset>父节点下的<Id>取出来；即读取每个文件的UUID
		path.clear();path.push_back("Id");
		tmpN = getNodeByXpath(tmpN->getParentNode()->getParentNode()->getParentNode(),path);
		if(tmpN==NULL)
		{
			error += "parseAssetmap(fileId) is error!\n";
			LOGERRFMT("parseAssetmap(fileId) is error!\n");
			return -1;
		}
		std::string fileId = StrX(tmpN->getTextContent()).localForm();
		
		Remove_PrefixOfUuid( fileId );

		std::string sDir = xmlFile.substr(0,xmlFile.rfind(DIR_SEPCHAR)+1);
		file_Info_t fileInfo;
		fileInfo.f_path = sDir + fileName;
		fileInfo.f_id   = fileId;
		//@author zhangmiao@oristartech.com
		//@date [2013-07-15]
		//主要增加了相关字幕文件的处理代码
		//@new
		fileInfo.f_PathOri = PathfileName;
		//@modify end;
		assetmapFile.push_back(fileInfo);

		//----zhangmiao---parseAssetmap(Length)-------
		//将每个文件<Length>解析出来
		unsigned long long AssetFileLength = 0;
		path.clear();path.push_back("Length");
		if(tmpPath==NULL)
		{
			error += "parseAssetmap(path) is error!\n";
			LOGERRFMT("parseAssetmap(path) is error!\n");
			return -1;
		}
		//const XMLCh * n=  tmpPath->getParentNode()->getNodeName();
		tmpN = getNodeByXpath(tmpPath->getParentNode(),path);
		if(tmpN==NULL)
		{
			error += "parseAssetmap(Length) is error:"+fileName+" not find Length !\n";
			LOGERRFMT("parseAssetmap(Length) is error:%s not find Length !\n" , fileName.c_str() );
			//return -1;
		}
		else
		{
			std::string fileLength = StrX(tmpN->getTextContent()).localForm();
			AssetFileLength =atoll(fileLength.c_str());
		}
        
		asset_assetmap_t asset_assetmap;
		asset_assetmap.uuid = fileId;

		//std::string sDir = xmlFile.substr(0,xmlFile.length()-8);	
		//@author zhangmiao@oristartech.com
		//@date [2013-07-15]
		//主要增加了相关字幕文件的处理代码
		//@new
		asset_assetmap.Path_NotFullPath = PathfileName;
		//@modify end;

		asset_assetmap.path = sDir + fileName;
		asset_assetmap.Length = AssetFileLength;
		
		assetmap_info.asset.push_back(asset_assetmap);
		//---------------
	}
	if(errorOccurred)
	{
		return -1;
	}
	assetmap_InfoVt.push_back(assetmap_info);

	aAMInfo.aAssetmap_Info = assetmap_info;
	aAMInfo.assetmapName = xmlFile;
	aAMInfo.dcpPath = xmlFile.substr(0,xmlFile.rfind(DIR_SEPCHAR)+1);

	//对pkl的相关文件信息做解析，用于下一步对pkl文件解析
	for(int i=0;PKLInfo::num!=0;i++) 
	{
		LOGERRFMT("delete pklList[%d]\n",i);
		delete pklList[i];
		pklList[i] = NULL;
	}

	pklN = 0;
	DOMNodeList *nlist = docAsset->getElementsByTagName(X("PackingList"));
	nodeNum = nlist->getLength(); 
	//LOGERRFMT("nodeNum=%d\n",nodeNum);
	if(nodeNum==0)
	{
		error += "Parse Assetmap error:not find PackingList!\n";
		LOGERRFMT("Parse Assetmap error:not find PackingList!\n");
		return -1;
	}
	for(size_t i=0;i<nodeNum;i++)
	{
		//获得<PackingList>的父节点<Asset>，并判定是否存在
		DOMNode *nParent = nlist->item(i)->getParentNode();
		//@author zhangmiao@oristartech.com
		//@date [2012-12-21]
		//@new
		if ( nParent == NULL )
		{
			error += "Parse Assetmap error:not find ParentNode Of PackingList!\n";
			LOGERRFMT("Parse Assetmap error:not find ParentNode Of PackingList!\n");
			return -1;
		}
		//@modify end;
		
		//读取pkl文件的Path，大多是个文件名
		path.clear();
		path.push_back("ChunkList");path.push_back("Chunk");path.push_back("Path");
		tmpN = getNodeByXpath(nParent,path);
		if(tmpN==NULL) 
		{
			error += "parseAssetmap(chunk,path) is error!\n";
			LOGERRFMT("parseAssetmap(chunk,path) is error!\n");
			return -1;
		}
		std::string pklPath = StrX(tmpN->getTextContent()).localForm();
		std::string pklDir = xmlFile.substr(0,xmlFile.rfind(DIR_SEPCHAR)+1);

		//读取PKL文件的Uuid
		path.clear();path.push_back("Id");
		tmpN = getNodeByXpath(nParent,path);
		if(tmpN==NULL) 
		{
			error += "parseAssetmap(pklId) is error!\n";
			LOGERRFMT("parseAssetmap(pklId) is error!\n");
			return -1;
		}
		std::string pklId = StrX(tmpN->getTextContent()).localForm();

		Remove_PrefixOfUuid( pklId );

		if(++pklN>PKL_N_MAX) //目前最多支持64个
		{
			error += "pklN>PKL_N_MAX is error!\n";
			LOGERRFMT("%d\n",pklN);
			return -1;
		}
		PKLInfo *p = new PKLInfo();
		if(p==NULL) 
		{
			error += "new() PKLInfo is failed!\n";
			LOGERRFMT("new() PKLInfo is failed!\n");
			return -1;
		}
		//将pkl文件的uuid和path记录下来
		pklList[PKLInfo::num-1] = p;
		pklList[PKLInfo::num-1]->path = pklDir + pklPath;
		pklList[PKLInfo::num-1]->id = pklId;
	}
 
	assetmapIdVt.push_back(assetmapId);
	return 0;
}

int DcpInfo::Parse_Pkl_New( PKLInfo *pkl , dcp_Info_t& dcp , string& error )
{
	if( 0!= init() )
	{
		error = "DcpInfo::init() fail.\n";
		LOGERRFMT("fail to init()!\n");
		return -1;
	}

	pkl_Info_t aPkl_Info;
	

	std::vector<std::string> path;
	int i,nodeNum = 0;
	//XMLCh *xmlC = NULL;
	bool errorOccurred = false;

	path.clear();
	if(-1==ParseInit(0,path))
	{
		error = "Pkl fail to ParseInit()!\n";
		return -1;
	}
	DOMDocument *docPkl = ParseXML((pkl->path).c_str());
	if(docPkl==NULL)
	{
		char buffer[BUF_SIZE]="";
		sprintf( buffer,"pkl fail to ParseXML()!\n parse(%s) is error!\n",(pkl->path).c_str() );
		error = buffer;
		LOGERRFMT("parse(%s) is error!\n",(pkl->path).c_str());
		return 1;
	}

	aPkl_Info.uuid = pkl->id;

	DOMElement *rootEle = docPkl->getDocumentElement();

	//读取节点<AnnotationText>的值
	path.clear();path.push_back("AnnotationText");
	DOMNode    *tmpN = getNodeByXpath(rootEle,path);
	if(tmpN!=NULL) 
	{
		std::string annoText = StrX(tmpN->getTextContent()).localForm();
		aPkl_Info.annotationText = annoText;
	}
	else 
	{
		//@author zhangmiao@oristartech.com
		//@date 2013-06-19
		//@brief 如果<AnnotationText>可选元素不存在，则程序不报错。:begin
		//说明:pkl文件中<PackingList>下的<AnnotationText>元素为可选标记，因此在此处先不报错返回。
		//@new		
		//errorOccurred = true;

		char buffer[BUF_SIZE] = "";
		sprintf( buffer , "Warning:Parse PKL Element<AnnotationText> NOT exist in <PackingList>(%s)\n" , (pkl->path).c_str() );
		error += buffer;
		LOGERRFMT( "%s" , buffer );
		//@modify end;
	}

	//逐个解析节点<IssueDate>，<Issuer>，<Creator>
	path.clear();
	path.push_back("IssueDate");path.push_back("Issuer");path.push_back("Creator");
	for(size_t j=0;(errorOccurred==false)&&(j<path.size());j++)
	{
		std::vector<std::string> tmpV(1,path[j]);
		DOMNode    *tmpN = getNodeByXpath(rootEle,tmpV);
		if(tmpN==NULL)
		{
			error = "getNodeByXpath is NULL\n";
			LOGERRFMT("getNodeByXpath is NULL\n");
			errorOccurred = true;
		}

		std::string aText = StrX(tmpN->getTextContent()).localForm();
		if (path[j]=="IssueDate")
		{
			aPkl_Info.issueDate = aText;
		}
		else if (path[j]=="Issuer")
		{
			aPkl_Info.issuer = aText;
		}
		else if(path[j]=="Creator")
		{
			aPkl_Info.creator = aText;
		}
	}
	//如果节点不存在，出错返回
	if(errorOccurred)
	{
		return 1;
	}

	dcpSize = 0ull;	//用于保存pkl包的大小

	//搜索所有的<Asset>节点
	DOMNodeList *nList = docPkl->getElementsByTagName(X("Asset"));
	nodeNum = nList->getLength(); 
	//LOGERRFMT("Asset num=%d\n",nodeNum);
	if(nodeNum==0)
	{
		error = "error:Asset num = 0\n";
		errorOccurred = true;
	}

	//逐个解析<Asset>节点，保存到aPkl_Info.asset结构中
	for(i=0;(errorOccurred==false)&&(i<nodeNum);i++)
	{
		asset_pkl_t aAsset_Pkl;

		std::string idValue   = "";
		std::string pathValue = "";
		std::string typeValue = "";
		std::string hashValue = "";
		//@author zhangmiao@oristartech.com
		//@date [2013-07-15]
		//主要增加了相关字幕文件的处理代码
		std::string Path_NotFullPath = "";
		//@modify end;

		unsigned long long  nSizeValue =0ull;

		DOMNode *tmpN = nList->item(i);

		//逐个解析<Asset>节点下面的元素 <Id>，<Size>，<Type>
		path.clear();
		path.push_back("Id");path.push_back("Size");path.push_back("Type");
		for(size_t j=0;j<path.size();j++)
		{
			XMLCh xmlT[1024] = {0};  
			std::vector<std::string> tmpV(1,path[j]);
			DOMNode    *tmpN = getNodeByXpath(nList->item(i),tmpV);
			if(tmpN==NULL)
			{
				error = "getNodeByXpath(Asset)\n";
				LOGERRFMT("getNodeByXpath(Asset)\n");
				errorOccurred=true;
				break;
			}

			XMLString::copyString(xmlT,tmpN->getTextContent());
			
			if(path[j]=="Id")
			{
				idValue = StrX(xmlT).localForm();
				Remove_PrefixOfUuid(idValue); 
			}
			if(path[j]=="Size")
			{
				nSizeValue = atoll(StrX(xmlT).localForm());
				dcpSize += atoll(StrX(xmlT).localForm());	//计算pkl包中所有文件大小总和
			}
			if(path[j]=="Type")typeValue = StrX(xmlT).localForm();
		}
		if(errorOccurred)
		{
			break;
		}
		//下一步直接从缓存assetmapFile列表中来读取idValue对应的path
		for(size_t j=0;j<assetmapFile.size();j++)
		{
			if(idValue==assetmapFile[j].f_id)
			{
				pathValue = assetmapFile[j].f_path;
				//@author zhangmiao@oristartech.com
				//@date [2013-07-15]
				//主要增加了相关字幕文件的处理代码
				Path_NotFullPath = assetmapFile[j].f_PathOri;
				//@modify end;
				break;
			}
		}
		if(pathValue.empty())
		{
			errorOccurred = true;
			break;
		}
		
		//cpl the sha1 check;
		//if(0!=typeValue.compare("text/xml;asdcpKind=CPL")) continue;

		//再读取出 <Hash>对应的hashValue
		path.clear();path.push_back("Hash");
		tmpN = getNodeByXpath(nList->item(i),path);
		if(tmpN==NULL)
		{
			char buffer[BUF_SIZE]="";
			sprintf( buffer,"getNodeByXpath %s is failed!\n",path[0].c_str() );
			error = buffer;

			LOGERRFMT("getNodeByXpath %s is failed!\n",path[0].c_str());
			errorOccurred = true;
			break;
		}
		hashValue = StrX(tmpN->getTextContent()).localForm();
		//最后将数据都保存入asset_pkl_t结构中
		aAsset_Pkl.hash = hashValue;
		aAsset_Pkl.type = typeValue;
		aAsset_Pkl.uuid = idValue;
		aAsset_Pkl.originalFileName = pathValue;
		aAsset_Pkl.size = nSizeValue;

		//@author zhangmiao@oristartech.com
		//@date [2013-07-15]
		//主要增加了相关字幕文件的处理代码
		aAsset_Pkl.Path_NotFullPath = Path_NotFullPath;
		//@modify end;

		aPkl_Info.asset.push_back(aAsset_Pkl);
	}

	if(errorOccurred)
	{
		return 1;
	}

	char strT[32];
	sprintf(strT,"%lld",dcpSize);

	aPkl_Info.allFileLength = dcpSize;

	pkl_Info_Vt.push_back(aPkl_Info);

	dcp.aPkl = aPkl_Info;

	return 0;
}

int DcpInfo::Parse_Cpl_New( const char *cplPath, cpl_Info_t &cpl , string& error )
{
	std::vector<std::string> path;
	DOMNode *tmpN = NULL;
	//首先进行cpl的schema验证
#ifndef WIN32
	std::string strExePath;
	C_RunPara *pRunPara = C_RunPara::GetInstance();
	pRunPara->GetExePath(strExePath);
	path.push_back(strExePath + "/cplSchema/xmldsig-core-schema.xsd");
	path.push_back(strExePath + "/cplSchema/xml.xsd");
	path.push_back(strExePath + "/cplSchema/CPL_schema.xsd");
	path.push_back(strExePath + "/cplSchema/SMPTE-429-7-2006-CPL.xsd");
	path.push_back(strExePath + "/cplSchema/SMPTE-429-10-2008-Main-Stereo-Picture-CPL.xsd");
	path.push_back(strExePath + "/cplSchema/Asdcp-Main-Stereo-Picture-CPL.xsd");
#else
	path.push_back("./cplSchema/xmldsig-core-schema.xsd");
	path.push_back("./cplSchema/xml.xsd");
	path.push_back("./cplSchema/CPL_schema.xsd");
	path.push_back("./cplSchema/SMPTE-429-7-2006-CPL.xsd");
	path.push_back("./cplSchema/SMPTE-429-10-2008-Main-Stereo-Picture-CPL.xsd");
	path.push_back("./cplSchema/Asdcp-Main-Stereo-Picture-CPL.xsd");
#endif
	if( ParseInit(1,path) == -1 )
	{
		error += "Cpl error:parse init is error!\n";
		LOGERRFMT("parse init is error!\n");
		return -1;
	}
	string strXml = cplPath;
	DOMDocument *docCpl = ParseXML(strXml);
	if(docCpl==NULL) 
	{
		char buffer[BUF_SIZE]="";
		sprintf( buffer,"parse(%s) is error!\n",cplPath );
		error += buffer;
		LOGERRFMT("parse(%s) is error!\n",cplPath);
		return -2;
	}

	DOMElement *root = docCpl->getDocumentElement();

	//对cpl文件的如下节点进行解析 "Id","AnnotationText","IssueDate","Issuer","Creator","ContentTitleText","ContentKind"
	string nameTmp[7]  = {"Id","AnnotationText","IssueDate",
		"Issuer","Creator","ContentTitleText",
		"ContentKind"};
	string *valueTmp[7] = {&cpl.uuid,       &cpl.annotationText,&cpl.issueDate,
		&cpl.issuer,     &cpl.creator,       &cpl.contentTitleText,
		&cpl.contentKind}; 

	for(int i=0;i<7;i++)
	{
		path.clear();path.push_back(nameTmp[i]);
		tmpN = getNodeByXpath(root,path);
		if(tmpN!=NULL)
			*valueTmp[i] = StrX(tmpN->getTextContent()).localForm();
		else
			*valueTmp[i] = "";
		
	}

	//过滤cpl uuid的前缀"urn:uuid:"
	std::string::size_type pos;
	if((pos=cpl.uuid.find("urn:uuid:"))!=string::npos)
		cpl.uuid.erase(pos,9);
	cpl.path = cplPath;
	//cpl.asset.clear();

	//我们按照可能出现的如下节点的类型，分类搜索，逐个解析
	string nodeName[4] = { "MainPicture","MainSound","MainSubtitle","MainStereoscopicPicture" }; //"MainMarkers","###other" "msp-cpl:MainStereoscopicPicture"
	string nodeType[4] = { "MainPicture", "MainSound", "MainSubtitle", "MainStereoscopicPicture" }; //"MainMarkers","###other"
	for(int k=0;k<4;k++)
	{
		//DOMNodeList *lMain = docCpl->getElementsByTagName(X(nodeName[k].c_str()));
		DOMNodeList *lMain = docCpl->getElementsByTagNameNS( X("*") , X(nodeName[k].c_str()) );
		int nMain = lMain->getLength();
		//LOGERRFMT("nMain=%d\n",nMain);
		if(nMain!=0)
		{
			//unsigned long long duration = 0lu;
			//unsigned long long fRate = 0lu;
			//然后对下面的子节点进行解析
			asset_cpl_t assetTmp;
			for(int i=0;i<nMain;i++)
			{
				//搜索、读取下面节点的值，如果不存在，就填充"空串"
				string nameTmp[10] ={"Id","AnnotationText","EditRate",
					"IntrinsicDuration","EntryPoint","Duration",
					"KeyId","FrameRate","ScreenAspectRatio","Language"};
				string *valueTmp[10]={&assetTmp.uuid,&assetTmp.annotationText,&assetTmp.editRate,
					&assetTmp.intrinsicDuration,&assetTmp.entryPoint,&assetTmp.duration,
					&assetTmp.keyId,&assetTmp.frameRate,&assetTmp.screenAspectRatio,&assetTmp.language};
				
				for(int j=0;j<10;j++)
				{
					path.clear();path.push_back(nameTmp[j]);
					tmpN = getNodeByXpath(lMain->item(i),path);
					if(tmpN!=NULL)
						*valueTmp[j] = StrX(tmpN->getTextContent()).localForm();
					else
						*valueTmp[j] = "";
					
				}

				//过滤uuid的前缀"urn:uuid:"
				std::string::size_type pos;
				if((pos=assetTmp.uuid.find("urn:uuid:"))!=string::npos)
					assetTmp.uuid.erase(pos,9);
				if((pos=assetTmp.keyId.find("urn:uuid:"))!=string::npos)
					assetTmp.keyId.erase(pos,9);

				//填充节点类型名
				assetTmp.kind = nodeType[k];
				cpl.asset.push_back(assetTmp);
			}
		}
	}
	return 0;
}

int DcpInfo::GetKdmInfo( const std::string& KdmFileName, KdmInfo& kdmInfo, string& error )
{
	int ret = 0;

	if( 0!= init() )
	{
		error = "DcpInfo::init() fail.\n";
		LOGERRFMT("fail to init()!\n");
		return -1;
	}

	//首先进行Kdm文件的schema验证
	vector<string> pathVt;
	pathVt.clear();
#ifndef WIN32
	std::string strExePath;
	C_RunPara *pRunPara = C_RunPara::GetInstance();
	pRunPara->GetExePath(strExePath);
	pathVt.push_back(strExePath + "/kdmSchema/etm.xsd");
	pathVt.push_back(strExePath + "/kdmSchema/kdm.xsd");
	pathVt.push_back(strExePath + "/kdmSchema/xml.xsd");
	pathVt.push_back(strExePath + "/kdmSchema/xmldsig-core-schema.xsd");
	pathVt.push_back(strExePath + "/kdmSchema/xenc-schema.xsd");
	pathVt.push_back(strExePath + "/kdmSchema/KDM_schema.xsd");
#else
	pathVt.push_back("./kdmSchema/etm.xsd");
	pathVt.push_back("./kdmSchema/kdm.xsd");
	pathVt.push_back("./kdmSchema/xml.xsd");
	pathVt.push_back("./kdmSchema/xmldsig-core-schema.xsd");
	pathVt.push_back("./kdmSchema/xenc-schema.xsd");
	pathVt.push_back("./kdmSchema/KDM_schema.xsd");
#endif
	if( -1==ParseInit(1,pathVt) )
	{
		error = "Kdm error:parse init is error!\n";
		if(parser!=NULL)parser->release();
		if(output!=NULL)output->release();
		if(serializer!=NULL)serializer->release();
		//XMLPlatformUtils::Terminate();zhangm
		return -1;
	}

	string sFile = KdmFileName;
	if ( ( ret = ParseKdm_New( KdmFileName , kdmInfo ,error ) ) !=0 )
	{
		char buffer[BUF_SIZE]="";
		sprintf( buffer,"parserKdm(%s) is failed!\n", sFile.c_str() );
		error += buffer;
#ifdef _TEST_
			LOGERRFMT( "parserKdm(%s) is failed!\n", sFile.c_str() );
#endif
		return -2;
	}

	return ret;
}
int DcpInfo::GetKdmInfo(  std::string &path, std::vector<KdmInfo> &KdmList, string& error )
{
	char *kdmList[KDM_N_MAX];
	int kdmN = 0;
	int ret = 0;

	if(ret!=init()) 
	{
		error = "DcpInfo::init() fail.\n";
		LOGERRFMT("fail to init()!\n");
		return -1;
	}

	//搜索指定路径下的kdm文件
	ret = getKdmList( path.c_str(),kdmList,&kdmN,KDM_N_MAX , error );
	if(ret!=0)
	{
#ifdef _TEST_
		LOGERRFMT("fail to getKdmList:%s\n",strerror(ret));
#endif
		while(--kdmN>=0){if(kdmList[kdmN]!=NULL) free(kdmList[kdmN]);}
		if(parser!=NULL)parser->release();
		if(output!=NULL)output->release();
		if(serializer!=NULL)serializer->release();
		//XMLPlatformUtils::Terminate();	zhangm
		return ret;
	}
#if 0
	if(kdmN==0)
	{
#ifdef _TEST_
		LOGERRFMT("there is no kdm file in the USB!\n");
#endif
		
		//while(--kdmN>=0){if(kdmList[kdmN]!=NULL) free(kdmList[kdmN]);}
		return 1;
	}
#endif

	//Kdm文件的schema验证
	vector<string> pathVt;
	pathVt.clear();
#ifndef WIN32
	std::string strExePath;
	C_RunPara *pRunPara = C_RunPara::GetInstance();
	pRunPara->GetExePath(strExePath);
	pathVt.push_back(strExePath + "/kdmSchema/etm.xsd");
	pathVt.push_back(strExePath + "/kdmSchema/kdm.xsd");
	pathVt.push_back(strExePath + "/kdmSchema/xml.xsd");
	pathVt.push_back(strExePath + "/kdmSchema/xmldsig-core-schema.xsd");
	pathVt.push_back(strExePath + "/kdmSchema/xenc-schema.xsd");
	pathVt.push_back(strExePath + "/kdmSchema/KDM_schema.xsd");
#else
	pathVt.push_back("./kdmSchema/etm.xsd");
	pathVt.push_back("./kdmSchema/kdm.xsd");
	pathVt.push_back("./kdmSchema/xml.xsd");
	pathVt.push_back("./kdmSchema/xmldsig-core-schema.xsd");
	pathVt.push_back("./kdmSchema/xenc-schema.xsd");
	pathVt.push_back("./kdmSchema/KDM_schema.xsd");
#endif
	if(-1==ParseInit(1,pathVt))
	{
		error = "Kdm error:parse init is error!\n";
		if(parser!=NULL)parser->release();
		if(output!=NULL)output->release();
		if(serializer!=NULL)serializer->release();
		//XMLPlatformUtils::Terminate();	zhangm
		return -1;
	}

	//对搜索到的文件逐个解析
	for(int i=0;i<kdmN;i++)
	{
		KdmInfo kdmInfo;
		string sFile = kdmList[i];
		if ( ( ret = ParseKdm_New( sFile , kdmInfo , error ) ) !=0 )
		{
#ifdef _TEST_
			LOGERRFMT( "parserKdm(%s) is failed!\n", sFile.c_str() );
#endif
			continue;
			//return 2;
		}
		KdmList.push_back( kdmInfo );
	}

	//release kdmList
	while(--kdmN>=0)
	{ 
		if( kdmList[kdmN] != NULL )
		{
			free(kdmList[kdmN]);
			kdmList[kdmN] = NULL;
		}
	}

	if(parser!=NULL)parser->release();
	if(output!=NULL)output->release();
	if(serializer!=NULL)serializer->release();
	//XMLPlatformUtils::Terminate();	zhangm
	if (KdmList.size()==0)
	{
		error = "error:KdmList is Null;Kdm no found!";
		ret = -2;
		return ret;
	}
	return 0;
}

int DcpInfo::ParseKdm_New( string sFile  , KdmInfo& kdmInfo, string& error )
{
	DOMDocument *docKdm = ParseXML(sFile.c_str());
	if(docKdm==NULL)
	{
		char buffer[BUF_SIZE]="";
		sprintf( buffer,"parse(%s) is error!\n",sFile.c_str() );
		error = buffer;
		return -1;
	}

	string fileName = sFile.substr( sFile.rfind(DIR_SEPCHAR)+1 );
    string path = sFile.substr( 0 , sFile.rfind(DIR_SEPCHAR)+1 );      //可能是磁盘文件路径或ftp url
	string kdmId;
	string cplUuid;
	string playerSn;
	string startTime;
	string endTime;
	
	XMLCh xmlC[1024] = {0};  
	DOMNodeList *nlist = NULL;
	
	//解析出kdmId
	nlist = docKdm->getElementsByTagName(X("MessageId"));
	if(nlist==NULL) 
		return 1;
	
	//@author zhangmiao@oristartech.com
	//@date 2012-12-21
	//@brief 
	//@new
	XMLSize_t nLength = nlist->getLength();
	if( nLength == 0 ) 
	{
		error = "not find the E of MessageId!\n";
		LOGERRFMT("not find the E of MessageId!\n");
		return 1;
	}
	//@modify end;

	XMLString::subString(xmlC,nlist->item(0)->getTextContent(),9,45);
	kdmId = StrX(xmlC).localForm();

	Remove_PrefixOfUuid(kdmId); 

	//解析出cplUuid
	nlist = docKdm->getElementsByTagName(X("CompositionPlaylistId"));
	if(nlist==NULL) 
		return 1;
	if(0==nlist->getLength()) 
	{
		error = "not find the E of CompositionPlaylistId!\n";
		LOGERRFMT("not find the E of CompositionPlaylistId!\n");
		return 1;
	}
	memset(xmlC,0,sizeof(XMLCh)*1024);
	XMLString::subString(xmlC,nlist->item(0)->getTextContent(),9,45);
	cplUuid = StrX(xmlC).localForm();

	Remove_PrefixOfUuid(cplUuid); 

	//解析kdm的起始时间
	nlist = docKdm->getElementsByTagName(X("ContentKeysNotValidBefore"));
	if(nlist==NULL) 
		return 1;
	if(0==nlist->getLength()) 
	{
		error = "not find the E of ContentKeysNotValidBefore!\n";
		LOGERRFMT("not find the E of ContentKeysNotValidBefore!\n");
		return 1;
	}
	memset(xmlC,0,sizeof(XMLCh)*1024);
	XMLString::copyString(xmlC,nlist->item(0)->getTextContent());
    startTime = StrX(xmlC).localForm();

	//@author zhangmiao@oristartech.com
	//@date 2012-10-23
	//@brief 对时间字符串进行处理，"2009-07-04T10:00:00+09:00"，转换为"2009-07-04 10:00:00"；暂时忽略时区
	//@new
	/*string::size_type pos = 0;
	if( (pos = startTime.rfind('+')) != string::npos || (pos = startTime.rfind('-')) != string::npos )
	{
		startTime.erase(pos);
		pos = startTime.find('T');
		if(pos != string::npos)
			startTime.replace(pos,1," ");
	}*/
	//@modify end;

	//@author zhangmiao@oristartech.com
	//@date 2014-10-08
	//@brief 增加新的时区处理代码，默认转换为北京时间+08:00，如:"2009-07-04T10:00:00+09:00"，转换为"2009-07-04 09:00:00"；
	//@new
	int ret = 0;
	string sDestTimeStr_startTime;
	ret = ConvertTimeZoneInfo( startTime , "+08:00" , sDestTimeStr_startTime );
	startTime = sDestTimeStr_startTime;
	//@modify(时区处理)end;
	

	//解析kdm的终止时间
	nlist = docKdm->getElementsByTagName(X("ContentKeysNotValidAfter"));
	if(nlist==NULL) 
		return 1;
	if(0==nlist->getLength()) 
	{
		error = "not find the E of ContentKeysNotValidAfter!\n";
		LOGERRFMT("not find the E of ContentKeysNotValidAfter!\n");
		return 1;
	}
	memset(xmlC,0,sizeof(XMLCh)*1024);
	XMLString::copyString(xmlC,nlist->item(0)->getTextContent());
	endTime = StrX(xmlC).localForm();

	//@author zhangmiao@oristartech.com
	//@date 2012-10-23
	//@brief 对时间字符串进行处理，暂时忽略时区
	//@new
	/*pos = 0;
	if( (pos = endTime.rfind('+')) != string::npos || (pos = endTime.rfind('-')) != string::npos )
	{
		endTime.erase(pos);
		pos = endTime.find('T');
		if(pos != string::npos)
			endTime.replace(pos,1," ");
	}*/
	//@modify end;

	//@author zhangmiao@oristartech.com
	//@date 2014-10-08
	//@brief 增加新的时区处理代码，默认转换为北京时间+08:00，如:"2009-07-04T10:00:00+09:00"，转换为"2009-07-04 09:00:00"；
	//@new
	string sDestTimeStr_endTime;
	ret = ConvertTimeZoneInfo( endTime ,"+08:00", sDestTimeStr_endTime );
	endTime = sDestTimeStr_endTime;
	//@modify(时区处理)end;

	//解析标记<DeviceListIdentifier> 设备ID
	nlist = docKdm->getElementsByTagName(X("DeviceListIdentifier"));
	if(nlist==NULL) 
		return 1;
	if(0==nlist->getLength()) 
	{
		error = "not find the E of DeviceListIdentifier!\n";
		LOGERRFMT("not find the E of DeviceListIdentifier!\n");
		//@author zhangmiao@oristartech.com
		//@date 2013-04-02
		//@brief 如果该标记<DeviceListIdentifier>没有找到，可以忽略。
		//modify
		//return 1;
		//@modify end;
	}
	//@author zhangmiao@oristartech.com
	//@date 2013-04-02
	//@brief 如果该标记<DeviceListIdentifier>找到，则继续解析。
	//@new
	else
	{
		memset(xmlC,0,sizeof(XMLCh)*1024);
		XMLString::subString(xmlC,nlist->item(0)->getTextContent(),9,45);
		playerSn = StrX(xmlC).localForm();

		Remove_PrefixOfUuid(playerSn); 
	}
	//@modify end;

	// X509SubjectName
	nlist = docKdm->getElementsByTagName(X("X509SubjectName"));
	if(nlist==NULL) 
		return 1;
	if(0==nlist->getLength()) 
	{
		error = "not find the E of X509SubjectName!\n";
		LOGERRFMT("not find the E of X509SubjectName!\n");
		return 1;
	}
	memset(xmlC,0,sizeof(XMLCh)*1024);
	XMLString::copyString(xmlC,nlist->item(0)->getTextContent());
	string deviceTypeNumber = StrX(xmlC).localForm();
	if (deviceTypeNumber.size())
	{
		//解析deviceTypeNumber
		std::string::size_type pos;
		if( ( pos = deviceTypeNumber.find("CN=") ) != string::npos )
		{
			//找到后,继续找 '.'
			string deviceTypeNumber1 = deviceTypeNumber.substr(pos+3);
			pos = deviceTypeNumber1.find(",");
			if( pos != string::npos )
			{
				deviceTypeNumber1 = deviceTypeNumber1.substr(0,pos);
			}
			else
			{
				deviceTypeNumber1 = deviceTypeNumber1.substr(0);
			}
			deviceTypeNumber = deviceTypeNumber1;
			//去除末尾的字符 '.'
			pos = deviceTypeNumber.rfind('.');
			if( pos != string::npos )
			{
				if( pos == deviceTypeNumber.size()-1 )
					deviceTypeNumber.erase(pos);
			}
		}
		else
		{
			error = "No 'CN=' the Elem of X509SubjectName!\n";
			LOGERRFMT("No 'CN=' the Elem of X509SubjectName!\n");
			return 1;
		}
	}
	else
	{
		error = "empty the Elem of X509SubjectName!\n";
		LOGERRFMT("empty the Elem of X509SubjectName!\n");
		return 1;
	}

	kdmInfo.filename = fileName;
	kdmInfo.path = path;
	kdmInfo.cplUuid = cplUuid;
	kdmInfo.kdmUuid = kdmId;
	kdmInfo.playerSn = playerSn;
	kdmInfo.startTime = startTime;
    kdmInfo.endTime = endTime;
    kdmInfo.deviceTypeNumber = deviceTypeNumber;
    kdmInfo.status = 0;

	return 0;
}
//---zhangmiao:end---20120605----

//----zhangmiao---begin---20120615----
int DcpInfo::CreateDCPInfo(string srcPath ,string& error )
{   
	char *assetmapList[ASSETMAP_N_MAX];
	int assetmapN = 0;
	int ret = 0;

	if(ret!=init())
	{
		error = "DcpInfo::init() fail.\n";
		return ret;
	}

	bool bIsFileName = false;
	if ( srcPath.rfind(DIR_SEPCHAR) != (srcPath.length()-1) )
	{
		bIsFileName = true;
	}
	
	if(srcPath.rfind(DIR_SEPCHAR)==srcPath.length()-1)
		srcPath.erase(srcPath.length()-1);

	
	if ( bIsFileName )
	{
		if( assetmapN >= ASSETMAP_N_MAX )
		{
#ifdef _TEST_
			LOGERRFMT( "nfile=%d is bigger than max!\n", assetmapN );
#endif
			char buffer[BUF_SIZE]="";
			sprintf( buffer,"nfile=%d is bigger than max!\n", assetmapN );
			error = buffer;
			//if(*nfile>0) while(--(*nfile)>=0) free(filelist[*nfile]);
			ret = -2;
		}
		assetmapList[assetmapN] = (char *)malloc(PATH_MAX);
		sprintf(assetmapList[assetmapN],"%s",srcPath.c_str());
		//LOGERRFMT("%s,%d\n",filelist[*nfile],*nfile);
		assetmapN++;
	}
	else
	{
		ret = getAssetmapList(srcPath.c_str(),assetmapList,&assetmapN,ASSETMAP_N_MAX , error );
	}


	if(ret!=0)
	{
		char buffer[BUF_SIZE]="";
		sprintf( buffer,"fail to getAssetmapList:%s\n", strerror(ret) );
		error = buffer;
		//LOGERRFMT("fail to getAssetmapList:%s\n",strerror(ret));
		for(int i=0;i<assetmapN;i++) free(assetmapList[i]);
		if(parser!=NULL)parser->release();
		if(docRet!=NULL)docRet->release();
		if(output!=NULL)output->release();
		if(serializer!=NULL)serializer->release();
		//XMLPlatformUtils::Terminate();	zhangm
		return ret;
	}
#if 0
	if(assetmapN==0)
	{
		LOGERRFMT("did not get the ASSETMAP list!\n");
		return -1;
	}
#endif
	//LOGERRFMT("assetmapN=%d\n",assetmapN);
	assetmapIdVt.clear();
	for(int i=0;i<assetmapN;i++)
	{
		AMInfo_t aAMInfo;

		//std::cout << assetmapList[i] << std::endl;
		//解析Assetmap文件
		ret = Parse_Assetmap_New( assetmapList[i], aAMInfo , error );
		if(ret==-1)
		{
			LOGERRFMT("parseAssetmap error!:%s\n",assetmapList[i]);
			//LOGERRFMT("parseAssetmap error!\n");
			continue;
		}
		vector<dcp_Info_t> vDcp;

		for(int j=0;j<pklN;j++)
		{
			dcp_Info_t dcp;
			//std::cout << pklList[j]->path << std::endl;
			//解析PKL文件
			ret = 0;
			ret = Parse_Pkl_New( pklList[j], dcp , error );
			if( ret == -1 )
			{
				LOGERRFMT( "parsePkl error!:%s\n" , ( pklList[j]->path ).c_str() );

				for(int i=0;PKLInfo::num!=0;i++){delete pklList[i];pklList[i]=NULL;}
				for(int i=0;i<assetmapN;i++) free(assetmapList[i]);
				if(parser!=NULL)parser->release();
				if(docRet!=NULL)docRet->release();
				if(output!=NULL)output->release();
				if(serializer!=NULL)serializer->release();
				//XMLPlatformUtils::Terminate();	zhangm
				return -1;
			}
			//----zhangmiao:begin---2012-12-11----
			//如果PKL文件不存在，则忽略，继续解析下一个pkl文件。
			else if( ret == 1 )
			{
				continue;
			}
			//----zhangmiao:end----2012-12-11----

			//if(r!=0){LOGERRFMT("parsePkl error!:%s\n",pklList[j]);}

			//解析cpl文件
			int pklAssetN = dcp.aPkl.asset.size();
			//@author zhangmiao@oristartech.com
			//@date [2013-09-22]
			//@brief 主要增加了对SMPTE格式的处理代码，目前支持字幕文件
			//@new
			//vector<asset_pkl_t> asset_pkl_Vt = dcp.aPkl.asset;
			vector<asset_pkl_t>& asset_pkl_Vt = dcp.aPkl.asset;
			//@modify end;
			for( int k = 0; k < pklAssetN; k++ )
			{
				cpl_Info_t cpl;

				//@author zhangmiao@oristartech.com
				//@date [2013-09-22]
				//@brief 主要增加了对SMPTE格式的处理代码，目前支持字幕文件
				//@new
				//asset_pkl_t asset_pkl_t = asset_pkl_Vt[k];
				asset_pkl_t& asset_pkl_t = asset_pkl_Vt[k];
				//@modify end;
				string typeValue = asset_pkl_t.type;
				if ( typeValue!="text/xml;asdcpKind=CPL" )       //if(0!=typeValue.compare("text/xml;asdcpKind=CPL")) continue;
				{
					//@author zhangmiao@oristartech.com
					//@date [2013-09-18]
					//@brief 主要增加了对SMPTE格式的处理代码
					//@new
					//continue;
					if( typeValue == "text/xml" )
					{
						LOGERRFMT("This PKL is SMPTE Format file!\n");

						int IsCplFile=0;
						ret = VerifyIsCplFile4Pkl_Asset( asset_pkl_t.originalFileName.c_str() , IsCplFile , error );
						if ( ret < 0)
						{
							continue;
						}
						
						if ( IsCplFile ==1 )
						{
							asset_pkl_t.type = "text/xml;asdcpKind=CPL";
						}
						else //是字幕文件
						{
							//按照asdcp处理
							asset_pkl_t.type = "text/xml;asdcpKind=Subtitle";
							continue;
						}
						
					}
					else
					{
						continue;
					}
					//@modify end;
				}
				string cplPath = asset_pkl_t.originalFileName;
				if (Parse_Cpl_New( cplPath.c_str() , cpl , error )<0)
				{
					LOGERRFMT("parseCpl(%s) is error!\n",cplPath.c_str());
					continue;
				}

				/*bool bIsInsertCpl=false;
				int nSize_vDcp = vDcp.size();
				for(int i1=0;i1<nSize_vDcp;i1++)
				{
					int nSize_vCpl = vDcp[i1].vCpl.size();
                    for(int j1=0;j1<nSize_vCpl;j1++)
						if(vDcp[i1].vCpl[j1].issueDate==cpl.issueDate &&
							vDcp[i1].vCpl[j1].issuer   == cpl.issuer &&
							vDcp[i1].vCpl[j1].creator  == cpl.creator)
						{
							vDcp[i1].vCpl.push_back(cpl);
							bIsInsertCpl =true;
							
							break;
						}
						if (bIsInsertCpl)
						{
							break;
						}
						
				}
					

				if ( bIsInsertCpl )
				{
					continue;
				}*/


				//char uuid[UUID_LEN] = {0};
				//LOGERRFMT("getPkkuuid:%s\n",cpl.uuid.c_str());
				//if(0!=getPklUuid(cpl.uuid.c_str(),uuid,sfd))        
				//        getUuid(uuid,UUID_LEN);
				//	dcp.dcpUuid = uuid;
				//LOGERRFMT("getPkkuuid over:%s\n",uuid);
				//cout << "uuid="<< dcp.dcpUuid << endl;
				//dcp.dcpPath = "doby" + cpl.annotationText + ".pkl.xml"
				/*dcp.dcpSource = cpl.path.substr(0,cpl.path.rfind(DIR_SEPCHAR)+1);
				dcp.annotationText = cpl.contentTitleText; 
				dcp.issueDate = cpl.issueDate;
				dcp.issuer = cpl.issuer;
				dcp.creator = cpl.creator;*/
				dcp.vCpl.push_back(cpl);
				
			}

			//@author zhangmiao@oristartech.com
			//@date [2013-07-15]
			//如果cpl列表为空，则考虑不插入
			if ( dcp.vCpl.size() == 0 ) 
				continue;
			//@modify end;
			
			vDcp.push_back(dcp);

		}
		for(int k=0;PKLInfo::num!=0;k++)
		{ 
			delete pklList[k];
			pklList[k]=NULL;
		}
		
		//vDcp.push_back(dcp);

		//@author zhangmiao@oristartech.com
		//@date [2013-07-15]
		//如果vDcp列表为空，则考虑不插入
		if ( vDcp.size() == 0 ) 
			continue;
		//@modify end;

		aAMInfo.vDcp = vDcp;
		vAMInfo_t.push_back(aAMInfo);
	}

	return 0;
}
//----zhangmiao:end--20120615------

//@author zhangmiao@oristartech.com
//@date [2013-09-22]
//@brief 主要增加了对SMPTE格式的处理代码
//@new
//验证是否是cpl文件
//IsCplFile：0 = 不是cpl文件，1 = 是cpl文件。
int DcpInfo::VerifyIsCplFile4Pkl_Asset( const char* Pkl_AssetPath, int& IsCplFile , string& error )
{
	if( 0!= init() )
	{
		error += "DcpInfo::init() fail.\n";
		LOGERRFMT("fail to init()!\n");
		return -1;
	}

	std::vector<std::string> path;
	
	//进行cpl的schema验证
#ifndef WIN32
	std::string strExePath;
	C_RunPara *pRunPara = C_RunPara::GetInstance();
	pRunPara->GetExePath(strExePath);
	path.push_back(strExePath + "/cplSchema/xmldsig-core-schema.xsd");
	path.push_back(strExePath + "/cplSchema/xml.xsd");
	path.push_back(strExePath + "/cplSchema/CPL_schema.xsd");
	path.push_back(strExePath + "/cplSchema/SMPTE-429-7-2006-CPL.xsd");
	path.push_back(strExePath + "/cplSchema/SMPTE-429-10-2008-Main-Stereo-Picture-CPL.xsd");
	path.push_back(strExePath + "/cplSchema/Asdcp-Main-Stereo-Picture-CPL.xsd");
#else
	path.push_back("./cplSchema/xmldsig-core-schema.xsd");
	path.push_back("./cplSchema/xml.xsd");
	path.push_back("./cplSchema/CPL_schema.xsd");
	path.push_back("./cplSchema/SMPTE-429-7-2006-CPL.xsd");
	path.push_back("./cplSchema/SMPTE-429-10-2008-Main-Stereo-Picture-CPL.xsd");
	path.push_back("./cplSchema/Asdcp-Main-Stereo-Picture-CPL.xsd");
#endif

	if( ParseInit(0,path) == -1 )
	{
		error += "Cpl error:parse init is error!\n";
		LOGERRFMT("parse init is error!\n");
		return -1;
	}

	string strXml = Pkl_AssetPath;
	DOMDocument *doc = ParseXML(strXml);
	if( doc == NULL ) 
	{
		IsCplFile = false;

		char buffer[BUF_SIZE]="";
		sprintf( buffer,"parse(%s) is error!\n",Pkl_AssetPath );
		error += buffer;
		LOGERRFMT("parse(%s) is error!\n",Pkl_AssetPath);

		return 0;
	}
	else
	{
		//IsCplFile = true;
		DOMNodeList *nlist = doc->getElementsByTagName(X("CompositionPlaylist"));
		XMLSize_t nodeNum = nlist->getLength(); 
		LOGERRFMT("<CompositionPlaylist>RootNode Num = %d\n",nodeNum);
		if( nodeNum == 0 )
		{
			error += "Parse error:not find CompositionPlaylist!\n";
			LOGERRFMT("Parse error:not find CompositionPlaylist!\n");

			IsCplFile = false;

			char buffer[BUF_SIZE]="";
			sprintf( buffer,"VerifyIsCplFile(%s) is error!\n",Pkl_AssetPath );
			error += buffer;
			LOGERRFMT("VerifyIsCplFile(%s) is error!\n",Pkl_AssetPath);

			return 0;
		}
		else
		{
			IsCplFile = true;
		}
	}
	return 0;
}
//@modify end;

//--------------------------------------------
DOMCountErrorHandler::DOMCountErrorHandler():fSawErrors(false)
{
}

DOMCountErrorHandler::~DOMCountErrorHandler()
{
}
// ---------------------------------------------------------------------------
//  DOMCountHandlers: Overrides of the DOM ErrorHandler interface
// ---------------------------------------------------------------------------
bool DOMCountErrorHandler::handleError(const DOMError& domError)
{
    fSawErrors = true;
#if 1
    if (domError.getSeverity() == DOMError::DOM_SEVERITY_WARNING)
        XERCES_STD_QUALIFIER cerr << "\nWarning at file ";
    else if (domError.getSeverity() == DOMError::DOM_SEVERITY_ERROR)
        XERCES_STD_QUALIFIER cerr << "\nError at file ";
    else
        XERCES_STD_QUALIFIER cerr << "\nFatal Error at file ";

    XERCES_STD_QUALIFIER cerr << StrX(domError.getLocation()->getURI())
         << ", line " << domError.getLocation()->getLineNumber()
         << ", char " << domError.getLocation()->getColumnNumber()
         << "\n  Message: " << StrX(domError.getMessage()) << XERCES_STD_QUALIFIER endl;
#endif
    return true;
}

void DOMCountErrorHandler::resetErrors()
{
    fSawErrors = false;
}

//@author zhangmiao@oristartech.com
//@date [2014-04-04]
//@brief 主要增加写错误日志的代码
//@new
#if defined(WRITE_DCP_ELOG) 
#ifndef TMS20_LOG
#include "log/C_LogManage.h"
#endif
#endif
int DcpInfo::WriteErrorLog( const std::string& strError )
{
	int ret = 0;
	int iLevel=1, iModule=19, iSubModule=0;
	int errorCode = 0;
#ifdef WRITE_DCP_ELOG
	ret = C_LogManage::GetInstance()->WriteLog( iLevel,  iModule, iSubModule, errorCode, strError);
#endif // WRITE_ELOG
	return ret;
}
//@modify end;
