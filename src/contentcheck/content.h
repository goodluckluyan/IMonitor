/*
*@file:content.h
*@brief:导入dcp包 本地和ftp
*@author: zhangmiao@oristartech.com
*@date: 2012/06/05
*@History:
*@ zhangmiao@oristartech.com  2012-08-30  修改CplInfo结构体，增加CPL字段status、allFileLength。
*/
#ifndef __CONTENT_H__
#define __CONTENT_H__
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#ifndef __DCPINFO_H__
#include "DcpInfo.h"
#endif

#define DCPCPY 0

namespace Content
{
	typedef struct OtherFileInfo
	{
		//@author zhangmiao@oristartech.com
		//@date [2013-07-18]
		//@brief 主要增加了FTP下载相关字幕文件的处理代码
		//@new
		std::string Uuid;
		std::string originalfileName;
		//@modify end;
		std::string fileName;
		long long int fileLength;
	}OtherFileInfo;

	//说明：表示Mxf的结构体
	typedef struct MxfInfo
	{
		std::string mxfUuid;
		std::string fileName;
		long long int length;
	}MxfInfo;

	//说明：表示Cpl的结构体
	typedef struct CplInfo
	{
		std::string cplUuid;
		std::string cplName;
		std::string cplTitle;
		std::string cplIssueDate;
		std::string cplssuer;
		std::string cplCreator;			
		long long int cplLength;   //cpl文件的大小
		std::vector<std::string> mainPicList;//记录UUID
		std::vector<std::string> mainSoundList;//记录UUID
		int type;//影片类型，广告、正片、短篇等10种类型
		int is3D;
		int rate;//贞率
		int duration;              //持续时间，这里以秒为单位，int类型
		double Duration_double;    //持续时间，这里以秒为单位，double类型
		float ScreenAspectRatio;//宽高比
		int needKdm;     //是否加密，1需要，0不需要

		//@author zhangmiao@oristartech.com
		//@date 2012-08-30
		//@brief 修改CplInfo结构体，增加CPL字段status、allFileLength。
		//@new
		long long int allFileLength;    //CPL包大小（文件长度总合）
		int status;      //状态，是否正常，0正常，1 DCP不完整
		//@modify end;

		//@author zhangmiao@oristartech.com
		//@date 2013-12-09
		//@brief 主要修改CplInfo结构体，增加CPL字段MainSoundLanguage的处理代码
		//@new
		std::string MainSoundLanguage;       //表示影片语言，返回语言代码，如果cpl文件中没有该字段，则默认返回未知,"unknown"。
		//@modify end;

	}CplInfo;

	//说明：表示Pkl的结构体，相当于1个dcp电影包。
	typedef struct PklInfo
	{
		std::string pklUuid;
		std::string pklName;
		std::string AnnotationText;
		std::string IssueDate;
		std::string	Issuer;
		std::string	Creator;
		long long int pklLength;//pkl文件的大小
		long long int allFileLength;//pkl中包涵的所有文件大小的总和
		std::vector<CplInfo> cplList;  //cpl 列表
		std::vector<MxfInfo> mxfList;  //mxf 列表
		//@author zhangmiao@oristartech.com
		//@date 2013-07-18
		//主要增加了相关字幕文件的处理代码
		//@new
		std::vector<OtherFileInfo> fileInfoList;    //其他相关文件列表，比如：字幕相关文件
		//@modify end;
	}PklInfo;
	//说明：表示Assetmap的结构体，相当于1个Assetmap。
	typedef struct AssetmapInfo
	{
		std::string assetmapUuid;
		std::string assetmapName;//assetmap文件名，区分大小写
		long long int assetmapLength;//assetmap文件大小
		long long int dcpLength;//整个dcp所有文件大小的总和
		std::string dcpPath;//dcp所在路径，可能是磁盘绝对路径或ftp url
		std::vector<PklInfo> pklList;
		std::vector<OtherFileInfo> fileInfo;
	}AssetmapInfo;
	//说明：返回指定DCP包校验完成百分比。
	typedef struct HashInfo_t
	{
		std::string path;
		int  percent;
		int  status;          //checking,notstart,pausing,finished,error,etc.
		std::string filename; //如果校验异常，则返回异常文件名，否则该字段无效
	}Hashinfo;

