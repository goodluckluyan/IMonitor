/*
*@file:content.cpp
*@brief:导入dcp包 本地和ftp
*@author: zhangmiao@oristartech.com
*@date: 2012/06/05
*/
//#ifndef __DCPINFO_H__
//#include "DcpInfo.h"
//#endif
#ifndef __CONTENT_H__
#include "content.h"
#endif

using namespace Content;
using namespace Content::Dcp;
using namespace std;

#ifndef WIN32
#include <pthread.h>
pthread_mutex_t work_mutex;
#include <sys/time.h>
#else
HANDLE hMutex = CreateMutex(NULL, FALSE, (LPCWSTR)"DCPContent");
#define _USE_32BIT_TIME_T
#include "gettimeofday_win.h"
#define usleep Sleep
#endif
#include "para/C_Para.h"
#include "para/C_RunPara.h"
#define _READDB_DEST_PATH2
#define _READDB_KDM_DEST_PATH2

//定义、声明全局变量，用于支持获取进度。
std::string g_sCopyDcp_PklUuidName;//拷贝的pklUUID名
std::string g_sCopyDcp_Copying_FileName;//正在拷贝的文件名
int g_flag_DcpCopyStop = 0;//标志位，1=取消导入DCP
unsigned long long g_Dcp_AllFile_Size = 0ull;//Dcp所有文件的大小。
unsigned long long g_hasDcpCopied_Size = 0ull;//已经拷贝的dcp包的大小。
std::map< std::string , FileCopyInfo > g_map_FileCopyInfo;//映射表，用于生成文件拷贝状态信息【文件路径名，FileCopyInfo】
cftp::FtpClient g_FtpClient;//ftp客户端

//g_bIsFtp_UploadFile_Failed：表示是否ftp上传操作在某个地方出现过失败
int g_bIsFtp_UploadFile_Failed = 0;  //=1,表示出现失败

//@author zhangmiao@oristartech.com
//@date [2012-07-10]
//@brief 声明用于hash校验的全局变量
//@new
extern int g_flag_SHA1CheckStop;
extern std::string g_sHashDcp_PklUuidName;
extern std::string g_sHashDcp_Checking_FileName;
extern unsigned long long g_AllFile_Size;
extern unsigned long long g_hasHashChecked_Size;
//@modify end;

std::map<std::string,CPL_CONTENT_KIND> mapCPL_ContentKind;

void init_maplist()
{
	mapCPL_ContentKind["feature"]=feature;
	mapCPL_ContentKind["trailer"]=trailer;
	mapCPL_ContentKind["test"]=test;
	mapCPL_ContentKind["teaser"]=teaser;
	mapCPL_ContentKind["rating"]=rating;
	mapCPL_ContentKind["advertisement"]=advertisement;
	mapCPL_ContentKind["short"]=shorts;
	mapCPL_ContentKind["transitional"]=transitional;
	mapCPL_ContentKind["psa"]=psa;
	mapCPL_ContentKind["policy"]=policy;
}

//@author zhangmiao@oristartech.com
//@date [2013-08-12]
//@brief 主要增加了Satellite专用卫星接口的处理代码
//@new
#include "C_SatXmlParser.h"
#include "C_Satellite.h"
//在IP地址所指的DCP专用卫星接收设备中，搜索FTP服务器全部的DCP包。
int SearchSatDcpList( const string &Ip , vector<Content::AssetmapInfo> &dcpList , string& error  , unsigned short usPort )
{
	int ret(0);
	vector<string> vTmp;
	vTmp.clear();

	C_Satellite Sate;

	//初始化建立与卫星接收设备tcp连接
	//unsigned short usPort = 20080;
	ret = Sate.Init( Ip.c_str() , usPort );
	if( ret != 0 )
	{
		printf("Error:C_Satellite::Init() , IP = %s !\n" , Ip.c_str() );

		char buffer[BUF_SIZE]="";
		sprintf( buffer , "Error:C_Satellite::Init() , IP = %s !\n" , Ip.c_str() );
		error = buffer + error;
		WELOG(buffer);

		ret = CMTT(ERROR_CONTENT_DCP_SATE_INIT);
		return ret;
	}

	//建立连接认证请求
	ret = Sate.CreateCertConnect();
	if( ret != 0 )
	{
		printf( "Error:C_Satellite::CreateCertConnect() , IP = %s !\n" , Ip.c_str() );

		char buffer[BUF_SIZE]="";
		sprintf( buffer , "Error:C_Satellite::CreateCertConnect() , IP = %s !\n" , Ip.c_str() );
		error = buffer + error;
		WELOG(buffer);

		ret = CMTT(ERROR_CONTENT_DCP_SATE_CREATE_CERT_CONNECT);
		return ret;
	}

	//获得影片信息列表
	vector< Satellite::DCPInfo > vSatDcpList;
	ret = Sate.GetDcpList( vSatDcpList );
	if( ret != 0 )
	{
		printf( "Error:C_Satellite::GetDcpList() , IP = %s !\n" , Ip.c_str() );

		char buffer[BUF_SIZE]="";
		sprintf( buffer , "Error:C_Satellite::GetDcpList() , IP = %s !\n" , Ip.c_str() );
		error = buffer + error;
		WELOG(buffer);

		ret = CMTT(ERROR_CONTENT_DCP_SATE_GET_DCP_LIST);
		return ret;
	}
	//如果影片不存在，则直接返回即可。
	int nDcpListCount =  vSatDcpList.size();
	if ( nDcpListCount == 0 )
	{
		printf("Warning:C_Satellite::GetDcpList(),nDcpListCount = 0 !\n");
		return 0;
	}

	//获得对应影片FTP信息
	vector< Satellite::FTP_DCPInfo > vtFTP_DCPInfoList;
	for ( int i = 0; i < nDcpListCount; i++ )
	{
		Satellite::FTP_DCPInfo aFTP_DCPInfo;

		Satellite::DCPInfo& rDCPInfo = vSatDcpList[i];
		string& rPklUuid = rDCPInfo.PklUuid;

		//过滤掉重复的PklUuid
		int result = count(vTmp.begin(),vTmp.end(), rPklUuid );
		if( result != 0 ) 
		{
			printf( "Warning Sate:PklUuid Repeat:%s!\n", rPklUuid.c_str() );
			continue;
		}
		else
		{
			vTmp.push_back(rPklUuid);
		}

		ret = Sate.GetFTPDCPInfoById( rPklUuid , aFTP_DCPInfo );
		if( ret != 0 )
		{
			printf( "Error:C_Satellite::GetFTPDCPInfoById(), Uuid= %s !\n" , rPklUuid.c_str() );

			char buffer[BUF_SIZE]="";
			sprintf( buffer , "Error:C_Satellite::GetFTPDCPInfoById() , Uuid= %s !\n" , rPklUuid.c_str() );
			error = buffer + error;
			WELOG(buffer);

			continue;
		}

		vtFTP_DCPInfoList.push_back( aFTP_DCPInfo );
	}

	int nFTP_DCPInfoListCount = vtFTP_DCPInfoList.size();
	if ( ( nFTP_DCPInfoListCount < nDcpListCount ) && ( nFTP_DCPInfoListCount != 0 ) )
	{
		printf("Warning Sate:nFTP_DCPInfoListCount < nDcpListCount!\n");
	}
	else if( ( nFTP_DCPInfoListCount == 0 ) && ( nDcpListCount != 0 ) )
	{
		printf("error:Sate vtFTP_DCPInfoList = 0 , IP = %s !\n" , Ip.c_str() );

		char buffer[BUF_SIZE]="";
		sprintf( buffer , "Error:Sate vtFTP_DCPInfoList = 0 , IP = %s !\n" , Ip.c_str() );
		error = buffer + error;
		WELOG(buffer);

		ret = CMTT(ERROR_CONTENT_DCP_SATE_GET_FTP_DCPINFO_BY_ID);
		return ret;
	}

	//搜索FTP服务器上的DCP包
	vTmp.clear();

	for ( int j = 0; j < nFTP_DCPInfoListCount; j++ )
	{
		Satellite::FTP_DCPInfo& rFTP_DCPInfo = vtFTP_DCPInfoList[j];
		string sFTP_URL_PATH = rFTP_DCPInfo.ftp_url_path;

		//过滤掉重复的FTP_URL_PATH
		int result = count( vTmp.begin(),vTmp.end(), sFTP_URL_PATH );
		if( result != 0 ) 
		{
			printf( "Warning Sate FTP_URL_PATH Repeat:%s!\n", sFTP_URL_PATH.c_str() );
			continue;
		}
		else
		{
			vTmp.push_back(sFTP_URL_PATH);
		}
		
		ret = SearchFtpDcpList( sFTP_URL_PATH , dcpList , error );
		if( ret != 0 )
		{
			printf( "Warning Sate:SearchFtpDcpList(),error FTP_URL= %s !\n" , sFTP_URL_PATH.c_str() );

			char buffer[BUF_SIZE]="";
			sprintf( buffer,"Warning Sate:error,SearchFtpDcpList(),FTP_URL= %s !\n" , sFTP_URL_PATH.c_str() );
			error = buffer + error;
			WELOG(buffer);
			
			continue;
		}
	}

	int nSatDCPListCount = dcpList.size();
	if ( ( nFTP_DCPInfoListCount != 0 ) && ( nSatDCPListCount != 0 ) )
	{
		if ( nSatDCPListCount < nFTP_DCPInfoListCount )
		{
			printf( "Warning Sate:SearchSatDcpList(), IP = %s , nSatDCPListCount < nFTP_DCPInfoListCount !\n" , Ip.c_str() );
		}
		ret = 0;
	}
	else if ( ( nFTP_DCPInfoListCount != 0 ) && ( nSatDCPListCount == 0 ) )
	{
		printf( "Error Sate:SearchSatDcpList() , IP = %s !\n" , Ip.c_str() );

		char buffer[BUF_SIZE]="";
		sprintf( buffer,"Error Sate:SearchSatDcpList() , IP = %s !\n" , Ip.c_str() );
		WELOG(buffer);
		error = buffer + error;
		ret = CMTT(ERROR_CONTENT_DCP_SEARCH_SATE_DCP_LIST);
	}

	return ret;
}

//对应影片下载完毕请求；如果status = 1，表示影片下载完毕。status = 0，表示下载失败。
int Sate_SetFTPDownLoadFinished( const string& Ip , const string& PklUuid , const int Status , string& error )
{
	int ret(0);
	
	C_Satellite Sate;

	//初始化建立与卫星接收设备tcp连接
	unsigned short usPort = 20080;
	ret = Sate.Init( Ip.c_str() , usPort );
	if( ret != 0 )
	{
		printf("Error:C_Satellite::Init() , IP = %s !\n" , Ip.c_str() );

		char buffer[BUF_SIZE]="";
		sprintf( buffer , "Error:C_Satellite::Init() , IP = %s !\n" , Ip.c_str() );
		error = buffer + error;
		WELOG(buffer);

		ret = CMTT(ERROR_CONTENT_DCP_SATE_INIT);
		return ret;
	}
	
	//建立连接认证请求
	ret = Sate.CreateCertConnect();
	if( ret != 0 )
	{
		printf( "Error:C_Satellite::CreateCertConnect() , IP = %s !\n" , Ip.c_str() );

		char buffer[BUF_SIZE]="";
		sprintf( buffer , "Error:C_Satellite::CreateCertConnect() , IP = %s !\n" , Ip.c_str() );
		error = buffer + error;
		WELOG(buffer);

		ret = CMTT(ERROR_CONTENT_DCP_SATE_CREATE_CERT_CONNECT);
		return ret;
	}

	//对应影片下载完毕；如果status = 1，表示影片下载完毕。status = 0，表示下载失败。
	ret = Sate.SetFTPDownLoadFinished( PklUuid , Status );
	if( ret != 0 )
	{
		printf( "Error:C_Satellite::SetFTPDownLoadFinished() , IP = %s , PklUuid = %s , Status = %d !\n" , Ip.c_str() , PklUuid.c_str() , Status );

		char buffer[BUF_SIZE]="";
		sprintf( buffer , "Error:C_Satellite::SetFTPDownLoadFinished() , IP = %s , PklUuid = %s , Status = %d !\n" , Ip.c_str() , PklUuid.c_str() , Status );
		error = buffer + error;
		WELOG(buffer);

		ret = CMTT(ERROR_CONTENT_DCP_SATE_SET_FTP_DOWNLOAD_FINISHED);
		return ret;
	}

	return ret;
}
//@modify end;

int SearchFtpDcpList( const std::string &ftpUrl, std::vector<AssetmapInfo> &dcpList ,std::string& error )
{
	//@author zhangmiao@oristartech.com
	//@date [2013-07-18]
	//@brief 主要增加了FTP下载相关的处理代码，用于解决assetmap 文件重复出现的问题。
	//@new
	vector<string> vTmp;
	vTmp.clear();
	vector<AssetmapInfo>::size_type nAssetmapNum = 0;
	nAssetmapNum = dcpList.size();
	if ( nAssetmapNum != 0 )
	{	
		for ( vector<AssetmapInfo>::size_type i=0; i<nAssetmapNum; i++ )
		{
			AssetmapInfo& aAssetMapInfo = dcpList[i];
			string& assetmapUuid = aAssetMapInfo.assetmapUuid;
			int result = 0;
			result = count( vTmp.begin() , vTmp.end() , assetmapUuid );
			if( result != 0 ) 
			{
				continue;
			}
			else
			{
				vTmp.push_back(assetmapUuid);
			}
		}
	}
	//@modify end;

	string url = ftpUrl;
	cftp::FtpClient ftp;
	//log in
	int result = ftp.LogIn(ftpUrl);
	if(result != cftp::NOANYERROR)
		return CMTT2(result);

	string AssetMapFileNameSet[4]  = { "ASSETMAP","assetmap","ASSETMAP.xml","assetmap.xml" };
	for ( int nAssetMapFNCount = 0; nAssetMapFNCount < 4; nAssetMapFNCount++ )
	{
		string filename = AssetMapFileNameSet[nAssetMapFNCount];

		std::vector<cftp::FileInformation> vfileList;
		result = listFilesOnFTP( ftp, "" ,vfileList ,filename );
		if( result != cftp::NOANYERROR )
		{
			//old code:return CMTT2(result);
			int nRet = 0;
			nRet = ftp.ReLogIn();
			if( nRet != cftp::NOANYERROR )
			{
#ifdef _TEST_
				printf("\nError:FtpClient ReLogIn()!\n" );
#endif
				nRet = ftp.LogIn(ftpUrl);
				continue;
			}

			nRet = listFilesOnFTP( ftp , "" ,vfileList ,filename );
			if( nRet != cftp::NOANYERROR )
			{
				nRet = ftp.ReLogIn();
				continue;
			}
		}

		int assetmapN = 0;
		assetmapN = vfileList.size();
		for ( int i=0; i<assetmapN; i++ )
		{
			cftp::FileInformation FileInfo = vfileList[i];
			string name = FileInfo.name; //获取文件名;  带有相对路径，是相当于ftpurl上的路径，如：amovie/ASSETMAP

			string ip, userName, passwd, dir;
			string port = "21";

			//如:url = ftp://zhangm:123456@127.0.0.1/dcproot/
			result = ftp.ParseUrl(url, ip, userName, passwd, port, dir);
			if(result != cftp::NOANYERROR)
				return CMTT2(result);

			//string URL_PFX = url.substr(0,end);
			if(filename.compare(name.substr( name.rfind( '/' )+1 ))==0)
			{
				//取出name的目录部分,如;amovie/
				string sFilePath;
				if ( name.rfind( '/' )!= string::npos )
				{
					sFilePath = name.substr( 0,name.rfind( '/' )+1);
				}
				//将ftpurl的目录部分与name的目录部分，组合成完整的下载目录
				//如:url = ftp://zhangm:123456@127.0.0.1/dcproot/  如dir='/dcproot/' sFilePath='amovie/'
				dir += sFilePath;

				string sFileUrl = ftpUrl + name;
#ifdef _TEST_
				printf("\nFileUrl= %s\n" ,sFileUrl.c_str());
#endif
				//下载相应的文件到当前目录下
				result = ftp.TransportFile( dir  , "." , name.substr( name.rfind( '/' )+1) );
				if(result != cftp::NOANYERROR)
					return CMTT2(result);

				//解析ASSETMAP文件，创建目录，以assetmapUuid为名字。
				AMInfo_t aAMInfo;
				int ret = 0;
				{
					DcpInfo dcp;
					ret = dcp.Parse_Assetmap_New( string(".")+DIR_SEPCHAR+filename, aAMInfo , error );
					if(ret==-1)
					{
						char buffer[BUF_SIZE]="";
						sprintf( buffer,"parseAssetmap error!:%s\n" , (string(".")+DIR_SEPCHAR+filename).c_str() );
						error = buffer;
						
						printf( "parseAssetmap error!:%s\n" , (string(".")+DIR_SEPCHAR+filename).c_str() );
						//printf( "parseAssetmap error!\n" );

						//return -1;
						WELOG(error)
						return CMTT2(ERROR_CONTENT_DCP_PARSE_ASSETMAP_ERROR);
					}
				}

				// delete old ASSETMAP 
				//ret = system( (RMDIR + string(".\ASSETMAP")).c_str() );
				ret = remove( filename.c_str() );
				string assetmapUuid = ((aAMInfo).aAssetmap_Info).assetmapUuid;

				//@author zhangmiao@oristartech.com
				//@date [2013-07-18]
				//@brief 主要增加了FTP下载相关的处理代码，用于解决assetmap 文件重复出现的问题。
				//@new
				int result = count(vTmp.begin(),vTmp.end(), assetmapUuid );
				if( result != 0 ) 
				{
					printf( "Warning AssetmapUuid Repeat:%s!\n", sFileUrl.c_str() );
					continue;
				}
				else
				{
					vTmp.push_back(assetmapUuid);
				}
				//@modify end;

				//Parameters Check
				string dest = ".";
				string sFilmFilePath = dest;
				if( sFilmFilePath.rfind(DIR_SEPCHAR) != sFilmFilePath.length()-1 )
					sFilmFilePath.append( 1, DIR_SEPCHAR );

				size_t pos;
				if ( ( pos = assetmapUuid.find("urn:uuid:") ) != assetmapUuid.npos && pos==0 )
				{
					sFilmFilePath = sFilmFilePath + assetmapUuid.substr(9);
				}
				else
				{
					sFilmFilePath = sFilmFilePath + assetmapUuid;
				}
				sFilmFilePath.append( 1, DIR_SEPCHAR );

				if ( sFilmFilePath.length()==0 )
				{
					error = "Error:FilmFilePath is NULL!\n";
					//errno = -2;
					errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
					WELOG(error)
					return CMTT2(errno);
				}

				string  Dest = sFilmFilePath;
				ret = MakeDirectory(Dest);
				if(ret!=0)
				{
					error = "Error:Make Directory fail.\n";
					WELOG(error)
					ret = CMTT2(ERROR_CONTENT_DCP_MAKE_DIR_FAIL);
					return ret;
				}

				//下载ASSETMAP文件 
				result = ftp.TransportFile( dir , sFilmFilePath, name.substr( name.rfind( '/' )+1));
				if(result != cftp::NOANYERROR)
					return CMTT2(result);

				//下载dcp配置文件    
				vector<asset_assetmap_t> asset = ((aAMInfo).aAssetmap_Info).asset;
				vector<asset_assetmap_t>::size_type nSize = asset.size();
				for ( vector<asset_assetmap_t>::size_type j=0;j<nSize;j++ )
				{
					asset_assetmap_t asset_assetmap = asset[j];
					//@author zhangmiao@oristartech.com
					//@date [2013-07-18]
					//@brief 主要增加了FTP下载相关字幕文件的处理代码
					//@new
					/*string Path = asset_assetmap.path;
					if ( Path.rfind(".mxf")== string::npos )
					{
						result = ftp.TransportFile( dir , sFilmFilePath ,Path.substr( Path.rfind( DIR_SEPCHAR )+1) );
						if(result != cftp::NOANYERROR)
							return CMTT2(result);
					}*/
					string Path = asset_assetmap.Path_NotFullPath;
					if ( Path.rfind(".mxf")== string::npos && Path.rfind(".png")== string::npos && Path.rfind("/")== string::npos )
					{
						result = ftp.TransportFile( dir , sFilmFilePath , Path );
						if(result != cftp::NOANYERROR)
							return CMTT2(result);
					}
					//@modify end;
				}
				string srcPath = sFilmFilePath;
				ret = SearchDcpList(srcPath,dcpList,error);
				if(ret!=0)
				{
					char buffer[BUF_SIZE]="";
					sprintf( buffer,"error:SearchDcpList() = %s!\n",srcPath.c_str() );
					error = buffer;
					printf("error:SearchDcpList() = %s!\n",srcPath.c_str());

					WELOG(error)
					ret = CMTT2(ERROR_CONTENT_DCP_SEARCH_DCP_LIST);
					return ret;
				}

				//重新设定路径名 dir
				int nAMInfoCount = dcpList.size();
				for ( int j=0; j< nAMInfoCount; j++ )
				{
					AssetmapInfo& aAssetMapInfo = dcpList[j];
					if (assetmapUuid==aAssetMapInfo.assetmapUuid)
					{
						aAssetMapInfo.dcpPath = dir;
						string assetmapName = aAssetMapInfo.assetmapName;
						aAssetMapInfo.assetmapName = dir + assetmapName.substr(assetmapName.rfind(DIR_SEPCHAR)+1);
						vector<PklInfo>& pklList = aAssetMapInfo.pklList;
						int pklN = pklList.size();
						for( int k=0; k<pklN; k++ )
						{
							PklInfo& aPkl = pklList[k];	
							string pklName = aPkl.pklName;
							aPkl.pklName = dir + pklName.substr(pklName.rfind(DIR_SEPCHAR)+1);
							vector<CplInfo>& cplList = aPkl.cplList;
							vector<MxfInfo>& mxfList = aPkl.mxfList;
							int cplCount = cplList.size();
							for ( int m = 0; m < cplCount; m++ )
							{
								CplInfo& cpl = cplList[m];
								string cplName  = cpl.cplName;
								cpl.cplName =  dir + cplName.substr(cplName.rfind(DIR_SEPCHAR)+1);
							}
							int mxfCount = mxfList.size();
							for ( int m = 0; m < mxfCount; m++ )
							{
								MxfInfo& aMxfInfo = mxfList[m];
								string mxfName = aMxfInfo.fileName;
								aMxfInfo.fileName =  dir + mxfName.substr(mxfName.rfind(DIR_SEPCHAR)+1);
							}

							//@author zhangmiao@oristartech.com
							//@date [2013-07-18]
							//@brief 主要增加了FTP下载相关字幕文件的处理代码
							//@new
							vector<OtherFileInfo>& fileInfoList = aPkl.fileInfoList;
							int otherCount = fileInfoList.size();
							for ( int m = 0; m < otherCount; m++ )
							{
								OtherFileInfo& aFileInfo = fileInfoList[m];
								string originalfileName = aFileInfo.originalfileName;
								aFileInfo.fileName =  dir + originalfileName;
							}
							//@modify end;
						}
					}
				}

				//检查cpl文件有效状态
				/*ret = CheckCPLIsValid4Ftp( dir, assetmapUuid, ftp, dcpList );
				if( ret != 0 )
				{
					error = "Error:Check CPL Is InValid For FTP!\n";
					WELOG(error)
					ret = CMTT2(ERROR_CONTENT_DCP_CHECKCPL_INVALID_FORFTP);
					return ret;
				}*/

				//删除 ASSSETMAP 目录
				if( ( ret = access( Dest.c_str() , F_OK ) ) == 0 )     
				{
#ifdef _TEST_
					printf( "File Directory already exists :%s\n", Dest.c_str() );
#endif
					ret = system( (RMDIR + Dest).c_str() );
					if( ret == 0 )
					{
#ifdef _TEST_
						printf( "\nDirectory deleted :%s\n", Dest.c_str() );
#endif
						errno = 0;
					}
					else
					{
#ifdef _TEST_
						perror("\nUnable to delete directory\n");
#endif
						error = "\nUnable to delete ASSSETMAP directory\n";
						
						WELOG(error)
					}
				}
			}

		}
	}
		
	//@author zhangmiao@oristartech.com
	//@date [2014-07-18]
	//@brief 主要FTP下载时协议命令(quit)相应返回信息不稳定的问题
	//@old 以前代码
	/*result = ftp.LogOut();
	if(result != cftp::NOANYERROR)
		return CMTT2(result);*/
	//@new
	ftp.LogOut();
	//@modify end;

	return 0;
}
int SearchDcpList( const string &path, vector<AssetmapInfo> &dcpList ,std::string& error )
{
	init_maplist();

	int ret(0);
	DcpInfo dcp;
	vector< AMInfo > aAMInfo_Vt;
	ret = dcp.CreateDCPInfo( path , error );
	if( ret!=0 ) 
	{
#ifdef _TEST_
		printf("error:CreateDCPInfo()!\n");
#endif
		//error = "error:CreateDCPInfo()!\n";
		error = "错误：未找到有效的DCP包！";
		WELOG(error)
		ret = CMTT(ERROR_CONTENT_DCP_CREATE_DCPINFO_ERROR);
		return ret;
	}
	else if ( ret==0 )
	{
		aAMInfo_Vt = dcp.vAMInfo_t;
		vector< AMInfo >::size_type nAMInfoCount = aAMInfo_Vt.size();
		vector< AMInfo >::size_type i = 0;
		for ( i = 0; i < nAMInfoCount; i++ )
		{
			AssetmapInfo aAssetMapInfo;
			AMInfo_t aAMInfo = aAMInfo_Vt[i];
			//assetmap:uuid
			aAssetMapInfo.assetmapUuid = aAMInfo.aAssetmap_Info.assetmapUuid;
			//assetmap 文件名，区分大小写
			aAssetMapInfo.assetmapName = aAMInfo.assetmapName;
			///dcp所在路径，可能是磁盘绝对路径或ftp url
			aAssetMapInfo.dcpPath = aAMInfo.dcpPath;
			//整个dcp所有文件大小的总和，asssetmap中文件大小之和。
			long long allFileLength=0;
			vector<asset_assetmap_t>& aAssetVt = aAMInfo.aAssetmap_Info.asset;
			vector<asset_assetmap_t>::size_type nAssetVtCount = aAssetVt.size();
			for ( vector<asset_assetmap_t>::size_type j = 0; j < nAssetVtCount; j++ )
			{
				//assetmap文件中Length标记是可选的，如果某个Asset文件没有Length标记，则尝试计算文件长度。但无法保证计算不一定能成功。
				//主要是pkl、cpl文件的长度Length标记，可能没有，所以导致dcp总长计算出错误。
				unsigned long long nTmpFileLength = aAssetVt[j].Length;
				if ( nTmpFileLength == 0 )
				{
					string sTmpFilePath = aAssetVt[j].path;
					struct stat tmpstsrc;
					if( lstat( sTmpFilePath.c_str(), &tmpstsrc ) == 0 )
					{
						nTmpFileLength = tmpstsrc.st_size;
						aAssetVt[j].Length = nTmpFileLength;
					}
					else
						nTmpFileLength = 0;

					allFileLength += nTmpFileLength;
				}
				else 
					allFileLength += aAssetVt[j].Length;
			}
			aAssetMapInfo.dcpLength = allFileLength;
			//assetmap文件大小,assetmapLength.
			struct stat stsrc;
			if( lstat( aAssetMapInfo.assetmapName.c_str(), &stsrc ) == 0 )
				aAssetMapInfo.assetmapLength = stsrc.st_size;
			else{
#ifdef _TEST_
				printf("lstat %s is err:%s!\n",aAssetMapInfo.assetmapName.c_str(),strerror(errno));
#endif
				char buffer[BUF_SIZE]="";
				sprintf( buffer,"lstat %s is error:%s!\n",aAssetMapInfo.assetmapName.c_str(),strerror(errno) );
				error = buffer;
				break;
			}
			//-------------------
			vector< dcp_Info_t > vDcp = aAMInfo.vDcp;
			vector< dcp_Info_t >::size_type pklN = vDcp.size();
			for( vector< dcp_Info_t >::size_type j = 0; j < pklN; j++ )
			{
				PklInfo aPklInfo;

				dcp_Info_t dcp = vDcp[j];
				pkl_Info_t aPkl = dcp.aPkl;
				aPklInfo.pklUuid = aPkl.uuid;
				//----zhangmiao:begin---2012-09-24----
				//Content::PklInfo结构新增加了如下几个属性：AnnotationText、IssueDate、Issuer、Creator。
				aPklInfo.AnnotationText = aPkl.annotationText;
				aPklInfo.IssueDate      = aPkl.issueDate;
				aPklInfo.Issuer         = aPkl.issuer;
				aPklInfo.Creator        = aPkl.creator;
				//----zhangmiao:end-----2012-09-24----
				aPklInfo.allFileLength = aPkl.allFileLength;
				//设置pkl文件的大小、名字。
				vector<asset_assetmap_t>::size_type nAssetVtSize = aAssetVt.size();
				for ( vector<asset_assetmap_t>::size_type k=0; k < nAssetVtSize; k++ )
				{
					if ( aPklInfo.pklUuid == aAssetVt[k].uuid )
					{
						aPklInfo.pklName =  aAssetVt[k].path;
						aPklInfo.pklLength = aAssetVt[k].Length;
#ifdef _TEST_
						printf( "pklName:%s,pklLength=%llu\n", aPklInfo.pklName.c_str() ,aPklInfo.pklLength );
#endif
						break;
					}
				}

				//PklInfo：create cpl list
                vector< cpl_Info_t >::size_type cplCount = dcp.vCpl.size();
				vector< cpl_Info_t > vCpl = dcp.vCpl;
				for ( vector< cpl_Info_t >::size_type k = 0; k < cplCount; k++ )
				{
					CplInfo cpl;
					cpl.is3D = 0;
					cpl.cplUuid = vCpl[k].uuid;
					cpl.cplName = vCpl[k].path;
					cpl.type    = mapCPL_ContentKind[vCpl[k].contentKind];
					//----zhangmiao:begin---2012-09-25----
					//Content::CplInfo结构新增加了如下几个属性：cplTitle、cplIssueDate、cplssuer、cplCreator。
					cpl.cplTitle     = vCpl[k].contentTitleText;
					cpl.cplIssueDate = vCpl[k].issueDate;		//发行时间
					cpl.cplssuer     = vCpl[k].issuer;			//发行者
					cpl.cplCreator   = vCpl[k].creator;
					//----zhangmiao:end-----2012-09-25----
					//mainpicture
					string strEditRate  = "";
					string strFrameRate = "";
					string strKeyid     = "";
					string strSrcAspRat = "";

					long long fEditRate = 24;
					long long duration = 0ll;		//持续时间，以帧为单位;(duration/fEditRate)后为秒
					long long fRate = 0ll;			//frameRate,如果frameRate为空，就用editRate代替
					float fScreenAspectRatio = 0;	//横纵比
					int needKdm = 0;
					int is3D = 0;
					vector< asset_cpl_t > vAsset_Cpl = vCpl[k].asset;
					vector< asset_cpl_t >::size_type nAsset_CPL_Count = vAsset_Cpl.size();
					for ( vector< asset_cpl_t >::size_type m=0; m<nAsset_CPL_Count; m++ )
					{
						asset_cpl_t assetCpl = vAsset_Cpl[m];
						string kind = assetCpl.kind;
						/*if ( kind=="application/x-smpte-mxf;asdcpKind=Picture" 
							|| stricmp(kind.substr(kind.rfind(';')+1).c_str() ,"asdcpKind=Picture")==0 )*/
						if (  kind == "MainPicture" 
						   || kind == "MainStereoscopicPicture" )
						{
							int result = count( cpl.mainPicList.begin(),cpl.mainPicList.end(), assetCpl.uuid );
							if(result==0)
							//记录UUID
							cpl.mainPicList.push_back(assetCpl.uuid);
							
							//duration
							if ( assetCpl.duration.empty() )
							{
								duration += atoll( assetCpl.intrinsicDuration.c_str() );
							}
							else
							{
								duration += atoll( assetCpl.duration.c_str() );
							}

							//EditRate
							if(assetCpl.editRate.empty())
							{
								fEditRate = 24;
							}
							else
							{
								strEditRate = assetCpl.editRate;
								fEditRate = atoll(strEditRate.substr(0,strEditRate.find(' ')).c_str());
							}
							//fRate
							if ( assetCpl.frameRate.empty() )
							{
								strFrameRate = assetCpl.editRate;
							}
							else
							{
								strFrameRate = assetCpl.frameRate;
							}
							fRate = atoll(strFrameRate.substr(0,strFrameRate.find(' ')).c_str());

							//ScreenAspectRatio
							if ( !assetCpl.screenAspectRatio.empty() )
							{
								strSrcAspRat = assetCpl.screenAspectRatio;
								fScreenAspectRatio = (float)atof(assetCpl.screenAspectRatio.c_str());
							}

							//keyId
							if ( !assetCpl.keyId.empty() )
							{
								strKeyid = assetCpl.keyId;
								needKdm=1;
							}

							//is3D
							if ( kind == "MainStereoscopicPicture" )
							{
								is3D = 1;
							}
						}
						/*else if ( kind=="application/x-smpte-mxf;asdcpKind=Sound" 
							     || stricmp(kind.substr(kind.rfind(';')+1).c_str() ,"asdcpKind=Sound")==0 )*/
						else if ( kind=="MainSound"  )
						{
							int result = count( cpl.mainSoundList.begin(),cpl.mainSoundList.end(), assetCpl.uuid );
							if(result==0)
							//记录UUID
							cpl.mainSoundList.push_back(assetCpl.uuid);

							//keyId
							if ( !assetCpl.keyId.empty() )
							{
								strKeyid = assetCpl.keyId;
								needKdm=1;
							}

							//@author zhangmiao@oristartech.com
							//@date 2013-12-09
							//@brief 主要修改CplInfo结构体，增加CPL字段MainSoundLanguage的处理代码
							//@new
							//表示影片语言，返回语言代码，如果cpl文件中没有该字段，则默认返回未知,"unknown"。
							if ( assetCpl.language == "" )
							{
								//如果不存在，则默认返回未知,"unknown"。
								cpl.MainSoundLanguage = "unknown";
							}
							else
							{
								cpl.MainSoundLanguage = assetCpl.language;
							}
							//@modify end;
						}

					}

					cpl.is3D = is3D;
					cpl.rate = (int)fRate;
					cpl.duration = (int)(duration/fEditRate);
					cpl.Duration_double = (double)duration/fEditRate;    //double类型的duration，以秒为单位
					cpl.ScreenAspectRatio = fScreenAspectRatio;
					cpl.needKdm = needKdm;
					//set cpl文件的大小
					vector<asset_pkl_t> vAsset_pkl = aPkl.asset;
					vector<asset_pkl_t>::size_type nSize_Asset_Pkl = vAsset_pkl.size();
					for ( vector<asset_pkl_t>::size_type k = 0; k < nSize_Asset_Pkl; k++ )
					{
						asset_pkl_t aAsset_Pkl = vAsset_pkl[k];
						string type = aAsset_Pkl.type;
						//if ( (type == "text/xml;asdcpKind=CPL") && (aAsset_Pkl.uuid == "urn:uuid:"+cpl.cplUuid) )
						//@author zhangmiao@oristartech.com
						//@date [2013-09-18]
						//@brief 主要增加了对SMPTE格式的处理代码
						//@new
						//if ( (type == "text/xml;asdcpKind=CPL") && (aAsset_Pkl.uuid == cpl.cplUuid) )
						if ( aAsset_Pkl.uuid == cpl.cplUuid )
						//@modify end;
						{
							cpl.cplLength = aAsset_Pkl.size;
							break;
						}
					}

					//计算CPL包的大小
					vector<string> vTmp;
					long long allFileLength = 0ll;    //CPL包大小（文件长度总合）
					for ( vector< asset_cpl_t >::size_type m = 0; m < nAsset_CPL_Count; m++ )
					{
						asset_cpl_t assetCpl = vAsset_Cpl[m];
						string asset_cpl_Uuid = assetCpl.uuid;
						int result = count(vTmp.begin(),vTmp.end(), asset_cpl_Uuid );
						if(result!=0) continue;
							else   vTmp.push_back(asset_cpl_Uuid);
						for ( vector<asset_pkl_t>::size_type k = 0; k < nSize_Asset_Pkl; k++ )
						{
							asset_pkl_t aAsset_Pkl = vAsset_pkl[k];
							string Uuid_Asset_Pkl = aAsset_Pkl.uuid;
							//if ( Uuid_Asset_Pkl == "urn:uuid:" + asset_cpl_Uuid )
							if ( Uuid_Asset_Pkl == asset_cpl_Uuid )
							{
								allFileLength += aAsset_Pkl.size;
								break;
							}
						}
					}
					cpl.allFileLength = allFileLength;

					//检查cpl文件有效状态
					bool bIsInvalid  = false;
					bIsInvalid = CheckCPLIsValid( vAsset_Cpl , vAsset_pkl , error );					
					cpl.status = bIsInvalid;

					//将新的cpl插入PklInfo.cplList.
					aPklInfo.cplList.push_back(cpl);
				}
			   
			   //PklInfo： create mxf list
				vector<asset_pkl_t> vAsset_pkl = aPkl.asset;
				vector<asset_pkl_t>::size_type nSize_Asset_Pkl = vAsset_pkl.size();
				for ( vector<asset_pkl_t>::size_type k = 0; k < nSize_Asset_Pkl; k++ )
				{
					MxfInfo aMxfInfo;
					asset_pkl_t aAsset_Pkl = vAsset_pkl[k];
					string type = aAsset_Pkl.type;
					//@author zhangmiao@oristartech.com
					//@date [2013-09-18]
					//@brief 主要增加了对SMPTE格式的处理代码
					//@old
					/*if ( type == "application/x-smpte-mxf;asdcpKind=Picture" || 
						 type == "application/x-smpte-mxf;asdcpKind=Sound" || 
						 stricmp(type.substr(type.rfind(';')+1).c_str() ,"asdcpKind=Picture")==0 || 
						 stricmp(type.substr(type.rfind(';')+1).c_str() ,"asdcpKind=Sound")==0  )
				      */
					//@new
					if ( type == "application/x-smpte-mxf;asdcpKind=Picture" || 
						 type == "application/x-smpte-mxf;asdcpKind=Sound" || 
						 stricmp(type.substr(type.rfind(';')+1).c_str() ,"asdcpKind=Picture") ==0 || 
						 stricmp(type.substr(type.rfind(';')+1).c_str() ,"asdcpKind=Sound") ==0   || 
						 type == "application/mxf" ||
						 stricmp( type.c_str() ,"application/mxf" ) == 0 )
					//@modify end;
					{
						aMxfInfo.mxfUuid  = aAsset_Pkl.uuid;
						aMxfInfo.fileName = aAsset_Pkl.originalFileName;
						aMxfInfo.length   = aAsset_Pkl.size;
						aPklInfo.mxfList.push_back(aMxfInfo);
					}
				}
#ifdef _TEST_
			   printf( "create mxf list Number:%d\n", aPklInfo.mxfList.size() );
#endif

			   //@author zhangmiao@oristartech.com
			   //@date [2013-07-18]
			   //@brief 主要增加了FTP下载相关字幕文件的处理代码
			   //@new
			   //PklInfo： create Subtitle list
			   for ( vector<asset_pkl_t>::size_type k = 0; k < nSize_Asset_Pkl; k++ )
			   {
				   OtherFileInfo aOtherInfo;
				   asset_pkl_t aAsset_Pkl = vAsset_pkl[k];
				   string type = aAsset_Pkl.type;
				   //@author zhangmiao@oristartech.com
				   //@date [2013-09-22]
				   //@brief 主要增加了对SMPTE格式的处理代码
				   //@old
				   /*if ( type == "text/xml;asdcpKind=Subtitle" || 
					   type == "image/png" || 
					   stricmp(type.substr(type.rfind(';')+1).c_str() , "asdcpKind=Subtitle") == 0 || 
					   stricmp(type.c_str() , "image/png") == 0 )*/
				   //@new
				   if ( type == "text/xml;asdcpKind=Subtitle" || 
					    type == "image/png" || 
					   stricmp(type.substr(type.rfind(';')+1).c_str() , "asdcpKind=Subtitle") == 0 || 
					   stricmp(type.c_str() , "image/png") == 0 ||
					   type == "application/ttf" ||
					   stricmp( type.c_str() ,"application/ttf" ) == 0 )
				   //@modify end;
				   {
					   aOtherInfo.Uuid             = aAsset_Pkl.uuid;
					   aOtherInfo.fileName         = aAsset_Pkl.originalFileName;
					   aOtherInfo.originalfileName = aAsset_Pkl.Path_NotFullPath;
					   aOtherInfo.fileLength       = aAsset_Pkl.size;
					   aPklInfo.fileInfoList.push_back(aOtherInfo);
				   }
			   }
#ifdef _TEST_
			   printf( "create fileInfo list Number:%d\n", aPklInfo.fileInfoList.size() );
#endif
			   //@modify end;
			   
			   aAssetMapInfo.pklList.push_back(aPklInfo);
			}
			//dcpList
			dcpList.push_back(aAssetMapInfo);
		}
		//对dcpList进行排序，如果大于1个
		if( dcpList.size() > 1)
		{
		    sort(dcpList.begin(),dcpList.end(),FuncCompDcpList());
		}
	}
	

	return 0;
}
int CheckCPLIsValid( const vector< asset_cpl_t >& vAsset_Cpl , const vector< asset_pkl_t >& vAsset_pkl ,std::string& error )
{
	bool bIsInvalid  = false;
    vector< asset_pkl_t >::size_type nSize_Asset_Pkl = vAsset_pkl.size();
    vector< asset_cpl_t >::size_type nAsset_CPL_Count = vAsset_Cpl.size();
    vector< asset_cpl_t >::size_type m = 0;
	for ( m = 0; m < nAsset_CPL_Count; m++ )
	{
		asset_cpl_t assetCpl = vAsset_Cpl[m];
		string asset_cpl_Uuid = assetCpl.uuid;

		vector< asset_pkl_t >::size_type k = 0;
		for ( k = 0; k < nSize_Asset_Pkl; k++ )
		{
			asset_pkl_t aAsset_Pkl = vAsset_pkl[k];
			string Uuid_Asset_Pkl = aAsset_Pkl.uuid;
			//if ( Uuid_Asset_Pkl == "urn:uuid:" + asset_cpl_Uuid )
			if ( Uuid_Asset_Pkl == asset_cpl_Uuid )
			{
				string fileName = aAsset_Pkl.originalFileName;
				//获得文件大小,assetLength
				unsigned long long assetLength = 0;
				struct stat stsrc;
				if( lstat( fileName.c_str(), &stsrc ) == 0 )
					assetLength = stsrc.st_size;
				else{
					bIsInvalid = true;
#ifdef _TEST_
					printf("lstat %s is error:%s!\n",fileName.c_str(),strerror(errno));
#endif
					char buffer[BUF_SIZE]="";
					sprintf( buffer,"lstat %s is error:%s!\n",fileName.c_str(),strerror(errno) );
					error += buffer;
					break;
				}
				//比较文件大小
				unsigned long long filesize = aAsset_Pkl.size;
				if ( filesize != assetLength )
				{
					bIsInvalid = true;
					break;
				}
				break;
			}
		}
		//发现出了问题，可以终止检查。
		if ( bIsInvalid == true )
		{
			break;
		}
	}

	return bIsInvalid;
}
int CheckCPLIsValid4Ftp( const string dir , const string assetmapUuid , const cftp::FtpClient& ftp , vector<AssetmapInfo>& dcpList )
{
	int ret(0);
	int nAMInfoCount = dcpList.size();
	for ( int j=0; j < nAMInfoCount; j++ )
	{
		AssetmapInfo& aAssetMapInfo = dcpList[j];
		if ( assetmapUuid == aAssetMapInfo.assetmapUuid )
		{
			vector<PklInfo>& pklList = aAssetMapInfo.pklList;
			int pklN = pklList.size();
			for( int k=0; k < pklN; k++ )
			{
				PklInfo& aPkl = pklList[k];	
				vector<CplInfo>& cplList = aPkl.cplList;
				vector<MxfInfo>& mxfList = aPkl.mxfList;
				int cplCount = cplList.size();
				for ( int m = 0; m < cplCount; m++ )
				{
					bool bIsInvalid  = false;
					vector< string > mainItemList;//记录UUID

					//这里将先将视频、音频文件uuid取出插入到mainItemList中，再与mxfList比较做筛选，找出对应mxf文件信息
					CplInfo& cpl = cplList[m];	
					for ( vector< string >::size_type i = 0; i < cpl.mainPicList.size(); i++ )
						mainItemList.push_back( cpl.mainPicList[i] );

					for ( vector< string >::size_type i = 0; i < cpl.mainSoundList.size(); i++ )
						mainItemList.push_back( cpl.mainSoundList[i] );

					int nListCount = mainItemList.size();
					for ( int i = 0; i < nListCount; i++ )
					{
						string Uuid = mainItemList[i];
						int mxfCount = mxfList.size();
						for ( int n = 0; n < mxfCount; n++ )
						{
							MxfInfo& aMxfInfo = mxfList[n];
							//if ( "urn:uuid:" + Uuid == aMxfInfo.mxfUuid )
							if ( Uuid == aMxfInfo.mxfUuid )	//与mxfList比较做筛选，找出对应mxf文件信息
							{
								string mxfName = aMxfInfo.fileName;
								string fileName = mxfName;
								//check file exist 
								vector<cftp::FileInformation> fileList;
								ret = listFilesOnFTP( (cftp::FtpClient&)ftp, fileName.substr(0,fileName.rfind('/')+1), fileList, fileName.substr(fileName.rfind('/')+1) );
								if( ret != cftp::NOANYERROR )
									return ret;
								ret = ( fileList.size()!=0 && fileList[0].name==fileName );
								if( ret  )
								{
#ifdef _TEST_
									printf( "File  exists :%s\n", fileName.c_str() );
#endif
									long long nFileLength = atoll(fileList[0].size.c_str());
									if ( nFileLength == aMxfInfo.length )
									{
										bIsInvalid = false;
									}
									else
									{
										bIsInvalid = true;
										break;
									}
								}
								else
								{
#ifdef _TEST_
									printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif
									bIsInvalid = true;
									break;
								}
								break;
							}
						}
						//发现出了问题，可以终止检查。
						if ( bIsInvalid == true )
						{
							break;
						}
					}

					cpl.status = bIsInvalid;
				}
			}
		}
	}

	return 0;
}

