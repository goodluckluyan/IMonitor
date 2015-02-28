/*
*@file:DcpInfo.h
*@brief:DCP解析工具函数库
*@author: zhangmiao@oristartech.com
*@date: 2012/06/05
*/
#ifndef __DCPINFO_H__
#define __DCPINFO_H__

#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOM.hpp>

#include <xercesc/framework/StdOutFormatTarget.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/util/XMLUni.hpp>

#include <xercesc/util/OutOfMemoryException.hpp>
#include <xercesc/util/XercesDefs.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/dom/DOMError.hpp>
//#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/dom/DOMErrorHandler.hpp>

#if defined(XERCES_NEW_IOSTREAMS)
#include <iostream>
#else
#include <iostream.h>
#endif

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <exception>

#ifndef __FUN_H__
#include "fun.h"
#endif
#ifndef __PKLINFO_H__
#include "PKLInfo.h"
#endif
#ifndef __HASHINFO_H__
#include "HashInfo.h"
#endif

//@author zhangmiao@oristartech.com
//@date [2013-07-15]
//主要增加了相关字幕文件的处理代码
#include <uuid/uuid.h>
//@modify end;

XERCES_CPP_NAMESPACE_USE

namespace Content
{
#ifndef KDM_INFO
#define KDM_INFO
	//KdmInfo
	typedef struct KdmInfo
	{
		std::string path;//可能是磁盘文件路径或ftp url
		std::string filename;
		std::string kdmUuid;
		std::string cplUuid;
		std::string playerSn;
		std::string startTime;
		std::string endTime;
		int  status;//success:0,failed:1,nofile:2,error,etc.
		std::string deviceTypeNumber;   //设备型号,与X509SubjectName中，“CN=”相关
	}KdmInfo;
#endif

	namespace Dcp
	{
		//说明：表示Pkl的结构体，相当于1个dcp电影包。
		//pklinfo
		typedef struct asset_pkl{
			string uuid;
			string annotationtext;
			string hash;
			string type;
			string originalFileName;
			//@author zhangmiao@oristartech.com
			//@date [2013-07-15]
			//主要增加了相关字幕文件的处理代码
			string Path_NotFullPath;	//在assetmap中原始路径，并不是完整路径
			//@modify end;
			unsigned long long size;
		}asset_pkl_t;
		typedef struct pkl_Info{
			string uuid;
			string annotationText;
			string issueDate;
			string issuer;
			string creator;

			vector<asset_pkl_t> asset;
			unsigned long long int allFileLength;
		}pkl_Info_t;

		//说明：表示Cpl的结构体
		//cplinfo
		typedef struct asset_cpl{
			string uuid;
			string annotationText;
			string editRate;
			string intrinsicDuration;
			string entryPoint;
			string duration;
			string keyId;
			string frameRate;
			string screenAspectRatio;
			string language;
			string kind; //mainpicture, mainsound
		}asset_cpl_t;
		typedef struct cpl_Info{
			string uuid;
			string annotationText;
			string issueDate;
			string issuer;
			string creator;
			string contentTitleText;
			string contentKind;
			string path;

			vector<asset_cpl_t> asset;
		}cpl_Info_t;

		//说明：表示Assetmap的结构体，相当于1个Assetmap。
		//assetmapinfo
		typedef struct asset_assetmap{
			string uuid;//must
			string annotationtext;
			string kind;
			string path;//must
			//@author zhangmiao@oristartech.com
			//@date [2013-07-15]
			//主要增加了相关字幕文件的处理代码
			string Path_NotFullPath;	//在assetmap中原始路径，并不是完整路径
			//@modify end;
			unsigned long long Length;//must
		}asset_assetmap_t;
		typedef struct assetmap_Info{
			string assetmapUuid;
			vector<asset_assetmap_t> asset;
		}assetmap_Info_t;