	//File Copy 相关的数据结构
	typedef struct FileCopyInfo      //仅仅用于断点续传
	{
		int bIsFinished;
		std::string filename;
		int fileSize;           //文件大小，单位MB
		int transferredSize;    //已传输大小，单位MB
		FileCopyInfo()
		{
			bIsFinished=0;
			fileSize=0;
			transferredSize=0;
			filename="";
		}
	}FileCopyInfo;

	typedef struct CopyInfo
	{
		std::string source;
		std::string dest;
		int percent;
		int status;          //coping, notstart, pausing,finished,error.etc
		std::vector<FileCopyInfo> copyInfo;   //给出每个文件的拷贝状况
	}CopyInfo;
};
//CPL内容类型
typedef enum CPL_CONTENT_KIND 
{
	feature=1, //正片
	trailer,   //电影预告片;预告节目（2-3分）
	test,      //测试片
	teaser,    ////短电影预告片;预告节目（<1分）
	rating,    // 年龄段提示
	advertisement,  //广告
	shorts,    // 短片
	transitional,  //过渡，过场
	psa,     // 公益短片
	policy    // 政策
}CPL_CONTENT_KIND;
//CPL内容类型，map映射表
extern std::map<std::string,CPL_CONTENT_KIND> mapCPL_ContentKind;

//hash check状态
typedef enum HASH_CHECK_STATUS_KIND 
{
	STATUS_ERROR=-1,STATUS_CHECKING=1,STATUS_NOSTART,STATUS_PAUSEING,STATUS_FINISHED,
}HASH_CHECK_STATUS;
//Kdminfo copy状态
typedef enum KDMINFO_STATUS_KIND 
{
	KDMINFO_STATUS_ERROR=-1,KDMINFO_STATUS_SUCESSED,
}KDMINFO_STATUS_KIND;

//dcp copy状态
typedef enum DCP_COPY_STATUS_KIND 
{
	DCP_STATUS_ERROR=-1,DCP_STATUS_COPING=1,DCP_STATUS_NOSTART,DCP_STATUS_PAUSEING,DCP_STATUS_FINISHED,
}DCP_COPY_STATUS_KIND;


//@author zhangmiao@oristartech.com
//@date [2013-08-12]
//@brief 主要增加了Satellite专用卫星接口的处理代码
//@new
/************************************************* 
*@brief：  在IP地址所指的DCP专用卫星接收设备中，搜索FTP服务器全部的DCP包。
*@param：  Ip ： 用于dcp搜索的专用卫星接收设备的IP地址     :[in]
*@param：  dcpList     ：返回的dcp包的列表		           :[out]
*@param：  error     ：返回错误消息		                   :[out]
*@Return： 错误码,0 = 成功，非0，错误.
*************************************************/
int SearchSatDcpList( const std::string &Ip , std::vector<Content::AssetmapInfo> &dcpList ,std::string& error , unsigned short usPort = 20080 );

/************************************************* 
*@brief：  向IP地址所指的DCP专用卫星接收设备，发送对应影片下载完毕请求。
*@param：  Ip ： 用于dcp搜索的专用卫星接收设备的IP地址     :[in]
*@param：  PklUuid ： 只针对指定id的PKL文件包进行操作      :[in]
*@param：  Status ：如果status = 1，表示影片下载完毕。status = 0，表示下载失败  :[in]
*@param：  error     ：返回错误消息		                   :[out]
*@Return： 错误码,0 = 成功，非0，错误.
*************************************************/
//对应影片下载完毕；如果status = 1，表示影片下载完毕。status = 0，表示下载失败。
int Sate_SetFTPDownLoadFinished( const std::string& Ip , const std::string& PklUuid , const int Status , std::string& error );
//@modify end;