#if DCPCPY == 1
int GetKdmInfo( const std::string &path, std::vector<KdmInfo> &kdmList, std::string& error )
{
	int ret(0);
	std::string Path;
	DcpInfo dcp;
	bool bIsFileName = false;
	if ( path.rfind(DIR_SEPCHAR) != (path.size()-1) )
	{
		bIsFileName = true;
	}

	if ( bIsFileName )
	{
		KdmInfo kdmInfo;
		ret = dcp.GetKdmInfo( path , kdmInfo , error );
		if( ret!=0 )
		{
			printf("GetKdmInfo error!\n");
#ifdef _TEST_
			error = "GetKdmInfo error!\n" + error;
#else
			error = "错误：未找到有效的KDM！";
#endif
			ret = CMTT(ERROR_CONTENT_KDM_GETKDMINFOERROR);
			WELOG(error)
		}
		else
		{
			kdmList.push_back(kdmInfo);
		}
		return ret;
	}
	else
	{
		Path = path;
		//Path[Path.length()-1] = '\0';
		Path.erase(Path.length()-1);
	}
	
	ret = dcp.GetKdmInfo( Path ,kdmList , error );
	if( ret!=0 )
	{
		printf("GetKdmInfo error!\n");
#ifdef _TEST_
		error = "GetKdmInfo error!\n" + error;
#else
		error = "错误：未找到有效的KDM！";
#endif
		ret = CMTT(ERROR_CONTENT_KDM_GETKDMINFOERROR);
		WELOG(error)
	}
	
	return ret;
}
#endif // #if DCPCPY == 1

#if DCPCPY == 1
int DeleteKdm( const std::string &path  ,std::string& error )
{
	int ret(0);
	int nIsLocal_Delete_Failed = 0;

	/*if ( path.length()==0 )
	{
		error = "DeleteKdm() error!:bad Path";
		//return -1;
		WELOG(error);
		return CMTT2(ERROR_CONTENT_KDM_DELETEKDM_ERROR);
	}
	if( ( ret = access( path.c_str() , F_OK ) ) == 0 )     //check file exist
	{
#ifdef _TEST_
		printf( "File already exists :%s\n", path.c_str() );
		printf( "Delete file :%s\n", path.c_str() );
#endif

		ret = remove( path.c_str() );
		if( ret == 0 )
		{
#ifdef _TEST_
			printf( "\nDelete Kdm file Ok:%s\n", path.c_str() );
#endif

			errno = 0;
		}
		else
		{
#ifdef _TEST_
			perror("\nUnable to delete Kdm File\n");
#endif
			error = "\nUnable to delete Kdm File\n";

			nIsLocal_Delete_Failed = 1;
		}
	}
	else
	{
		//Do not found Kdm
		error = "Do not found Kdm!\n";
		WELOG(error);
		errno = ERROR_CONTENT_KDM_DONOT_FOUND_KDM_ERROR;
		//return CMTT2(ERROR_CONTENT_KDM_DONOT_FOUND_KDM_ERROR);

		nIsLocal_Delete_Failed = 1;
	}

	if ( nIsLocal_Delete_Failed == 1 )  //继续删除ftp上的kdm
	{
		string Dest2;
		if( path.length() != 0 && Dest2.length() == 0 ) 
		{
#ifdef _READDB_KDM_DEST_PATH2
			//直接读取数据库表
			int iResult(0);
			//C_Para::GetInstance()->ReadPara();
			C_RunPara *pRunPara = C_RunPara::GetInstance();
			iResult = pRunPara->GetKdmPath2( Dest2 );
			if( iResult != 0 )
			{
				error += "Error:KDM DestPath2 is NULL!\n";
				WELOG(error);
				errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
				return CMTT2(errno);
			}
#endif // _READDB_KDM_DEST_PATH2
		}
		
		if ( Dest2.length() != 0 )
		{
			cftp::FtpClient ftp;

			string ftpUrl = Dest2;

			//log in
			ret = ftp.LogIn(ftpUrl);
			if(ret != cftp::NOANYERROR)
			{
				printf("\nFTP Url LogIn error:%s\n" , ftpUrl.c_str() );

				error += "\nFTP Url LogIn error!\n";

				WELOG(error);
				return CMTT2(ERROR_CONTENT_FTP_UNKNOWERROR);
			}

			string ip, userName, passwd, dir;
			string port = "21";

			ret = ftp.ParseUrl( ftpUrl , ip , userName , passwd , port , dir );
			if(ret != cftp::NOANYERROR)
			{
				printf("\nFTP Url ParseUrl error:%s\n" , ftpUrl.c_str() );

				error += "\nFTP Url ParseUrl error!\n";

				WELOG(error);
				errno = ERROR_CONTENT_FTP_PARAMETERERROR;
				return CMTT2(ERROR_CONTENT_FTP_PARAMETERERROR);
			}

			string sKdmFileName;
			if ( path.rfind( DIR_SEPCHAR ) != string::npos )
			{
				sKdmFileName = path.substr( path.rfind( DIR_SEPCHAR ) + 1 );
			}

			ret = ftp.DeleteFile( dir + sKdmFileName );
			if( ret == cftp::NOANYERROR )
			{
#ifdef _TEST_
				printf( "\nFTP Kdm File deleted :%s\n", (dir + sKdmFileName).c_str() );
#endif

				error += "\nFTP Delete KDM File local failed , but SLAVE KDM Dest Path2 OK!\n";

				WELOG(error);

				//Delete KDM File local failed , but SLAVE KDM Dest Path2 OK!
				errno = ERROR_CONTENT_KDM_DELKDM_ERROR_LOCAL_FAIL_BUT_SLAVE_OK;
				return CMTT2(errno);
			}
			else
			{
#ifdef _TEST_
				printf("\nFTP Unable to delete Kdm File :%s\n" , Dest2.c_str() );
#endif
				error += "\nFTP Unable to delete Kdm File!\n";

				WELOG(error);
				errno = ERROR_CONTENT_KDM_DELETEKDM_ERROR;
				return CMTT2(errno);
			}
		}
	}

	//@author zhangmiao@oristartech.com
	//@date [2014-12-16]
	//@brief 主要增加删除KDM支持Ftp目标目录的处理代码
	//@new
	string Dest2;
	if( path.length() != 0 && Dest2.length() == 0 ) 
	{
#ifdef _READDB_KDM_DEST_PATH2
		//直接读取数据库表
		int iResult(0);
		//C_Para::GetInstance()->ReadPara();
		C_RunPara *pRunPara = C_RunPara::GetInstance();
		iResult = pRunPara->GetKdmPath2( Dest2 );
		if( iResult != 0 )
		{
			error = "Error:KDM DestPath2 is NULL!\n";
			WELOG(error);
			//errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
			errno = ERROR_CONTENT_KDM_DELETE_SLAVE_KDM_ERROR;
			return CMTT2(errno);
		}
#endif // _READDB_KDM_DEST_PATH2
	}

	if ( Dest2.length() != 0 )
	{
		cftp::FtpClient ftp;

		string ftpUrl = Dest2;

		//log in
		ret = ftp.LogIn(ftpUrl);
		if(ret != cftp::NOANYERROR)
		{
			printf("\nFTP SLAVE Url LogIn error:%s\n" , ftpUrl.c_str() );
			
			error += "\nFTP SLAVE Url LogIn error!\n";

			WELOG(error);
			//return CMTT2(ret);
			return CMTT2(ERROR_CONTENT_KDM_DELETE_SLAVE_KDM_ERROR);
		}
		
		string ip, userName, passwd, dir;
		string port = "21";

		ret = ftp.ParseUrl( ftpUrl , ip , userName , passwd , port , dir );
		if(ret != cftp::NOANYERROR)
		{
			printf("\nFTP SLAVE Url ParseUrl error:%s\n" , ftpUrl.c_str() );

			error += "\nFTP SLAVE Url ParseUrl error!\n";

			WELOG(error);

			//return CMTT2(ret);
			return CMTT2(ERROR_CONTENT_KDM_DELETE_SLAVE_KDM_ERROR);
		}

		string sKdmFileName;
		if ( path.rfind( DIR_SEPCHAR ) != string::npos )
		{
			sKdmFileName = path.substr( path.rfind( DIR_SEPCHAR ) + 1 );
		}

		ret = ftp.DeleteFile( dir + sKdmFileName );
		if( ret == cftp::NOANYERROR )
		{
#ifdef _TEST_
			printf( "\nFTP SLAVE Kdm File deleted :%s\n", (dir + sKdmFileName).c_str() );
#endif
			
			errno = 0;
		}
		else
		{
			printf("\nFTP SLAVE Unable to delete Kdm File :%s\n" , (dir + sKdmFileName).c_str() );

			error += "\nFTP SLAVE Unable to delete Kdm File!\n";

			WELOG(error);

			//return CMTT2(ret);
			return CMTT2(ERROR_CONTENT_KDM_DELETE_SLAVE_KDM_ERROR);
		}
	}
	//@modify [2014-12-16] end;
*/
	return ret;
}
#endif // #if DCPCPY == 1

#if DCPCPY == 1
int ImportKdm( std::vector<KdmInfo> &kdmList, const std::string &dest ,std::string& error , std::string dest2 )
{
	int ret(0); 
	vector<KdmInfo>::size_type nKdmInfoCount = kdmList.size();
	if (nKdmInfoCount==0)
	{
#ifdef _TEST_
		printf("error:KdmInfoCount is NULL !\n");
#endif
		error = "error:KdmInfoCount is NULL !\n";
		//return -1;
		WELOG(error)
		return CMTT2(ERROR_CONTENT_KDM_KDMINFOCOUNTISNULL_ERROR);
	}

     //check dest file exist
	if( ( ret = access( dest.c_str() , F_OK ) ) == 0 )
	{
#ifdef _TEST_
		printf( "File already exists :%s\n", dest.c_str() );
#endif
	}
	else
	{
#ifdef	WIN32
		//ret = mkdir( dest.c_str() );
		ret = system( (MKDIR + dest).c_str() );
#else
		//ret = mkdir( dest.c_str() ,0644 );
		ret = system( (MKDIR + dest).c_str() );
#endif

	}

	//@author zhangmiao@oristartech.com
	//@date [2014-12-03]
	//@brief 主要增加导入KDM支持双目标目录的处理代码
	//@new
	string Dest2;
	if( dest.length() != 0 && dest2.length() == 0 ) 
	{
#ifdef _READDB_KDM_DEST_PATH2
		//直接读取数据库表
		int iResult(0);
		//C_Para::GetInstance()->ReadPara();
		C_RunPara *pRunPara = C_RunPara::GetInstance();
		iResult = pRunPara->GetKdmPath2( Dest2 );
		if( iResult != 0 )
		{
			error = "Error:KDM DestPath2 is NULL!\n";
			WELOG(error);
			errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
			return CMTT2(errno);
		}
#endif // _READDB_KDM_DEST_PATH2

	}
	else
	{
		Dest2 = dest2;
	}

	if ( Dest2.length() != 0 )
	{
		if( Dest2.rfind( '/' ) != Dest2.length()-1 )	//检测Dest2目录的末尾是否带分隔符 "/" ，如果不带则给它补上。
			Dest2.append( 1, '/' );

		//建立目录，如果目标目录存在则不删除。
		//ret = Ftp_MakeDirectory2( Dest2 ,"" );
	}
	//@modify [2014-12-03] end;
	
	//copy kdm
	for ( vector<KdmInfo>::size_type i=0; i< nKdmInfoCount; i++ )
	{
		int nKdm_Import_Status = 0;
		KdmInfo& kdmInfo = kdmList[i];
		string path = kdmInfo.path;
		string srcFileName = path + kdmInfo.filename;
		string destFileName = dest + kdmInfo.filename;
		ret = cp_file( srcFileName.c_str(), destFileName.c_str() );
		if ( ret == 0 )
		{
			nKdm_Import_Status = 0;

			//@author zhangmiao@oristartech.com
			//@date [2014-12-03]
			//@brief 主要增加导入KDM支持双目标目录的处理代码
			//@new
			if ( Dest2.length() != 0 )
			{
				cftp::FtpClient ftp;

				string ftpUrl = Dest2;

				string url = ftpUrl;

				string ip, userName, passwd, dir;
				string port = "21";

				//log in
				ret = ftp.LogIn( ftpUrl );
				if( ret == cftp::NOANYERROR )
				{
					ret = ftp.ParseUrl( url , ip , userName , passwd , port , dir );
					if( ret == cftp::NOANYERROR )
					{
						ret = ftp.UploadFile( srcFileName , dir , kdmInfo.filename );
					}
				}
				
				string destFileName2 = Dest2 + kdmInfo.filename;
				//ret = cp_file( srcFileName.c_str(), destFileName2.c_str() );

				if ( ret == 0 )
				{
#ifdef _TEST_
					printf( "KDM File2 Upload File finished :%s\n", destFileName2.c_str() );
#endif

					nKdm_Import_Status = 0;
				}
				else
				{
					char buffer[BUF_SIZE]="";
					sprintf( buffer,"KDM File2 Upload File Failed :%s\n", destFileName2.c_str() );
					error += buffer;
#ifdef _TEST_
					printf( "KDM File2 Upload File Failed :%s\n", destFileName2.c_str() );
#endif

					WELOG(error);

					nKdm_Import_Status = -2;   //-2---从机上传失败 
				}
			}
			//@modify [2014-12-03] end;
		}
		else
		{
			nKdm_Import_Status = -1;  //-1---主机导入失败
		}
		
		kdmInfo.status = nKdm_Import_Status;		//0--success,非0--failed：-1---主机导入失败;-2---从机上传失败 
	}

	return 0;
}
#endif // #if DCPCPY == 1