		//dcpinfo
		typedef struct dcp_Info{
			string dcpUuid;
			string dcpSource;//the absolute path of dcp storage

			string annotationText; //dcp name
			string issueDate;
			string issuer;
			string creator;

			vector< cpl_Info_t > vCpl;
			//assetmap_Info_t aAsset;
			pkl_Info_t      aPkl;

		}dcp_Info_t;

		typedef struct AMInfo
		{
			vector< dcp_Info_t > vDcp;
			assetmap_Info_t aAssetmap_Info;

			string dcpPath;//dcp所在路径，可能是磁盘绝对路径或ftp url
			string assetmapName;//assetmap文件名，区分大小写
			long long int assetmapLength;//assetmap文件大小
		}AMInfo_t;

		typedef struct file_Info{
			string f_id;
			string f_path;    //完整路径
			//@author zhangmiao@oristartech.com
			//@date [2013-07-15]
			//主要增加了相关字幕文件的处理代码
			//@new
			string f_PathOri;  //assetmap中原始路径
			//@modify end;
		}file_Info_t;
	};
};

#define ASSETMAP_N_MAX 512 
#define PKL_N_MAX 64 
#define KDM_N_MAX 512 
#define CPL_N_MAX 512

//用于进行xml解析的的辅助类
// ---------------------------------------------------------------------------
//  Simple error handler deriviative to install on parser
// ---------------------------------------------------------------------------
class DOMCountErrorHandler : public DOMErrorHandler
{
public:
    //  Constructors and Destructor
    DOMCountErrorHandler();
    ~DOMCountErrorHandler();
    //  Getter methods
    bool getSawErrors() const;
    //  Implementation of the DOM ErrorHandler interface
    bool handleError(const DOMError& domError);
    void resetErrors();
private :
    //  Unimplemented constructors and operators
    DOMCountErrorHandler(const DOMCountErrorHandler&);
    void operator=(const DOMCountErrorHandler&);
    bool    fSawErrors;
};
inline bool DOMCountErrorHandler::getSawErrors() const
{
        return fSawErrors;
}

//用于进行DCP解析的类
class DcpInfo
{
public:
    DcpInfo();
    ~DcpInfo();

	//----zhangmiao:begin--20120605---
	/************************************************* 
	*@brief：  根据路径，解析并生成DCP的相关信息
	*@param：  srcPath ： 搜索DCP的本地路径  :[in]
	*@Return： 错误码,0=成功，非0，错误.
	*************************************************/ 
	int CreateDCPInfo(string srcPath , string& error );

	/************************************************* 
	*@brief：  解析Assetmap文件并将结果保持到AMInfo结构体中
	*@param：  xmlFile ： Assetmap文件的本地路径名  :[in]
	*@param：  aAMInfo ： 将解析结果保持到AMInfo结构中  :[out]
	*@Return： 错误码,0=成功，非0，错误.
	*************************************************/ 
	int Parse_Assetmap_New( std::string xmlFile , Content::Dcp::AMInfo& aAMInfo , string& error );

	/************************************************* 
	*@brief：  解析PKL文件并将结果保持到dcp_Info_t结构体中
	*@param：  pkl ： PKLInfo是表示pkl文件的结构类型       :[in]
	*@param：  dcp ： 将解析结果保持到dcp_Info_t结构中     :[out]
	*@Return： 错误码,0=成功，非0，错误.
	*************************************************/ 
	int Parse_Pkl_New( PKLInfo *pkl , Content::Dcp::dcp_Info_t& dcp , string& error );

	/************************************************* 
	*@brief：  解析CPL文件并将结果保持到cpl_Info_t结构体中
	*@param：  cplPath ： CPL文件的本地路径名             :[in]
	*@param：  cpl ： 将解析结果保持到cpl_Info_t结构体中  :[out]
	*@Return： 错误码,0=成功，非0，错误.
	*************************************************/ 
	int Parse_Cpl_New(const char *cplPath, Content::Dcp::cpl_Info_t &cpl , string& error );