/************************************************* 
*@brief：  在path所指的本地路径中，搜索全部的DCP包。
*@param：  path ： 用于dcp搜索的本地路径，必须以"/"结尾  :[in]
*@param：  dcpList     ：返回的dcp包的列表		       :[out]
*@Return： 错误码,0=成功，非0，错误.
*************************************************/  
//本地搜索
int SearchDcpList( const std::string &path, std::vector<Content::AssetmapInfo> &dcpList ,std::string& error );
/************************************************* 
*@brief：  在ftpUrl所指的路径中，搜索FTP服务器全部的DCP包。
*@param：  ftpUrl ： 用于dcp搜索的FTP服务器URL，必须以"/"结尾  :[in]
*@param：  dcpList     ：返回的dcp包的列表		               :[out]
*@Return： 错误码,0 = 成功，非0，错误.
*************************************************/
//搜索FTP服务器
//ftpUrl：字符串，例：ftp://userName:passwd@172.23.140.50:21/dir1/dir2/
int SearchFtpDcpList(const std::string &ftpUrl, std::vector<Content::AssetmapInfo> &dcpList ,std::string& error );
/************************************************* 
*@brief：  该函数用于针对DCP包进行完整性校验。
*@param：  path ： 须是DCP包的完整路径，必须以"/"结尾  :[in]
*@param：  pklUuid ： 只针对指定id的PKL文件包进行操作  :[in]
*@Return： 错误码,0 = 成功，-1=错误:PKL文件包没有找到.
*************************************************/
//Dcp校验
int HashDcp( const std::string &path, const std::string &pklUuid ,std::string& error );
/************************************************* 
*@brief：  该函数获取hash校验进度
*@param：  path ： 须是DCP包的完整路径，必须以"/"结尾  :[in]
*@param：  pklUuid ： 只针对指定id的PKL文件包进行操作  :[in]
*@param：  hash  ： 返回指定DCP包校验完成百分比。      :[out]
*@Return： 错误码,0 = 成功，-1=错误:PKL文件包没有找到.
*************************************************/
//获取hash校验进度
int GetHashPercent( const std::string &path, const std::string &pklUuid, Content::Hashinfo &hash ,std::string& error );
/************************************************* 
*@brief：  该函数取消hash校验
*@param：  path ： 须是DCP包的完整路径，必须以"/"结尾  :[in]
*@param：  pklUuid ： 只针对指定id的PKL文件包进行操作  :[in]
*@Return： 错误码,0 = 成功，-1=错误:PKL文件包没有找到.
*************************************************/
//取消hash校验
int CancelHash( const std::string &path, const std::string &pklUuid ,std::string& error );

#if DCPCPY == 1
/************************************************* 
*@brief：  该函数删除DCP
*@param：  path ： 须是DCP包的完整路径，必须以"/"结尾  :[in]
*@param：  pklUuid ： 只针对指定id的PKL文件包进行操作  :[in]
*@Return： 错误码,0 = 成功，-1=错误:PKL文件包没有找到.
*************************************************/
//删除DCP
int DeleteDcp( const std::string &path, const std::string &pklUuid , std::string& error );
/************************************************* 
*@brief：  该函数DCP导入，支持2种模式，1次传输或断点续传。
*@param：  source ： source必须为DCP包的直接路径。必须以"/"结尾  :[in]
*@param：  pklUuid ： 只针对指定id的PKL文件包进行操作            :[in]
*@param：  dest ：函数将DCP直接拷贝到dest目录下，如果没有dest目录，则建立该路径。必须以"/"结尾  :[in]
*@param：  copyInfo ： 第一次传输，copyInfo参数为空；断点续传时，用于指示每个文件的完成状况。:[in|out]
*@Return： 错误码,0 = 成功，-1=错误:PKL文件包没有找到.
*************************************************/
//DCP导入。如果一个ASSETMAP文件中，包含多个PKL文件的信息，则只导入pklUuid所指定的影片包，同时，为影片包生成ASSETMAP文件。
int CopyDcp( const std::string &source, const std::string &pklUuid, const std::string &dest,
			 std::vector<Content::FileCopyInfo> &copyInfo , std::string& error , const std::string dest2 = "" , int nLimitedSpeed = -1  );
//Copy PKL AssetList File
//@author zhangmiao@oristartech.com
//@date [2013-07-15]
//主要增加了相关字幕文件的处理代码
//@new
//int CopyDcpSub1( const std::string &source, const std::string &Dest, Content::Dcp::pkl_Info_t& aPkl, std::string& error );
int CopyDcpSub1( const std::string &source, const std::string &Dest, Content::Dcp::pkl_Info_t& aPkl, std::string& error ,
				 const std::string Dest2 = "" , int nLimitedSpeed = -1 );
//@modify end;
//Copy PKL File
int CopyDcpSub2( const std::string &pklName, const std::string &Dest, std::string& error , const std::string Dest2 = "" );
//Copy AssetMap File 拷贝完后，需要重建AssetMap文件
int CopyDcpSub3( const std::string &AssetmapName, const std::string &pklUuid, const std::string &Dest,
				 DcpInfo& dcp, Content::Dcp::pkl_Info_t& aPkl , std::string& error  , const std::string Dest2 = "" );