#if DCPCPY == 1
int DownLoadKdm( vector<KdmInfo> &kdmList, const string &dest , string& error )
{
	int ret(0);
	int nKdmInfoCount = kdmList.size();
	if (nKdmInfoCount==0)
	{
#ifdef _TEST_
		printf("error:KdmInfoCount is NULL !\n");
#endif
		error = "error:KdmInfoCount is NULL !\n";
		//return -1;
		WELOG(error)
		return CMTT2(ERROR_CONTENT_KDM_KDMINFOCOUNTISNULL_ERROR);
	}

	//check dest file exist
	if( ( ret = access( dest.c_str() , F_OK ) ) == 0 )
	{
#ifdef _TEST_
		printf( "File already exists :%s\n", dest.c_str() );
#endif
	}
	else
	{
#ifdef	WIN32
		//ret = mkdir( dest.c_str() );
		ret = system( (MKDIR + dest).c_str() );
#else
		//ret = mkdir( dest.c_str() ,0644 );
		ret = system( (MKDIR + dest).c_str() );
#endif

	}

	for ( int i=0; i< nKdmInfoCount; i++ )
	{
		KdmInfo kdmInfo = kdmList[i];
		string ftpUrl = kdmInfo.path;
		string srcFileName = kdmInfo.filename;
		//string sFileUrl = ftpUrl + kdmInfo.filename;
		//string destFileName = dest + kdmInfo.filename;
		cftp::FtpClient ftp;
		//log in
		int result = ftp.LogIn(ftpUrl);
		if(result != cftp::NOANYERROR)
			return CMTT2(result);

		string url = ftpUrl;
		
		string ip, userName, passwd, dir;
		string port = "21";

		result = ftp.ParseUrl(url, ip, userName, passwd, port, dir);
		if(result != cftp::NOANYERROR)
			return CMTT2(result);
		
		if ( srcFileName.rfind( '/' )!= string::npos )
		{
			srcFileName = srcFileName.substr( srcFileName.rfind( '/' )+1 );
		}

		ret = ftp.TransportFile( dir  , dest , srcFileName );
		if(ret != cftp::NOANYERROR)
			return CMTT2(ret);

		//@author zhangmiao@oristartech.com
		//@date [2014-07-18]
		//@brief 主要FTP下载时协议命令(quit)相应返回信息不稳定的问题
		//@old 以前代码
		/*ret = ftp.LogOut();
		if(ret != cftp::NOANYERROR)
			return CMTT2(ret);*/
		//@new
		ftp.LogOut();
		//@modify end;
	}

	return 0;
}
#endif // #if DCPCPY == 1
#if DCPCPY == 1
int DeleteDcp( const std::string &path, const std::string &pklUuid , std::string& error )
{
	int ret(0);
	int nIsLocal_Delete_Failed = 0;

	string sFilmFilePath = path;
	if(sFilmFilePath.rfind(DIR_SEPCHAR)!=sFilmFilePath.length()-1) 
		sFilmFilePath.append( 1, DIR_SEPCHAR );

	string::size_type pos;
	if ( ( pos = pklUuid.find("urn:uuid:") ) != pklUuid.npos && pos==0 )
	{
		sFilmFilePath = sFilmFilePath + pklUuid.substr(9);
	}
	else
	{
		sFilmFilePath = sFilmFilePath + pklUuid;
	}
	
	if ( sFilmFilePath.length()==0 )
	{
		error = "Param Path is Null!";
		WELOG(error)
		errno = ERROR_CONTENT_DCP_PARAM_PATH_IS_NULL;
		return CMTT2(errno);
	}

	if( ( ret = access( sFilmFilePath.c_str() , F_OK ) ) == 0 )     //check file exist
	{
#ifdef _TEST_
		printf( "File Directory already exists :%s\n", sFilmFilePath.c_str() );
#endif

		ret = system( (RMDIR + sFilmFilePath).c_str() );
		if( ret == 0 )
		{
#ifdef _TEST_
			printf( "\nDirectory deleted :%s\n", sFilmFilePath.c_str() );
#endif
			errno = 0;
			//return 0;
		}
		else
		{
#ifdef _TEST_
			perror("\nUnable to delete directory\n");
#endif
			error = "\nUnable to delete directory\n";

			WELOG(error);

			nIsLocal_Delete_Failed = 1;
			//return CMTT2(ERROR_CONTENT_DCP_UNABLE_TO_DELETE_DIR);
		}
	}
	else
	{
		//Do not found pkl
		error += "Do not found pkl";
		WELOG(error)
		errno = ERROR_CONTENT_DCP_DONOT_FOUND_PKL;
		//return CMTT2(ERROR_CONTENT_DCP_DONOT_FOUND_PKL);
        
		nIsLocal_Delete_Failed = 1;
	}

	if ( nIsLocal_Delete_Failed == 1 )   //继续删除ftp
	{
		string sDest_Path_2;    //保存Ftp目标目录

#ifdef _READDB_DEST_PATH2
		//直接读取数据库表
		int iResult(0);
		//C_Para::GetInstance()->ReadPara();
		C_RunPara *pRunPara = C_RunPara::GetInstance();
		iResult = pRunPara->GetSavePath2( sDest_Path_2 );
		if( iResult != 0 )
		{
			error += "Error:Ftp DestPath2 is NULL!\n";
			WELOG(error);
			errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
			return CMTT2(errno);
		}
#endif // _READDB_DEST_PATH2

		string Dest2;
		if ( sDest_Path_2.length() != 0 )
		{
			string sFTP_URL, sDeletedDir;
			string sFilmFilePath2 = sDest_Path_2;
			if( sFilmFilePath2.rfind( '/' ) != sFilmFilePath2.length()-1 )	//检测dest2目录的末尾是否带分隔符 "/" ，如果不带则给它补上。
				sFilmFilePath2.append( 1 ,  '/' );

			sFTP_URL = sFilmFilePath2;

			string::size_type pos;
			if ( ( pos = pklUuid.find("urn:uuid:") ) != string::npos && pos==0 )
			{
				sFilmFilePath2 = sFilmFilePath2 + pklUuid.substr(9);
				sDeletedDir = pklUuid.substr(9);
			}
			else
			{
				sFilmFilePath2 = sFilmFilePath2 + pklUuid;
				sDeletedDir = pklUuid;
			}

			sFilmFilePath2.append( 1 ,  '/' );
			sDeletedDir.append( 1 ,  '/' );

			if ( sFilmFilePath2.length()==0 )
			{
				error += "Error:Ftp FilmFilePath2 is NULL!\n";
				WELOG(error);
				errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
				return CMTT2(errno);
			}

			Dest2 = sFilmFilePath2;	//最终的Dest2目录为"sDest_Path_2目录" + '/' + "pklUuid" + "/"

			ret = FtpRemoveDir( Dest2 );
			if( ret == 0 )
			{
#ifdef _TEST_
				printf( "\nFTP Directory deleted :%s\n", Dest2.c_str() );
#endif

				//Delete DCP local failed , but SLAVE Dest Path2 OK!
				errno = ERROR_CONTENT_DCP_DELDCP_ERROR_LOCAL_FAIL_BUT_SLAVE_DIR_OK;
				return CMTT2(errno);
			}
			else
			{
#ifdef _TEST_
				printf("\nFTP Unable to delete directory:%s\n" , Dest2.c_str() );
#endif
				error = "\nFTP Unable to delete directory\n";

				WELOG(error);
				return CMTT2(ERROR_CONTENT_DCP_UNABLE_TO_DELETE_DIR);
			}
		}
	}

	//@author zhangmiao@oristartech.com
	//@date [2014-12-16]
	//@brief 主要增加删除Dcp支持Ftp目标目录的处理代码
	//@new
	string sDest_Path_2;    //保存Ftp目标目录

#ifdef _READDB_DEST_PATH2
	//直接读取数据库表
	int iResult(0);
	//C_Para::GetInstance()->ReadPara();
	C_RunPara *pRunPara = C_RunPara::GetInstance();
	iResult = pRunPara->GetSavePath2( sDest_Path_2 );
	if( iResult != 0 )
	{
		error = "Error:Ftp DestPath2 is NULL!\n";
		WELOG(error);
		//errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
		errno = ERROR_CONTENT_DCP_UNABLE_TO_DELETE_SLAVE_DIR;
		return CMTT2(errno);
	}
#endif // _READDB_DEST_PATH2

	string Dest2;
	if ( sDest_Path_2.length() != 0 )
	{
		string sFTP_URL, sDeletedDir;
		string sFilmFilePath2 = sDest_Path_2;
		if( sFilmFilePath2.rfind( '/' ) != sFilmFilePath2.length()-1 )	//检测dest2目录的末尾是否带分隔符 "/" ，如果不带则给它补上。
			sFilmFilePath2.append( 1 ,  '/' );

		sFTP_URL = sFilmFilePath2;

		string::size_type pos;
		if ( ( pos = pklUuid.find("urn:uuid:") ) != string::npos && pos==0 )
		{
			sFilmFilePath2 = sFilmFilePath2 + pklUuid.substr(9);
			sDeletedDir = pklUuid.substr(9);
		}
		else
		{
			sFilmFilePath2 = sFilmFilePath2 + pklUuid;
			sDeletedDir = pklUuid;
		}

		sFilmFilePath2.append( 1 ,  '/' );
		sDeletedDir.append( 1 ,  '/' );

		if ( sFilmFilePath2.length()==0 )
		{
			error = "Error:Ftp FilmFilePath2 is NULL!\n";
			WELOG(error);
			//errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
			errno = ERROR_CONTENT_DCP_UNABLE_TO_DELETE_SLAVE_DIR;
			return CMTT2(errno);
		}

		Dest2 = sFilmFilePath2;	//最终的Dest2目录为"sDest_Path_2目录" + '/' + "pklUuid" + "/"
		
		ret = FtpRemoveDir( Dest2 );
		if( ret == 0 )
		{
#ifdef _TEST_
			printf( "\nFTP Directory deleted :%s\n", Dest2.c_str() );
#endif
			errno = 0;
		}
		else
		{
#ifdef _TEST_
			printf("\nFTP Unable to delete directory:%s\n" , Dest2.c_str() );
#endif
			error = "\nFTP Unable to delete directory\n";

			WELOG(error);
			return CMTT2(ERROR_CONTENT_DCP_UNABLE_TO_DELETE_SLAVE_DIR);
		}
	}
	//@modify [2014-12-16] end;

	return ret;
}
#endif //#if DCPCPY == 1

int HashDcp( const std::string &path, const std::string &pklUuid ,std::string& error )
{
	bool errorOccurred = false;
	bool bIsFoundPKL   = false;
	int ret(0);
	DcpInfo dcp;
	vector< AMInfo > aAMInfo_Vt;
	ret = dcp.CreateDCPInfo( path , error );
	if( ret!=0 )
	{
		error = "error:CreateDCPInfo()!\n" + error;
		printf("error:CreateDCPInfo()!\n");
		WELOG(error)
		ret = CMTT(ERROR_CONTENT_DCP_CREATE_DCPINFO_ERROR);
	}

	if ( ret==0 )
	{
		aAMInfo_Vt = dcp.vAMInfo_t;
		int nAMInfoCount = aAMInfo_Vt.size();
		for ( int i=0; i< nAMInfoCount; i++ )
		{
			AMInfo_t aAMInfo = aAMInfo_Vt[i];
			//-------------------
			vector< dcp_Info_t > vDcp = aAMInfo.vDcp;
			int pklN = vDcp.size();
			for( int j=0; j<pklN; j++ )
			{
				dcp_Info_t dcp = vDcp[j];
				pkl_Info_t aPkl = dcp.aPkl;	
				if ( pklUuid == aPkl.uuid )
				{
#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif
					g_sHashDcp_PklUuidName = pklUuid;
					g_AllFile_Size = aPkl.allFileLength;
					g_hasHashChecked_Size = 0ull;

#ifdef WIN32
					ReleaseMutex(hMutex);
#else
					pthread_mutex_unlock(&work_mutex);
#endif

					//变量bStopSha1CheckFlag做为返回值，用于返回状态值：hash校验是否被取消;
					//如果hash校验被取消，则返回值等于=1；否则返回值等于=0；
					int bStopSha1CheckFlag = 0;

					bIsFoundPKL = true;
					vector<asset_pkl_t> vAsset_pkl = aPkl.asset;
					int nSize_Asset_Pkl = vAsset_pkl.size();
					for ( int k=0; k< nSize_Asset_Pkl; k++ )
					{
						asset_pkl_t aAsset_Pkl = vAsset_pkl[k];
						string hashValue = aAsset_Pkl.hash;
						string fileName = aAsset_Pkl.originalFileName;
     
						//先检查文件是否存在，check file exist
						if( ( ret = access( fileName.c_str() , F_OK ) ) == 0 )
						{
#ifdef _TEST_
							printf( "File  exists :%s\n", fileName.c_str() );
#endif
						}
						else  //file NOT exist
						{
							char buffer[BUF_SIZE]="";
							sprintf( buffer,"%s:%s\n", fileName.c_str(), strerror(errno) );
							error = buffer;
#ifdef _TEST_
							printf( "File NOT exists :%s\n", fileName.c_str() );

							printf( "%s:%s\n", fileName.c_str(),strerror(errno) );
#endif
							errno = ERROR_CONTENT_DCP_FILE_NOT_EXIST;
							WELOG(error)
							return errno;
						}


						//making hash check begin
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						g_sHashDcp_Checking_FileName = fileName;

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif

						HashInfo sha1;
						char bin_buffer[SHA1_DIGEST_SIZE] = {0};
						//string hashStr = sha1.getSha1FromFile(fileName.c_str(),bin_buffer,NULL,NULL,0);
						string hashStr = sha1.getSha1FromFile(fileName.c_str(),bin_buffer,NULL, &bStopSha1CheckFlag ,0);
						if(hashStr.empty())
						{
							if ( bStopSha1CheckFlag == 1 )
							{
								//CODE_SHA1CHECKCANCEL
								error = "ERROR:getSha1FromFile is Canceled!\n SHA1 Check Is Canceled!\n";
								cout << "getSha1FromFile is Canceled! SHA1 Check Is Canceled!" << endl;
							}
							else
							{
								error = "ERROR:getSha1FromFile is failed!";
								cout << "getSha1FromFile is failed!" << endl;
							}
							
							errorOccurred = true;
							break;
						}
						std::string b64Str = sha1.toBase64(bin_buffer);
						if(b64Str.empty())
						{
							error = "ERROR:toBase64 is failed!";
							cout << "toBase64 is failed!" << endl;
							errorOccurred = true;
							break;
						}
						if(0!=hashValue.compare(b64Str))
						{
							char buffer[BUF_SIZE]="";
							sprintf( buffer,"hash check is failed! value:%s %s\n",hashValue.c_str(),b64Str.c_str() );
							error = buffer;

							printf("hash check is failed! value:%s %s\n",hashValue.c_str(),b64Str.c_str());
							errorOccurred = true;
							break;
						}
						//making hash check end
					}
#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif
					if( errorOccurred )
					{
						//hash校验被取消
						if ( bStopSha1CheckFlag == 1 )
						{
							errno = CODE_SHA1CHECKCANCEL;
							g_sHashDcp_PklUuidName = "";
							g_AllFile_Size = 0ull;
#ifdef WIN32
							ReleaseMutex(hMutex);
#else
							pthread_mutex_unlock(&work_mutex);
#endif
							WELOG(error)
							return CMTT2(ERROR_CONTENT_HASH_SHA1_CHECK_CANCEL);
						}
						
						errno = CODE_SHA1CHECK_ERROR;
						g_sHashDcp_PklUuidName="";
						g_AllFile_Size = 0ull;
#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif
						error = "SHA1 CHECK ERROR!" + error;
						//return 1;
						WELOG(error)
						return CMTT2(ERROR_CONTENT_HASH_SHA1_CHECK_ERROR);
					}
					if ( bIsFoundPKL && (errorOccurred==false) )
					{
						errno = 0;
						g_sHashDcp_PklUuidName="";
						g_sHashDcp_Checking_FileName="";
						//g_AllFile_Size = 0ull;
#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif
						return 0;
					}
				}
				
			}
			
		}
		//Do not found pkl
		if (bIsFoundPKL==false)
		{
#ifdef WIN32
			WaitForSingleObject(hMutex,INFINITE);
#else
			pthread_mutex_lock(&work_mutex);
#endif
			g_sHashDcp_PklUuidName="";
			g_sHashDcp_Checking_FileName="";
			g_hasHashChecked_Size = g_AllFile_Size = 0ull;
#ifdef WIN32
			ReleaseMutex(hMutex);
#else
			pthread_mutex_unlock(&work_mutex);
#endif
			
			error = "Do not found PKL!";
			//return -1;
			WELOG(error)
			errno = ERROR_CONTENT_DCP_DONOT_FOUND_PKL;
			return CMTT2(ERROR_CONTENT_DCP_DONOT_FOUND_PKL);
		}
	}

#ifdef WIN32
	WaitForSingleObject(hMutex,INFINITE);
#else
	pthread_mutex_lock(&work_mutex);
#endif

	g_sHashDcp_PklUuidName="";
	g_AllFile_Size = 0ull;

#ifdef WIN32
	ReleaseMutex(hMutex);
#else
	pthread_mutex_unlock(&work_mutex);
#endif

	return ret;
}

int GetHashPercent( const std::string &path, const std::string &pklUuid, Hashinfo &hash ,std::string& error )
{
#ifdef WIN32
	WaitForSingleObject(hMutex,INFINITE);
#else
	pthread_mutex_lock(&work_mutex);
#endif

	int nPercent = 0;
	if ( g_sHashDcp_PklUuidName == pklUuid )
	{
		long double  fRate = ((long double)(g_hasHashChecked_Size)) / ((long double)(g_AllFile_Size));
		nPercent = int(fRate*100.0);
		if (nPercent==100)
		{
			hash.status = STATUS_FINISHED;
		}
		else
		{
			hash.status = STATUS_CHECKING;
		}
		
		if ( errno == CODE_SHA1CHECK_ERROR )
		{
			error = "error:DCP Hash Check is STATUS_ERROR.";
			hash.filename = g_sHashDcp_Checking_FileName;
			hash.status = STATUS_ERROR;
			WELOG(error)
		}

		hash.percent = nPercent;
		
		hash.path = path;
		
	}
	else
	{
		hash.status = STATUS_NOSTART;
		hash.percent = 0;
		hash.path = path;

#ifdef WIN32
		ReleaseMutex(hMutex);
#else
		pthread_mutex_unlock(&work_mutex);
#endif

		error = "error:DCP Hash Check is NO START.";
		WELOG(error)
		return CMTT2(ERROR_CONTENT_HASH_HASHCHECK_NO_START);
	}

#ifdef WIN32
	ReleaseMutex(hMutex);
#else
	pthread_mutex_unlock(&work_mutex);
#endif

	return 0;
}

int CancelHash( const std::string &path, const std::string &pklUuid ,std::string& error )
{
#ifdef WIN32
	WaitForSingleObject(hMutex,INFINITE);
#else
	pthread_mutex_lock(&work_mutex);
#endif

	if ( g_sHashDcp_PklUuidName == pklUuid )
	{
		if ( g_flag_SHA1CheckStop == 0 )
		{
			g_flag_SHA1CheckStop = 1;
		}
		else if ( g_flag_SHA1CheckStop == 1 && errno == CODE_SHA1CHECKCANCEL )
		{
			g_flag_SHA1CheckStop = 0;
			g_sHashDcp_PklUuidName = "";
		}
	}
	else
	{
		g_sHashDcp_PklUuidName = "";

#ifdef WIN32
		ReleaseMutex(hMutex);
#else
		pthread_mutex_unlock(&work_mutex);
#endif

		error = "PklUuid is error.";
		WELOG(error)
		return CMTT2(ERROR_CONTENT_DCP_PKL_UUID_ERROR);
	}

#ifdef WIN32
	ReleaseMutex(hMutex);
#else
	pthread_mutex_unlock(&work_mutex);
#endif

	return 0;
}

#if DCPCPY == 1
//获取当前的限速值
int GetCurrentLimitedSpeed( int& nLimitedSpeed )
{
	int iResult(0);
	int iNum = 0;

	C_RunPara *pRunPara = C_RunPara::GetInstance();
	iResult = pRunPara->GetAllPlayingDevice( iNum );
	if( iResult != 0 )
	{
		string error = "Error:GetAllPlayingDevice()!\n";
		printf( "%s\n" , error.c_str() );
		WELOG(error);
		return iResult;
	}

#ifdef _TEST_
	//printf( "AllPlayingDevice=%d\n" , iNum );
#endif

	//本机sms播放的个数与导入速度对应关系

	if ( iNum <= 1 )       //小于等于1 不限制
	{
		nLimitedSpeed = -1;
	} 
	else if ( iNum == 2 )  //等于2     20M/S
	{
		nLimitedSpeed = 20;
	}
	else if ( iNum == 3 )  //等于3     15M/S
	{
		nLimitedSpeed = 15;
	}
	else if ( iNum == 4 )  //等于4     10M/S
	{
		nLimitedSpeed = 10;
	}
	else if ( iNum == 5 )  //等于5     5M/S
	{
		nLimitedSpeed = 5;
	}
	else                  //大于等于6  0MB/S
	{
		nLimitedSpeed = 0;
	}

	return 0;
}
#endif //#if DCPCPY == 1

#if DCPCPY == 1
int cp_file_BP( const char *src, const char *des , unsigned long long& rlTransferredSize , unsigned long long from , const char *dest2 
			    , int nLimitedSpeed )    //nLimitedSpeed限速值，单位为MB/S
{
	//src can not be same as the des;
	int in,out,n=0,nW=0;
	int out2 = -1 , nW2 = 0;
	unsigned long long nTransferredSize = 0ull;
	char buf[BSIZE]="";

	if( src == NULL || des == NULL ) 
	{
		printf("cp_file_BP:para is NULL!\n");
		return -1;
	}

	if( strcmp(src,des) == 0 )
	{
		printf("src is the same as des!\n");
		return -1;
	}
	
	if( dest2 != NULL && strlen( dest2 ) == 0 )
	{
#ifdef _TEST_
		printf("dest2 Name is null string!\n");
#endif
		return -1;
	}

	if( dest2 != NULL && strcmp( src , dest2 ) ==0 )
	{
#ifdef _TEST_
		printf("src is the same as dest2!\n");
#endif
		return -1;
	}

	if (from<0)
	{
#ifdef _TEST_
		printf( "Error:File start position is %llu\n",from );
#endif
		return -1;
	}

	if((in=open(src,O_RDONLY|O_BINARY))==-1)
	{
#ifdef _TEST_
		printf("cp_f:fail to open %s:%s\n",src,strerror(errno));
#endif
		return -1;
	}

	if( ( out = open( des , O_WRONLY|O_CREAT|O_BINARY,0644 ) ) == -1 )
	{
#ifdef _TEST_
		printf("cp_f:fail to open %s:%s\n",des,strerror(errno));
#endif
		close(in);
		return -1;
	}

#ifdef _TEST_
	printf("in=%d out=%d\n",in,out);
#endif

	
	if( dest2 != NULL )
	{
		if( ( out2 = open( dest2 , O_WRONLY|O_CREAT|O_TRUNC|O_BINARY , 0644 ) ) == -1 )
		{

#ifdef _TEST_
			printf("cp_f:fail to open %s:%s\n",dest2,strerror(errno));
#endif
			close(in);
			close(out);
			return -1;
		}

#ifdef _TEST_
		printf( "in=%d out2=%d\n" , in , out2 );
#endif
	}


	//------------------------
	unsigned long long nlPos = lseek(in, (long)from, SEEK_SET);
	if (nlPos<0)
	{
		close(out);
		close(in);

		if( out2 != -1 ) 
			close(out2);

#ifdef _TEST_
		printf( "Error:File start position is %llu\n",nlPos );
#endif
		return -1;
	}
	nlPos = lseek(out, (long)from, SEEK_SET);
	if (nlPos<0)
	{
		close(out);
		close(in);

		if( out2 != -1 ) 
			close(out2);

#ifdef _TEST_
		printf( "Error:File start position is %llu\n",nlPos );
#endif
		return -1;
	}

	if( dest2 != NULL && out2 != -1 )
	{
		nlPos = lseek( out2 , (long)from , SEEK_SET );
		if ( nlPos <0 )
		{
			close(out);
			close(out2);
			close(in);
#ifdef _TEST_
			printf( "Error:Output File2 start position is %llu\n",nlPos );
#endif
			return -1;
		}
	}
	

	nTransferredSize += from;
	//--------------------

	//@author zhangmiao@oristartech.com
	//@date [2014-09-15]
	//主要增加支持磁盘写限速的处理代码
	//@new
#ifdef LIMITED_SPEED
	//bool bIsReset = false;
	long long iWriteCount= 0ll;
	struct timeval begTimeVal , curTimeVal;
	begTimeVal.tv_sec = 0;
	begTimeVal.tv_usec = 0;
	curTimeVal.tv_sec = 0;
	curTimeVal.tv_usec = 0;
	gettimeofday( &begTimeVal , NULL );
	int  tUse = 0 , t = 0;

	//如果未设置，则尝试先取出新的限速值，如果取限速值出错，则按照不限速处理。
	if ( nLimitedSpeed == -1 )
	{
		//尝试先取出新的限速值，如果取限速值出错，则按照不限速处理。
		int iResult = 0 , nNew_LimitedSpeed = -1;
		iResult = GetCurrentLimitedSpeed( nNew_LimitedSpeed );
		if ( iResult != 0 ) 
		{
			nNew_LimitedSpeed = -1;
		}

		nLimitedSpeed = nNew_LimitedSpeed;
	}
#endif // LIMITED_SPEED
	//@modify [2014-09-15] end;

	while( ( n = read( in,buf,BSIZE ) ) >0 )
	{
		nW = 0;
		if( ( nW = write(out,buf,n) ) != n )
		{
			close(out);
			close(in);

			if( out2 != -1 )
				close(out2);

#ifdef _TEST_
			printf( "file handle %d and %d is closed\n",in,out );
			printf( "file handle out2 %d is closed\n" , out2 );
#endif
			return -1;
		}


		//@author zhangmiao@oristartech.com
		//@date [2014-09-03]
		//@brief 主要增加导入影片支持双目标目录的处理代码
		//@new
		if( dest2 != NULL && out2 != -1 )
		{
			nW2 = 0;
			if( ( nW2 = write( out2 , buf , n ) ) != n )
			{
				close(out);
				close(out2);
				close(in);
#ifdef _TEST_
				printf( "file in handle %d and out1:%d is closed\n" , in , out );
				printf( "file handle out2 %d is closed\n" , out2 );
#endif
				return -1;
			}

			if ( nW != nW2 )
			{
				close(out);
				close(out2);
				close(in);
#ifdef _TEST_
				printf("Output file out1 and out2:not sync!\n"  );
#endif
				return -1;
			}
		}
		//@modify [2014-09-03] end;

		memset( buf, 0, BSIZE );

#ifdef WIN32
		WaitForSingleObject(hMutex,INFINITE);
#else
		pthread_mutex_lock(&work_mutex);
#endif
		g_hasDcpCopied_Size += nW;
		nTransferredSize += nW;

		//@author zhangmiao@oristartech.com
		//@date [2013-07-15]
		//主要增加了相关字幕文件的处理代码
		//@new
		//FileCopyInfo& copyInfo   = g_map_FileCopyInfo[string(des)]; 
		FileCopyInfo& copyInfo   = g_map_FileCopyInfo[string(src)];
		//@modify end;
		copyInfo.transferredSize = int(nTransferredSize/ONEMB);

		if( g_flag_DcpCopyStop == 1 )
		{
			close(out);
			close(in);

			if( out2 != -1 ) 
				close(out2);

			rlTransferredSize = nTransferredSize;

#ifdef _TEST_
			printf("copy DCP received the stop signal! Code is %d\n",CODE_DCPCOPYCANCEL);
#endif
#ifdef WIN32
			ReleaseMutex(hMutex);
#else
			pthread_mutex_unlock(&work_mutex);
#endif

			errno = CODE_DCPCOPYCANCEL;
			return CODE_DCPCOPYCANCEL;
		}

#ifdef WIN32
		ReleaseMutex(hMutex);
#else
		pthread_mutex_unlock(&work_mutex);
#endif

		//@author zhangmiao@oristartech.com
		//@date [2014-09-15]
		//主要增加支持磁盘写限速的处理代码
		//@new
#ifdef LIMITED_SPEED		
		if ( nLimitedSpeed != -1 )
		{
			iWriteCount += nW;
			if ( iWriteCount >= nLimitedSpeed*ONEMB )	//达到限定速度值
			{
				gettimeofday( &curTimeVal , NULL );
				//时间以毫秒ms为单位
				tUse = (curTimeVal.tv_sec - begTimeVal.tv_sec)*1000 + abs(curTimeVal.tv_usec - begTimeVal.tv_usec)/1000;
				if( tUse < 1000 ) //<1s ，判断是否超出限定速度
				{
					t = 1000 - tUse;

					if ( tUse!=0 )
					{
						double fSpeed = 0;

						fSpeed = ((iWriteCount)*1000.0)/tUse;
						fSpeed = fSpeed /ONEMB;
#ifdef _TEST_
						printf("Limited WriteSpeed:%dMB,UseTime %d millisecond,Speed:%.3lf MB/S\n" , nLimitedSpeed , tUse, fSpeed );
#endif
					}

#ifndef WIN32
					usleep(t*1000);
#else
					usleep(t);
#endif
				}

				//bIsReset = true;
				iWriteCount = 0;
				//curTimeVal.tv_sec = 0;
				//curTimeVal.tv_usec = 0;
				begTimeVal.tv_sec = 0;
				begTimeVal.tv_usec = 0;
				gettimeofday( &begTimeVal , NULL );

				//尝试更新限速值
				//尝试先取出新的限速值，如果取限速值出错，则按照不限速处理。
				int iResult = 0 , nNew_LimitedSpeed = -1;
				iResult = GetCurrentLimitedSpeed( nNew_LimitedSpeed );
				if ( iResult != 0 ) 
				{
					nNew_LimitedSpeed = -1;
				}

				//如果新、旧值不同，则更新nLimitedSpeed.
				if( nNew_LimitedSpeed != nLimitedSpeed )
				{
					nLimitedSpeed = nNew_LimitedSpeed;
				}
			}
		}
		else    //如果当前未限速nLimitedSpeed=-1，则再做进一步处理。
		{
			//尝试先取出新的限速值，如果取限速值出错，则按照不限速处理。
			int iResult = 0 , nNew_LimitedSpeed = -1;
			iResult = GetCurrentLimitedSpeed( nNew_LimitedSpeed );
			if ( iResult != 0 ) 
			{
				nNew_LimitedSpeed = -1;
			}

			//如果新、旧值不同，则更新nLimitedSpeed.
			if( nNew_LimitedSpeed != nLimitedSpeed )
			{
				nLimitedSpeed = nNew_LimitedSpeed;

				//重新初始化
				iWriteCount = 0;
				begTimeVal.tv_sec = 0;
				begTimeVal.tv_usec = 0;
				gettimeofday( &begTimeVal , NULL );
			}
		}
#endif // LIMITED_SPEED
		//@modify [2014-09-15] end;

	}

	rlTransferredSize = nTransferredSize;
	
	close(out);
	close(in);

	if( out2 != -1 ) 
		close(out2);

#ifdef _TEST_
	printf( "file handle %d and %d is closed\n" , in , out );
	printf( "file handle out2 %d is closed\n" , out2 );
#endif
	
	if(n<0) 
		return -1;
	
	return 0;
}
#endif //#if DCPCPY == 1