	/************************************************* 
	*@brief：  重建Assetmap文件并将结果保持到KdmInfo结构体中
	*@param：  xmlFile ： Assetmap文件的本地路径名                         :[in]
    *@param：  pklUuid ： 只针对指定id的PKL文件包进行操作                  :[in]
	*@param：  aPkl ：  根据这个Pkl文件的结构体中进行筛选重建Assetmap文件  :[in]
	*@Return： 错误码,0=成功，非0，错误.
	*************************************************/ 
	int ReBuild_Assetmap(  std::string xmlFile ,const string &pklUuid, Content::Dcp::pkl_Info_t& aPkl , string& error );
	
	/************************************************* 
	*@brief：  解析KDM文件并将结果保持到KdmInfo结构体中
	*@param：  sFile ： KDM文件的本地路径名                :[in]
	*@param：  kdmInfo ： 将解析结果保持到KdmInfo结构体中  :[out]
	*@Return： 错误码,0=成功，非0，错误.
	*************************************************/ 
	int ParseKdm_New( std::string sFile , Content::KdmInfo& kdmInfo, string& error );
    /************************************************* 
	*@brief：  解析KDM文件并将结果保持到KdmInfo结构体中
	*@param：  path    ： KDM文件的追索目录            :[in]
	*@param：  kdmList     ：返回KDM列表		       :[out]
	*@Return： 错误码,0=成功，非0，错误.
	*************************************************/ 
	int GetKdmInfo(  std::string &path, std::vector<Content::KdmInfo> &KdmList, string& error);
    /************************************************* 
	*@brief：  解析KDM文件并将结果保持到KdmInfo结构体中
	*@param：  KdmFileName ： KDM文件的本地路径名                :[in]
	*@param：  kdmInfo ： 将解析结果保持到KdmInfo结构体中        :[out]
	*@Return： 错误码,0=成功，非0，错误.
	*************************************************/ 
	int GetKdmInfo( const std::string &KdmFileName, Content::KdmInfo& kdmInfo, string& error );
	//---zhangmiao:end---20120605----

	//----zhangmiao---begin---20120615----
	//AMInfo列表，每个元素表示1个AMInfo对象
	vector< Content::Dcp::AMInfo > vAMInfo_t;
	//assetmap_Info列表，1个assetmap_Info对象表示assetmap文件中的具体内容
	std::vector< Content::Dcp::assetmap_Info > assetmap_InfoVt;
	//pkl_Info_t列表，1个pkl_Info_t对象表示PKL文件中的具体内容
	std::vector< Content::Dcp::pkl_Info_t > pkl_Info_Vt;
	//返回pkl List指针
	//PKLInfo* getPKLInfo(){ return (PKLInfo*)pklList;}
	//----zhangmiao:end--20120615------

	//@author zhangmiao@oristartech.com
	//@date [2014-04-04]
	//@brief 主要增加写错误日志的代码
	//@new
#define WRITE_DCP_ELOG 
	#define WDELOG(str) WriteErrorLog(str);
	int WriteErrorLog( const std::string& strError );
	//@modify end;

private:
	/************************************************* 
	*@brief：  DcpInfo初始化。
	*@Return： 错误码,0=成功，非0，错误.
	*************************************************/ 
    int init();
    /************************************************* 
	*@brief：  XML文档的模式（Schema）验证进行初始化。
	*@param：  f_schechk ： 表示是否进行模式（Schema）验证      :[in]
    *@param：  schePath ： xml模式（Schema）验证文件的本地路径  :[in]
	*@Return： 错误码,0=成功，非0，错误.
	*************************************************/ 
    int ParseInit(int f_schechk,const std::vector<std::string> &schePath);
	/************************************************* 
	*@brief：  Parse XML文档。
	*@param：  xmlPath ： xml文件的本地路径  :[in]
	*@Return： 解析成功返回DOMDocument对象的指针，否则NULL=错误
	*************************************************/ 
    DOMDocument *ParseXML(const std::string &xmlPath);
	/************************************************* 
	*@brief：  搜索ASSETMAP文件
	*@param：  path ：用于搜索的文件路径，必须以"/"或“\”结尾。 :[in]       
	*@param：  assetmapList： 返回搜索到地文件名路径，是1个2维数组         【out】
	*@param：  max： 返回数组最大文件数         【in】
	*@param：  mapN ： 返回搜索到的文件个数。   :[out]
	*@Return： 错误码,0 = 成功，-1=错误:PKL文件包没有找到.
	*************************************************/
    int getAssetmapList(const char *path,char **assetmapList,int *mapN,int max , string& error );//search ASSETMAP in the path; 
	/************************************************* 
	*@brief：  搜索KDM文件
	*@param：  path ：用于搜索的文件路径，必须以"/"或“\”结尾。 :[in]         
	*@param：  kdmList： 返回搜索到地文件名路径，是1个2维数组         【out】
	*@param：  max： 返回数组最大文件数         【in】
	*@param：  kdmN ： 返回搜索到的文件个数。   :[out]
	*@Return： 错误码,0 = 成功，-1=错误:PKL文件包没有找到.
	*************************************************/
    int getKdmList(const char *path,char **kdmList, int *kdmN, int max , string& error);//search KDM in the path ;
  