/************************************************* 
*@brief：  该函数取得DCP导入进度
*@param：  path ： 须是DCP包的完整路径，必须以"/"结尾  :[in]
*@param：  pklUuid ： 只针对指定id的PKL文件包进行操作  :[in]
*@param：  info ： 返回指定DCP包导入进度完成百分比。   :[out]
*@Return： 错误码,0 = 成功，-1=错误:PKL文件包没有找到.
*************************************************/
//取得DCP导入进度
int GetCopyPercent(const std::string &source , const std::string &pklUuid, Content::CopyInfo &info, std::string& error );
/************************************************* 
*@brief：  该函数取消DCP导入
*@param：  path ： 须是DCP包的完整路径，必须以"/"结尾  :[in]
*@param：  pklUuid ： 只针对指定id的PKL文件包进行操作  :[in]
*@Return： 错误码,0 = 成功，-1=错误:PKL文件包没有找到.
*************************************************/
//取消DCP导入
int CancelCopy( const std::string &source, const std::string &pklUuid , std::string& error );
/************************************************* 
*@brief：  在path所指的本地路径中，搜索全部的KDM列表。
*@param：  path ： 路径，可以包含文件名。不包含文件名则追索目录；包含文件名则直取得该文件信息。路径，必须以"/"结尾  :[in]
*@param：  kdmList     ：返回KDM列表		       :[out]
*@Return： 错误码,0=成功，非0，错误.
*************************************************/  
//KDM列表
int GetKdmInfo( const std::string &path, std::vector<Content::KdmInfo> &kdmList, std::string& error );
/************************************************* 
*@brief：  在path所指的本地路径中，KDM删除。
*@param：  path ： 路径，可以包含文件名。不包含文件名则追索目录；包含文件名则直取得该文件信息。路径，必须以"/"结尾  :[in]
*@Return： 错误码,0=成功，非0，错误.
*************************************************/ 
//KDM删除
int DeleteKdm( const std::string &path , std::string& error );
/************************************************* 
*@brief：  KDM导入。
*@param：  kdmList ：通过调用GetKdmInfo()后，返回的KDM列表 :[in]
*@param：  dest ： 下载后的目标路径。路径，必须以"/"结尾   :[in]
*@Return： 错误码,0=成功，非0，错误.
*************************************************/ 
//KDM导入，dest不存在则直接建立。
int ImportKdm( std::vector<Content::KdmInfo> &kdmList, const std::string &dest , std::string& error , std::string dest2 = "" );
/************************************************* 
*@brief：  从FTP服务器下载KDM，目前只实现了直接下载KDM的功能。
*@param：  kdmList ：通过KdmInfo中的path、filename指定呀下载的文件  :[in]
*@param：  dest ： 下载后的目标路径。路径，必须以"/"结尾            :[in]
*@Return： 错误码,0=成功，非0=错误
*************************************************/ 
//从FTP服务器下载KDM
int DownLoadKdm( std::vector<Content::KdmInfo> &kdmList, const std::string &dest , std::string& error );

/************************************************* 
*@brief：  普通文件拷贝。
*@param：  src ：源文件名            :[in]
*@param：  des ：目标文件名          :[in]
*@Return： 错误码,0=成功，非0=错误
*************************************************/ 
//普通文件拷贝
int cp_file( const char *src, const char *des );
/************************************************* 
*@brief：  文件拷贝，断点续传。支持获取进度。
*@param：  src ：源文件名                                 :[in]
*@param：  des ：目标文件名                               :[in]
*@param：  from ：从from所指的位置开始传输                :[in]
*@param：  rlTransferredSize ：返回已经传输完的字节数     :[out]
*@Return： 错误码,0=成功，非0=错误
*************************************************/ 
//文件拷贝，断点续传。支持获取进度
int cp_file_BP( const char *src, const char *des , unsigned long long& rlTransferredSize ,unsigned long long from = 0 , const char *dest2 = NULL 
			    , int nLimitedSpeed = -1  );    //nLimitedSpeed限速值，单位为MB/S