#if DCPCPY == 1
int cp_file_Mode2( const char *src, const char *des , const char *dest2 , int nLimitedSpeed )  //nLimitedSpeed限速值，单位为MB/S
{
	//src can not be same as the des;
	int in,out,n=0,nW=0;
	int nW2 = 0;
	
	int ret(0);
	cftp::FtpClient Ftp;
	
	unsigned long long nTransferredSize = 0ull;
	char buf[BSIZE]="";

	if( src == NULL || des == NULL ) 
	{
		printf("cp_file_Mode2:para is NULL!\n");
		return -1;
	}

	if( strcmp(src,des) == 0 )
	{
		printf("src is the same as des!\n");
		return -1;
	}

	if( dest2 != NULL && strlen( dest2 ) == 0 )
	{
#ifdef _TEST_
		printf("dest2 Name is null string!\n");
#endif
		return -1;
	}

	if( dest2 != NULL && strcmp( src , dest2 ) ==0 )
	{
#ifdef _TEST_
		printf("src is the same as dest2!\n");
#endif
		return -1;
	}

	if((in=open(src,O_RDONLY|O_BINARY))==-1)
	{
#ifdef _TEST_
		printf("cp_f:fail to open %s:%s\n",src,strerror(errno));
#endif
		return -1;
	}

	if( (out=open(des,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,0644))==-1 )
	{
#ifdef _TEST_
		printf("cp_f:fail to open %s:%s\n",des,strerror(errno));
#endif
		close(in);
		return -1;
	}

#ifdef _TEST_
	printf( "in=%d out=%d\n" , in , out );
#endif

	//if( dest2 != NULL )
	if( ( dest2 != NULL ) && ( g_bIsFtp_UploadFile_Failed == 0 ) )
	{
		string ftpUrl_file = dest2;

		string url = ftpUrl_file;

		string ip, userName, passwd, dir;
		string port = "21";

		ret = Ftp.ParseUrl( url , ip , userName , passwd , port , dir );
		if( ret == cftp::NOANYERROR )
		{
			string tmpdir = "";
			ret = Ftp.LogIn( ip, userName, passwd, tmpdir /*dir*/ , port );
			//ret = Ftp.LogIn( dest2 );
			if( ret == cftp::NOANYERROR )
			{
#ifdef _TEST_
				printf( "cp_file_Mode2:LogIn()-OK-%s!\n" , dest2 );
#endif

				string ftp_fileName = dir.substr( dir.rfind( '/' )+1);
				string ftp_file_dir =  dir.substr( 0, dir.rfind( '/' ) );

				ret = Ftp.UploadFile_Part_Start( ftp_file_dir , ftp_fileName );
				if( ret != cftp::NOANYERROR )
				{
					/*close(out);
					close(in);
					return ret;*/

					errno = CODE_DCP_COPY_SLAVE_FAILED;
					g_bIsFtp_UploadFile_Failed = 1;
				}
				else
				{
					g_bIsFtp_UploadFile_Failed = 0;
				}
			} 
			else
			{

#ifdef _TEST_
				printf("cp_file_Mode2:fail to LogIn %s:%d\n" , dest2 , errno );
#endif

				/*close(in);
				close(out);
				return ret;*/
				g_bIsFtp_UploadFile_Failed = 1;
			}
		}
		else
		{
			/*close(in);
			close(out);
			return ret;*/
			g_bIsFtp_UploadFile_Failed = 1;
		}
	}

	//@author zhangmiao@oristartech.com
	//@date [2014-09-15]
	//主要增加支持磁盘写限速的处理代码
	//@new
#ifdef LIMITED_SPEED
	//bool bIsReset = false;
	long long iWriteCount= 0ll;
	struct timeval begTimeVal , curTimeVal;
	begTimeVal.tv_sec = 0;
	begTimeVal.tv_usec = 0;
	curTimeVal.tv_sec = 0;
	curTimeVal.tv_usec = 0;
	gettimeofday( &begTimeVal , NULL );
	int  tUse = 0 , t = 0;

	//如果未设置，则尝试先取出新的限速值，如果取限速值出错，则按照不限速处理。
	if ( nLimitedSpeed == -1 )
	{
		//尝试先取出新的限速值，如果取限速值出错，则按照不限速处理。
		int iResult = 0 , nNew_LimitedSpeed = -1;
		iResult = GetCurrentLimitedSpeed( nNew_LimitedSpeed );
		if ( iResult != 0 ) 
		{
			nNew_LimitedSpeed = -1;
		}

		nLimitedSpeed = nNew_LimitedSpeed;
	}
#endif // LIMITED_SPEED
	//@modify [2014-09-15]end;

	while( ( n = read( in,buf,BSIZE ) ) >0 )
	{
		nW = 0;
		if( ( nW = write(out,buf,n) ) != n )
		{
			close(out);
			close(in);

#ifdef _TEST_
			printf("file handle %d and %d is closed\n",in,out);
#endif

			if( dest2 != NULL )
			{
				ret = Ftp.UploadFile_Part_End();

				Ftp.LogOut();
			}
			
			return -1;
		}


#ifndef WIN32
		//fsync(out);
#endif

		//@author zhangmiao@oristartech.com
		//@date [2014-12-10]
		//@brief 主要增加导入影片支持Ftp目标目录的处理代码
		//@new
		//如果ftp上传失败，目前继续本地拷贝，不再ftp上传；
		//if (  dest2 != NULL )
		if( ( dest2 != NULL ) && ( g_bIsFtp_UploadFile_Failed == 0 ) )
		{
			nW2 = 0;
			ret = Ftp.UploadFile_Part_SendFileInfo( buf , n , nW2 );
			if( ret != cftp::NOANYERROR )
			{
				//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 begin
				//close(out);
				//close(in);
				//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 end

#ifdef _TEST_
				printf( "file in handle %d and out1:%d is closed\n" , in , out );
#endif

				ret = Ftp.UploadFile_Part_End();

				Ftp.LogOut();


				//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 begin
				//return -1;
				errno = CODE_DCP_COPY_SLAVE_FAILED;
				//bIsFtp_UploadFile_Failed = 1;
				g_bIsFtp_UploadFile_Failed = 1;
				//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 end
			}

			if ( nW != nW2 )
			{
				//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 begin
				//close(out);
				//close(in);
				//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 end

				ret = Ftp.UploadFile_Part_End();

#ifdef _TEST_
				printf("Output file out1 and out2:not sync!\n"  );
#endif

				//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 begin
				//return -1;
				errno = CODE_DCP_COPY_SLAVE_FAILED;
				//bIsFtp_UploadFile_Failed = 1;
				g_bIsFtp_UploadFile_Failed = 1;
				//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 end
			}
		}
		//@modify [2014-12-10] end;

		memset( buf, 0, BSIZE );

#ifdef WIN32
		WaitForSingleObject(hMutex,INFINITE);
#else
		pthread_mutex_lock(&work_mutex);
#endif
		g_hasDcpCopied_Size += nW;
		nTransferredSize += nW;

		//@author zhangmiao@oristartech.com
		//@date [2013-07-15]
		//主要增加了相关字幕文件的处理代码
		//@new
		//FileCopyInfo& copyInfo   = g_map_FileCopyInfo[string(des)];
		FileCopyInfo& copyInfo   = g_map_FileCopyInfo[ string(src) ];
		//@modify end;
		copyInfo.transferredSize = int(nTransferredSize/ONEMB);

		if( g_flag_DcpCopyStop == 1 )
		{
			close(out);
			close(in);

			if( dest2 != NULL )
			{
				if ( Ftp.BeConnected() )
				{
					ret = Ftp.UploadFile_Part_End();

					Ftp.LogOut();
				}
			}

#ifdef _TEST_
			printf("copy DCP received the stop signal! Code is %d\n",CODE_DCPCOPYCANCEL);
#endif

#ifdef WIN32
			ReleaseMutex(hMutex);
#else
			pthread_mutex_unlock(&work_mutex);
#endif

			errno = CODE_DCPCOPYCANCEL;
			return CODE_DCPCOPYCANCEL;
		}

#ifdef WIN32
		ReleaseMutex(hMutex);
#else
		pthread_mutex_unlock(&work_mutex);
#endif

		//@author zhangmiao@oristartech.com
		//@date [2014-09-15]
		//主要增加支持磁盘写限速的处理代码
		//@new
#ifdef LIMITED_SPEED
		if ( nLimitedSpeed != -1 )
		{
			iWriteCount += nW;
			if ( iWriteCount >= nLimitedSpeed*ONEMB )	//达到限定速度值
			{
				gettimeofday( &curTimeVal , NULL );
				//时间以毫秒ms为单位
				tUse = (curTimeVal.tv_sec - begTimeVal.tv_sec)*1000 + abs(curTimeVal.tv_usec - begTimeVal.tv_usec)/1000;
				if( tUse < 1000 ) //<1s ，判断是否超出限定速度
				{
					t = 1000 - tUse;

					if ( tUse!=0 )
					{
						double fSpeed = 0;

						fSpeed = ((iWriteCount)*1000.0)/tUse;
						fSpeed = fSpeed /ONEMB;
#ifdef _TEST_
						printf("Limited WriteSpeed:%dMB,UseTime %d millisecond,Speed:%.3lf MB/S\n" , nLimitedSpeed , tUse, fSpeed );
#endif
					}

#ifndef WIN32
					usleep(t*1000);
#else
					usleep(t);
#endif
				}

				//增加特殊处理：本机sms播放的个数大于等于6，导入速度=0MB/S时 @date [2015-01-06]
				if ( nLimitedSpeed == 0 )
				{
					while( nLimitedSpeed == 0 )
					{
#ifndef WIN32
						usleep(1000*1000);
#else
						usleep(1000);
#endif
						
						//增加判断是否copy停止处理
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif
						if( g_flag_DcpCopyStop == 1 )
						{
							close(out);
							close(in);

							if( dest2 != NULL )
							{
								if ( Ftp.BeConnected() )
								{
									ret = Ftp.UploadFile_Part_End();

									Ftp.LogOut();
								}
							}

#ifdef _TEST_
							printf("copy DCP received the stop signal! Code is %d\n",CODE_DCPCOPYCANCEL);
#endif

#ifdef WIN32
							ReleaseMutex(hMutex);
#else
							pthread_mutex_unlock(&work_mutex);
#endif

							errno = CODE_DCPCOPYCANCEL;
							return CODE_DCPCOPYCANCEL;
						}

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif

						//尝试更新限速值
						//尝试先取出新的限速值，如果取限速值出错，则按照不限速处理。
						int iResult = 0 , nNew_LimitedSpeed = -1;
						iResult = GetCurrentLimitedSpeed( nNew_LimitedSpeed );
						if ( iResult != 0 ) 
						{
							nNew_LimitedSpeed = -1;
						}

						//如果新、旧值不同，则更新nLimitedSpeed.
						if( nNew_LimitedSpeed != nLimitedSpeed )
						{
							nLimitedSpeed = nNew_LimitedSpeed;
							printf( "Old nLimitedSpeed=0 ,New Limited WriteSpeed = %dMB/S\n" , nLimitedSpeed );
						}
					}
				}
				//增加特殊处理：本机sms播放的个数大于等于6，导入速度=0MB/S时 @modify [2015-01-06]end;

				//bIsReset = true;
				iWriteCount = 0;
				//curTimeVal.tv_sec = 0;
				//curTimeVal.tv_usec = 0;
				begTimeVal.tv_sec = 0;
				begTimeVal.tv_usec = 0;
				gettimeofday( &begTimeVal , NULL );

				//尝试更新限速值
				//尝试先取出新的限速值，如果取限速值出错，则按照不限速处理。
				int iResult = 0 , nNew_LimitedSpeed = -1;
				iResult = GetCurrentLimitedSpeed( nNew_LimitedSpeed );
				if ( iResult != 0 ) 
				{
					nNew_LimitedSpeed = -1;
				}

				//如果新、旧值不同，则更新nLimitedSpeed.
				if( nNew_LimitedSpeed != nLimitedSpeed )
				{
					nLimitedSpeed = nNew_LimitedSpeed;
					printf( "New Limited WriteSpeed = %dMB/S\n" , nLimitedSpeed );
				}
			}
		}
		else    //如果当前未限速nLimitedSpeed=-1，则再做进一步处理。
		{
			//尝试先取出新的限速值，如果取限速值出错，则按照不限速处理。
			int iResult = 0 , nNew_LimitedSpeed = -1;
			iResult = GetCurrentLimitedSpeed( nNew_LimitedSpeed );
			if ( iResult != 0 ) 
			{
				nNew_LimitedSpeed = -1;
			}

			//如果新、旧值不同，则更新nLimitedSpeed.
			if( nNew_LimitedSpeed != nLimitedSpeed )
			{
				nLimitedSpeed = nNew_LimitedSpeed;

				//重新初始化
				iWriteCount = 0;
				begTimeVal.tv_sec = 0;
				begTimeVal.tv_usec = 0;
				gettimeofday( &begTimeVal , NULL );
			}
		}
#endif // LIMITED_SPEED
		//@modify [2014-09-15]end;

	}

	close(out);
	close(in);

	/*if( out2 != -1 ) 
		close(out2);*/

	if( dest2 != NULL )
	{
		if ( Ftp.BeConnected() )
		{
			ret = Ftp.UploadFile_Part_End();
		}
	}

#ifdef _TEST_
	printf( "file handle %d and %d is closed\n" , in , out );
	
	if ( dest2 != NULL )
	{
		printf( "file Upload out2 %s is closed\n" , dest2 );
	}
#endif

	if(n<0)
		return -1;

	//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 begin
	if( g_bIsFtp_UploadFile_Failed == 1 )
	{
		errno = CODE_DCP_COPY_SLAVE_FAILED;
		return errno;
	}
	//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 end

	return 0;
}
#endif //#if DCPCPY == 1

#if DCPCPY == 1
int cp_file(const char *src, const char *des)
{
	//src can not be same as the des;
	int in,out,n;
	char buf[BSIZE];

	if(src==NULL || des==NULL) {printf("para is NULL!\n");return -1;}
	if(strcmp(src,des)==0){printf("src is the same as des!\n");return -1;}
	if((in=open(src,O_RDONLY|O_BINARY))==-1){
		printf("cp_f:fail to open %s:%s\n",src,strerror(errno));
		return -1;
	}
	if((out=open(des,O_WRONLY|O_CREAT|O_TRUNC|O_BINARY,0644))==-1){
		printf("cp_f:fail to open %s:%s\n",des,strerror(errno));
		close(in);
		return -1;
	}
	//printf("in=%d out=%d\n",in,out);
	while(0<(n=read(in,buf,BSIZE))){
		if(n!=write(out,buf,n))
		{
			close(out);close(in);//printf("%d and %d is closed\n",in,out);
			return -1;
		}
		/*if (eof (in))
			break;*/
	}
	close(out);close(in);//printf("%d and %d is closed\n",in,out);
	if(n<0) return -1;
	return 0;
}
#endif // #if DCPCPY == 1


#if DCPCPY == 1
//取消DCP导入
int CancelCopy( const string &source, const string &pklUuid, string& error )
{
#ifdef WIN32
	WaitForSingleObject(hMutex,INFINITE);
#else
	pthread_mutex_lock(&work_mutex);
#endif

	if (g_sCopyDcp_PklUuidName==pklUuid)
	{
		if ( g_flag_DcpCopyStop == 0 )
		{
			g_flag_DcpCopyStop = 1;
		}
		else if ( g_flag_DcpCopyStop == 1 && errno==CODE_DCPCOPYCANCEL )
		{
			g_flag_DcpCopyStop = 0;
			g_sCopyDcp_PklUuidName="";
		}
	}
	else
	{
		g_sCopyDcp_PklUuidName="";

#ifdef WIN32
		ReleaseMutex(hMutex);
#else
		pthread_mutex_unlock(&work_mutex);
#endif
		error = "PklUuid is error.";
		//return -1;
		WELOG(error)
		return CMTT2(ERROR_CONTENT_DCP_PKL_UUID_ERROR);
	}

#ifdef WIN32
	ReleaseMutex(hMutex);
#else
	pthread_mutex_unlock(&work_mutex);
#endif

	return 0;
}
#endif // #if DCPCPY == 1

#if DCPCPY == 1
//取得DCP导入进度
int GetCopyPercent( const string &source , const string &pklUuid, CopyInfo &info, string& error )
{
#ifdef WIN32
	WaitForSingleObject(hMutex,INFINITE);
#else
	pthread_mutex_lock(&work_mutex);
#endif

	int nPercent = 0;
	if ( g_sCopyDcp_PklUuidName == pklUuid )
	{
		long double  fRate = ((long double)(g_hasDcpCopied_Size)) / ((long double)(g_Dcp_AllFile_Size));
		nPercent = int(fRate*100.0);
		if (nPercent==100)
		{
			info.status = DCP_STATUS_FINISHED;
		}
		else
		{
			info.status = DCP_STATUS_COPING;
		}

		if ( errno != 0 )
		{
			error = "error:DCP Copy Percent is DCP_STATUS_ERROR.";
			info.status = DCP_STATUS_ERROR;
			WELOG(error)
		}

		info.percent = nPercent;

		info.dest = "";
		info.source = source;

		if (g_map_FileCopyInfo.size())
		{
			info.copyInfo.clear();
			map< std::string , FileCopyInfo >::iterator iFirst = g_map_FileCopyInfo.begin();
			map< std::string , FileCopyInfo >::iterator iLast = g_map_FileCopyInfo.end();
			for (; iFirst != iLast; ++iFirst)
				info.copyInfo.push_back((*iFirst).second);
		}
	}
	else
	{
		info.status = DCP_STATUS_NOSTART;
		info.percent = 0;
		info.source = source;
		info.copyInfo.clear();

#ifdef WIN32
		ReleaseMutex(hMutex);
#else
		pthread_mutex_unlock(&work_mutex);
#endif

		error = "error:DCP Copy is NO START.";
		//return -1;
		WELOG(error)
		return CMTT2(ERROR_CONTENT_DCP_COPY_NO_START);
	}

#ifdef WIN32
	ReleaseMutex(hMutex);
#else
	pthread_mutex_unlock(&work_mutex);
#endif

	return 0;
}
#endif // #if DCPCPY == 1


#if DCPCPY == 1
//断点续传方式，拷贝copyInfo中的每个文件，对于CPL文件直接拷贝
//@author zhangmiao@oristartech.com
//@date [2013-07-15]
//主要增加了相关字幕文件的处理代码
//@new
//int CopyDcp_BPTransSub1( const std::string &Dest, Content::Dcp::pkl_Info_t& aPkl, std::vector<Content::FileCopyInfo> &copyInfo , std::string& error )
int CopyDcp_BPTransSub1( const std::string &source, const std::string &Dest, pkl_Info_t& aPkl, std::vector<FileCopyInfo> &copyInfo , std::string& error , 
						 const std::string Dest2 , int nLimitedSpeed )