	/************************************************* 
	*@brief：  搜索xml文档节点Node，根据Path路径。
	*@param：  path ：用于搜索的xml文件结点路径，是一个路径列表   :[in]
	*@param：  root： 基于root根结点，进行搜索                   ：【in】
	*@Return： 成功返回找到的DOMNode对象的指针，否则NULL=错误，没有找到.
	*************************************************/
    DOMNode *getNodeByXpath(DOMNode *root,std::vector<std::string> path);


	//@author zhangmiao@oristartech.com
	//@date [2013-09-22]
	//@brief 主要增加了对SMPTE格式的处理代码
	//@new
	//验证是否是cpl文件
	//IsCplFile：0 = 不是cpl文件，1 = 是cpl文件。
	int VerifyIsCplFile4Pkl_Asset(const char* cPkl_AssetPath, int& IsCplFile , string& error);
    //@modify end;

private:
	//搜索得到的Assetmap文件信息列表
    std::vector<Content::Dcp::file_Info_t> assetmapFile;

	//解析后得到的pkl文件列表
    PKLInfo *pklList[PKL_N_MAX];
	//pkl文件的个数
    int  pklN;
	//assetmap Id 列表
    std::vector<std::string> assetmapIdVt;
  
	//xml解析相关类的指针
    DOMImplementation *impl;
    DOMLSSerializer   *serializer;
    DOMLSOutput       *output;//XMLFormatTarget   *formatTarget;
    DOMLSParser       *parser;
    DOMDocument       *docRet;
    DOMCountErrorHandler errorHandler;
	//dcp包的大小
    unsigned long long dcpSize;

public:
	
};

//用于进行xml解析的的辅助类
class StrX
{
public :
    //  Constructors and Destructor
    StrX(const XMLCh* const toTranscode)
    {   // Call the private transcoding method
        fLocalForm = XMLString::transcode(toTranscode);
        //std::cout << "constractor:" << fLocalForm << std::endl;
    }
    ~StrX()
    {
        //std::cout << "deconstractor:" << fLocalForm << std::endl;
        XMLString::release(&fLocalForm);
    }
    //  Getter methods
    const char* localForm() const
    {
        return fLocalForm;
    }
private :
    char*   fLocalForm;
};
inline XERCES_STD_QUALIFIER ostream& operator<<(XERCES_STD_QUALIFIER ostream& target, const StrX& toDump)
{
    target << toDump.localForm();
    return target;
}

class XStr
{
public :
    //  Constructors and Destructor
    XStr(const char* const toTranscode)
    {
        fUnicodeForm = XMLString::transcode(toTranscode);
    }
    ~XStr()
    {
        XMLString::release(&fUnicodeForm);
    }
    const XMLCh* unicodeForm() const
    {
        return fUnicodeForm;
    }
private :
    XMLCh* fUnicodeForm;
};
#define X(str) XStr(str).unicodeForm()


#endif