/************************************************* 
*@brief：  普通文件拷贝，支持获取进度
*@param：  src ：源文件名                                 :[in]
*@param：  des ：目标文件名                               :[in]
*@Return： 错误码,0=成功，非0=错误
*************************************************/ 
//文件拷贝，支持获取进度
int cp_file_Mode2( const char *src, const char *des , const char *dest2 = NULL , int nLimitedSpeed = -1 );    //nLimitedSpeed限速值，单位为MB/S
/************************************************* 
*@brief：  断点续传方式，拷贝DCP包。
*@param：  source ： source必须为DCP包的直接路径。必须以"/"结尾     :[in]
*@param：  pklUuid ： 只针对指定id的PKL文件包进行操作               :[in]
*@param：  dest ：函数将DCP直接拷贝到dest目录下，如果没有dest目录，则建立该路径。必须以"/"结尾  :[in]
*@param：  copyInfo ： 第一次传输，copyInfo参数为空；断点续传时，用于指示每个文件的完成状况。:[in|out]
*@Return： 错误码,0=成功，非0=错误
*************************************************/ 
//断点续传方式，拷贝DCP包。
int CopyDcp_BPTrans( const std::string &source, const std::string &pklUuid, const std::string &dest,
					 std::vector<Content::FileCopyInfo> &copyInfo , std::string& error , const std::string dest2 = "" , int nLimitedSpeed = -1 );
//断点续传方式，拷贝copyInfo中的每个文件，对于CPL文件直接拷贝
//@author zhangmiao@oristartech.com
//@date [2013-07-15]
//主要增加了相关字幕文件的处理代码
//@new
//int CopyDcp_BPTransSub1( const std::string &Dest, Content::Dcp::pkl_Info_t& aPkl, std::vector<Content::FileCopyInfo> &copyInfo , std::string& error );
int CopyDcp_BPTransSub1( const std::string &source, const std::string &Dest, Content::Dcp::pkl_Info_t& aPkl, std::vector<Content::FileCopyInfo> &copyInfo , std::string& error ,
						 const std::string Dest2 = "" , int nLimitedSpeed = -1 );
//@modify end;
/************************************************* 
*@brief：  FTP导入DCP
*@param：  ftpUrl ：用于dcp搜索的FTP服务器URL，必须以"/"结尾。     :[in]
*@param：  pklUuid ： 只针对指定id的PKL文件包进行操作                           :[in]
*@param：  dest ： 函数将DCP直接拷贝到dest目录下，如果没有dest目录，则建立该路径。必须以"/"结尾  :[in]
*@param：  copyInfo ： 第一次传输，copyInfo参数为空；断点续传时，用于指示每个文件的完成状况。:[in|out]
*@Return： 错误码,0=成功，非0=错误
*************************************************/ 
//FTP导入DCP
int DownLoadDcp(const std::string &ftpUrl, const std::string &pklUuid, const std::string &dest, std::vector<Content::FileCopyInfo> &copyInfo, std::string& error );
/************************************************* 
*@brief：  取消FTP导入DCP
*@param：  ftpUrl ：用于dcp搜索的FTP服务器URL，必须以"/"结尾。     :[in]
*@param：  pklUuid ： 只针对指定id的PKL文件包进行操作              :[in]
*@Return： 错误码,0=成功，非0=错误
*************************************************/ 
//取消FTP导入DCP
int CancelDownLoad( const std::string &ftpUrl, const std::string &pklUuid, std::string& error );

/************************************************* 
*@brief：  取得FTP导入DCP进度
*@param：  ftpUrl ：用于dcp搜索的FTP服务器URL，必须以"/"结尾。 :[in]
*@param：  pklUuid ： 只针对指定id的PKL文件包进行操作  :[in]
*@param：  info ： 返回指定DCP包导入进度完成百分比。   :[out]
*@Return： 错误码,0 = 成功，-1=错误:PKL文件包没有找到.
*************************************************/
//取得FTP导入DCP进度
int GetDownLoadPercent(const std::string &ftpUrl, const std::string &pklUuid, Content::CopyInfo &info, std::string& error );
/*************************************************
*@brief：  断点续传方式，下载DCP包。
*@param：  ftpUrl ：用于dcp搜索的FTP服务器URL，必须以"/"结尾。                  :[in]
*@param：  pklUuid ： 只针对指定id的PKL文件包进行操作                           :[in]
*@param：  dest ：函数将DCP直接拷贝到dest目录下，如果没有dest目录，则建立该路径。必须以"/"结尾  :[in]
*@param：  copyInfo ： 第一次传输，copyInfo参数为空；断点续传时，用于指示每个文件的完成状况。:[in|out]
*@Return： 错误码,0=成功，非0=错误
*************************************************/
//断点续传方式，下载DCP包。
int DownLoadDcp_BPTrans( const std::string &ftpUrl, const std::string &pklUuid, const std::string &dest,
				         std::vector<Content::FileCopyInfo> &copyInfo , std::string& error );