//@modify end;
{
	int ret(0);
	std::vector<FileCopyInfo>::size_type nSize_Copy_Info = copyInfo.size();
	std::vector<FileCopyInfo>::size_type s = 0;
	for( s=0; s < nSize_Copy_Info; s++ )
	{
		FileCopyInfo& rFileCopyInfo = copyInfo[s];

		if (rFileCopyInfo.bIsFinished==1)		//如果拷贝完成，则继续下一个文件。
		{
			//----zhangmiao:begin--[7/20/2012]----
			long long Length = 0;
			//@author zhangmiao@oristartech.com
			//@date [2013-07-15]
			//主要增加了相关字幕文件的处理代码
			//@new
			//ret = GetFileSize_PKLAssetList( rFileCopyInfo.filename , aPkl, Length );
			string sFullPathFileName;
			ret = GetFileSize_PKLAssetList( rFileCopyInfo.filename , aPkl, Length , sFullPathFileName );
			if (ret==0)
			{
				g_hasDcpCopied_Size += Length;
			}
			//----zhangmiao:end----[7/20/2012]----
			
			//@author zhangmiao@oristartech.com
			//@date [2012-12-26]
			//更新获取进度时的状态
#ifdef WIN32
			WaitForSingleObject(hMutex,INFINITE);
#else
			pthread_mutex_lock(&work_mutex);
#endif
			//@author zhangmiao@oristartech.com
			//@date [2013-07-15]
			//主要增加了相关字幕文件的处理代码
			//@new
			/*string destFileName = Dest + rFileCopyInfo.filename;
			map< string , FileCopyInfo >::size_type nCont = g_map_FileCopyInfo.count(destFileName);
			if ( nCont == 1 )
			{
				FileCopyInfo& copyInfo   = g_map_FileCopyInfo[destFileName]; 
				copyInfo.bIsFinished = 1;
			}*/

			string srcFileName = sFullPathFileName;
			map< string , FileCopyInfo >::size_type nCont = g_map_FileCopyInfo.count(srcFileName);
			if ( nCont == 1 )
			{
				FileCopyInfo& copyInfo   = g_map_FileCopyInfo[srcFileName]; 
				copyInfo.bIsFinished = 1;
			}
			//@modify end;
#ifdef WIN32
			ReleaseMutex(hMutex);
#else
			pthread_mutex_unlock(&work_mutex);
#endif
			//@modify [2012-12-26] end;
			
			continue;
		}

		string sFileCopyName = rFileCopyInfo.filename;
		if ( sFileCopyName.length()==0 )
		{
			error = "Error:FileCopyName is NULL!\n";
			//errno = -2;
			errno = ERROR_CONTENT_DCP_FILECOPYNAME_IS_NULL;
			WELOG(error)
			return errno;
		}

		//Copy PKL AssetList File
		vector<asset_pkl_t> vAsset_pkl = aPkl.asset;
		vector<asset_pkl_t>::size_type nSize_Asset_Pkl = vAsset_pkl.size();
		vector<asset_pkl_t>::size_type k = 0;
		for ( k=0; k < nSize_Asset_Pkl; k++ )
		{
			asset_pkl_t aAsset_Pkl = vAsset_pkl[k];
			string fileType = aAsset_Pkl.type;
			string fileName = aAsset_Pkl.originalFileName;
			if ( fileName.length()==0 )
			{
				error = "Error:Pkl AssetFile Path Is NULL!\n";
				//errno = -2;
				WELOG(error)
				errno = ERROR_CONTENT_DCP_PKL_ASSEFILE_PATH_IS_NULL;
				return errno;
			}

			if ( sFileCopyName == fileName.substr(fileName.rfind(DIR_SEPCHAR)+1) )    //判断文件是否找到
			{
				if ( fileType == string("text/xml;asdcpKind=CPL") )      //直接拷贝CPL文件
				{
#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif

					g_sCopyDcp_Copying_FileName = fileName;

#ifdef WIN32
					ReleaseMutex(hMutex);
#else
					pthread_mutex_unlock(&work_mutex);
#endif
					if( ( ret = access( fileName.c_str() , F_OK ) ) == 0 )     //check file exist
					{
#ifdef _TEST_
						printf( "File  exists :%s\n", fileName.c_str() );
#endif
						//printf( "Delete file :%s\n", fileName.c_str() );
						//ret = remove( fileName.c_str() );

						//@author zhangmiao@oristartech.com
						//@date [2013-07-15]
						//主要增加了相关字幕文件的处理代码
						//@new
/*
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						FileCopyInfo aCopyInfo;
						aCopyInfo.filename = Dest + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);
						aCopyInfo.fileSize = static_cast<int>(aAsset_Pkl.size/ONEMB);
						aCopyInfo.transferredSize = 0;
						g_map_FileCopyInfo[Dest + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1)]=aCopyInfo;
#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif
*/
						//@modify end;

						string srcFileName = fileName;
						string destFileName = Dest + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);

						//@author zhangmiao@oristartech.com
						//@date [2014-09-04]
						//@brief 主要增加导入影片支持双目标目录的处理代码
						//@new
						//ret = cp_file_Mode2( srcFileName.c_str(), destFileName.c_str() );
						string DestFileName2;
						if ( Dest2.length() != 0 )
						{
							DestFileName2 = Dest2 + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);
						}

						if ( DestFileName2.length() == 0 )
						{
							ret = cp_file_Mode2( srcFileName.c_str(), destFileName.c_str() , NULL , nLimitedSpeed );
						}
						else
						{
							ret = cp_file_Mode2( srcFileName.c_str(), destFileName.c_str() , DestFileName2.c_str() , nLimitedSpeed );
						}
						//@modify [2014-09-04] end;
						
						if (ret==0)
						{
							//@author zhangmiao@oristartech.com
							//@date [2012-12-26]
							//更新获取进度时的状态，copy CPL file
#ifdef WIN32
							WaitForSingleObject(hMutex,INFINITE);
#else
							pthread_mutex_lock(&work_mutex);
#endif
							
							//@author zhangmiao@oristartech.com
							//@date [2013-07-15]
							//主要增加了相关字幕文件的处理代码
							//@new
							//FileCopyInfo& copyInfo   = g_map_FileCopyInfo[destFileName];
							FileCopyInfo& copyInfo   = g_map_FileCopyInfo[srcFileName];
							//@modify end;

							copyInfo.bIsFinished = 1;

#ifdef WIN32
							ReleaseMutex(hMutex);
#else
							pthread_mutex_unlock(&work_mutex);
#endif
							//@modify [2012-12-26] end;

							rFileCopyInfo.bIsFinished = 1;
#ifdef _TEST_
							printf( "File copy finished :%s\n", destFileName.c_str() );
#endif
						}
						else
						{
#ifdef WIN32
							WaitForSingleObject(hMutex,INFINITE);
#else
							pthread_mutex_lock(&work_mutex);
#endif

							g_sCopyDcp_PklUuidName="";
							g_Dcp_AllFile_Size = 0ull;

							if ( ret == CODE_DCPCOPYCANCEL )
							{
								g_flag_DcpCopyStop = 0;

								char buffer[BUF_SIZE]="";
								sprintf( buffer,"%s:%s\n",fileName.c_str(),"DCP Copy CANCEL!" );
								error = buffer;
#ifdef _TEST_
								printf("%s:%s\n",fileName.c_str(),"DCP Copy CANCEL!");
#endif

								errno = ERROR_CONTENT_DCP_BP_DCPCOPYCANCEL;
							}
							else
							{
#ifdef _TEST_
								printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif
								errno = ERROR_CONTENT_DCP_FILE_COPY_FAILED;
							}

#ifdef WIN32
							ReleaseMutex(hMutex);
#else
							pthread_mutex_unlock(&work_mutex);
#endif
							//errno = 1;
							WELOG(error)
							return errno;
						}
					}
					else  //file NOT exist
					{
						char buffer[BUF_SIZE]="";
						sprintf( buffer,"%s:%s\n",fileName.c_str(),strerror(errno) );
						error = buffer;
#ifdef _TEST_
						printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						g_sCopyDcp_PklUuidName = "";
						g_Dcp_AllFile_Size = 0ull;

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif
						errno = ERROR_CONTENT_DCP_CPL_FILE_IS_NO_EXIST;
						WELOG(error)
						return errno;
					}
				}
				//@author zhangmiao@oristartech.com
				//@date [2013-07-15]
				//@new
				//主要增加了相关字幕文件的处理代码
				else if ( fileType == string("text/xml;asdcpKind=Subtitle") || fileType == string("image/png") || fileType == string("application/ttf") )
				{
#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif

					g_sCopyDcp_Copying_FileName = fileName;

#ifdef WIN32
					ReleaseMutex(hMutex);
#else
					pthread_mutex_unlock(&work_mutex);
#endif

					if( ( ret = access( fileName.c_str() , F_OK ) ) == 0 )     //check file exist
					{
#ifdef _TEST_
						printf( "File  exists :%s\n", fileName.c_str() );
#endif

						//如果目标目录包含子目录，则建立相应的子目录。
						string tmpPath,SubPath;
						std::string::size_type pos1 = fileName.rfind( source );
						if ( pos1 != string::npos )
						{
							tmpPath = fileName.substr( pos1 + source.length() );
						}
						std::string::size_type pos2 = tmpPath.rfind(DIR_SEPCHAR);
						if ( pos2 != string::npos )
						{
							SubPath = tmpPath.substr( 0, pos2+1 );
						}

						string DestFile_InPath = Dest + SubPath;
						ret = MakeDirectory2(DestFile_InPath);		//建立目录，如果目标目录存在则不删除。
						if(ret!=0)
						{
							error = "Error:CopyDcp_BPTransSub1--Make Director2 fail.\n";
							WELOG(error);
							ret = ERROR_CONTENT_DCP_MAKE_DIR_FAIL;
							return ret;
						}
	
						string srcFileName  = fileName;
						string destFileName = DestFile_InPath + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);

						//@author zhangmiao@oristartech.com
						//@date [2014-09-04]
						//@brief 主要增加导入影片支持双目标目录的处理代码
						//@new
						//ret = cp_file_Mode2( srcFileName.c_str(), destFileName.c_str() );
						string DestFileName2;
						if ( Dest2.length() != 0 )
						{
							string DestFile2_InPath = Dest2 + SubPath;
							ret = MakeDirectory2( DestFile2_InPath );		//建立目录，如果目标目录存在则不删除。
							if( ret != 0 )
							{
								error = "Error:CopyDcpSub1--Make Directory2 fail.\n";
								WELOG(error);
								ret = ERROR_CONTENT_DCP_MAKE_DIR_FAIL;
								return ret;
							}

							DestFileName2 = DestFile2_InPath + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);
						}

						if ( DestFileName2.length() == 0 )
						{
							ret = cp_file_Mode2( srcFileName.c_str(), destFileName.c_str() , NULL , nLimitedSpeed );
						}
						else
						{
							ret = cp_file_Mode2( srcFileName.c_str(), destFileName.c_str() , DestFileName2.c_str() , nLimitedSpeed );
						}
						//@modify [2014-09-04] end;

						if ( ret == 0 )
						{
							//更新获取进度时的状态，copy file
#ifdef WIN32
							WaitForSingleObject(hMutex,INFINITE);
#else
							pthread_mutex_lock(&work_mutex);
#endif

							FileCopyInfo& copyInfo   = g_map_FileCopyInfo[srcFileName];
							copyInfo.bIsFinished = 1;

#ifdef WIN32
							ReleaseMutex(hMutex);
#else
							pthread_mutex_unlock(&work_mutex);
#endif
							

							rFileCopyInfo.bIsFinished = 1;
#ifdef _TEST_
							printf( "File copy finished :%s\n", destFileName.c_str() );
#endif
						}
						else
						{
#ifdef WIN32
							WaitForSingleObject(hMutex,INFINITE);
#else
							pthread_mutex_lock(&work_mutex);
#endif

							g_sCopyDcp_PklUuidName="";
							g_Dcp_AllFile_Size = 0ull;

							if ( ret == CODE_DCPCOPYCANCEL )
							{
								g_flag_DcpCopyStop = 0;

								char buffer[BUF_SIZE]="";
								sprintf( buffer,"%s:%s\n",fileName.c_str(),"DCP Copy CANCEL!" );
								error = buffer;
#ifdef _TEST_
								printf("%s:%s\n",fileName.c_str(),"DCP Copy CANCEL!");
#endif

								errno = ERROR_CONTENT_DCP_BP_DCPCOPYCANCEL;
							}
							else
							{
#ifdef _TEST_
								printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif
								errno = ERROR_CONTENT_DCP_FILE_COPY_FAILED;
							}

#ifdef WIN32
							ReleaseMutex(hMutex);
#else
							pthread_mutex_unlock(&work_mutex);
#endif
							//errno = 1;
							WELOG(error);
							return errno;
						}
					}
					else  //file NOT exist
					{
						char buffer[BUF_SIZE]="";
						sprintf( buffer,"%s:%s\n",fileName.c_str(),strerror(errno) );
						error = buffer;
#ifdef _TEST_
						printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						g_sCopyDcp_PklUuidName = "";
						g_Dcp_AllFile_Size = 0ull;

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif
						errno = ERROR_CONTENT_DCP_FILE_NOT_EXIST;
						WELOG(error);
						return errno;
					}
				}
				//@modify end;
				else  //copy mxf file
				{
#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif

					g_sCopyDcp_Copying_FileName = fileName;

#ifdef WIN32
					ReleaseMutex(hMutex);
#else
					pthread_mutex_unlock(&work_mutex);
#endif
					if( ( ret = access( fileName.c_str() , F_OK ) ) == 0 )     //check file exist
					{
#ifdef _TEST_
						printf( "File  exists :%s\n", fileName.c_str() );
#endif
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						//@author zhangmiao@oristartech.com
						//@date [2013-07-15]
						//主要增加了相关字幕文件的处理代码
						//@new
						/*FileCopyInfo aCopyInfo;
						aCopyInfo.filename = Dest + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);
						aCopyInfo.fileSize = static_cast<int>(aAsset_Pkl.size/ONEMB);
						aCopyInfo.transferredSize = 0;
						g_map_FileCopyInfo[Dest + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1)]=aCopyInfo;*/
						//@modify end;

						string srcFileName = fileName;
						string destFileName = Dest + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);

						unsigned long long nlTransferredSize = 0;
						unsigned long long nfrom = 0;
						nlTransferredSize = rFileCopyInfo.transferredSize*ONEMB;
						nfrom = nlTransferredSize;
						//@author zhangmiao@oristartech.com
						//@date [2012-07-20]
						g_hasDcpCopied_Size += nlTransferredSize;
						//@modify [2012-07-20] end;

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif

						//@author zhangmiao@oristartech.com
						//@date [2014-09-04]
						//@brief 主要增加导入影片支持双目标目录的处理代码
						//@new
						//ret = cp_file_BP(  srcFileName.c_str(), destFileName.c_str() , nlTransferredSize, nfrom );
						string DestFileName2;
						if ( Dest2.length() != 0 )
						{
							DestFileName2 = Dest2 + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);
						}

						if ( DestFileName2.length() == 0 )
						{
							ret = cp_file_BP( srcFileName.c_str(), destFileName.c_str() , nlTransferredSize , nfrom , NULL , nLimitedSpeed );
						}
						else
						{
							ret = cp_file_BP( srcFileName.c_str(), destFileName.c_str() , nlTransferredSize , nfrom , DestFileName2.c_str() , nLimitedSpeed );
						}
						//@modify [2014-09-04] end;

						rFileCopyInfo.transferredSize = static_cast<int>(nlTransferredSize/ONEMB);
						rFileCopyInfo.fileSize = static_cast<int>(aAsset_Pkl.size/ONEMB);

						if ( ret == 0 )
						{
							//@author zhangmiao@oristartech.com
							//@date [2012-12-26]
							//更新获取进度时的状态，copy mxf file
#ifdef WIN32
							WaitForSingleObject(hMutex,INFINITE);
#else
							pthread_mutex_lock(&work_mutex);
#endif

							//@author zhangmiao@oristartech.com
							//@date [2013-07-15]
							//主要增加了相关字幕文件的处理代码
							//@new
							//FileCopyInfo& copyInfo   = g_map_FileCopyInfo[destFileName]; 
							FileCopyInfo& copyInfo   = g_map_FileCopyInfo[srcFileName];
							//@modify end;
							copyInfo.bIsFinished = 1;

#ifdef WIN32
							ReleaseMutex(hMutex);
#else
							pthread_mutex_unlock(&work_mutex);
#endif
							//@modify [2012-12-26] end;

							rFileCopyInfo.bIsFinished = 1;
#ifdef _TEST_
							printf( "File copy finished :%s\n", destFileName.c_str() );
#endif
						}
						else 
						{
#ifdef WIN32
							WaitForSingleObject(hMutex,INFINITE);
#else
							pthread_mutex_lock(&work_mutex);
#endif

							g_sCopyDcp_PklUuidName = "";
							g_Dcp_AllFile_Size = 0ull;

							if ( ret == CODE_DCPCOPYCANCEL )
							{
								g_flag_DcpCopyStop = 0;
								
								char buffer[BUF_SIZE]="";
								sprintf( buffer,"%s:%s\n",fileName.c_str(),"DCP Copy BPTrans CANCEL!" );
								error = buffer;

#ifdef _TEST_
								printf("%s:%s\n",fileName.c_str(),"DCP Copy BPTrans CANCEL!");
#endif

								errno = ERROR_CONTENT_DCP_BP_DCPCOPYCANCEL;
							}
							else
							{
#ifdef _TEST_
								printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif
								errno = ERROR_CONTENT_DCP_MXF_FILE_COPY_IS_UNFINISHED;
							}


#ifdef WIN32
							ReleaseMutex(hMutex);
#else
							pthread_mutex_unlock(&work_mutex);
#endif
							//errno = 1;
							WELOG(error)
							return errno;
						}
					}
					else  //file NOT exist
					{
						char buffer[BUF_SIZE]="";
						sprintf( buffer,"%s:%s\n",fileName.c_str(),strerror(errno) );
						error = buffer;
#ifdef _TEST_
						printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						g_sCopyDcp_PklUuidName="";
						g_Dcp_AllFile_Size = 0ull;

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif

						errno = ERROR_CONTENT_DCP_MXF_FILE_IS_NO_EXIST;
						WELOG(error)
						return errno;
					}
				}
			}
			else
			{	//没有找到，则继续查找下一个文件
				continue;
			}

		}

	}

	return ret;
}
#endif // #if DCPCPY == 1

#if DCPCPY == 1
//断点续传方式，拷贝DCP包。
int CopyDcp_BPTrans( const string &source, const string &pklUuid, const string &dest, vector<FileCopyInfo> &copyInfo , std::string& error ,
					  const std::string dest2 , int nLimitedSpeed )
{
	bool errorOccurred = false;
	bool bIsFoundPKL   = false;
	int ret(0);
	DcpInfo dcp;
	vector< AMInfo > aAMInfo_Vt;
	ret = dcp.CreateDCPInfo( source , error );	//程序会先到source所指目录下去搜索，看是否搜索到影片，source目录可以设定为外接移动硬盘挂载目录。
	if(ret!=0)
	{
#ifdef _TEST_
		printf("error CreateDCPInfo !\n");
#endif
		error = "error:CreateDCPInfo()!\n" + error;

		WELOG(error)
		ret = ERROR_CONTENT_DCP_CREATE_DCPINFO_ERROR;
		return ret;
	}
	//Parameters Check
	string sFilmFilePath = dest;
	if( sFilmFilePath.rfind(DIR_SEPCHAR) != sFilmFilePath.length()-1 )	//检测dest目录的末尾是否带分隔符 "/" 或"\"，如果不带则给它补上。
		sFilmFilePath.append( 1, DIR_SEPCHAR );

	string::size_type pos;
	if ( ( pos = pklUuid.find("urn:uuid:") ) != string::npos && pos==0 )
	{
		sFilmFilePath = sFilmFilePath + pklUuid.substr(9);
	}
	else
	{
		sFilmFilePath = sFilmFilePath + pklUuid;
	}
	sFilmFilePath.append( 1, DIR_SEPCHAR );

	if ( sFilmFilePath.length()==0 )
	{
		error = "Error:FilmFilePath is NULL!\n";
		//errno = -2;
		WELOG(error)
		errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
		return errno;
	}

	string  Dest = sFilmFilePath;	//最终的Dest目录为"dest目录" + '/' + "pklUuid" + "/"

	ret = MakeDirectory2(Dest);		//建立目录，如果目标目录存在则不删除。
	
	//@author zhangmiao@oristartech.com
	//@date [2014-09-03]
	//@brief 主要增加导入影片支持双目标目录的处理代码
	//@new
	string Dest2;
	if ( dest2.length() != 0 )
	{
		if(ret!=0)
		{
			error = "Error:Make Directory fail.\n";
			WELOG(error);
			ret = ERROR_CONTENT_DCP_MAKE_DIR_FAIL;
			return CMTT2(ret);
		}

		string sFilmFilePath2 = dest2;
		if( sFilmFilePath2.rfind(DIR_SEPCHAR) != sFilmFilePath2.length()-1 )	//检测dest2目录的末尾是否带分隔符 "/" 或"\"，如果不带则给它补上。
			sFilmFilePath2.append( 1, DIR_SEPCHAR );

		string::size_type pos;
		if ( ( pos = pklUuid.find("urn:uuid:") ) != string::npos && pos==0 )
		{
			sFilmFilePath2 = sFilmFilePath2 + pklUuid.substr(9);
		}
		else
		{
			sFilmFilePath2 = sFilmFilePath2 + pklUuid;
		}
		sFilmFilePath2.append( 1, DIR_SEPCHAR );

		if ( sFilmFilePath2.length()==0 )
		{
			error = "Error:FilmFilePath2 is NULL!\n";
			WELOG(error);
			errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
			return CMTT2(errno);
		}

		Dest2 = sFilmFilePath2;	//最终的Dest2目录为"dest2目录" + '/' + "pklUuid" + "/"
		ret = MakeDirectory2( Dest2 );
	}
	//@modify [2014-09-03] end;

	if(ret!=0)
	{
		error = "Error:Make Directory fail.\n";
		WELOG(error)
		ret = ERROR_CONTENT_DCP_MAKE_DIR_FAIL;
		return ret;
	}
	else if ( ret==0 )	//dcp copy files
	{
		aAMInfo_Vt = dcp.vAMInfo_t;
		vector< AMInfo >::size_type nAMInfoCount = aAMInfo_Vt.size();
		vector< AMInfo >::size_type i = 0;
		for ( i=0; i< nAMInfoCount; i++ )
		{
			AMInfo_t aAMInfo = aAMInfo_Vt[i];
			//-------------------
			vector<asset_assetmap_t> aAssetVt = aAMInfo.aAssetmap_Info.asset;
			vector< dcp_Info_t > vDcp = aAMInfo.vDcp;
			vector< dcp_Info_t >::size_type pklN = vDcp.size();
			vector< dcp_Info_t >::size_type j = 0;
			for( j=0; j<pklN; j++ )
			{
				string pklName;
				//查找pklName文件路径名
				vector<asset_assetmap_t>::size_type nAssetVtCount = aAssetVt.size();
				vector<asset_assetmap_t>::size_type k = 0;
				for ( k=0; k < nAssetVtCount; k++ )
				{
					if ( pklUuid == aAssetVt[k].uuid )
					{
						pklName =  aAssetVt[k].path;
						if (pklName.length()==0)	//如果名字为空，做错误处理
						{
							char buffer[BUF_SIZE]="";
							sprintf( buffer,"pklName %s Error!\n", pklName.c_str() );
							error = buffer;
#ifdef _TEST_
							printf("pklName %s Error!\n", pklName.c_str());
#endif
							
							//删除垃圾文件夹，Remove DCP Directory
							int nResult = 0 , nResult2 = 0;
							nResult = RemoveDir( Dest );
							if( nResult != 0 )
							{
#ifdef _TEST_
								printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif
								error += "Warning:Remove DCP Directory fail.\n";
							}
							

							//@author zhangmiao@oristartech.com
							//@date [2014-09-02]
							//@brief 主要增加导入影片支持双目标目录的处理代码
							//@new
							if ( Dest2.length() != 0 )
							{
								nResult2 = RemoveDir( Dest2 );
								if( nResult2 != 0 )
								{
#ifdef _TEST_
									printf( "Warning:Remove DCP Dest2 Directory Fail.:%s\n", Dest2.c_str() );
#endif
									error += "Warning:Remove DCP Dest2 Directory Fail.\n";
								}
							}
							//@modify [2014-09-03] end;

							WELOG(error);
							errno = ERROR_CONTENT_DCP_PKL_PKLNAME_ERROR;
							return errno;
						}

#ifdef _TEST_
						printf( "pklName:%s\n", pklName.c_str() );
#endif
						break;
					}
				}

				dcp_Info_t dcpInf = vDcp[j];
				pkl_Info_t aPkl = dcpInf.aPkl;	
				if ( pklUuid == aPkl.uuid )
				{
#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif

					g_sCopyDcp_PklUuidName = pklUuid;
					g_Dcp_AllFile_Size = aPkl.allFileLength;
					g_hasDcpCopied_Size = 0ull;

					bIsFoundPKL = true;        //pkl找到
					g_map_FileCopyInfo.clear();

					//@author zhangmiao@oristartech.com
					//@date [2012-12-26]
					//初始化g_map_FileCopyInfo 映射结构，返回pkl的所有文件状态。
					vector<asset_pkl_t> vAsset_pkl = aPkl.asset;
					vector<asset_pkl_t>::size_type nSize_Asset_Pkl = vAsset_pkl.size();
					if (nSize_Asset_Pkl==0)
					{
						error = "Error:PKL AssetList is null!\n";
						errno = ERROR_CONTENT_DCP_PKL_ASSELIST_IS_NULL;
						WELOG(error)
						return errno;
					}
					vector<asset_pkl_t>::size_type m = 0;
					for ( m=0; m< nSize_Asset_Pkl; m++ )
					{
						asset_pkl_t aAsset_Pkl = vAsset_pkl[m];
						string fileName = aAsset_Pkl.originalFileName;
						if ( fileName.length() == 0 )
						{
							error = "Error:Pkl AssetFile Path Is NULL!\n";
							errno = ERROR_CONTENT_DCP_PKL_ASSEFILE_PATH_IS_NULL;
							WELOG(error)
							return errno;
						}

						FileCopyInfo aCopyInfo;
						//@author zhangmiao@oristartech.com
						//@date [2013-07-15]
						//主要增加了相关字幕文件的处理代码
						//@new
						//aCopyInfo.filename = Dest + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);
						aCopyInfo.filename = fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);
						//@modify end;
						aCopyInfo.fileSize = static_cast<int>(aAsset_Pkl.size/ONEMB);
						aCopyInfo.bIsFinished = 0;
						//@author zhangmiao@oristartech.com
						//@date [2013-07-15]
						//主要增加了相关字幕文件的处理代码
						//@new
						//g_map_FileCopyInfo[Dest + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1)] = aCopyInfo;
						g_map_FileCopyInfo[ fileName ] = aCopyInfo;
						//@modify end;
					}
					//@modify zhangmiao [2012-12-26] end;

#ifdef WIN32
					ReleaseMutex(hMutex);
#else
					pthread_mutex_unlock(&work_mutex);
#endif
					
					//------拷贝copyInfo中的每个文件------
					//@author zhangmiao@oristartech.com
					//@date [2013-07-15]
					//主要增加了相关字幕文件的处理代码
					//@new
					//ret = CopyDcp_BPTransSub1( Dest, aPkl, copyInfo , error );
					ret = CopyDcp_BPTransSub1( source , Dest, aPkl, copyInfo , error , Dest2 , nLimitedSpeed );
					//@modify end;
					if ( ret != 0 )
					{

#ifdef _BP_DCPCOPYCANCEL_
						//如果断点续传时，DCP拷贝中途被取消，则暂时目前先删除未拷贝完的DCP目录；等以后增加“暂停功能”时再不删除未拷贝完的DCP目录；
						//其他原因引发的拷贝中断，则删除垃圾文件夹；
						//if ( ret != ERROR_CONTENT_DCP_BP_DCPCOPYCANCEL )
						{
							//删除垃圾文件夹，Remove DCP Directory
							int nResult = 0 , nResult2 = 0;
							nResult = RemoveDir( Dest );
							if( nResult != 0 )
							{
#ifdef _TEST_
								printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif //_TEST_
								error += "Warning:Remove DCP Directory fail.\n";
							}

							//@author zhangmiao@oristartech.com
							//@date [2014-09-03]
							//@brief 主要增加导入影片支持双目标目录的处理代码
							//@new
							if ( Dest2.length() != 0 )
							{
								nResult2 = RemoveDir( Dest2 );
								if( nResult2 != 0 )
								{
#ifdef _TEST_
									printf( "Warning:Remove DCP Dest2 Directory Fail.:%s\n", Dest2.c_str() );
#endif
									error += "Warning:Remove DCP Dest2 Directory Fail.\n";
								}
							}
							//@modify [2014-09-03] end;

						}
#endif //_BP_DCPCOPYCANCEL_

						return ret;
					}
					//----拷贝copyInfo中的每个文件:end--------

					//Copy PKL File
					ret = CopyDcpSub2( pklName, Dest, error , Dest2 );
					if ( ret != 0 )
					{
						//删除垃圾文件夹，Remove DCP Directory
						int nResult = 0 , nResult2 = 0;
						nResult = RemoveDir( Dest );
						if( nResult != 0 )
						{
#ifdef _TEST_
							printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif
							error += "Warning:Remove DCP Directory fail.\n";
						}

						//@author zhangmiao@oristartech.com
						//@date [2014-09-03]
						//@brief 主要增加导入影片支持双目标目录的处理代码
						//@new
						if ( Dest2.length() != 0 )
						{
							nResult2 = RemoveDir( Dest2 );
							if( nResult2 != 0 )
							{
#ifdef _TEST_
								printf( "Warning:Remove DCP Dest2 Directory Fail.:%s\n", Dest2.c_str() );
#endif
								error += "Warning:Remove DCP Dest2 Directory Fail.\n";
							}
						}
						//@modify [2014-09-03] end;

						return ret;
					}

					//Copy AssetMap File
					string AssetmapName = aAMInfo.assetmapName;
					ret = CopyDcpSub3( AssetmapName, pklUuid, Dest, dcp, aPkl , error , Dest2 );
					if ( ret != 0 )
					{
						//删除垃圾文件夹，Remove DCP Directory
						int nResult = 0 , nResult2 = 0;
						nResult = RemoveDir( Dest );
						if( nResult != 0 )
						{
#ifdef _TEST_
							printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif
							error += "Warning:Remove DCP Directory fail.\n";
						}


						//@author zhangmiao@oristartech.com
						//@date [2014-09-03]
						//@brief 主要增加导入影片支持双目标目录的处理代码
						//@new
						if ( Dest2.length() != 0 )
						{
							nResult2 = RemoveDir( Dest2 );
							if( nResult2 != 0 )
							{
#ifdef _TEST_
								printf( "Warning:Remove DCP Dest2 Directory Fail.:%s\n", Dest2.c_str() );
#endif
								error += "Warning:Remove DCP Dest2 Directory Fail.\n";
							}
						}
						//@modify [2014-09-03] end;

						return ret;
					}

					if ( bIsFoundPKL && (errorOccurred==false) )
					{
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						errno = 0;
						g_sCopyDcp_PklUuidName="";
						g_sCopyDcp_Copying_FileName="";

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif
						return 0;
					}
				}
			}
		}
		//Do not found pkl 没有找到PKL时，做必要的特殊处理
		if (bIsFoundPKL==false)
		{
			//删除垃圾文件夹，Remove DCP Directory
			ret = RemoveDir(Dest);
			if( ret != 0 )
			{
#ifdef _TEST_
				printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif
				error += "Warning:Remove DCP Directory Fail.\n";
			}


			//@author zhangmiao@oristartech.com
			//@date [2014-09-03]
			//@brief 主要增加导入影片支持双目标目录的处理代码
			//@new
			int nResult = 0;
			if ( Dest2.length() != 0 )
			{
				nResult = RemoveDir( Dest2 );
				if( nResult != 0 )
				{
#ifdef _TEST_
					printf( "Warning:Remove DCP Dest2 Directory Fail.:%s\n", Dest2.c_str() );
#endif
					error += "Warning:Remove DCP Dest2 Directory Fail.\n";
				}
			}
			//@modify [2014-09-03] end;

#ifdef WIN32
			WaitForSingleObject(hMutex,INFINITE);
#else
			pthread_mutex_lock(&work_mutex);
#endif

			g_sCopyDcp_PklUuidName="";
			g_sCopyDcp_Copying_FileName="";
			g_hasDcpCopied_Size = g_Dcp_AllFile_Size = 0ull;

#ifdef WIN32
			ReleaseMutex(hMutex);
#else
			pthread_mutex_unlock(&work_mutex);
#endif
			error = "Do not found pkl\n";

			//return -1;
			WELOG(error)
			errno = ERROR_CONTENT_DCP_DONOT_FOUND_PKL;
			return errno;
		}
	}
	return 0;
}
#endif // #if DCPCPY == 1


#if DCPCPY == 1
//断点续传方式，下载DCP包。
int DownLoadDcp_BPTrans( const string &ftpUrl, const string &pklUuid, const string &dest,
						 vector<FileCopyInfo> &copyInfo, string& error )
{
	bool errorOccurred = false;
	bool bIsFoundPKL   = false;
	int ret(0);
	
	vector<AssetmapInfo> dcpList;
	ret = SearchFtpDcpList( ftpUrl, dcpList ,error );
	if(ret!=0)
	{
#ifdef _TEST_
		printf("error SearchFtpDcpList !\n");
#endif
		WELOG("error SearchFtpDcpList !\n")
		return ret;
	}

	//Parameters Check
	string sFilmFilePath = dest;
	if( sFilmFilePath.rfind(DIR_SEPCHAR) != sFilmFilePath.length()-1 )	//检测dest目录的末尾是否带分隔符 "/" ，如果不带则给它补上。
		sFilmFilePath.append( 1, DIR_SEPCHAR );

	size_t pos;
	if ( ( pos = pklUuid.find("urn:uuid:") ) != pklUuid.npos && pos==0 )
	{
		sFilmFilePath = sFilmFilePath + pklUuid.substr(9);
	}
	else
	{
		sFilmFilePath = sFilmFilePath + pklUuid;
	}
	sFilmFilePath.append( 1, DIR_SEPCHAR );

	if ( sFilmFilePath.length()==0 )
	{
		error = "Error:FilmFilePath is NULL!\n";
		//errno = -2;
		WELOG(error)
		errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
		return errno;
	}

	//login FTP Server
	int result = g_FtpClient.LogIn(ftpUrl);
	if(result != cftp::NOANYERROR)
		return result;

	string ip, userName, passwd, dir;
	string port = "21";
	string url = ftpUrl;
	result = g_FtpClient.ParseUrl(url, ip, userName, passwd, port, dir);
	if(result != cftp::NOANYERROR)
		return result;

	//string URL_PFX = url.substr(0,end);

	string  Dest = sFilmFilePath;	//最终的Dest目录为"dest目录" + '/' + "pklUuid" + "/"

	ret = MakeDirectory2(Dest);		//建立目录，如果目标目录存在则不删除。
	if(ret!=0)
	{
		error = "Error:Make Directory fail.\n";
		WELOG(error)
		ret = ERROR_CONTENT_DCP_MAKE_DIR_FAIL;
		return ret;
	}
	
	//dcp copy files
	if ( ret==0 )
	{
		int nAMInfoCount = dcpList.size();
		for ( int i=0; i< nAMInfoCount; i++ )
		{
			AssetmapInfo aAMInfo = dcpList[i];
			vector< PklInfo > vDcp = aAMInfo.pklList;
			int pklN = vDcp.size();
			for( int j=0; j<pklN; j++ )
			{
				string pklName;

				PklInfo aPkl = vDcp[j];	
				if ( pklUuid == aPkl.pklUuid )
				{
					pklName = vDcp[j].pklName;
#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif
					g_sCopyDcp_PklUuidName = pklUuid;
					g_Dcp_AllFile_Size = aPkl.allFileLength;
					g_hasDcpCopied_Size = 0ull;

					bIsFoundPKL = true;//pkl找到
					g_map_FileCopyInfo.clear();

					// 12/26/2012 zhangmiao begin 
					//初始化g_map_FileCopyInfo 映射结构，返回pkl的所有文件状态。
					vector<CplInfo> vAsset_pkl = aPkl.cplList;
					int nSize_Asset_Pkl = vAsset_pkl.size();
					if (nSize_Asset_Pkl==0)
					{
						error = "Error:PKL AssetList is null!\n";
						errno = ERROR_CONTENT_DCP_PKL_ASSELIST_IS_NULL;
						WELOG(error)
						return errno;
					}
					for ( int k=0; k< nSize_Asset_Pkl; k++ )
					{
						CplInfo aAsset_Pkl = vAsset_pkl[k];
						string fileName = aAsset_Pkl.cplName;
						if ( fileName.length()==0 )
						{
							error = "Error:Pkl AssetFile Path Is NULL!\n";
							errno = ERROR_CONTENT_DCP_PKL_ASSEFILE_PATH_IS_NULL;
							WELOG(error)
							return errno;
						}

						FileCopyInfo aCopyInfo;
						aCopyInfo.filename = Dest + fileName.substr(fileName.rfind('/')+1);
						aCopyInfo.fileSize = static_cast< int >(aAsset_Pkl.cplLength/ONEMB);
						aCopyInfo.bIsFinished = 0;
						g_map_FileCopyInfo[Dest + fileName.substr(fileName.rfind('/')+1)] = aCopyInfo;
					}
					
					vector<MxfInfo> vMxfInfo_pkl = aPkl.mxfList;
					nSize_Asset_Pkl = vMxfInfo_pkl.size();
					if (nSize_Asset_Pkl==0)
					{
						error = "Error:PKL AssetList is null!\n";
						errno = ERROR_CONTENT_DCP_PKL_ASSELIST_IS_NULL;
						WELOG(error)
						return errno;
					}
					for ( int k=0; k< nSize_Asset_Pkl; k++ )
					{
						MxfInfo aAsset_Pkl = vMxfInfo_pkl[k];
						string fileName = aAsset_Pkl.fileName;
						if ( fileName.length()==0 )
						{
							error = "Error:Pkl AssetFile mxf File Path Is NULL!\n";
							errno = ERROR_CONTENT_DCP_PKL_ASSEFILE_PATH_IS_NULL;
							WELOG(error)
							return errno;
						}
						FileCopyInfo aCopyInfo;
						aCopyInfo.filename = Dest + fileName.substr(fileName.rfind('/')+1);
						aCopyInfo.fileSize = static_cast<int>(aAsset_Pkl.length/ONEMB);
						aCopyInfo.bIsFinished = 0;
						g_map_FileCopyInfo[Dest + fileName.substr(fileName.rfind('/')+1)] = aCopyInfo;
					}

					//@author zhangmiao@oristartech.com
					//@date [2013-07-18]
					//@brief 主要增加了FTP下载相关字幕文件的处理代码
					//@new
					vector<OtherFileInfo>& vFileInfoList_pkl = aPkl.fileInfoList;
					nSize_Asset_Pkl = vFileInfoList_pkl.size();
					for ( int k=0; k< nSize_Asset_Pkl; k++ )
					{
						OtherFileInfo& aAsset_Pkl = vFileInfoList_pkl[k];
						string originalfileName = aAsset_Pkl.originalfileName;
						if ( originalfileName.length()==0 )
						{
							error = "Error:Pkl AssetFile OtherFile Path Is NULL!\n";
							errno = ERROR_CONTENT_DCP_PKL_ASSEFILE_PATH_IS_NULL;
							WELOG(error);
							return errno;
						}
						
						FileCopyInfo aCopyInfo;
						aCopyInfo.filename = Dest + originalfileName;
						aCopyInfo.fileSize = static_cast<int>(aAsset_Pkl.fileLength/ONEMB);
						aCopyInfo.bIsFinished = 0;
						g_map_FileCopyInfo[ Dest + originalfileName ] = aCopyInfo;
					}
					//@modify end;
					
#ifdef WIN32
					ReleaseMutex(hMutex);
#else
					pthread_mutex_unlock(&work_mutex);
#endif
					//------下载copyInfo中的每个文件------
					ret = DownLoadDcp_BPTransSub1( Dest, aPkl, copyInfo , error );
					if ( ret != 0 )
					{

#ifdef _BP_DCPCOPYCANCEL_
						//如果断点续传时，DCP拷贝中途被取消，则暂时目前先删除未拷贝完的DCP目录；等以后增加“暂停功能”时再不删除未拷贝完的DCP目录；
						//其他原因引发的拷贝中断，则删除垃圾文件夹；
						//if ( ret != ERROR_CONTENT_DCP_BP_DCPCOPYCANCEL )
						{
							//删除垃圾文件夹，Remove DCP Directory
							int nResult = RemoveDir( Dest );
							if( nResult != 0 )
							{
#ifdef _TEST_
								printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif //_TEST_
								error += "Warning:Remove DCP Directory fail.\n";
							}
						}
#endif //_BP_DCPCOPYCANCEL_

						return ret;
					}
					//----下载copyInfo中的每个文件:end--------

					//Download PKL File
					ret = DownLoadDcp_BPTransSub2( pklName, Dest , error );
					if ( ret != 0 )
					{
						//删除垃圾文件夹，Remove DCP Directory
						int nResult = RemoveDir( Dest );
						if( nResult != 0 )
						{
#ifdef _TEST_
							printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif
							error += "Warning:Remove DCP Directory fail.\n";
						}

						return ret;
					}

					//Download AssetMap File
					string AssetmapName = aAMInfo.assetmapName;
					ret = DownLoadDcp_BPTransSub3( AssetmapName, pklUuid, Dest, aPkl , error );
					if ( ret != 0 )
					{
						//删除垃圾文件夹，Remove DCP Directory
						int nResult = RemoveDir( Dest );
						if( nResult != 0 )
						{
#ifdef _TEST_
							printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif
							error += "Warning:Remove DCP Directory fail.\n";
						}

						return ret;
					}

					if ( bIsFoundPKL && (errorOccurred==false) )
					{
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						errno = 0;
						g_sCopyDcp_PklUuidName="";
						g_sCopyDcp_Copying_FileName="";
						g_FtpClient.LogOut();

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif
						return 0;
					}
				}
			}
		}
		//Do not found pkl
		if (bIsFoundPKL==false)
		{
			//删除垃圾文件夹，Remove DCP Directory
			ret = RemoveDir(Dest);
			if( ret != 0 )
			{
#ifdef _TEST_
				printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif
				error += "Warning:Remove DCP Directory Fail.\n";
			}

#ifdef WIN32
			WaitForSingleObject(hMutex,INFINITE);
#else
			pthread_mutex_lock(&work_mutex);
#endif

			g_sCopyDcp_PklUuidName="";
			g_sCopyDcp_Copying_FileName="";
			g_hasDcpCopied_Size = g_Dcp_AllFile_Size = 0ull;
			g_FtpClient.LogOut();

#ifdef WIN32
			ReleaseMutex(hMutex);
#else
			pthread_mutex_unlock(&work_mutex);
#endif
			error = "Do not found pkl\n";
			
			//return -1;
			WELOG(error)
			errno = ERROR_CONTENT_DCP_DONOT_FOUND_PKL;
			return errno;
		}
	}

	return 0;
}
#endif //#if DCPCPY == 1

