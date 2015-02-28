/*
*@file:fun.h
*@brief:工具函数库
*@author: zhangmiao@oristartech.com
*@date: 2012/06/05
*/
#ifndef __FUN_H__
#define __FUN_H__

//#undef _FILE_OFFSET_BITS
//#define _FILE_OFFSET_BITS 64
#include <string>
#include <vector>
#include "errdef.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <signal.h>

#include <dirent.h>
#include <time.h>

#ifdef WIN32
#include <io.h>
#define DIR_SEPCHAR '\\'
#define DIR_FMTSTR "%s\\%s"

#include <limits.h>
#define PATH_MAX            512
#define NAME_MAX 255
#include <direct.h>
#else
#define DIR_SEPCHAR '/'
#define DIR_FMTSTR "%s/%s"
#endif

#ifdef WIN32
//#define atoll atol
#define atoll atof
//#define lstat stat
#define lstat _stat64
#define stat  _stat64
#define MKDIR "mkdir  "
#define RMDIR "rmdir /q/s "
#else
#define MKDIR "mkdir -p "
#define RMDIR "rm -rf "
#endif
#ifdef WIN32
#define	F_OK	0
#define	R_OK	4
#define	W_OK	2
#define	X_OK	1
#endif

#ifdef __cplusplus 
extern "C" {
#endif

#define BUF_SIZE 1024
#define BSIZE 8192 

/************************************************* 
*@brief：  搜索文件，按文件名filename
*@param：  pathname ：用于搜索的文件路径，必须以"/"结尾。 :[in]
*@param：  filename ：要搜索的文件名                  :[in]
*@param：  filelist： 返回搜索到地文件名路径，是1个2维数组         【out】
*@param：  max： 返回数组最大文件数         【in】
*@param：  nfile ： 返回搜索到的文件个数。   :[out]
*@Return： 错误码,0 = 成功，-1=错误:PKL文件包没有找到.
*************************************************/
//搜索文件，按文件名filename.
int searchFile(const char *pathname, const char *filename, char **filelist, int *nfile, int max);
//搜索文件，按文件扩展名。如：.xml
int searchFile2(const char *pathname, const char *filename, char **filelist, int *nfile, int max);
//建立目录
int MakeDirectory( const std::string DestDir );
//建立目录，如果目标目录存在则不删除。
int MakeDirectory2( const std::string DestDir );
//删除目录
int RemoveDir( const std::string DestDir );
int PreRemoveDir( const std::string DestDir );

int Remove_PrefixOfUuid( std::string& rUuid );

#ifdef __cplusplus
}
#endif

//@author zhangmiao@oristartech.com
//@date 2014-10-08
//@brief 增加新的时区处理代码，sDestTZ='+08:00'为北京时间，如:sSrcTimeStr="2009-07-04T10:00:00+09:00"，转换为sDestTimeStr="2009-07-04 09:00:00"；
//@new
int ConvertTimeZoneInfo( std::string sSrcTimeStr , std::string sDestTZ , std::string& sDestTimeStr );
//@modify(时区处理)end;

//---zhangmiao:begin---[7/10/2012]----
#include "FtpClient.h"
/*
 * @brief：功能: 遍历ftp服务器上的所有文件（linux），列当前目录
 * @param：  sFTP_URL: 指定的FTP URL                             : [in]
 * @param：  fileList: 返回搜索到的文件的[FileInformation]列表   :【out】
 * @Return：返回值: 错误码,0 = 成功，-1=错误:没有找到.
 */
//list 当前目录
int listFilesOnFTP( std::string sFTP_URL ,std::vector<cftp::FileInformation> &fileList );
/************************************************* 
*@brief：  遍历ftp服务器上指定目录下的所有文件
*@param：  rftp: ftp连接                                        :[in]
*@param：  dir_address: 要遍历的目录                            :[in]
*@param：  filelist： 返回搜索到的文件的[FileInformation]列表   :【out】
*@param：  filename： 要搜索的文件名                            :【in】
*@Return： 错误码,0 = 成功，-1=错误:没有找到.
*************************************************/
//list 所有文件
int listFilesOnFTP( cftp::FtpClient& rftp ,std::string dir_address ,std::vector<cftp::FileInformation> &fileList ,std::string filename="" );
/************************************************* 
*@brief：  根据指定的FTP URL，遍历ftp服务器上指定目录下的所有文件
*@param：  sFTP_URL: 根据指定的FTP URL                           :[in]
*@param：  dir_address: 要遍历的目录                             :[in]
*@param：  filelist： 返回搜索到的文件的[FileInformation]列表    :【out】
*@param：  filename： 要搜索的文件名                             :【in】
*@Return： 错误码,0 = 成功，-1=错误:没有找到.
*************************************************/
//根据指定的FTP URL，list 所有文件
int listFilesOnFTPWithUrl( std::string sFTP_URL ,std::string dir_address ,std::vector<cftp::FileInformation> &fileList , std::string filename="" );
//---zhangmiao:end--[7/10/2012]--

//ftp删除目录，删除所有文件，包括目录本身。
//参数 如："ftp://admin:admin@172.23.140.165/DCP/zhangm/testdcp/"
int FtpRemoveDir( std::string sFTP_URL );
//ftp删除目录，删除所有文件，包括目录本身。只是参数不同
int FtpRemoveDir( cftp::FtpClient& rftp , std::string dir_address );

//FTP 删除目录下的所有文件，包括下面的子文件夹。但是目标目录本身不删除
int FtpRemoveDir_AllFile( cftp::FtpClient& rftp , std::string dir_address );

//建立目录，如果存在目标存在，则先删除该目录。然后，再创建该目标目录。
//参数，如：sFTP_URL="ftp://admin:admin@172.23.140.165/DCP/zhangm/testdcp/"   ,sCreatedDir= "abc/" 
int Ftp_MakeDirectory(  std::string sFTP_URL , std::string sCreatedDir );

//建立目录，如果目标目录存在则不删除，不再创建该目录。
//参数，如：sFTP_URL="ftp://admin:admin@172.23.140.165/DCP/zhangm/testdcp/"   ,sCreatedDir= "abc/" 
int Ftp_MakeDirectory2( std::string sFTP_URL , std::string sCreatedDir );

//建立目录，支持建立多级目录；如果目标目录存在则不删除，不再创建该目录。
//参数，如：sFTP_URL="ftp://admin:admin@172.23.140.165/DCP/zhangm/testdcp/"   ,sCreatedDir= "a1/b2/c3/de4/" 
int Ftp_MultiMakeDir( std::string sFTP_URL , std::string sCreatedDir );
int Ftp_MultiMakeDir2( std::string sFTP_URL , std::string sCreatedDir );

#endif