//下载copyInfo中的每个文件
int DownLoadDcp_BPTransSub1( const std::string &Dest, Content::PklInfo& aPkl, std::vector<Content::FileCopyInfo> &copyInfo, std::string& error );
//Download PKL File
int DownLoadDcp_BPTransSub2( const std::string &pklName, const std::string &Dest, std::string& error );
//Download AssetMap File
int DownLoadDcp_BPTransSub3( const std::string &AssetmapName, const std::string &pklUuid, const std::string &Dest,
				              Content::PklInfo& aPkl, std::string& error );
#endif

#define CODE_DCPCOPYCANCEL  -90001

#define CODE_DCP_COPY_SLAVE_FAILED  -40001

#define ONEMB  1048576

#ifndef WIN32
#define O_BINARY       0x8000  
#ifndef strcmpi
#define strcmpi strcasecmp
#endif
#ifndef stricmp
#define stricmp strcasecmp
#endif
#endif

//初始化 mapCPL_ContentKind 映射表
void init_maplist();

//获取PKL的资产列表AssetList中，指定文件FileName的大小；FileName是文件名，不带路径；
//本地
//@author zhangmiao@oristartech.com
//@date [2013-07-15]
//主要增加了相关字幕文件的处理代码
//@new
//int GetFileSize_PKLAssetList( const std::string FileName ,const Content::Dcp::pkl_Info_t& aPkl, long long& Length );
int GetFileSize_PKLAssetList( const std::string FileName ,const Content::Dcp::pkl_Info_t& aPkl, long long& Length , std::string& FullPathFileName );
//@modify end;

//ftp DownLoad
//@author zhangmiao@oristartech.com
//@date [2013-07-18]
//@brief 主要增加了FTP下载相关字幕文件的处理代码
//@new
//int GetFileSize_PKLAssetList( const std::string FileName ,const Content::PklInfo& aPkl, long long& Length );
int GetFileSize_PKLAssetList( const std::string FileName ,const Content::PklInfo& aPkl, long long& Length ,std::string& originalfileName );
//@modify end;

//************************************
// Method:    CheckCPLIsValid
// FullName:  CheckCPLIsValid
// Access:    public 
// Returns:   int
// Qualifier:检查cpl文件有效状态，在本地。
// Parameter: const std::vector< asset_cpl_t > & vAsset_Cpl
// Parameter: const std::vector< asset_pkl_t > & vAsset_pkl
//************************************
int CheckCPLIsValid( const std::vector< Content::Dcp::asset_cpl_t >& vAsset_Cpl , const std::vector< Content::Dcp::asset_pkl_t >& vAsset_pkl ,std::string& error );

int CheckCPLIsValid4Ftp( const std::string dir ,const std::string assetmapUuid , const cftp::FtpClient& ftp ,
	                     std::vector<Content::AssetmapInfo>& dcpList );

//获取当前的限速值
int GetCurrentLimitedSpeed( int& nLimitedSpeed );

//二元函数对象，用于对vector<AssetmapInfo> & dcpList，进行排序，默认用“<”运算符，为升序。
struct FuncCompDcpList
{
	// functor for operator<
	bool operator()(const Content::AssetmapInfo& _Left, const Content::AssetmapInfo& _Right) const
	{
		// apply operator< to operands
		//return (_Left.assetmapName < _Right.assetmapName);
		return (_Left.dcpPath < _Right.dcpPath);
	}
};

//error code list
//错误码区分：内容管理使用1001以上的值。
const int nCONTENT_ERROR_Code_Base_Value = 1000;
const int ERROR_CONTENT_FTP_TCPTRANSPORTFAILED		 = -1;
const int ERROR_CONTENT_FTP_FILECANNOTOPEN			 = -2;
const int ERROR_CONTENT_FTP_PARAMETERERROR			 = -3;
const int ERROR_CONTENT_FTP_SERVICEERROR			 = -10;
const int ERROR_CONTENT_FTP_UNKNOWERROR				 = -100;
const int ERROR_CONTENT_FTP_CANCEL_TRANSFILE_ERROR	 = -20;			//FtpClient::CancelTransFile() Error!