#if DCPCPY == 1
//下载copyInfo中的每个文件
int DownLoadDcp_BPTransSub1( const std::string &Dest, PklInfo& aPkl, std::vector<FileCopyInfo> &copyInfo, std::string& error )
{
	int ret(0);
	int nSize_Copy_Info = copyInfo.size();
	for( int s=0; s<nSize_Copy_Info; s++ )
	{
		FileCopyInfo& rFileCopyInfo = copyInfo[s];

		if (rFileCopyInfo.bIsFinished==1)		//如果拷贝完成，则继续下一个文件。
		{
			//----zhangmiao:begin--[7/20/2012]----
			long long Length = 0;
			//@author zhangmiao@oristartech.com
			//@date [2013-07-18]
			//@brief 主要增加了FTP下载相关字幕文件的处理代码
			//@new
			//ret = GetFileSize_PKLAssetList( rFileCopyInfo.filename , aPkl, Length );
			std::string originalfileName;
			ret = GetFileSize_PKLAssetList( rFileCopyInfo.filename , aPkl, Length , originalfileName );
			//@modify end;
			if (ret==0)
			{
				g_hasDcpCopied_Size += Length;
			}
			//----zhangmiao:end----[7/20/2012]----

			//更新获取进度时的状态
			//----zhangmiao:begin--20121226----
#ifdef WIN32
			WaitForSingleObject(hMutex,INFINITE);
#else
			pthread_mutex_lock(&work_mutex);
#endif
			//@author zhangmiao@oristartech.com
			//@date [2013-07-18]
			//@brief 主要增加了FTP下载相关字幕文件的处理代码
			//@new
			//string destFileName = Dest + rFileCopyInfo.filename;
			string destFileName = Dest + originalfileName;
			//@modify end;
			map< string , FileCopyInfo >::size_type nCont = g_map_FileCopyInfo.count(destFileName);
			if ( nCont == 1 )
			{
				FileCopyInfo& copyInfo   = g_map_FileCopyInfo[destFileName]; 
				copyInfo.transferredSize = copyInfo.fileSize;
				copyInfo.bIsFinished = 1;
			}
#ifdef WIN32
			ReleaseMutex(hMutex);
#else
			pthread_mutex_unlock(&work_mutex);
#endif
			//----zhangmiao:end----20121226----

			continue;
		}

		string sFileCopyName = rFileCopyInfo.filename;
		if ( sFileCopyName.length()==0 )
		{
			error = "Error:FileCopyName is NULL!\n";
			//errno = -2;
			errno = ERROR_CONTENT_DCP_FILECOPYNAME_IS_NULL;
			WELOG(error)
			return errno;
		}

		//Download CPL File
		vector<CplInfo> vAsset_pkl = aPkl.cplList;
		int nSize_Asset_Pkl = vAsset_pkl.size();
		for ( int k=0; k< nSize_Asset_Pkl; k++ )
		{
			CplInfo aAsset_Pkl = vAsset_pkl[k];
			string fileName = aAsset_Pkl.cplName;
			if ( fileName.length()==0 )
			{
				error = "Error:CPL File Path Is NULL!\n";
				//errno = -2;
				errno = ERROR_CONTENT_DCP_CPL_FILE_PATH_IS_NULL;
				WELOG(error)
				return errno;
			}

			if ( sFileCopyName == fileName.substr(fileName.rfind('/')+1) )    //判断文件是否找到
			{
				//直接下载CPL文件
#ifdef WIN32
				WaitForSingleObject(hMutex,INFINITE);
#else
				pthread_mutex_lock(&work_mutex);
#endif

				g_sCopyDcp_Copying_FileName = fileName;

#ifdef WIN32
				ReleaseMutex(hMutex);
#else
				pthread_mutex_unlock(&work_mutex);
#endif
				//check file exist
				vector<cftp::FileInformation> fileList;
				ret = listFilesOnFTP(g_FtpClient,fileName.substr(0,fileName.rfind('/')+1),fileList, fileName.substr(fileName.rfind('/')+1));
				if(ret != cftp::NOANYERROR)
					return ret;
				ret = ( fileList.size()!=0 && fileList[0].name==fileName );
				if( ret )
				{
#ifdef _TEST_
					printf( "File  exists :%s\n", fileName.c_str() );
#endif

#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif	

					FileCopyInfo aCopyInfo;
					aCopyInfo.filename = Dest + fileName.substr(fileName.rfind('/')+1);
					aCopyInfo.fileSize = static_cast< int >(aAsset_Pkl.cplLength/ONEMB);
					aCopyInfo.transferredSize = 0;
					g_map_FileCopyInfo[Dest + fileName.substr(fileName.rfind('/')+1)]=aCopyInfo;

#ifdef WIN32
					ReleaseMutex(hMutex);
#else
					pthread_mutex_unlock(&work_mutex);
#endif
					//string srcFileName = URL_PFX  +  fileName;
					string destFileName = Dest + fileName.substr(fileName.rfind('/')+1);

					string dir = fileName.substr( 0, fileName.rfind( '/' ) );
					ret = g_FtpClient.TransportFile( dir  , Dest , fileName.substr( fileName.rfind( '/' )+1) );
					if (ret==0)
					{
						//----zhangmiao:begin--20121226----
						//更新获取进度时的状态，copy CPL file
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						FileCopyInfo& copyInfo   = g_map_FileCopyInfo[destFileName]; 
						copyInfo.transferredSize = copyInfo.fileSize;
						copyInfo.bIsFinished = 1;

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif
						//----zhangmiao:end----20121226----

						rFileCopyInfo.bIsFinished = 1;
						//----zhangmiao:begin--[7/20/2012]----
						g_hasDcpCopied_Size += aAsset_Pkl.cplLength;
						//----zhangmiao:end----[7/20/2012]----
#ifdef _TEST_
						printf( "File copy finished :%s\n", destFileName.c_str() );
#endif
					}
					else
					{
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						g_sCopyDcp_PklUuidName="";
						g_Dcp_AllFile_Size = 0ull;

						if ( ret == CODE_DCPCOPYCANCEL )
						{
							g_flag_DcpCopyStop = 0;

							char buffer[BUF_SIZE]="";
							sprintf( buffer,"%s:%s\n",fileName.c_str(),"DCP Copy CANCEL!" );
							error = buffer;
#ifdef _TEST_
							printf("%s:%s\n",fileName.c_str(),"DCP Copy CANCEL!");
#endif

							errno = ERROR_CONTENT_DCP_BP_DCPCOPYCANCEL;
						}
						else
						{
#ifdef _TEST_
							printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif

							errno = ERROR_CONTENT_DCP_FILE_COPY_FAILED;
						}

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif
						//errno = 1;
						WELOG(error)
						return errno;
					}
				}
				else  //file NOT exist
				{
					char buffer[BUF_SIZE]="";
					sprintf( buffer,"%s:%s\n",fileName.c_str(),strerror(errno) );
					error = buffer;
#ifdef _TEST_
					printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif

#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif

					g_sCopyDcp_PklUuidName="";
					g_Dcp_AllFile_Size = 0ull;

#ifdef WIN32
					ReleaseMutex(hMutex);
#else
					pthread_mutex_unlock(&work_mutex);
#endif
					errno = ERROR_CONTENT_DCP_CPL_FILE_IS_NO_EXIST;
					WELOG(error)
					return errno;
				}


			}
			else
			{	//没有找到，则继续查找下一个文件
				continue;
			}

		}

		//@author zhangmiao@oristartech.com
		//@date [2013-07-18]
		//@brief 主要增加了FTP下载相关字幕文件的处理代码
		//@new
		//Download Other File
		vector<OtherFileInfo>& vFileInfoList_pkl = aPkl.fileInfoList;
		int nSize_OtherFileInfo_Asset_Pkl = vFileInfoList_pkl.size();
		for ( int k = 0; k < nSize_OtherFileInfo_Asset_Pkl; k++ )
		{
			OtherFileInfo& aAsset_Pkl = vFileInfoList_pkl[k];
			string fileName = aAsset_Pkl.fileName;
			string originalfileName = aAsset_Pkl.originalfileName;
			if ( fileName.length()==0 )
			{
				error = "Error:Pkl AssetFile Path Is NULL!\n";
				errno = ERROR_CONTENT_DCP_PKL_ASSEFILE_PATH_IS_NULL;
				WELOG(error);
				return errno;
			}

			if ( sFileCopyName == fileName.substr(fileName.rfind('/')+1) )
			{
				//直接下载其它相关文件，比如字幕文件
#ifdef WIN32
				WaitForSingleObject(hMutex,INFINITE);
#else
				pthread_mutex_lock(&work_mutex);
#endif

				g_sCopyDcp_Copying_FileName = fileName;

#ifdef WIN32
				ReleaseMutex(hMutex);
#else
				pthread_mutex_unlock(&work_mutex);
#endif

				//check file exist
				vector<cftp::FileInformation> fileList;
				ret = listFilesOnFTP(g_FtpClient,fileName.substr(0,fileName.rfind('/')+1),fileList, fileName.substr(fileName.rfind('/')+1));
				if(ret != cftp::NOANYERROR)
					return ret;
				ret = ( fileList.size()!=0 && fileList[0].name==fileName );
				if( ret )
				{
#ifdef _TEST_
					printf( "File  exists :%s\n", fileName.c_str() );
#endif

#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif	

					FileCopyInfo aCopyInfo;
					aCopyInfo.filename = Dest + originalfileName;
					aCopyInfo.fileSize = static_cast< int >(aAsset_Pkl.fileLength/ONEMB);
					aCopyInfo.transferredSize = 0;
					g_map_FileCopyInfo[ Dest + originalfileName ] = aCopyInfo;

#ifdef WIN32
					ReleaseMutex(hMutex);
#else
					pthread_mutex_unlock(&work_mutex);
#endif

					//如果目标目录包含子目录则，则建立相应的目录。
					string SubPath;
					std::string::size_type pos = originalfileName.rfind('/');
					if ( pos != string::npos )
					{
						SubPath = originalfileName.substr( 0, pos ) + DIR_SEPCHAR;
					}
					string DestFile_InPath = Dest + SubPath;
					ret = MakeDirectory2(DestFile_InPath);		//建立目录，如果目标目录存在则不删除。
					if(ret!=0)
					{
						error = "Error:DownLoadDcp_BPTransSub1--MakeDirectory2 fail.\n";
						WELOG(error);
						ret = ERROR_CONTENT_DCP_MAKE_DIR_FAIL;
						return ret;
					}

					
					string destFileName = Dest + originalfileName;

					string dir = fileName.substr( 0, fileName.rfind( '/' ) );
					ret = g_FtpClient.TransportFile( dir  , DestFile_InPath , fileName.substr( fileName.rfind( '/' )+1) );
					if (ret==0)
					{
						//更新获取进度时的状态，copy file 成功
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						FileCopyInfo& copyInfo   = g_map_FileCopyInfo[destFileName]; 
						copyInfo.transferredSize = copyInfo.fileSize;
						copyInfo.bIsFinished = 1;

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif

						rFileCopyInfo.bIsFinished = 1;
						
						g_hasDcpCopied_Size += aAsset_Pkl.fileLength;
						
#ifdef _TEST_
						printf( "File copy finished :%s\n", destFileName.c_str() );
#endif
					}
					else
					{
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						g_sCopyDcp_PklUuidName="";
						g_Dcp_AllFile_Size = 0ull;

						if ( ret == CODE_DCPCOPYCANCEL )
						{
							g_flag_DcpCopyStop = 0;

							char buffer[BUF_SIZE]="";
							sprintf( buffer,"%s:%s\n",fileName.c_str(),"DCP Copy CANCEL!" );
							error = buffer;
#ifdef _TEST_
							printf("%s:%s\n",fileName.c_str(),"DCP Copy CANCEL!");
#endif

							errno = ERROR_CONTENT_DCP_BP_DCPCOPYCANCEL;
						}
						else
						{
#ifdef _TEST_
							printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif

							errno = ERROR_CONTENT_DCP_FILE_COPY_FAILED;
						}

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif
						WELOG(error);
						return errno;
					}
				}
				else  //file NOT exist
				{
					char buffer[BUF_SIZE]="";
					sprintf( buffer,"%s:%s\n",fileName.c_str(),strerror(errno) );
					error = buffer;
#ifdef _TEST_
					printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif

#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif

					g_sCopyDcp_PklUuidName="";
					g_Dcp_AllFile_Size = 0ull;

#ifdef WIN32
					ReleaseMutex(hMutex);
#else
					pthread_mutex_unlock(&work_mutex);
#endif
					errno = ERROR_CONTENT_DCP_FILE_NOT_EXIST;
					WELOG(error);
					return errno;
				}
			}
			else
			{	//没有找到，则继续查找下一个文件
				continue;
			}
		}
		//Download Other File end
		//@modify end;

		//Download mxf File
		vector<MxfInfo> vMxfInfo_pkl = aPkl.mxfList;
		int nSize_MxfInfo_Asset_Pkl = vMxfInfo_pkl.size();
		for ( int k=0; k< nSize_MxfInfo_Asset_Pkl; k++ )
		{
			MxfInfo aAsset_Pkl = vMxfInfo_pkl[k];
			string fileName = aAsset_Pkl.fileName;
			if ( fileName.length()==0 )
			{
				error = "Error:mxf File Path Is NULL!\n";
				//errno = -2;
				errno = ERROR_CONTENT_DCP_MXF_FILE_PATH_IS_NULL;
				WELOG(error)
				return errno;
			}

			if ( sFileCopyName == fileName.substr(fileName.rfind('/')+1) )    //判断文件是否找到
			{
				//下载mxf文件
#ifdef WIN32
				WaitForSingleObject(hMutex,INFINITE);
#else
				pthread_mutex_lock(&work_mutex);
#endif

				g_sCopyDcp_Copying_FileName = fileName;

#ifdef WIN32
				ReleaseMutex(hMutex);
#else
				pthread_mutex_unlock(&work_mutex);
#endif
				//check file exist
				vector<cftp::FileInformation> fileList;
				ret = listFilesOnFTP(g_FtpClient,fileName.substr(0,fileName.rfind('/')+1),fileList, fileName.substr(fileName.rfind('/')+1));
				if(ret != cftp::NOANYERROR)
					return ret;
				ret = ( fileList.size()!=0 && fileList[0].name==fileName );
				if( ret  )
				{
#ifdef _TEST_
					printf( "File  exists :%s\n", fileName.c_str() );
#endif

#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif
					FileCopyInfo aCopyInfo;
					aCopyInfo.filename = Dest + fileName.substr(fileName.rfind('/')+1);
					aCopyInfo.fileSize = static_cast<int>(aAsset_Pkl.length/ONEMB);
					aCopyInfo.transferredSize = 0;
					g_map_FileCopyInfo[Dest + fileName.substr(fileName.rfind('/')+1)]=aCopyInfo;

					//string srcFileName = URL_PFX  +  fileName;
					string destFileName = Dest + fileName.substr(fileName.rfind('/')+1);

					unsigned long long nlTransferredSize = 0;
					unsigned long long nfrom = 0;
					nlTransferredSize = rFileCopyInfo.transferredSize*ONEMB;
					nfrom = nlTransferredSize;
					//----zhangmiao:begin--[7/20/2012]----
					g_hasDcpCopied_Size += nlTransferredSize;
					//----zhangmiao:end----[7/20/2012]----
#ifdef WIN32
					ReleaseMutex(hMutex);
#else
					pthread_mutex_unlock(&work_mutex);
#endif

					string dir = fileName.substr( 0, fileName.rfind( '/' ) );

					ret = g_FtpClient.TransportFileOnBreakPoint( dir  , Dest , fileName.substr( fileName.rfind( '/' )+1) , nfrom );
					nlTransferredSize = g_FtpClient.FileTransportSize();
					rFileCopyInfo.transferredSize = static_cast<int>(nlTransferredSize/ONEMB);
					rFileCopyInfo.fileSize = static_cast<int>(aAsset_Pkl.length/ONEMB);

					if (ret==0)
					{
						//----zhangmiao:begin--20121226----
						//更新获取进度时的状态，copy mxf file
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						FileCopyInfo& copyInfo   = g_map_FileCopyInfo[destFileName]; 
						copyInfo.transferredSize = copyInfo.fileSize;
						copyInfo.bIsFinished = 1;

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif
						//----zhangmiao:end----20121226----

						rFileCopyInfo.bIsFinished = 1;
#ifdef _TEST_
						printf( "File copy finished :%s\n", destFileName.c_str() );
#endif
					}
					else 
					{
#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						g_sCopyDcp_PklUuidName="";
						g_Dcp_AllFile_Size = 0ull;

						if ( ret == CODE_DCPCOPYCANCEL )
						{
							g_flag_DcpCopyStop = 0;

							char buffer[BUF_SIZE]="";
							sprintf( buffer,"%s:%s\n",fileName.c_str(),"DCP Copy CANCEL!" );
							error = buffer;

#ifdef _TEST_
							printf("%s:%s\n",fileName.c_str(),"DCP Copy CANCEL!");
#endif

							errno = ERROR_CONTENT_DCP_BP_DCPCOPYCANCEL;
						}
						else
						{
#ifdef _TEST_
							printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif

							errno = ERROR_CONTENT_DCP_MXF_FILE_COPY_IS_UNFINISHED;
						}
							


#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif
						//errno = 1;
						WELOG(error)
						return errno;
					}
				}
				else  //file NOT exist
				{
					char buffer[BUF_SIZE]="";
					sprintf( buffer,"%s:%s\n",fileName.c_str(),strerror(errno) );
					error = buffer;
#ifdef _TEST_
					printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif

#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif

					g_sCopyDcp_PklUuidName="";
					g_Dcp_AllFile_Size = 0ull;

#ifdef WIN32
					ReleaseMutex(hMutex);
#else
					pthread_mutex_unlock(&work_mutex);
#endif
					errno = ERROR_CONTENT_DCP_MXF_FILE_IS_NO_EXIST;
					WELOG(error)
					return errno;
				}


			}
			else
			{	//没有找到，则继续查找下一个文件
				continue;
			}

		}
	}
	return ret;
}
#endif //#if DCPCPY == 1

#if DCPCPY == 1
//Download PKL File
int DownLoadDcp_BPTransSub2( const std::string &pklName, const std::string &Dest, std::string& error )
{
	int ret(0);
	if (pklName.length())
	{
		vector<cftp::FileInformation> fileList;
		string fileName = pklName;
		ret = listFilesOnFTP(g_FtpClient,fileName.substr(0,fileName.rfind('/')+1), fileList, fileName.substr(fileName.rfind('/')+1));
		if(ret != cftp::NOANYERROR)
			return ret;
		ret = fileList.size()!=0 && ( fileList[0].name==fileName );    //check file exist,从ftp服务器上搜索
		if( ret )
		{
			string destFileName = Dest + pklName.substr(pklName.rfind('/')+1);

			string dir = pklName.substr( 0, pklName.rfind( '/' ) );
			ret = g_FtpClient.TransportFile( dir  , Dest , pklName.substr( pklName.rfind( '/' )+1) );	//下载真正pkl文件
			if (ret==0)
			{
#ifdef _TEST_
				printf( "PKL File copy finished :%s\n", destFileName.c_str() );
#endif
			}
			else if( ret )
			{
				char buffer[BUF_SIZE]="";
				sprintf( buffer,"PKL File copy Failed :%s\n", destFileName.c_str() );
				error = buffer;
#ifdef _TEST_
				printf( "PKL File copy Failed :%s\n", destFileName.c_str() );
#endif
				//errno = 1;
				errno = ERROR_CONTENT_DCP_PKL_FILE_COPY_FAILED;
				WELOG(error)
				return errno;
			}
		}
		else
		{
			char buffer[BUF_SIZE]="";
			sprintf( buffer,"Error:PKL File(%s) no exist!\n", pklName.c_str() );
			error = buffer;
#ifdef _TEST_
			printf( "Error:PKL File(%s) no exist!\n", pklName.c_str() );
#endif
			//errno = 10;
			errno = ERROR_CONTENT_DCP_PKL_FILE_NO_EXIST;
			WELOG(error)
			return errno;
		}
	}
	else
	{
		error = "Error:Param pklName is Null string!\n";
		//errno = -5;
		errno = ERROR_CONTENT_DCP_PARAM_PKLNAME_IS_NULL_STR;
		WELOG(error)
		return errno;
	}
	return ret;
}
#endif //#if DCPCPY == 1

#if DCPCPY == 1
//Download AssetMap File
int DownLoadDcp_BPTransSub3( const std::string &AssetmapName, const std::string &pklUuid, const std::string &Dest,
							  PklInfo& aPkl , std::string& error )
{
	int ret(0);
	if (AssetmapName.length())
	{
		vector<cftp::FileInformation> fileList;
		string fileName = AssetmapName;
		ret = listFilesOnFTP(g_FtpClient,fileName.substr(0,fileName.rfind('/')+1), fileList, fileName.substr(fileName.rfind('/')+1));
		if(ret != cftp::NOANYERROR)
			return ret;
		ret = fileList.size()!=0 && ( fileList[0].name==fileName );    //check file exist,从ftp服务器上搜索
		if( ret )
		{
			string destFileName = Dest + AssetmapName.substr(AssetmapName.rfind('/')+1);

			string dir = AssetmapName.substr( 0, AssetmapName.rfind( '/' ) );
			ret = g_FtpClient.TransportFile( dir  , Dest , AssetmapName.substr( AssetmapName.rfind( '/' )+1) );		//下载真正AssetMap文件
			if (ret==0)
			{
#ifdef _TEST_
				printf( "File copy finished :%s\n", destFileName.c_str() );
#endif
			}
			else if( ret )
			{
				char buffer[BUF_SIZE]="";
				sprintf( buffer,"File copy Failed :%s\n", destFileName.c_str() );
				error = buffer;
#ifdef _TEST_
				printf( "File copy Failed :%s\n", destFileName.c_str() );
#endif
				//errno = 1;
				errno = ERROR_CONTENT_DCP_FILE_COPY_FAILED;
				WELOG(error)
				return errno;
			}

#if 1
			//----zhangmiao:begin---2012-12-10----
			//由Content::PklInfo& aPkl生成Content::Dcp::pkl_Info_t结构.
			Content::Dcp::pkl_Info_t aPkl_Info;
			aPkl_Info.uuid           = aPkl.pklUuid;
			aPkl_Info.annotationText = aPkl.AnnotationText;
			aPkl_Info.issueDate      = aPkl.IssueDate;
			aPkl_Info.issuer         = aPkl.Issuer;
			aPkl_Info.creator        = aPkl.Creator;
			aPkl_Info.allFileLength  = aPkl.allFileLength;

			int cplCount = aPkl.cplList.size();
			for ( int i = 0; i < cplCount; i++ )
			{
				asset_pkl_t aAsset_Pkl;
				CplInfo& aCpl = aPkl.cplList[i];
				aAsset_Pkl.uuid = aCpl.cplUuid;
				aPkl_Info.asset.push_back(aAsset_Pkl);
			}
			
			int mxfCount = aPkl.mxfList.size();
			for ( int j = 0; j < mxfCount; j++ )
			{
				asset_pkl_t aAsset_Pkl;
				MxfInfo aMxfInfo = aPkl.mxfList[j];
				aAsset_Pkl.uuid = aMxfInfo.mxfUuid;
				aPkl_Info.asset.push_back(aAsset_Pkl);
			}

			vector<OtherFileInfo>& fileInfoList = aPkl.fileInfoList;
			int otherCount = fileInfoList.size();
			for ( int k = 0; k < otherCount; k++ )
			{
				asset_pkl_t aAsset_Pkl;
				OtherFileInfo& aFileInfo = fileInfoList[k];
				aAsset_Pkl.uuid = aFileInfo.Uuid;
				aPkl_Info.asset.push_back(aAsset_Pkl);
			}
			
			//Rebuilt AssetMap File
			DcpInfo dcp;
			ret = dcp.ReBuild_Assetmap( destFileName ,pklUuid ,aPkl_Info , error );
			if (ret!=0)
			{
				char buffer[BUF_SIZE]="";
				sprintf( buffer,"Error ReBuild_Assetmap File FAILED:%s\n", destFileName.c_str() );
				error = buffer;
#ifdef _TEST_
				printf( "Error ReBuild_Assetmap File FAILED:%s\n", destFileName.c_str() );
#endif
				WELOG(error);
				errno = ERROR_CONTENT_DCP_REBUILD_ASSETMAP_FILE_FAILED;
				ret = errno;
				return ret;
			}
			//----zhangmiao:end-----2012-12-10----
#endif

			//@author zhangmiao@oristartech.com
			//@date [2014-09-30]
			//@brief 统一将拷贝后的Assetmap文件更名为固定的名字："ASSETMAP" 
			//@new
			int iRet = 0;
			string newDestFileName = Dest + "ASSETMAP";
			if ( newDestFileName != destFileName )
			{
				/* 更改文件名 */
				iRet = rename( destFileName.c_str() , newDestFileName.c_str() );
				if ( iRet == 0 )
				{
					printf("rename: %s to %s.\n", destFileName.c_str() , newDestFileName.c_str() );
				}
				else
				{
					perror("rename");
				}
			}
			//@modify end;
		}
		else
		{
			char buffer[BUF_SIZE]="";
			sprintf( buffer,"Error:Assetmap(%s) is no exist!\n", AssetmapName.c_str() );
			error = buffer;
#ifdef _TEST_
			printf( "Error:Assetmap(%s) is no exist!\n", AssetmapName.c_str() );
#endif
			//errno = 3020;
			errno = ERROR_CONTENT_DCP_ASSETMAP_IS_NO_EXIST;
			WELOG(error)
			ret = errno;
		}
	}
	else
	{
		error = "Error:Param AssetmapName is Null string!\n";
#ifdef _TEST_
		printf( "Error:Param AssetmapName is Null string!\n" );
#endif
		//errno = 3010;
		errno = ERROR_CONTENT_DCP_PARAM_ASSETMAPNAME_IS_NULL_STR;
		WELOG(error)
		ret = errno;
		return ret;
	}

	return ret;
}
#endif //#if DCPCPY == 1