const int ERROR_CONTENT_KDM_GETKDMINFOERROR							 = -101;	//GetKdmInfo error
const int ERROR_CONTENT_KDM_DELETEKDM_ERROR							 = -102;	//"DeleteKdm() error!:bad Path"
const int ERROR_CONTENT_KDM_KDMINFOCOUNTISNULL_ERROR				 = -103;	//error:KdmInfoCount is NULL !
const int ERROR_CONTENT_KDM_DONOT_FOUND_KDM_ERROR			    	 = -104;	//error:"Do not found Kdm"! 
const int ERROR_CONTENT_KDM_DELKDM_ERROR_LOCAL_FAIL_BUT_SLAVE_OK     = -105;	//Delete KDM local failed , but SLAVE KDM Dest Path2 OK!

const int ERROR_CONTENT_DCP_DONOT_FOUND_PKL						     = -120;	//Do not found pkl
const int ERROR_CONTENT_DCP_PARAM_PATH_IS_NULL						 = -121;	//"Param Path is Null!"
const int ERROR_CONTENT_DCP_UNABLE_TO_DELETE_DIR					 = -122;	//Unable to delete directory
const int ERROR_CONTENT_DCP_PKL_UUID_ERROR							 = -123;	//PklUuid is error
const int ERROR_CONTENT_DCP_CREATE_DCPINFO_ERROR    				 = -124;	//error:CreateDCPInfo()
const int ERROR_CONTENT_DCP_COPY_NO_START							 = -125;	//error:DCP Copy is NO START.
const int ERROR_CONTENT_DCP_PARAM_COPYINFO_ISNULL					 = -126;	//Param copyInfo is NULL!
const int ERROR_CONTENT_DCP_COPYDOWNLOAD_NO_START					 = -127;	//error:DCP DownLoad is NO START.
const int ERROR_CONTENT_DCP_PARSE_ASSETMAP_ERROR					 = -128;	//parseAssetmap error!
const int ERROR_CONTENT_DCP_MAKE_DIR_FAIL 							 = -129;	//Error:Make Directory fail.
const int ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL 					 = -130;	//Error:FilmFilePath is NULL!
const int ERROR_CONTENT_DCP_SEARCH_FTP_DCP_LIST 					 = -131;	//error: SearchFtpDcpList
const int ERROR_CONTENT_DCP_SEARCH_DCP_LIST 						 = -132;	//error: SearchDcpList
const int ERROR_CONTENT_DCP_UNABLE_TO_DELETE_ASSETMAP_DIR 			 = -133;	//Unable to delete ASSSETMAP directory
const int ERROR_CONTENT_DCP_CHECKCPL_INVALID_FORFTP					 = -134;	//Error:Check CPL Is InValid For FTP!
const int ERROR_CONTENT_DCP_PKL_FILE_NO_EXIST 						 = -135;	//Error:PKL File(%s) no exist!
const int ERROR_CONTENT_DCP_PKL_FILE_COPY_FAILED 					 = -136;	//PKL File copy Failed 
const int ERROR_CONTENT_DCP_PARAM_PKLNAME_IS_NULL_STR 				 = -137;	//Error:Param pklName is Null string!
const int ERROR_CONTENT_DCP_ASSETMAP_IS_NO_EXIST 					 = -138;	//Error:Assetmap(%s) is no exist!
const int ERROR_CONTENT_DCP_PARAM_ASSETMAPNAME_IS_NULL_STR 			 = -139;	//Error:Param AssetmapName is Null string!
const int ERROR_CONTENT_DCP_FILE_COPY_FAILED 				         = -140;	//File copy Failed
const int ERROR_CONTENT_DCP_FILECOPYNAME_IS_NULL 			         = -141;	//Error:FileCopyName is NULL
const int ERROR_CONTENT_DCP_CPL_FILE_PATH_IS_NULL 			         = -142;	//Error:CPL File Path Is NULL
const int ERROR_CONTENT_DCP_CPL_FILE_IS_NO_EXIST 					 = -143;	//Error:CPL FILE is no exist!
const int ERROR_CONTENT_DCP_MXF_FILE_PATH_IS_NULL 					 = -144;	//Error:mxf File Path Is NULL!
const int ERROR_CONTENT_DCP_MXF_FILE_IS_NO_EXIST 					 = -145;	//Error:mxf File is no exist
const int ERROR_CONTENT_DCP_MXF_FILE_COPY_IS_UNFINISHED 			 = -146;	//Error:mxf File copy is unfinished
const int ERROR_CONTENT_DCP_REBUILD_ASSETMAP_FILE_FAILED 			 = -147;	//Error ReBuild_Assetmap file FAILED
const int ERROR_CONTENT_DCP_PKL_ASSELIST_IS_NULL					 = -148;	//Error:PKL AssetList is null!
const int ERROR_CONTENT_DCP_PKL_ASSEFILE_PATH_IS_NULL				 = -149;	//Error:Pkl AssetFile Path Is NULL!
const int ERROR_CONTENT_DCP_FILE_NOT_EXIST							 = -150;	//Error:file NOT exist
const int ERROR_CONTENT_DCP_PKL_PKLNAME_ERROR						 = -151;	//pklName %s Error!
const int ERROR_CONTENT_DCP_BP_DCPCOPYCANCEL						 = -152;	//CODE_BP_DCPCOPYCANCEL
const int ERROR_CONTENT_DCP_DCPCOPYCANCEL							 = -153;	//CODE_DCPCOPYCANCEL	,非断点续传mode

//@author zhangmiao@oristartech.com
//@date [2015-01-09]
//从机文件上传失败的错误编码
const int ERROR_CONTENT_DCP_FILE_COPY_SLAVE_FAILED 				     = -154;	//File copy SLAVE Dest Path2 Failed
const int ERROR_CONTENT_DCP_UNABLE_TO_DELETE_SLAVE_DIR				 = -155;	//SLAVE Dest Path2 Failed:Unable to delete directory 
const int ERROR_CONTENT_KDM_DELETE_SLAVE_KDM_ERROR					 = -156;	//SLAVE Dest Path2 Failed:DeleteKdm() error!
const int ERROR_CONTENT_DCP_DELDCP_ERROR_LOCAL_FAIL_BUT_SLAVE_DIR_OK = -157;	//Delete DCP local failed , but SLAVE Dest Path2 OK!
//@modify [2015-01-09] end

const int ERROR_CONTENT_HASH_HASHCHECK_NO_START						 = -200;	//error:DCP Hash Check is NO START.
const int ERROR_CONTENT_HASH_SHA1_CHECK_ERROR  					     = -201;	//"SHA1 CHECK ERROR!"
const int ERROR_CONTENT_HASH_SHA1_CHECK_CANCEL 						 = -202;	//SHA1 CHECK CANCEL!

//@author zhangmiao@oristartech.com
//@date [2013-08-12]
//@brief 主要增加了Satellite专用卫星接口的处理代码
//@new
const int ERROR_CONTENT_DCP_SEARCH_SATE_DCP_LIST 					 = -401;	//error: SearchSatDcpList()
const int ERROR_CONTENT_DCP_SATE_INIT 			    		         = -402;	//error:C_Satellite::Init()
const int ERROR_CONTENT_DCP_SATE_CREATE_CERT_CONNECT 			     = -403;	//error:C_Satellite::CreateCertConnect()
const int ERROR_CONTENT_DCP_SATE_GET_DCP_LIST 						 = -404;	//error:C_Satellite::GetDcpList()
const int ERROR_CONTENT_DCP_SATE_GET_FTP_DCPINFO_BY_ID 			     = -405;	//error:C_Satellite::GetFTPDCPInfoById()
const int ERROR_CONTENT_DCP_SATE_SET_FTP_DOWNLOAD_FINISHED 			 = -406;	//error:C_Satellite::SetFTPDownLoadFinished()
//@modify end;

#define TMS2_CM_ERRCODE
#ifdef TMS2_CM_ERRCODE
#define CMTT(error) ContentErrorCodeToTmsErrorCode(error);
#define CMTT2(error) ContentErrorCodeToTmsErrorCode(error)
int ContentErrorCodeToTmsErrorCode(int error);
#else
#define CMTT(error) (error);
#define CMTT2(error) (error)
#endif // TMS2_CM_ERRCODE

#define WRITE_ELOG 
#ifdef WRITE_ELOG
#define WELOG(str) WriteErrorLog(str);
int WriteErrorLog( const std::string& strError );
#else
#define WELOG(str) 
#endif // WRITE_ELOG

#define LIMITED_SPEED

#define _BP_DCPCOPYCANCEL_

#endif