#if DCPCPY == 1
int CopyDcp( const string &source, const string &pklUuid, const string &dest, vector<FileCopyInfo> &copyInfo, std::string& error ,
			 const string dest2 , int nLimitedSpeed )
{
	string sDest_Path_1 , sDest_Path_2;

	int nPathMode = 2;	//nPathMode =1 使用单路径, = 2 使用双路径
	if( nPathMode == 1 )
	{
		sDest_Path_1 = dest;

		if ( dest.length() == 0 && dest2.length() != 0 )
		{
			sDest_Path_1 = dest2;
		} 
		else if( dest.length() != 0 && dest2.length() == 0 ) 
		{
			sDest_Path_1 = dest;
		}
		else if( dest.length() != 0 && dest2.length() != 0 ) 
		{
			sDest_Path_1 = dest;
		}
		else if( dest.length() == 0 && dest2.length() == 0 ) 
		{
			error = "Error:DestPath1 and DestPath2 is NULL!\n";
			WELOG(error);
			errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
			return CMTT2(errno);
		}
	}
	else
	{
		if ( dest.length() == 0 && dest2.length() != 0 )
		{
			sDest_Path_1 = dest2;
		} 
		else if( dest.length() != 0 && dest2.length() == 0 ) 
		{
			sDest_Path_1 = dest;

#ifdef _READDB_DEST_PATH2
			//直接读取数据库表
			int iResult(0);
			//C_Para::GetInstance()->ReadPara();
			C_RunPara *pRunPara = C_RunPara::GetInstance();
			iResult = pRunPara->GetSavePath2( sDest_Path_2 );
			if( iResult != 0 )
			{
				error = "Error:DestPath2 is NULL!\n";
				WELOG(error);
				errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
				return CMTT2(errno);
			}
#endif // _READDB_DEST_PATH2
			
		}
		else if( dest.length() != 0 && dest2.length() != 0 ) 
		{
			sDest_Path_1 = dest;
			sDest_Path_2 = dest2;
		}
		else if( dest.length() == 0 && dest2.length() == 0 ) 
		{
			error = "Error:DestPath1 and DestPath2 is NULL!\n";
			WELOG(error);
			errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
			return CMTT2(errno);
		}
	}

	//@author zhangmiao@oristartech.com
	//@date [2012-06-26]
	//@brief 用于处理断点续传
	//@new
	bool bIsBPTrans = false;
	if( copyInfo.size()!=0 )
	{
		bIsBPTrans = true;
	}
	if (bIsBPTrans)
	{
		int ret = 0;
		//int ret = CopyDcp_BPTrans( source, pklUuid, dest, copyInfo, error  , dest2 , nLimitedSpeed );
		ret = CopyDcp_BPTrans( source , pklUuid , sDest_Path_1 , copyInfo , error , sDest_Path_2 , nLimitedSpeed );
		if( ret != 0 )
			ret = CMTT2(ret);
		return ret;
	}
	//@modify [2012-06-26] end;

	//bIsFtp_UploadFile_Failed：表示是否ftp上传操作在某个地方出现过失败
	int bIsFtp_UploadFile_Failed = 0;  //=1,表示出现失败
	g_bIsFtp_UploadFile_Failed = 0;    //复位g_bIsFtp_UploadFile_Failed

	bool errorOccurred = false;
	bool bIsFoundPKL   = false;
	int ret(0);
	DcpInfo dcp;
	vector< AMInfo > aAMInfo_Vt;
	ret = dcp.CreateDCPInfo( source ,error );	//程序会先到source所指目录下去搜索，看是否搜索到影片，source目录可以设定为外接移动硬盘挂载目录。
	if(ret!=0)
	{
#ifdef _TEST_
		printf("error CreateDCPInfo !\n");
#endif
		error = "error:CreateDCPInfo()!\n" + error;
		WELOG(error)
		ret = CMTT(ERROR_CONTENT_DCP_CREATE_DCPINFO_ERROR);
		return ret;
	}
	//Parameters Check
	string sFilmFilePath = sDest_Path_1; //string sFilmFilePath = dest;
	if( sFilmFilePath.rfind(DIR_SEPCHAR) != sFilmFilePath.length()-1 )	//检测dest目录的末尾是否带分隔符 "/" 或"\"，如果不带则给它补上。
		sFilmFilePath.append( 1, DIR_SEPCHAR );

	string::size_type pos;
	if ( ( pos = pklUuid.find("urn:uuid:") ) != string::npos && pos==0 )
	{
		sFilmFilePath = sFilmFilePath + pklUuid.substr(9);
	}
	else
	{
		sFilmFilePath = sFilmFilePath + pklUuid;
	}
	sFilmFilePath.append( 1, DIR_SEPCHAR );

	if ( sFilmFilePath.length()==0 )
	{
		error = "Error:FilmFilePath is NULL!\n";
		//errno = -2;
		WELOG(error)
		errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
		return CMTT2(errno);
	}

	string  Dest = sFilmFilePath;	//最终的Dest目录为"dest目录" + '/' + "pklUuid" + "/"

	ret = MakeDirectory(Dest);
	
	//@author zhangmiao@oristartech.com
	//@date [2014-09-02]
	//@brief 主要增加导入影片支持双目标目录的处理代码
	//@new
	string Dest2;
	if ( sDest_Path_2.length() != 0 )
	{
		if(ret!=0)
		{
			error = "Error:Make Directory fail.\n";
			WELOG(error);
			ret = ERROR_CONTENT_DCP_MAKE_DIR_FAIL;
			return CMTT2(ret);
		}

		string sFTP_URL, sCreatedDir;
		string sFilmFilePath2 = sDest_Path_2;    //string sFilmFilePath2 = dest2;
		if( sFilmFilePath2.rfind( '/' ) != sFilmFilePath2.length()-1 )	//检测dest2目录的末尾是否带分隔符 "/" ，如果不带则给它补上。
			sFilmFilePath2.append( 1 ,  '/' );
		
		sFTP_URL = sFilmFilePath2;

		string::size_type pos;
		if ( ( pos = pklUuid.find("urn:uuid:") ) != string::npos && pos==0 )
		{
			sFilmFilePath2 = sFilmFilePath2 + pklUuid.substr(9);
			sCreatedDir = pklUuid.substr(9);
		}
		else
		{
			sFilmFilePath2 = sFilmFilePath2 + pklUuid;
			sCreatedDir = pklUuid;
		}
		sFilmFilePath2.append( 1,  '/' );
		sCreatedDir.append( 1,  '/' );

		if ( sFilmFilePath2.length()==0 )
		{
			error = "Error:FilmFilePath2 is NULL!\n";
			WELOG(error);
			errno = ERROR_CONTENT_DCP_FILM_FILE_PATH_ISNULL;
			return CMTT2(errno);
		}

		Dest2 = sFilmFilePath2;	//最终的Dest2目录为"dest2目录" + '/' + "pklUuid" + "/"
		//ret = MakeDirectory(Dest2);
		ret = Ftp_MakeDirectory( sFTP_URL , sCreatedDir );
		if ( ret != 0 )
		{
			ret = 0;
			bIsFtp_UploadFile_Failed = 1;
			g_bIsFtp_UploadFile_Failed = 1;
		}
	}
	//@modify [2014-09-02] end;

	if( ret != 0 )
	{
		error = "Error:Make Directory fail.\n";
		WELOG(error);
		ret = ERROR_CONTENT_DCP_MAKE_DIR_FAIL;
		return CMTT2(ret);
	}
	else if ( ret==0 )   //dcp copy files
	{
		aAMInfo_Vt = dcp.vAMInfo_t;
		vector< AMInfo >::size_type nAMInfoCount = aAMInfo_Vt.size();
		vector< AMInfo >::size_type i = 0;
		for ( i=0; i< nAMInfoCount; i++ )
		{
			AMInfo_t aAMInfo = aAMInfo_Vt[i];
			vector<asset_assetmap_t> aAssetVt = aAMInfo.aAssetmap_Info.asset;
			vector< dcp_Info_t > vDcp = aAMInfo.vDcp;
			vector< dcp_Info_t >::size_type pklN = vDcp.size();
			vector< dcp_Info_t >::size_type j = 0;
			for( j=0; j<pklN; j++ )
			{
				string pklName;
				//查找pklName文件路径名
				vector<asset_assetmap_t>::size_type nAssetVtCount = aAssetVt.size();
				vector<asset_assetmap_t>::size_type k = 0;
				for ( k=0; k< nAssetVtCount; k++ )
				{
					if ( pklUuid == aAssetVt[k].uuid )
					{
						pklName =  aAssetVt[k].path;
						if (pklName.length()==0)	//如果名字为空，做错误处理
						{
							char buffer[BUF_SIZE]="";
							sprintf( buffer,"pklName %s Error!\n", pklName.c_str() );
							error = buffer;
#ifdef _TEST_
							printf("pklName %s Error!\n", pklName.c_str());
#endif
							
							//删除垃圾文件夹，Remove DCP Directory
							int nResult = 0 , nResult2 = 0;
							nResult = RemoveDir( Dest );
							if( nResult != 0 )
							{
#ifdef _TEST_
								printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif
								error += "Warning:Remove DCP Directory fail.\n";
							}
							
							//@author zhangmiao@oristartech.com
							//@date [2014-09-02]
							//@brief 主要增加导入影片支持双目标目录的处理代码
							//@new
							if ( Dest2.length() != 0 && ( g_bIsFtp_UploadFile_Failed == 0 ) )
							{
								//nResult2 = RemoveDir( Dest2 );
								nResult2 = FtpRemoveDir( Dest2 );
								if( nResult2 != 0 )
								{
#ifdef _TEST_
									printf( "Warning:Remove DCP Dest2 Directory Fail.:%s\n", Dest2.c_str() );
#endif
									error += "Warning:Remove DCP Dest2 Directory Fail.\n";
								}
							}
							//@modify [2014-09-02] end;

							WELOG(error)
							errno = ERROR_CONTENT_DCP_PKL_PKLNAME_ERROR;
							return CMTT2(errno);
						}
#ifdef _TEST_
						printf( "pklName:%s\n", pklName.c_str() );
#endif
						break;
					}
				}

				dcp_Info_t dcpInf = vDcp[j];
				pkl_Info_t aPkl = dcpInf.aPkl;	
				if ( pklUuid == aPkl.uuid )
				{
#ifdef WIN32
					WaitForSingleObject(hMutex,INFINITE);
#else
					pthread_mutex_lock(&work_mutex);
#endif
					g_sCopyDcp_PklUuidName = pklUuid;
					g_Dcp_AllFile_Size = aPkl.allFileLength;
					g_hasDcpCopied_Size = 0ull;

					bIsFoundPKL = true;
					g_map_FileCopyInfo.clear();

					//@author zhangmiao@oristartech.com
					//@date [2012-12-25]
					//@new
					//初始化 g_map_FileCopyInfo 映射结构，返回pkl的所有文件状态。
					vector<asset_pkl_t> vAsset_pkl = aPkl.asset;
					vector<asset_pkl_t>::size_type nSize_Asset_Pkl = vAsset_pkl.size();
					if (nSize_Asset_Pkl==0)
					{
						error = "Error:PKL AssetList is null!\n";
						errno = ERROR_CONTENT_DCP_PKL_ASSELIST_IS_NULL;
						WELOG(error)
						return errno;
					}
					vector<asset_pkl_t>::size_type m=0;
					for ( m=0; m< nSize_Asset_Pkl; m++ )
					{
						asset_pkl_t aAsset_Pkl = vAsset_pkl[m];
						string fileName = aAsset_Pkl.originalFileName;
						if ( fileName.length() == 0 )
						{
							error = "Error:Pkl AssetFile Path Is NULL!\n";
							errno = ERROR_CONTENT_DCP_PKL_ASSEFILE_PATH_IS_NULL;
							WELOG(error)
							return errno;
						}
					
						FileCopyInfo aCopyInfo;
						//@author zhangmiao@oristartech.com
						//@date [2013-07-15]
						//主要增加了相关字幕文件的处理代码
						//@new
						//aCopyInfo.filename = Dest + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);
						aCopyInfo.filename =  fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);
						//@modify end;
						aCopyInfo.fileSize = static_cast<int>(aAsset_Pkl.size/ONEMB);
						aCopyInfo.transferredSize = 0;
						//@author zhangmiao@oristartech.com
						//@date [2013-07-15]
						//主要增加了相关字幕文件的处理代码
						//@new
						//g_map_FileCopyInfo[Dest + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1)] = aCopyInfo;
						g_map_FileCopyInfo[ fileName ] = aCopyInfo;
						//@modify end;
					}
					//@modify zhangmiao [2012-12-25] end;

#ifdef WIN32
					ReleaseMutex(hMutex);
#else
					pthread_mutex_unlock(&work_mutex);
#endif
					//Copy PKL AssetList File
					//@author zhangmiao@oristartech.com
					//@date [2013-07-15]
					//主要增加了相关字幕文件的处理代码
					//@new
					//ret = CopyDcpSub1( Dest, aPkl ,error );
					ret = CopyDcpSub1( source , Dest , aPkl , error , Dest2 , nLimitedSpeed );
					//@modify end;

					//@author zhangmiao@oristartech.com
					//@date [2015-01-09]
					//如果ftp上传失败，目前继续本地拷贝，不再进行ftp上传，设置出错标志位
					//if ( ret != 0 )
					if ( ( ret != 0 ) &&( ret == ERROR_CONTENT_DCP_FILE_COPY_SLAVE_FAILED ) )
					{
						if ( Dest2.length() != 0 )
						{
							char buffer[BUF_SIZE]="";
							sprintf( buffer,"Error:CopyDcpSub1()--File Copy SLAVE Dest Path2 Failed.:%s\n" , Dest2.c_str() );
							error += buffer;
							printf( "%s\n"  , buffer );
						}

						bIsFtp_UploadFile_Failed = 1;
						printf( "CopyDcpSub1():bIsFtp_UploadFile_Failed = %d\n" , bIsFtp_UploadFile_Failed );
					}
					else if ( ret != 0 )
					//@modify [2015-01-09] end
					{
						//删除垃圾文件夹，Remove DCP Directory
						int nResult = 0 , nResult2 = 0;
						nResult = RemoveDir( Dest );
						if( nResult != 0 )
						{
#ifdef _TEST_
							printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif
							error += "Warning:Remove DCP Directory fail.\n";
						}

						//@author zhangmiao@oristartech.com
						//@date [2014-09-02]
						//@brief 主要增加导入影片支持双目标目录的处理代码
						//@new
						if ( Dest2.length() != 0 && ( g_bIsFtp_UploadFile_Failed == 0 ) )
						{
							//nResult2 = RemoveDir( Dest2 );
							nResult2 = FtpRemoveDir( Dest2 );
							if( nResult2 != 0 )
							{
#ifdef _TEST_
								printf( "Warning:Remove DCP Dest2 Directory Fail.:%s\n", Dest2.c_str() );
#endif
								error += "Warning:Remove DCP Dest2 Directory Fail.\n";
							}
						}
						//@modify [2014-09-02] end;

						return CMTT2(ret);
					}

					//Copy PKL File
					ret = CopyDcpSub2( pklName, Dest ,error , Dest2 );

					//@author zhangmiao@oristartech.com
					//@date [2015-01-09]
					//如果ftp上传失败，目前继续本地拷贝，不再进行ftp上传，设置出错标志位
					//if ( ret != 0 )
					if ( ( ret != 0 ) &&( ret == ERROR_CONTENT_DCP_FILE_COPY_SLAVE_FAILED ) )
					{
						if ( Dest2.length() != 0 )
						{
							char buffer[BUF_SIZE]="";
							sprintf( buffer,"Error:CopyDcpSub2()--PKL File Copy SLAVE Dest Path2 Failed.:%s\n" , pklName.c_str() );
							error += buffer;
							printf( "%s\n"  , buffer );
						}

						bIsFtp_UploadFile_Failed = 1;
						printf( "CopyDcpSub2():bIsFtp_UploadFile_Failed = %d\n" , bIsFtp_UploadFile_Failed );
					}
					else if ( ret != 0 )
                    //@modify [2015-01-09] end
					{
						//删除垃圾文件夹，Remove DCP Directory
						int nResult = 0 , nResult2 = 0;
						nResult = RemoveDir( Dest );
						if( nResult != 0 )
						{
#ifdef _TEST_
							printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif
							error += "Warning:Remove DCP Directory fail.\n";
						}

						//@author zhangmiao@oristartech.com
						//@date [2014-09-02]
						//@brief 主要增加导入影片支持Ftp目标目录的处理代码
						//@new
						if ( Dest2.length() != 0 && ( g_bIsFtp_UploadFile_Failed == 0 ) )
						{
							//nResult2 = RemoveDir( Dest2 );
							 nResult2 = FtpRemoveDir( Dest2 );
							if( nResult2 != 0 )
							{
#ifdef _TEST_
								printf( "Warning:Remove DCP Dest2 Directory Fail.:%s\n", Dest2.c_str() );
#endif
								error += "Warning:Remove DCP Dest2 Directory Fail.\n";
							}
						}
						//@modify [2014-09-02] end;

						return CMTT2(ret);
					}

					//Copy AssetMap File
					string AssetmapName = aAMInfo.assetmapName;
					ret = CopyDcpSub3( AssetmapName, pklUuid, Dest, dcp, aPkl ,error , Dest2 );

					//@author zhangmiao@oristartech.com
					//@date [2015-01-09]
					//如果ftp上传失败，目前继续本地拷贝，不再进行ftp上传，设置出错标志位
					//if ( ret != 0 )
					if ( ( ret != 0 ) &&( ret == ERROR_CONTENT_DCP_FILE_COPY_SLAVE_FAILED ) )
					{
						if ( Dest2.length() != 0 )
						{
							char buffer[BUF_SIZE]="";
							sprintf( buffer,"Error:CopyDcpSub3()--AssetMap File Copy SLAVE Dest Path2 Failed.:%s\n" , AssetmapName.c_str() );
							error += buffer;
							printf( "%s\n"  , buffer );
						}

						bIsFtp_UploadFile_Failed = 1;
						printf( "CopyDcpSub3():bIsFtp_UploadFile_Failed = %d\n" , bIsFtp_UploadFile_Failed );
					}
					else if ( ret != 0 )
					//@modify [2015-01-09] end
					{
						int nResult = 0 , nResult2 = 0;
						nResult = RemoveDir( Dest );
						if( nResult != 0 )
						{
#ifdef _TEST_
							printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif
							error += "Warning:Remove DCP Directory fail.\n";
						}

						//@author zhangmiao@oristartech.com
						//@date [2014-09-02]
						//@brief 主要增加导入影片支持Ftp目标目录的处理代码
						//@new
						if ( Dest2.length() != 0 && ( g_bIsFtp_UploadFile_Failed == 0 ) )
						{
							//nResult2 = RemoveDir( Dest2 );
							nResult2 = FtpRemoveDir( Dest2 );
							if( nResult2 != 0 )
							{
#ifdef _TEST_
								printf( "Warning:Remove DCP Dest2 Directory Fail.:%s\n", Dest2.c_str() );
#endif
								error += "Warning:Remove DCP Dest2 Directory Fail.\n";
							}
						}
						//@modify [2014-09-02] end;

						return CMTT2(ret);
					}
					
					if ( bIsFoundPKL && (errorOccurred==false) )
					{
						errno = 0;

#ifdef WIN32
						WaitForSingleObject(hMutex,INFINITE);
#else
						pthread_mutex_lock(&work_mutex);
#endif

						g_sCopyDcp_PklUuidName="";
						g_sCopyDcp_Copying_FileName="";

#ifdef WIN32
						ReleaseMutex(hMutex);
#else
						pthread_mutex_unlock(&work_mutex);
#endif

						//@author zhangmiao@oristartech.com
						//@date [2015-01-09]
						//如果ftp上传失败，目前继续本地拷贝，不再进行ftp上传，返回错误编码 
						//@new
						if ( bIsFtp_UploadFile_Failed == 1 && ( g_bIsFtp_UploadFile_Failed != 0 ) )
						{
							if ( Dest2.length() != 0 )
							{
								char buffer[BUF_SIZE]="";
								sprintf( buffer,"Error:CopyDcp()---File Copy SLAVE Dest Path2 Failed.:%s\n" , Dest2.c_str() );
								error += buffer;
								printf( "%s\n"  , buffer );
								WELOG(error);
							}

							g_bIsFtp_UploadFile_Failed = 0;     //复位g_bIsFtp_UploadFile_Failed
							errno = ERROR_CONTENT_DCP_FILE_COPY_SLAVE_FAILED;
							return CMTT2(errno);	
						}
						//@modify [2015-01-09] end

						return 0;
					}
				}
			}
		}
		//Do not found pkl 没有找到PKL时，做必要的特殊处理
		if (bIsFoundPKL==false)
		{
			//删除垃圾文件夹，Remove DCP Directory
			ret = RemoveDir(Dest);
			if( ret != 0 )
			{
#ifdef _TEST_
                printf( "Warning:Remove DCP Directory Fail.:%s\n", Dest.c_str() );
#endif
				error += "Warning:Remove DCP Directory Fail.\n";
			}

			//@author zhangmiao@oristartech.com
			//@date [2014-09-02]
			//@brief 主要增加导入影片支持Ftp目标目录的处理代码
			//@new
			int nResult = 0;
			if ( Dest2.length() != 0  && ( g_bIsFtp_UploadFile_Failed == 0 ) )
			{
				//nResult = RemoveDir( Dest2 );
				nResult = FtpRemoveDir( Dest2 );
				if( nResult != 0 )
				{
#ifdef _TEST_
					printf( "Warning:Remove DCP Dest2 Directory Fail.:%s\n", Dest2.c_str() );
#endif
					error += "Warning:Remove DCP Dest2 Directory Fail.\n";
				}
			}

			g_bIsFtp_UploadFile_Failed = 0;     //复位g_bIsFtp_UploadFile_Failed
			//@modify [2014-09-02] end;


#ifdef WIN32
			WaitForSingleObject(hMutex,INFINITE);
#else
			pthread_mutex_lock(&work_mutex);
#endif

			g_sCopyDcp_PklUuidName="";
			g_sCopyDcp_Copying_FileName="";
			g_hasDcpCopied_Size = g_Dcp_AllFile_Size = 0ull;

#ifdef WIN32
			ReleaseMutex(hMutex);
#else
			pthread_mutex_unlock(&work_mutex);
#endif
			error = "Do not found pkl\n";
			//return -1;
			WELOG(error)
			errno = ERROR_CONTENT_DCP_DONOT_FOUND_PKL;
			return CMTT2(errno);
		}
	}
	return 0;
}
#endif // #if DCPCPY == 1


#if DCPCPY == 1
//Copy PKL AssetList File
//@author zhangmiao@oristartech.com
//@date [2013-07-15]
//主要增加了相关字幕文件的处理代码
//@new
//int CopyDcpSub1( const std::string &Dest, pkl_Info_t& aPkl, std::string& error )
int CopyDcpSub1( const std::string &source, const std::string &Dest, pkl_Info_t& aPkl, std::string& error , const std::string Dest2 , int nLimitedSpeed )
//@modify end;
{
	int ret(0);
	vector<asset_pkl_t> vAsset_pkl = aPkl.asset;
	vector<asset_pkl_t>::size_type nSize_Asset_Pkl = vAsset_pkl.size();
	if (nSize_Asset_Pkl==0)
	{
		error = "Error:PKL AssetList is null!\n";
		//errno = 1700;
		errno = ERROR_CONTENT_DCP_PKL_ASSELIST_IS_NULL;
		WELOG(error)
		ret = errno;
	}
	
	for ( vector<asset_pkl_t>::size_type k=0; k< nSize_Asset_Pkl; k++ )
	{
		asset_pkl_t aAsset_Pkl = vAsset_pkl[k];
		//string hashValue = aAsset_Pkl.hash;
		string fileName = aAsset_Pkl.originalFileName;
		if ( fileName.length()==0 )
		{
			error = "Error:Pkl AssetFile Path Is NULL!\n";
			//errno = -2;
			errno = ERROR_CONTENT_DCP_PKL_ASSEFILE_PATH_IS_NULL;
			WELOG(error)
			return errno;
		}
#ifdef WIN32
		WaitForSingleObject(hMutex,INFINITE);
#else
		pthread_mutex_lock(&work_mutex);
#endif
		g_sCopyDcp_Copying_FileName = fileName;

#ifdef WIN32
		ReleaseMutex(hMutex);
#else
		pthread_mutex_unlock(&work_mutex);
#endif

		if( ( ret = access( fileName.c_str() , F_OK ) ) == 0 )     //check file exist
		{
#ifdef _TEST_
			printf( "File  exists :%s\n", fileName.c_str() );
#endif
			//printf( "Delete file :%s\n", fileName.c_str() );
			//ret = remove( fileName.c_str() );

			//@author zhangmiao@oristartech.com
			//@date [2013-07-15]
			//主要增加了相关字幕文件的处理代码
			//@new
/*
#ifdef WIN32
			WaitForSingleObject(hMutex,INFINITE);
#else
			pthread_mutex_lock(&work_mutex);
#endif
			FileCopyInfo aCopyInfo;
			aCopyInfo.filename = Dest + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);
			aCopyInfo.fileSize = static_cast<int>(aAsset_Pkl.size/ONEMB);
			aCopyInfo.transferredSize = 0;
			g_map_FileCopyInfo[Dest + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1)] = aCopyInfo;
#ifdef WIN32
			ReleaseMutex(hMutex);
#else
			pthread_mutex_unlock(&work_mutex);
#endif			
*/
			//@modify end;

			string srcFileName = fileName;
			
			//@author zhangmiao@oristartech.com
			//@date [2013-07-15]
			//@new
			//主要增加了相关字幕文件的处理代码
			//string destFileName = Dest + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);

			//如果目标目录包含子目录则，则建立相应的目录。
			string tmpPath,SubPath;
			string FtpSubPath2;        //表示输出Ftp目标目录2，字幕文件的子目录，不带分隔符 '/'或'\'
			std::string::size_type pos1 = fileName.rfind( source );
			if ( pos1 != string::npos )
			{
				tmpPath = fileName.substr( pos1 + source.length() );
			}
			std::string::size_type pos2 = tmpPath.rfind(DIR_SEPCHAR);
			if ( pos2 != string::npos )
			{
				SubPath = tmpPath.substr( 0, pos2+1 );
				FtpSubPath2 =  tmpPath.substr( 0 , pos2 );    //字幕文件的子目录，不带分隔符 '/'或'\'
			}
			
			string DestFile_InPath = Dest + SubPath;
			ret = MakeDirectory2(DestFile_InPath);		//建立目录，如果目标目录存在则不删除。
			if(ret!=0)
			{
				error = "Error:CopyDcpSub1--Make Director2 fail.\n";
				WELOG(error)
				ret = ERROR_CONTENT_DCP_MAKE_DIR_FAIL;
				return ret;
			}

			string destFileName = DestFile_InPath + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);
			//@modify end;


			//@author zhangmiao@oristartech.com
			//@date [2014-09-03]
			//@brief 主要增加导入影片支持双目标目录的处理代码
			//@new
			//ret = cp_file_Mode2( srcFileName.c_str(), destFileName.c_str() );
			string DestFileName2;
			if ( Dest2.length() != 0 && ( g_bIsFtp_UploadFile_Failed == 0 ) )
			{
				if( FtpSubPath2.rfind( '/' ) != FtpSubPath2.length()-1 )	//检测目录的末尾是否带分隔符 "/"，如果不带则给它补上。
					FtpSubPath2.append( 1 , '/' );

				string DestFile2_InPath = Dest2 + FtpSubPath2;
				//ret = MakeDirectory2( DestFile2_InPath );	
				ret = Ftp_MakeDirectory2( Dest2 , FtpSubPath2 );	//建立目录，如果目标目录存在则不删除。
				if( ret != 0 )
				{
					error = "Error:CopyDcpSub1--Ftp Make Directory2 fail.\n";
					WELOG(error);
					ret = ERROR_CONTENT_DCP_MAKE_DIR_FAIL;
					//return ret;

					g_bIsFtp_UploadFile_Failed = 1;
				}

				DestFileName2 = DestFile2_InPath + fileName.substr(fileName.rfind(DIR_SEPCHAR)+1);
			}
			
			if ( DestFileName2.length() == 0 )
			{
				ret = cp_file_Mode2( srcFileName.c_str(), destFileName.c_str() , NULL , nLimitedSpeed );
			}
			else
			{
				ret = cp_file_Mode2( srcFileName.c_str(), destFileName.c_str() , DestFileName2.c_str() , nLimitedSpeed );
			}
			//@modify [2014-09-03] end;
			
			if (ret==0)
			{
				//----zhangmiao:begin--20121226----
				//更新获取进度时的状态信息，copy file 成功时.
#ifdef WIN32
				WaitForSingleObject(hMutex,INFINITE);
#else
				pthread_mutex_lock(&work_mutex);
#endif

				//@author zhangmiao@oristartech.com
				//@date [2013-07-15]
				//主要增加了相关字幕文件的处理代码
				//@new
				//FileCopyInfo& copyInfo   = g_map_FileCopyInfo[destFileName]; 
				FileCopyInfo& copyInfo   = g_map_FileCopyInfo[srcFileName]; 
				//@modify end;
				copyInfo.bIsFinished = 1;

#ifdef WIN32
				ReleaseMutex(hMutex);
#else
				pthread_mutex_unlock(&work_mutex);
#endif
				//----zhangmiao:end----20121226----

#ifdef _TEST_
				printf( "File copy finished :%s\n", destFileName.c_str() );
#endif
			}
			else 
			{
				//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 begin
				//bIsFtp_UploadFile_Failed：表示是否ftp上传操作在某个地方出现过失败
				if ( ret == CODE_DCP_COPY_SLAVE_FAILED )
				{
					char buffer[BUF_SIZE]="";
					sprintf( buffer,"%s:%s\n" , fileName.c_str() , "File copy SLAVE Dest Path2 Failed" );
					error += buffer;

					printf( "%s:%s\n" , fileName.c_str() , "File copy SLAVE Dest Path2 Failed" );

					//bIsFtp_UploadFile_Failed = 1;
					g_bIsFtp_UploadFile_Failed = 1;

					continue;
				}
				//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 end

#ifdef WIN32
				WaitForSingleObject(hMutex,INFINITE);
#else
				pthread_mutex_lock(&work_mutex);
#endif

				g_sCopyDcp_PklUuidName="";
				g_Dcp_AllFile_Size = 0ull;

				if ( ret == CODE_DCPCOPYCANCEL )
				{
					g_flag_DcpCopyStop = 0;

					char buffer[BUF_SIZE]="";
					sprintf( buffer,"%s:%s\n",fileName.c_str(),"DCP Copy CANCEL!" );
					error = buffer;
#ifdef _TEST_
					printf("%s:%s\n",fileName.c_str(),"DCP Copy CANCEL!");
#endif

					errno = ERROR_CONTENT_DCP_DCPCOPYCANCEL;
				}
				else
				{
#ifdef _TEST_
					printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif

					errno = ERROR_CONTENT_DCP_FILE_COPY_FAILED;
				}

#ifdef WIN32
				ReleaseMutex(hMutex);
#else
				pthread_mutex_unlock(&work_mutex);
#endif
				
				//errno = 1;
				WELOG(error)
				return errno;
			}
		}
		else//file NOT exist
		{
			char buffer[BUF_SIZE]="";
			sprintf( buffer,"%s:%s\n",fileName.c_str(),strerror(errno) );
			error = buffer;
#ifdef _TEST_
			printf("%s:%s\n",fileName.c_str(),strerror(errno));
#endif

#ifdef WIN32
			WaitForSingleObject(hMutex,INFINITE);
#else
			pthread_mutex_lock(&work_mutex);
#endif

			g_sCopyDcp_PklUuidName="";
			g_Dcp_AllFile_Size = 0ull;

#ifdef WIN32
			ReleaseMutex(hMutex);
#else
			pthread_mutex_unlock(&work_mutex);
#endif
			errno = ERROR_CONTENT_DCP_FILE_NOT_EXIST;
			WELOG(error)
			return errno;
		}
	}


	//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 begin
	if( g_bIsFtp_UploadFile_Failed == 1 )
	{
		printf( "CopyDcpSub1():%s\n" , "File copy SLAVE Dest Path2 Failed!" );
		errno = ERROR_CONTENT_DCP_FILE_COPY_SLAVE_FAILED;
		return errno;
	}
	//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 end

	return ret;
}
#endif //#if DCPCPY == 1

#if DCPCPY == 1
//Copy PKL File
int CopyDcpSub2( const std::string &pklName, const std::string &Dest, std::string& error , const std::string Dest2 )
{
	int ret(0);
	if (pklName.length())
	{
		if( ( ret = access( pklName.c_str() , F_OK ) ) == 0 )     //check file exist
		{
			string destFileName = Dest + pklName.substr(pklName.rfind(DIR_SEPCHAR)+1);
			ret = cp_file( pklName.c_str(), destFileName.c_str() );
			if (ret==0)
			{
#ifdef _TEST_
				printf( "PKL File copy finished :%s\n", destFileName.c_str() );
#endif
			}
			else if( ret )
			{
				char buffer[BUF_SIZE]="";
				sprintf( buffer,"PKL File copy Failed :%s\n", destFileName.c_str() );
				error += buffer;
#ifdef _TEST_
				printf( "PKL File copy Failed :%s\n", destFileName.c_str() );
#endif
				//errno = 1;
				errno = ERROR_CONTENT_DCP_PKL_FILE_COPY_FAILED;
				WELOG(error)
				return errno;
			}

			//@author zhangmiao@oristartech.com
			//@date [2014-09-03]
			//@brief 主要增加导入影片支持双目标目录的处理代码
			//@new
			if ( Dest2.length() != 0 && ( g_bIsFtp_UploadFile_Failed == 0 ) )
			{
				cftp::FtpClient ftp;

				string ftpUrl = Dest2;

				string url = ftpUrl;

				string ip, userName, passwd, dir;
				string port = "21";

				//log in
				ret = ftp.LogIn( ftpUrl );
				if( ret == cftp::NOANYERROR )
				{
					ret = ftp.ParseUrl( url , ip , userName , passwd , port , dir );
					if( ret == cftp::NOANYERROR )
					{
						string fileName = pklName.substr(pklName.rfind(DIR_SEPCHAR)+1);
						ret = ftp.UploadFile( pklName , dir , fileName );
					}
				}

				/*ret = ftp.ParseUrl( url , ip , userName , passwd , port , dir );
				if( ret != cftp::NOANYERROR )
					return ret;

				string fileName = pklName.substr(pklName.rfind(DIR_SEPCHAR)+1);
				ret = ftp.UploadFile( pklName , dir , fileName );*/

				string destFileName2 = Dest2 + pklName.substr(pklName.rfind(DIR_SEPCHAR)+1);
				//ret = cp_file( pklName.c_str(), destFileName2.c_str() );
				if ( ret == 0 )
				{
	#ifdef _TEST_
					printf( "PKL File2 Upload File finished :%s\n", destFileName2.c_str() );
	#endif
				}
				else if( ret )
				{
					char buffer[BUF_SIZE]="";
					sprintf( buffer,"PKL File2 Upload File Failed :%s\n", destFileName2.c_str() );
					error += buffer;
	#ifdef _TEST_
					printf( "PKL File2 Upload File Failed :%s\n", destFileName2.c_str() );
	#endif
					
					//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 begin
					//errno = ERROR_CONTENT_DCP_PKL_FILE_COPY_FAILED;
					errno = ERROR_CONTENT_DCP_FILE_COPY_SLAVE_FAILED;

					g_bIsFtp_UploadFile_Failed = 1;
					//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 end

					WELOG(error);
					return errno;
				}
			}
			//@modify [2014-09-03] end;
		}
		else
		{
			char buffer[BUF_SIZE]="";
			sprintf( buffer,"Error:PKL File(%s) no exist!\n", pklName.c_str() );
			error = buffer;
#ifdef _TEST_
			printf( "Error:PKL File(%s) no exist!\n", pklName.c_str() );
#endif
			//errno = 10;
			errno = ERROR_CONTENT_DCP_PKL_FILE_NO_EXIST;
			WELOG(error)
			return errno;
		}
	}
	else
	{
		error = "Error:Param pklName is Null string!\n";
#ifdef _TEST_
		printf( "Error:Param pklName is Null string!\n" );
#endif
		//errno = -5;
		errno = ERROR_CONTENT_DCP_PARAM_PKLNAME_IS_NULL_STR;
		WELOG(error)
		return errno;
	}
	return ret;
}
#endif //#if DCPCPY == 1

#if DCPCPY == 1
//Copy AssetMap File 拷贝完后，需要重建AssetMap文件
int CopyDcpSub3( const std::string &AssetmapName, const std::string &pklUuid, const std::string &Dest,
				DcpInfo& dcp, pkl_Info_t& aPkl , std::string& error , const std::string Dest2 )
{
	int ret(0);
	if (AssetmapName.length())
	{
		if( ( ret = access( AssetmapName.c_str() , F_OK ) ) == 0 )
		{
			//@author zhangmiao@oristartech.com
			//@date [2014-09-30]
			//@brief 统一将拷贝后的Assetmap文件更名为固定的名字："ASSETMAP" 
			//@new
			//string destFileName = Dest + AssetmapName.substr(AssetmapName.rfind(DIR_SEPCHAR)+1);
			string destFileName = Dest + "ASSETMAP";
			//@modify end;
			ret = cp_file( AssetmapName.c_str(), destFileName.c_str() );
			if (ret==0)
			{
#ifdef _TEST_
				printf( "Assetmap File1 copy finished :%s\n", destFileName.c_str() );
#endif
			}
			else if( ret )
			{
				char buffer[BUF_SIZE]="";
				sprintf( buffer,"Assetmap File1 copy Failed :%s\n", destFileName.c_str() );
				error += buffer;
#ifdef _TEST_
				printf( "Assetmap File1 copy Failed :%s\n", destFileName.c_str() );
#endif
				//errno = 1;
				errno = ERROR_CONTENT_DCP_FILE_COPY_FAILED;
				WELOG(error)
				return errno;
			}
			//Rebuilt AssetMap File
			ret = dcp.ReBuild_Assetmap( destFileName ,pklUuid ,aPkl , error );
			if (ret!=0)
			{
				char buffer[BUF_SIZE]="";
				sprintf( buffer,"Error ReBuild_Assetmap File FAILED:%s\n", destFileName.c_str() );
				error += buffer;
#ifdef _TEST_
				printf( "Error ReBuild_Assetmap File FAILED:%s\n", destFileName.c_str() );
#endif
				WELOG(error)
				errno = ERROR_CONTENT_DCP_REBUILD_ASSETMAP_FILE_FAILED;
				ret = errno;
				return ret;
			}

			//@author zhangmiao@oristartech.com
			//@date [2014-09-03]
			//@brief 主要增加导入影片支持双目标目录的处理代码
			//@new
			if ( Dest2.length() != 0 && ( g_bIsFtp_UploadFile_Failed == 0 ) )
			{
				string ftpUrl = Dest2;

				string url = ftpUrl;

				string ip, userName, passwd, dir;
				string port = "21";

				cftp::FtpClient ftp;

				//log in
				ret = ftp.LogIn( ftpUrl );
				if( ret == cftp::NOANYERROR )
				{
					ret = ftp.ParseUrl( url , ip , userName , passwd , port , dir );
					if( ret == cftp::NOANYERROR )
					{
						string fileName = AssetmapName.substr(AssetmapName.rfind(DIR_SEPCHAR)+1);
						ret = ftp.UploadFile( destFileName , dir , fileName );
					}
				}
				
				/*ret = ftp.ParseUrl( url , ip , userName , passwd , port , dir );
				if( ret != cftp::NOANYERROR )
					return ret;

				string fileName = AssetmapName.substr(AssetmapName.rfind(DIR_SEPCHAR)+1);
				ret = ftp.UploadFile( destFileName , dir , fileName );*/
				
				string destFileName2 = Dest2 + AssetmapName.substr(AssetmapName.rfind(DIR_SEPCHAR)+1);
				//ret = cp_file( destFileName.c_str(), destFileName2.c_str() );
				if ( ret == 0 )
				{
#ifdef _TEST_
					printf( "Assetmap File2 Upload File finished :%s\n", destFileName2.c_str() );
#endif
				}
				else if( ret )
				{
					char buffer[BUF_SIZE]="";
					sprintf( buffer,"Assetmap File2 Upload File Failed :%s\n", destFileName2.c_str() );
					error += buffer;
#ifdef _TEST_
					printf( "Assetmap File2 Upload File Failed :%s\n", destFileName2.c_str() );
#endif
					
					//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 begin
					//errno = ERROR_CONTENT_DCP_FILE_COPY_FAILED;
					errno = ERROR_CONTENT_DCP_FILE_COPY_SLAVE_FAILED;

					g_bIsFtp_UploadFile_Failed = 1;
					//如果ftp上传失败，目前继续本地拷贝，不再ftp上传 end

					WELOG(error);
					return errno;
				}
			}
			//@modify [2014-09-03] end;
		}
		else
		{
			char buffer[BUF_SIZE]="";
			sprintf( buffer,"Error:Assetmap(%s) is no exist!\n", AssetmapName.c_str() );
			error = buffer;
#ifdef _TEST_
			printf( "Error:Assetmap(%s) is no exist!\n", AssetmapName.c_str() );
#endif
			//errno = 2020;
			errno = ERROR_CONTENT_DCP_ASSETMAP_IS_NO_EXIST;
			WELOG(error)
			ret = errno;
		}
	}
	else
	{
		error = "Error:Param AssetmapName is Null string!\n";
#ifdef _TEST_
		printf( "Error:Param AssetmapName is Null string!\n" );
#endif
		//errno = 2022;
		errno = ERROR_CONTENT_DCP_PARAM_ASSETMAPNAME_IS_NULL_STR;
		WELOG(error)
		ret = errno;
		return ret;
	}
	return ret;
}
#endif // #if DCPCPY == 1


#if DCPCPY == 1
//FTP导入DCP
int DownLoadDcp( const string &ftpUrl, const string &pklUuid, const string &dest, vector<FileCopyInfo> &copyInfo, string& error )
{
	int ret(0);
	bool bIsBPTrans = false;
	if( copyInfo.size()!=0 )
	{
		bIsBPTrans = true;
	}
	else
	{
		//@author zhangmiao@oristartech.com
		//@date 2013-02-27
		//@brief 如果参数copyInfo为空则程序自动填充cpl、mxf文件名。:begin
		//@new
		//先搜索Search Ftp DcpList。
		vector<AssetmapInfo> dcpList;
		ret = SearchFtpDcpList( ftpUrl, dcpList ,error );
		if(ret!=0)
		{
#ifdef _TEST_
			printf("error SearchFtpDcpList !\n");
#endif
			WELOG("error SearchFtpDcpList !\n");
			return ret;
		}

		int nAMInfoCount = dcpList.size();
		for ( int i=0; i< nAMInfoCount; i++ )
		{
			AssetmapInfo aAMInfo = dcpList[i];
			vector< PklInfo > vDcp = aAMInfo.pklList;
			int pklN = vDcp.size();
			for( int j=0; j<pklN; j++ )
			{
				//string pklName;

				PklInfo aPkl = vDcp[j];	
				if ( pklUuid == aPkl.pklUuid )
				{
					//pklName = vDcp[j].pklName;
					//填充cpl文件信息代码
					vector<CplInfo> vAsset_pkl = aPkl.cplList;
					int nSize_Asset_Pkl = vAsset_pkl.size();
					if (nSize_Asset_Pkl==0)
					{
						error = "Error:PKL AssetList is null!\n";
						errno = ERROR_CONTENT_DCP_PKL_ASSELIST_IS_NULL;
						WELOG(error);
						return errno;
					}
					for ( int k=0; k< nSize_Asset_Pkl; k++ )
					{
						CplInfo aAsset_Pkl = vAsset_pkl[k];
						string fileName = aAsset_Pkl.cplName;
						if ( fileName.length() == 0 )
						{
							error = "Error:Pkl AssetFile Path Is NULL!\n";
							errno = ERROR_CONTENT_DCP_PKL_ASSEFILE_PATH_IS_NULL;
							WELOG(error);
							return errno;
						}

						FileCopyInfo aCopyInfo;
						aCopyInfo.filename = fileName.substr(fileName.rfind('/')+1);
						aCopyInfo.bIsFinished = 0;
						copyInfo.push_back(aCopyInfo);
					}

					//填充mxf文件信息代码
					vector<MxfInfo> vMxfInfo_pkl = aPkl.mxfList;
					nSize_Asset_Pkl = vMxfInfo_pkl.size();
					if (nSize_Asset_Pkl==0)
					{
						error = "Error:PKL AssetList is null!\n";
						errno = ERROR_CONTENT_DCP_PKL_ASSELIST_IS_NULL;
						WELOG(error);
						return errno;
					}
					for ( int k=0; k< nSize_Asset_Pkl; k++ )
					{
						MxfInfo aAsset_Pkl = vMxfInfo_pkl[k];
						string fileName = aAsset_Pkl.fileName;
						if ( fileName.length() == 0 )
						{
							error = "Error:Pkl AssetFile mxf File Path Is NULL!\n";
							errno = ERROR_CONTENT_DCP_PKL_ASSEFILE_PATH_IS_NULL;
							WELOG(error);
							return errno;
						}
						FileCopyInfo aCopyInfo;
						aCopyInfo.filename = fileName.substr(fileName.rfind('/')+1);
						aCopyInfo.bIsFinished = 0;
						copyInfo.push_back(aCopyInfo);
					}

					//@author zhangmiao@oristartech.com
					//@date [2013-07-18]
					//@brief 主要增加了FTP下载相关字幕文件的处理代码
					//@new
					//填充其它文件信息代码
					//主要是字幕相关的文件，但字幕文件可以没有
					vector<OtherFileInfo>& vFileInfoList_pkl = aPkl.fileInfoList;
					nSize_Asset_Pkl = vFileInfoList_pkl.size();
					if (nSize_Asset_Pkl==0)
					{
						error = "Subtitle files list is null!\n";
						printf( "Warning:%s\n" , error.c_str() );
					}

					vector<string> vTmp;

					for ( int k=0; k< nSize_Asset_Pkl; k++ )
					{
						OtherFileInfo aAsset_Pkl = vFileInfoList_pkl[k];
						string fileName = aAsset_Pkl.fileName;
						if ( fileName.length() == 0 )
						{
							error = "Error:Pkl AssetFile File Path Is NULL!\n";
							errno = ERROR_CONTENT_DCP_PKL_ASSEFILE_PATH_IS_NULL;
							WELOG(error);
							return errno;
						}

						FileCopyInfo aCopyInfo;
						aCopyInfo.filename = fileName.substr(fileName.rfind('/')+1);
						aCopyInfo.bIsFinished = 0;

						int result = count(vTmp.begin(),vTmp.end(), aCopyInfo.filename );
						if( result != 0 ) 
							continue;
						else   
							vTmp.push_back(aCopyInfo.filename);

						copyInfo.push_back(aCopyInfo);
					}
					//@modify end;

				}
			}
		}
		//如果参数copyInfo为空则程序自动填充cpl、mxf文件名。
		//@modify end;

		if( copyInfo.size() != 0 )
		{
			bIsBPTrans = true;
		}
		else
		{
			error = "Param copyInfo is NULL!\n";	
			WELOG(error);	//ret = -1;
			ret = CMTT2(ERROR_CONTENT_DCP_PARAM_COPYINFO_ISNULL);
		}
	}
	if (bIsBPTrans)
	{
		ret = DownLoadDcp_BPTrans( ftpUrl, pklUuid, dest, copyInfo, error );
		if( ret != 0 )
			ret = CMTT2(ret);
		return ret;
	}
	return ret;
}
#endif //#if DCPCPY == 1

#if DCPCPY == 1
//取消FTP导入DCP
int CancelDownLoad( const string &ftpUrl, const string &pklUuid, string& error )
{
#ifdef WIN32
	WaitForSingleObject(hMutex,INFINITE);
#else
	pthread_mutex_lock(&work_mutex);
#endif

	if (g_sCopyDcp_PklUuidName==pklUuid)
	{
		if ( g_flag_DcpCopyStop == 0 )
		{
			g_flag_DcpCopyStop = 1;

			int result = g_FtpClient.CancelTransFile();
			g_FtpClient.LogOut();
			if(result != cftp::NOANYERROR)
			{
#ifdef WIN32
				ReleaseMutex(hMutex);
#else
				pthread_mutex_unlock(&work_mutex);
#endif
				error = "FtpClient::CancelTransFile() Error!\n";
				WELOG(error)
				result = ERROR_CONTENT_FTP_CANCEL_TRANSFILE_ERROR;
				return CMTT2(result);
			}
				
		}
		else if ( g_flag_DcpCopyStop == 1 && errno==CODE_DCPCOPYCANCEL )
		{
			g_flag_DcpCopyStop = 0;
			g_sCopyDcp_PklUuidName="";
		}
	}
	else
	{
		g_sCopyDcp_PklUuidName="";

#ifdef WIN32
		ReleaseMutex(hMutex);
#else
		pthread_mutex_unlock(&work_mutex);
#endif
		error = "PklUuid is error.";
		//return -1;
		WELOG(error)
		return CMTT2(ERROR_CONTENT_DCP_PKL_UUID_ERROR);
	}

#ifdef WIN32
	ReleaseMutex(hMutex);
#else
	pthread_mutex_unlock(&work_mutex);
#endif

	return 0;
}
#endif //#if DCPCPY == 1

#if DCPCPY == 1
//取得FTP导入DCP进度
int GetDownLoadPercent(const string &ftpUrl, const string &pklUuid, CopyInfo &info , std::string& error )
{
#ifdef WIN32
	WaitForSingleObject(hMutex,INFINITE);
#else
	pthread_mutex_lock(&work_mutex);
#endif

	int nPercent = 0;
	if ( g_sCopyDcp_PklUuidName == pklUuid )
	{
		long double  fRate = ((long double)(g_hasDcpCopied_Size)) / ((long double)(g_Dcp_AllFile_Size));
		nPercent = int(fRate*100.0);
		if (nPercent==100)
		{
			info.status = DCP_STATUS_FINISHED;
		}
		else
		{
			info.status = DCP_STATUS_COPING;
		}

		if ( errno != 0 )
		{
			error = "error:DCP DownLoad Percent is DCP_STATUS_ERROR.";
			info.status = DCP_STATUS_ERROR;
			WELOG(error)
		}

		info.percent = nPercent;

		info.dest = "";
		info.source = ftpUrl;

		if (g_map_FileCopyInfo.size())
		{
			info.copyInfo.clear();
			map< std::string , FileCopyInfo >::iterator iFirst = g_map_FileCopyInfo.begin();
			map< std::string , FileCopyInfo >::iterator iLast = g_map_FileCopyInfo.end();
			for (; iFirst != iLast; ++iFirst)
				info.copyInfo.push_back((*iFirst).second);
		}
	}
	else
	{
		info.status = DCP_STATUS_NOSTART;
		info.percent = 0;
		info.source = ftpUrl;
		info.copyInfo.clear();

#ifdef WIN32
		ReleaseMutex(hMutex);
#else
		pthread_mutex_unlock(&work_mutex);
#endif

		error = "error:DCP DownLoad is NO START.";
		//return -1;
		WELOG(error)
		return CMTT2(ERROR_CONTENT_DCP_COPYDOWNLOAD_NO_START);
	}

#ifdef WIN32
	ReleaseMutex(hMutex);
#else
	pthread_mutex_unlock(&work_mutex);
#endif

	return 0;
}
#endif //#if DCPCPY == 1
//获取PKL的资产列表AssetList中，指定文件FileName的大小；FileName是文件名，不带路径；
//本地
//@author zhangmiao@oristartech.com
//@date [2013-07-15]
//主要增加了相关字幕文件的处理代码
//@new
//int GetFileSize_PKLAssetList( const std::string FileName ,const Content::Dcp::pkl_Info_t& aPkl, long long& Length )
int GetFileSize_PKLAssetList( const string FileName ,const pkl_Info_t& aPkl, long long& Length , string& FullPathFileName )
//@modify end;
{
	int ret(0);
	bool IsFound = false;

	string sFileCopyName = FileName;
	if ( sFileCopyName.length()==0 )
	{
		errno = -2;
		return errno;
	}

	vector<asset_pkl_t> vAsset_pkl = aPkl.asset;
	vector<asset_pkl_t>::size_type nSize_Asset_Pkl = vAsset_pkl.size();
	vector<asset_pkl_t>::size_type k = 0;
	for ( k=0; k < nSize_Asset_Pkl; k++ )
	{
		asset_pkl_t aAsset_Pkl = vAsset_pkl[k];
		
		string fileName = aAsset_Pkl.originalFileName;
		if ( fileName.length()==0 )
		{
			errno = -2;
			return errno;
		}

		if ( FileName == fileName.substr(fileName.rfind(DIR_SEPCHAR)+1) )    //判断文件是否找到
		{
			//@author zhangmiao@oristartech.com
			//@date [2013-07-15]
			//主要增加了相关字幕文件的处理代码
			//@new
			FullPathFileName = fileName;
			//@modify end;
			Length = aAsset_Pkl.size;
			if ( Length != 0 )
			{
				IsFound = true;
				break;
			}
		}
	}

	if ( IsFound == false )
	{
		return -1;
	}

	return ret;
}
//获取PKL的资产列表AssetList中，指定文件FileName的大小；FileName是文件名，不带路径；
//ftp DownLoad 时会使用
//@author zhangmiao@oristartech.com
//@date [2013-07-18]
//@brief 主要增加了FTP下载相关字幕文件的处理代码
//@new
//int GetFileSize_PKLAssetList( const std::string FileName ,const PklInfo& aPkl, long long& Length  )
int GetFileSize_PKLAssetList( const std::string FileName ,const PklInfo& aPkl, long long& Length ,std::string& originalfileName )
//@modify end;
{
	int ret(0);
	bool IsFound = false;

	string sFileCopyName = FileName;
	if ( sFileCopyName.length()==0 )
	{
		errno = -2;
		return errno;
	}

	//先从CPL File中找
	vector<CplInfo> vAsset_pkl = aPkl.cplList;
	vector<CplInfo>::size_type nSize_Asset_Pkl = vAsset_pkl.size();
	for ( vector<CplInfo>::size_type k = 0; k < nSize_Asset_Pkl; k++ )
	{
		CplInfo aAsset_Pkl = vAsset_pkl[k];
		string fileName = aAsset_Pkl.cplName;
		if ( fileName.length()==0 )
		{
			errno = -2;
			return errno;
		}

		if ( sFileCopyName == fileName.substr(fileName.rfind('/')+1) )    //判断文件是否找到
		{
			Length = aAsset_Pkl.cplLength;
			if ( Length != 0 )
			{
				IsFound = true;
				return 0;
			}
		}
	}

	//再从 MXF File中找
	vector<MxfInfo> vMxfInfo_pkl = aPkl.mxfList;
	vector<MxfInfo>::size_type nSize_MxfInfo_Asset_Pkl = vMxfInfo_pkl.size();
	for ( vector<MxfInfo>::size_type k = 0; k < nSize_MxfInfo_Asset_Pkl; k++ )
	{
		MxfInfo aAsset_Pkl = vMxfInfo_pkl[k];
		string fileName = aAsset_Pkl.fileName;
		if ( fileName.length()==0 )
		{
			errno = -2;
			return errno;
		}

		if ( sFileCopyName == fileName.substr(fileName.rfind('/')+1) )    //判断文件是否找到
		{
			Length = aAsset_Pkl.length;
			if ( Length != 0 )
			{
				IsFound = true;
				return 0;
			}
		}
	}

	//@author zhangmiao@oristartech.com
	//@date [2013-07-18]
	//@brief 主要增加了FTP下载相关字幕文件的处理代码
	//@new
	//从字幕相关文件中寻找
	vector<OtherFileInfo> vFileInfoList_pkl = aPkl.fileInfoList;
	vector<OtherFileInfo>::size_type nSize_OtherFileInfo_Asset_Pkl = vFileInfoList_pkl.size();
	for ( vector<OtherFileInfo>::size_type k = 0; k < nSize_OtherFileInfo_Asset_Pkl; k++ )
	{
		OtherFileInfo aAsset_Pkl = vFileInfoList_pkl[k];
		string fileName = aAsset_Pkl.fileName;
		if ( fileName.length()==0 )
		{
			errno = -2;
			return errno;
		}

		if ( sFileCopyName == fileName.substr(fileName.rfind('/')+1) )    //判断文件是否找到
		{
			originalfileName = aAsset_Pkl.originalfileName;
			Length = aAsset_Pkl.fileLength;
			if ( Length != 0 )
			{
				IsFound = true;
				return 0;
			}
		}
	}
	//@modify end;

	//如果都没找到，则返回-1
	if ( IsFound == false )
	{
		return -1;
	}

	return ret;
}

#if defined(TMS2_CM_ERRCODE) || defined(WRITE_ELOG) 
#ifndef TMS20_LOG
#include "log/C_LogManage.h"
#endif
#endif


#ifdef TMS2_CM_ERRCODE
//错误码区分：内容管理使用1001以上的值。
int ContentErrorCodeToTmsErrorCode(int error)
{
	const int nCONTENT_ErrorLevel = 3;
	const int nCONTENT_Module = 19;
	const int nCONTENT_SubModule = 0;
	const int nCONTENT_ERROR_Code_Base_Value = 1000;

	if(error == 0)
		return 0;
	else if(error < 0xFFFF)
		return C_LogManage::GetInstance()->CreateLogNumber(nCONTENT_ErrorLevel, nCONTENT_Module, nCONTENT_SubModule, nCONTENT_ERROR_Code_Base_Value + abs(error));
	else
		return error;
}
#endif // TMS2_CM_ERRCODE

#ifdef WRITE_ELOG
int WriteErrorLog( const std::string& strError )
{
	int ret = 0;
	int iLevel=1, iModule=19, iSubModule=0;
	int errorCode = 0;
	ret = C_LogManage::GetInstance()->WriteLog( iLevel,  iModule, iSubModule, errorCode, strError);
	return ret;
}
#endif // WRITE_ELOG
