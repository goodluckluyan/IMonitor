/*
*@file:fun.cpp
*@brief:工具函数库
*@author: zhangmiao@oristartech.com
*@date: 2012/06/05
*/
#ifndef __FUN_H__
#include "fun.h"
#endif

int searchFile(const char *pathname, const char *filename, char **filelist, int *nfile,int max)
{
    DIR *dir;
    struct dirent *dirt;
    char tmppath[PATH_MAX] = {0}; 
    //char pathname2[PATH_MAX] = {0}; 
    int ret =0;

    if(pathname==NULL || filename==NULL || nfile==NULL){printf("para is NULL!\n"); return -1;}// 
    //printf("%s\n",pathname);
    dir = opendir(pathname);
    if(dir==NULL)
    {
        //if(errno!=ENOENT && *nfile>0) 
        //if(*nfile>0) 
        //    while(--(*nfile)>=0) free(filelist[*nfile]);
        //printf("%s\n",strerror(errno));
        return errno;
    }
    while((dirt=readdir(dir))!=NULL)
    {
        //printf("%s\n",dirt->d_name);
        if(strcmp(dirt->d_name,".")==0 ||strcmp(dirt->d_name,"..")==0)
            continue;
#ifndef _DIRENT_HAVE_D_TYPE
        printf("_DIRENT_HAVE_D_TYPE is not defined!\n");
        closedir(dir);
        return -1;
#endif
		//@author zhangmiao@oristartech.com
		//@date [2013-06-26]
		//@brief 解决exFat文件系统不支持问题，新增加了stat()目录识别的相关代码。
		//因为在exFat文件系统下readdir()函数有问题无法识别目录，所以新增加了stat()目录识别的相关代码。
		//@new
		bool bIsDIR = false;
		char tmpPathname[PATH_MAX] = {0}; 
		sprintf(tmpPathname,DIR_FMTSTR,pathname,dirt->d_name);
		struct stat tmpst;
		if( stat( tmpPathname , &tmpst ) < 0 )
		{
			printf("stat %s is error.\n" , tmpPathname );
			return -1;
		}
		else if ( S_ISDIR(tmpst.st_mode) )
		{
			bIsDIR = true;
			printf("this is directory.\n");
		}
		//@modify end;

        if( dirt->d_type==DT_DIR  || bIsDIR )
        {
            //sprintf(tmppath,"%s/%s",pathname,dirt->d_name);
			sprintf(tmppath,DIR_FMTSTR,pathname,dirt->d_name);
            ret = searchFile(tmppath,filename,filelist,nfile,max);
            if(ret==-2)
            {
                closedir(dir);
                return -2;
            }
#if 0
            if(ret!=0)
            {
                if(ret==ENOENT) continue;
                closedir(dir);
                if(*nfile>0) while(--(*nfile)>=0) free(filelist[*nfile]);
                //printf("%s\n",strerror(errno));
                return ret;
            }
#endif
        }
		else
		{
            if(strcmp(dirt->d_name,filename)==0)
            {
                if(*nfile>=max)
                {
                    printf("nfile=%d is bigger than max!\n",*nfile);closedir(dir);
                    //if(*nfile>0) while(--(*nfile)>=0) free(filelist[*nfile]);
                    return -2;
                }
                filelist[*nfile] = (char *)malloc(PATH_MAX);
                //sprintf(filelist[*nfile],"%s/%s",pathname,filename);
				sprintf(filelist[*nfile],DIR_FMTSTR,pathname,filename);
                //printf("%s,%d\n",filelist[*nfile],*nfile);
                (*nfile)++;
            }
        }
    }
    closedir(dir);
    return 0;
}
int searchFile2(const char *pathname,const char *filename,char **filelist,int *nfile,int max)
{
	// the blurring search, e.g "*.xml"
    DIR *dir;
    struct dirent *dirt;
    char tmppath[PATH_MAX] = {0}; 
    int ret =0;

    if(pathname==NULL || filename==NULL || nfile==NULL){printf("para is NULL!\n"); return -1;}// 

    char srcPath[PATH_MAX] = {0};
    strcpy(srcPath,pathname);
    //if(srcPath[strlen(srcPath)-1]=='/')
	if( srcPath[strlen(srcPath)-1]== DIR_SEPCHAR )
        //strncpy(pathName,pathname,pathname.strlen()-1);
        srcPath[strlen(srcPath)-1] = '\0';
    dir = opendir(srcPath);
    if(dir==NULL)
    {
        //if(errno!=ENOENT && *nfile>0) 
        //if(*nfile>0) 
        //    while(--(*nfile)>=0) free(filelist[*nfile]);
        return errno;
    }
    while((dirt=readdir(dir))!=NULL)
    {
        //printf("%s\n",dirt->d_name);
        if(strcmp(dirt->d_name,".")==0 ||strcmp(dirt->d_name,"..")==0)
            continue;
#ifndef _DIRENT_HAVE_D_TYPE
        printf("_DIRENT_HAVE_D_TYPE is not defined!\n");
        closedir(dir);
        return -1;
#endif
		//@author zhangmiao@oristartech.com
		//@date [2013-06-26]
		//@brief 解决exFat文件系统不支持问题，新增加了stat()目录识别的相关代码。
		//因为在exFat文件系统下readdir()函数有问题无法识别目录，所以新增加了stat()目录识别的相关代码。
		//@new
		bool bIsDIR = false;
		char tmpPathname[PATH_MAX] = {0}; 
		sprintf(tmpPathname,DIR_FMTSTR,pathname,dirt->d_name);
		struct stat tmpst;
		if( stat( tmpPathname , &tmpst ) < 0 )
		{
			printf("stat %s is error.\n" , tmpPathname );
			return -1;
		}
		else if ( S_ISDIR(tmpst.st_mode) )
		{
			bIsDIR = true;
			printf("this is directory.\n");
		}
		//@modify end;

        if( dirt->d_type==DT_DIR || bIsDIR )
        {
            //sprintf(tmppath,"%s/%s",srcPath,dirt->d_name);
			sprintf(tmppath,DIR_FMTSTR,srcPath,dirt->d_name);
            ret = searchFile2(tmppath,filename,filelist,nfile,max);
            if(ret==-2) // arrived at the number Max
            {
                closedir(dir);
                return -2;
            }
#if 0
            if(ret!=0)
            {
                if(ret==ENOENT) continue;
                closedir(dir);
                if(*nfile>0) while(--(*nfile)>=0) free(filelist[*nfile]);
                return ret;
            }
#endif
        }
		else
		{
            if(strstr(dirt->d_name,filename)!=NULL)
            {
                //printf("%s\n",dirt->d_name);
                if(*nfile>=max) 
                {
                    printf("nfile=%d is bigger than max\n",*nfile);closedir(dir);
                    //if(*nfile>0) while(--(*nfile)>=0) free(filelist[*nfile]);
                    return -2;
                }
                char *ptmp = (char *)malloc(PATH_MAX);
                if(ptmp==NULL) return -3;
                filelist[*nfile] = ptmp; 
                //sprintf(filelist[*nfile],"%s/%s",srcPath,dirt->d_name);
				sprintf(filelist[*nfile],DIR_FMTSTR,srcPath,dirt->d_name);
                //printf("%s,%d\n",filelist[*nfile],*nfile);
                (*nfile)++;
            }
        }
    }
    closedir(dir);
    return 0;
}

int MakeDirectory( const std::string DestDir )
{
	int ret(0);

	std::string  Dest = DestDir;
	if ( Dest.length()==0 )
	{
		errno = -2;
		return errno;
	}
	//check file directory; 如果没有dest目录，则建立该路径。
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
			ret = system( (MKDIR + Dest).c_str() );
			if ( ret == 0 )
			{
#ifdef _TEST_
				printf( "File Directory Created :%s\n", Dest.c_str() );
#endif
			}
			else
			{
#ifdef _TEST_
				printf( "File Directory Create Fail :%s\n", Dest.c_str() );
#endif
				return errno;
			}
			errno = 0;
		}
		else
		{
#ifdef _TEST_
			perror("\nUnable to delete directory\n");
#endif
		}
	}
	else
	{
		//如果没有dest目录，则建立该路径。
		//ret = mkdir(Dest.c_str());
		ret = system( (MKDIR + Dest).c_str() );
		if ( ret == 0 )
		{
#ifdef _TEST_
			printf( "File Directory Created :%s\n", Dest.c_str() );
#endif
		}
		else
		{
#ifdef _TEST_
			printf( "File Directory Create Fail :%s\n", Dest.c_str() );
#endif
			return errno;
		}
	}

	return ret;
}

int MakeDirectory2( const std::string DestDir )
{
	int ret(0);

	std::string  Dest = DestDir;
	if ( Dest.length()==0 )
	{
		errno = -2;
		return errno;
	}
	//check file directory; 如果没有dest目录，则建立该路径。
	if( ( ret = access( Dest.c_str() , F_OK ) ) == 0 )     
	{
#ifdef _TEST_
		printf( "File Directory already exists :%s\n", Dest.c_str() );
#endif
		
	}
	else
	{
		//如果没有dest目录，则建立该路径。
		//ret = mkdir(Dest.c_str());
		ret = system( (MKDIR + Dest).c_str() );
		if ( ret == 0 )
		{
#ifdef _TEST_
			printf( "File Directory Created :%s\n", Dest.c_str() );
#endif
		}
		else
		{
#ifdef _TEST_
			printf( "File Directory Create Fail :%s\n", Dest.c_str() );
#endif
			return errno;
		}
	}

	return ret;
}

int RemoveDir( const std::string DestDir )
{
	int ret(0);

	std::string  Dest = DestDir;
	if ( Dest.length()==0 )
	{
		errno = -2;
		return errno;
	}

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
			
		}
	}

	return ret;
}

int PreRemoveDir( const std::string DestDir )
{
	int ret(0);

	std::string  Dest = DestDir;
	if ( Dest.length()==0 )
	{
		errno = -2;
		return errno;
	}

	if( ( ret = access( Dest.c_str() , F_OK ) ) == 0 )     
	{
#ifdef _TEST_
		printf( "File Directory already exists :%s\n", Dest.c_str() );
#endif
		//ret = system( (RMDIR + Dest).c_str() );
		std::string dirName;
		dirName = Dest;
		DIR *dir = opendir(dirName.c_str());
		dirent *dirFile;
		while((dirFile = readdir(dir)) != NULL)
		{
			if(strcmp(dirFile->d_name,".")==0 ||strcmp(dirFile->d_name,"..")==0)
				continue;

			if(dirFile->d_type==DT_DIR)
			{
				;
			}
			else
			{
				//获得文件大小
				unsigned long long Length = 0;
				struct stat stsrc;
				if( lstat( dirFile->d_name , &stsrc ) == 0 )
					Length = stsrc.st_size;
				else{
#ifdef _TEST_
					printf("lstat %s is err:%s!\n",dirFile->d_name , strerror(errno));
#endif
					closedir(dir);
					return -2;
				}
				//比较大小，大于100MB的清空
				if ( Length >= (100*1024*1024) )
				{
					std::string sCmd = "cat /dev/null > ";
					sCmd += (dirFile->d_name) ;
					ret = system( sCmd.c_str() );
					if( ret == 0 )
					{
						printf( "\n exec finished:%s\n", sCmd.c_str() );
						errno = 0;
					}
					else
					{
						printf( "\n exec fail:%s\n", sCmd.c_str() );
					}
				}
			}
		}
		closedir(dir);

	}

	return ret;
}
//---zhangmiao:begin---[7/10/2012]----
int listFilesOnFTP( std::string sFTP_URL ,std::vector<cftp::FileInformation> &fileList )   
{
	cftp::FtpClient m_ftp;
	//log in
	int result = m_ftp.LogIn(sFTP_URL);
	if(result != cftp::NOANYERROR)
		return result;
	//get file name from ftp server in current direction
	result = m_ftp.ListFile("", fileList);
	if(result != cftp::NOANYERROR)
		return result;

	return 0;
}
int listFilesOnFTP( cftp::FtpClient& rftp ,std::string dir_address ,std::vector<cftp::FileInformation> &fileList ,std::string filename )
{ 
	std::vector<cftp::FileInformation> vfileList;
	int result = rftp.ListFile(dir_address, vfileList);
	if(result != cftp::NOANYERROR)
		return result;
	if (vfileList.size()==0)
	{
		return 100;
	}

	std::vector<cftp::FileInformation>::size_type i = 0;
	for ( i = 0; i < vfileList.size(); i++ )
	{
		cftp::FileInformation FileInfo = vfileList[i];
		std::string type = FileInfo.type; //获取文件属性      
		std::string size = FileInfo.size; //获取文件大小     
		std::string name = FileInfo.name; //获取文件名      
		std::string address = dir_address + name; //文件的完整地址
		if( type[0] == 'd') //如果该文件是目录 
		{
			std::vector<cftp::FileInformation> tmp;
			int ret = listFilesOnFTP(rftp, dir_address+name+'/',tmp , filename ); //递归 
			if( ret == 0 )  
				fileList.insert(fileList.end(), tmp.begin(),tmp.end());
		}
		else //如果不是目录，逐项添加到数组 
		{
			if(filename.size())
			{
				size_t pos = address.rfind(filename);
				if( pos != std::string::npos )
				{
					FileInfo.name = address;
					fileList.push_back(FileInfo);
				}
				else
				{
					continue;
				}
			}
			else
			{
				FileInfo.name = address;
				fileList.push_back(FileInfo);
			}
		}
    
	}

	return result;
}

int listFilesOnFTPWithUrl( std::string sFTP_URL ,std::string dir_address ,std::vector<cftp::FileInformation> &fileList ,std::string filename ) 
{
	cftp::FtpClient m_ftp;
	//log in
	int result = m_ftp.LogIn(sFTP_URL);
	if(result != cftp::NOANYERROR)
		return result;

	result = listFilesOnFTP( m_ftp, dir_address ,fileList ,filename );
	if(result != cftp::NOANYERROR)
		return result;

	return 0;
}

//ftp删除目录，删除所有文件，包括目录本身。
//参数 如："ftp://admin:admin@172.23.140.165/DCP/zhangm/testdcp/"
int FtpRemoveDir( std::string sFTP_URL )
{
	int result = 0;

	cftp::FtpClient ftp;
	//log in
	result = ftp.LogIn(sFTP_URL);
	if( result != cftp::NOANYERROR )
		return result;
	
	std::string ip, userName, passwd, dir;
	std::string port = "21";

	result = ftp.ParseUrl( sFTP_URL , ip , userName , passwd , port , dir );
	if( result != cftp::NOANYERROR )
	{
		return result;
	}

	std::string dir_address = dir.substr( 0 , dir.rfind( '/' ) + 1 );
	result = FtpRemoveDir_AllFile( ftp , dir_address );
	if(result != cftp::NOANYERROR)
		return result;

	result = ftp.DeleteDir( dir_address );
	if( result != cftp::NOANYERROR )
		return result;

	return 0;
}

//FTP 删除目录下的所有文件，包括下面的子文件夹
int FtpRemoveDir_AllFile( cftp::FtpClient& rftp , std::string dir_address )
{
	int result = 0;

	std::vector<cftp::FileInformation> vfileList;
	result = rftp.ListFile( dir_address , vfileList );
	if( result != cftp::NOANYERROR )
		return result;

	if ( vfileList.size() == 0 )
	{
		return 0;
	}

	std::vector<cftp::FileInformation>::size_type i = 0;
	for ( i = 0; i < vfileList.size(); i++ )
	{
		cftp::FileInformation FileInfo = vfileList[i];
		std::string type = FileInfo.type; //获取文件属性      
		std::string size = FileInfo.size; //获取文件大小     
		std::string name = FileInfo.name; //获取文件名      
		std::string address = dir_address + name; //文件的完整地址
		if( type[0] == 'd') //如果该文件是目录 
		{
			result = FtpRemoveDir_AllFile( rftp , dir_address+name+'/' ); //递归 
			if( result == 0 )
			{
				result = rftp.DeleteDir( dir_address+name+'/' );
				if( result != cftp::NOANYERROR )
					return result;
			}
		}
		else //如果不是目录，删除文件
		{
			result = rftp.DeleteFile( address );
			if( result != cftp::NOANYERROR )
				return result;
		}
	}

	return result;
}

//ftp删除目录，删除所有文件，包括目录本身。只是参数不同
int FtpRemoveDir( cftp::FtpClient& rftp , std::string dir_address )
{
	int result = 0;
	std::vector<cftp::FileInformation> vfileList;
	result = rftp.ListFile( dir_address , vfileList );
	if( result != cftp::NOANYERROR )
		return result;

	if ( vfileList.size() == 0 )
	{
		result = rftp.DeleteDir( dir_address );
		if( result != cftp::NOANYERROR )
			return result;

		return 0;
	}

	std::vector<cftp::FileInformation>::size_type i = 0;
	for ( i = 0; i < vfileList.size(); i++ )
	{
		cftp::FileInformation FileInfo = vfileList[i];
		std::string type = FileInfo.type; //获取文件属性      
		std::string size = FileInfo.size; //获取文件大小     
		std::string name = FileInfo.name; //获取文件名      
		std::string address = dir_address + name; //文件的完整地址
		if( type[0] == 'd') //如果该文件是目录 
		{
			result = FtpRemoveDir( rftp , dir_address+name+'/' ); //递归 
			/*if( result == 0 )
			{
				result = rftp.DeleteDir( dir_address+name+'/' );
				if( result != cftp::NOANYERROR )
					return result;
			}*/

			if( result != cftp::NOANYERROR )
			{
				return result;
			}
		}
		else //如果不是目录，删除文件
		{
			result = rftp.DeleteFile( address );
			if( result != cftp::NOANYERROR )
				return result;
		}
	}
	
	//最后，删除目标目录本身
	result = rftp.DeleteDir( dir_address );
	if( result != cftp::NOANYERROR )
		return result;

	return result;
}

//建立目录，如果存在目标存在，则先删除该目录。然后，再创建该目标目录。
//参数，如：sFTP_URL="ftp://admin:admin@172.23.140.165/DCP/zhangm/testdcp/"   ,sCreatedDir= "abc/" 
int Ftp_MakeDirectory(  std::string sFTP_URL , std::string sCreatedDir )
{
	int result = 0;

	cftp::FtpClient ftp;

	//log in
	result = ftp.LogIn( sFTP_URL );
	if( result != cftp::NOANYERROR )
		return result;

	std::string ip, userName, passwd, dir;
	std::string port = "21";

	result = ftp.ParseUrl( sFTP_URL , ip , userName , passwd , port , dir );
	if( result != cftp::NOANYERROR )
	{
		return result;
	}

	std::string dir_address = dir.substr( 0 , dir.rfind( '/' ) + 1 );

	std::vector<cftp::FileInformation> vfileList;

	result = ftp.ListFile( "" , vfileList );
	if(result != cftp::NOANYERROR)
		return result;

	bool bIsExistCreatedDir = false;

	std::vector<cftp::FileInformation>::size_type i = 0;
	for ( i = 0; i < vfileList.size(); i++ )
	{
		cftp::FileInformation FileInfo = vfileList[i];
		std::string type = FileInfo.type; //获取文件属性      
		std::string size = FileInfo.size; //获取文件大小     
		std::string name = FileInfo.name; //获取文件名      
		std::string address = dir_address + name; //文件的完整地址

		if( type[0] == 'd') //如果该文件是目录 
		{
			if( address.rfind( '/' ) != address.length()-1 )	//检测目录的末尾是否带分隔符 "/"，如果不带则给它补上。
				address.append( 1 , '/' );

			std::string::size_type pos = address.rfind( sCreatedDir );
			if( pos != std::string::npos )
			{
				bIsExistCreatedDir = true;
				break;
			}
			else
			{
				continue;
			}

		}
	}

	if ( bIsExistCreatedDir == false )
	{
		if( sCreatedDir.length() )
		{
			result = ftp.CreateDir( dir_address + sCreatedDir );
		}
	}
	else
	{
		/*result = ftp.DeleteDir( dir_address + sCreatedDir );
		if(result != cftp::NOANYERROR)
			return result;*/

		result = FtpRemoveDir( ftp , dir_address + sCreatedDir );

		result = ftp.CreateDir( dir_address + sCreatedDir );
	}

	return result;
}

//建立目录，如果目标目录存在则不删除，不再创建该目录。
//参数，如：sFTP_URL="ftp://admin:admin@172.23.140.165/DCP/zhangm/testdcp/"   ,sCreatedDir= "abc/" 
int Ftp_MakeDirectory2( std::string sFTP_URL , std::string sCreatedDir )
{
	int result = 0;

	cftp::FtpClient ftp;

	//log in
	result = ftp.LogIn( sFTP_URL );
	if( result != cftp::NOANYERROR )
		return result;

	std::string ip, userName, passwd, dir;
	std::string port = "21";

	result = ftp.ParseUrl( sFTP_URL , ip , userName , passwd , port , dir );
	if( result != cftp::NOANYERROR )
	{
		return result;
	}

	std::string dir_address = dir.substr( 0 , dir.rfind( '/' ) + 1 );

	std::vector<cftp::FileInformation> vfileList;
	
	result = ftp.ListFile( "" , vfileList );
	if(result != cftp::NOANYERROR)
		return result;
	
	bool bIsExistCreatedDir = false;
	std::vector<cftp::FileInformation>::size_type i = 0;
	for ( i = 0; i < vfileList.size(); i++ )
	{
		cftp::FileInformation FileInfo = vfileList[i];
		std::string type = FileInfo.type; //获取文件属性      
		std::string size = FileInfo.size; //获取文件大小     
		std::string name = FileInfo.name; //获取文件名      
		std::string address = dir_address + name; //文件的完整地址

		if( type[0] == 'd') //如果该文件是目录 
		{
			if( address.rfind( '/' ) != address.length()-1 )	//检测目录的末尾是否带分隔符 "/"，如果不带则给它补上。
				address.append( 1 , '/' );

			std::string::size_type pos = address.rfind( sCreatedDir );
			if( pos != std::string::npos )
			{
				bIsExistCreatedDir = true;
				break;
			}
			else
			{
				continue;
			}
		}
	}

	//如果已经存在要创建的目录，则不删除，不再创建该目录。
	if ( bIsExistCreatedDir == false )
	{
		if( sCreatedDir.length() )
		{
			result = ftp.CreateDir( dir_address + sCreatedDir );
		}
	}

	return result;
}

//建立目录，支持建立多级目录；如果目标目录存在则不删除，不再创建该目录。
//参数，如：sFTP_URL="ftp://admin:admin@172.23.140.165/DCP/zhangm/testdcp/"   ,sCreatedDir= "a1/b2/c3/de4/" 
int Ftp_MultiMakeDir( std::string sFTP_URL , std::string sCreatedDir ) 
{
	int result = 0;

	//vector vCreatedDirList，表示要创建的各级目录，从索引1项开始，索引0项有特殊的作用，表示开始时的父目录。
	std::vector<std::string> vCreatedDirList;

	cftp::FtpClient ftp;

	std::string ip, userName, passwd, dir;
	std::string port = "21";

	result = ftp.ParseUrl( sFTP_URL , ip , userName , passwd , port , dir );
	if( result != cftp::NOANYERROR )
	{
		return result;
	}

	//生成列表 vCreatedDirList
	vCreatedDirList.push_back(dir);

	std::string::size_type begPos = 0;
	std::string::size_type endPos = sCreatedDir.find( "/" , begPos);
	while( begPos != std::string::npos && endPos != std::string::npos )
	{
		//std::string aFileStr( sCreatedDir , begPos , endPos-begPos );
		std::string aFileStr( sCreatedDir , 0 , endPos+1 );
		
		vCreatedDirList.push_back( aFileStr );

		begPos = endPos + strlen("/");
		endPos = sCreatedDir.find( "/" , begPos);
	}

	std::string tmpdir = dir;
	result = ftp.LogIn( ip, userName, passwd, tmpdir , port );
	if( result != cftp::NOANYERROR )
	{
		return result;
	}

	//std::string dir_address = dir.substr( 0 , dir.rfind( '/' ) + 1 );
	std::vector<std::string>::size_type k = 1;

	for ( k = 1; k < vCreatedDirList.size(); k++ )
	{
		std::string sTmpParentDir = vCreatedDirList[k-1];

		std::string sTmpCreateDir = vCreatedDirList[k];
		
		std::vector<cftp::FileInformation> vfileList;

		result = ftp.ListFile( sTmpParentDir , vfileList );
		if( result != cftp::NOANYERROR )
			return result;

		bool bIsExistCreatedDir = false;
		std::vector<cftp::FileInformation>::size_type i = 0;
		for ( i = 0; i < vfileList.size(); i++ )
		{
			cftp::FileInformation FileInfo = vfileList[i];
			std::string type = FileInfo.type; //获取文件属性      
			std::string size = FileInfo.size; //获取文件大小     
			std::string name = FileInfo.name; //获取文件名      

			std::string address = sTmpParentDir + name; //文件的完整地址

			if( type[0] == 'd') //如果该文件是目录 
			{
				if( address.rfind( '/' ) != address.length()-1 )	//检测目录的末尾是否带分隔符 "/"，如果不带则给它补上。
					address.append( 1 , '/' );

				std::string::size_type pos = address.rfind( sTmpCreateDir );
				if( pos != std::string::npos )
				{
					bIsExistCreatedDir = true;
					break;
				}
				else
				{
					continue;
				}
			}
		}


		//如果已经存在要创建的目录，则不删除，不再创建该目录。
		if ( bIsExistCreatedDir == false )
		{
			if( sTmpCreateDir.length() )
			{
				result = ftp.CreateDir( sTmpCreateDir );
				if( result != cftp::NOANYERROR )
					return result;
			}
		}

	}

	return result;
}

int Ftp_MultiMakeDir2( std::string sFTP_URL , std::string sCreatedDir ) 
{
	int result = 0;

	//vector vCreatedDirList，表示要创建的各级目录，从索引1项开始，索引0项有特殊的作用，表示开始时的父目录。
	std::vector<std::string> vCreatedDirList;

	cftp::FtpClient ftp;

	std::string ip, userName, passwd, dir;
	std::string port = "21";

	result = ftp.ParseUrl( sFTP_URL , ip , userName , passwd , port , dir );
	if( result != cftp::NOANYERROR )
	{
		return result;
	}

	//生成列表 vCreatedDirList
	vCreatedDirList.push_back(dir);

	std::string::size_type begPos = 0;
	std::string::size_type endPos = sCreatedDir.find( "/" , begPos);
	while( begPos != std::string::npos && endPos != std::string::npos )
	{
		//std::string aFileStr( sCreatedDir , begPos , endPos-begPos );
		std::string aFileStr( sCreatedDir , 0 , endPos+1 );

		vCreatedDirList.push_back( aFileStr );

		begPos = endPos + strlen("/");
		endPos = sCreatedDir.find( "/" , begPos);
	}

	std::vector<std::string>::size_type nLastDirIndex = 0;
	if ( vCreatedDirList.size() != 0 )
	{
		nLastDirIndex = vCreatedDirList.size()-1;
	}
	

	std::string tmpdir = dir;
	result = ftp.LogIn( ip, userName, passwd, tmpdir , port );
	if( result != cftp::NOANYERROR )
	{
		return result;
	}

	//std::string dir_address = dir.substr( 0 , dir.rfind( '/' ) + 1 );
	std::vector<std::string>::size_type k = 1;

	for ( k = 1; k < vCreatedDirList.size(); k++ )
	{
		std::string sTmpParentDir = vCreatedDirList[k-1];

		std::string sTmpCreateDir = vCreatedDirList[k];

		std::vector<cftp::FileInformation> vfileList;

		result = ftp.ListFile( sTmpParentDir , vfileList );
		if( result != cftp::NOANYERROR )
			return result;

		bool bIsExistCreatedDir = false;
		std::vector<cftp::FileInformation>::size_type i = 0;
		for ( i = 0; i < vfileList.size(); i++ )
		{
			cftp::FileInformation FileInfo = vfileList[i];
			std::string type = FileInfo.type; //获取文件属性      
			std::string size = FileInfo.size; //获取文件大小     
			std::string name = FileInfo.name; //获取文件名      

			std::string address = sTmpParentDir + name; //文件的完整地址

			if( type[0] == 'd') //如果该文件是目录 
			{
				if( address.rfind( '/' ) != address.length()-1 )	//检测目录的末尾是否带分隔符 "/"，如果不带则给它补上。
					address.append( 1 , '/' );

				std::string::size_type pos = address.rfind( sTmpCreateDir );
				if( pos != std::string::npos )
				{
					bIsExistCreatedDir = true;
					break;
				}
				else
				{
					continue;
				}
			}
		}


		//如果已经存在要创建的目录，则不删除，不再创建该目录。
		if ( bIsExistCreatedDir == false )
		{
			if( sTmpCreateDir.length() )
			{
				result = ftp.CreateDir( sTmpCreateDir );
				if( result != cftp::NOANYERROR )
					return result;
			}
		}
		else
		{
			if ( nLastDirIndex == k && nLastDirIndex > 0 )
			{
				if( sTmpCreateDir.length() )
				{
					result = FtpRemoveDir( ftp , sTmpCreateDir );

					result = ftp.CreateDir( sTmpCreateDir );
					if( result != cftp::NOANYERROR )
						return result;
				}
			}
			
		}
	}


	return result;
}


//---zhangmiao:end--[7/10/2012]--
int Remove_PrefixOfUuid( std::string& rUuid )
{
	size_t pos;
	if ( ( pos = rUuid.find("urn:uuid:") ) != rUuid.npos && pos==0 )
	{
		rUuid = rUuid.substr(9);
		return 0;
	}
	else{
		return 1;
	}
}

//@author zhangmiao@oristartech.com
//@date 2014-10-08
//@brief 增加新的时区处理代码，sDestTZ='+08:00'为北京时间，如:sSrcTimeStr="2009-07-04T10:00:00+09:00"，转换为sDestTimeStr="2009-07-04 09:00:00"；
//@new
int ConvertTimeZoneInfo( std::string sSrcTimeStr , std::string sDestTZ , std::string& sDestTimeStr )
{
	int ret = 0;
	int nYear=0,nMon=0,nDay=0,nHour=0,nMin=0,nSec=0,nTz_Hour=0,nTz_Min=0;
	//"2014-10-01T12:00:00+09:00";
	sscanf(sSrcTimeStr.c_str(),"%04d-%02d-%02dT%02d:%02d:%02d%d:%02d", &nYear , &nMon , &nDay , &nHour , &nMin , &nSec ,&nTz_Hour , &nTz_Min );
	struct tm *local=NULL , tmSrcTime;
	time_t timeVal = 0;

	tmSrcTime.tm_sec  = nSec;
	tmSrcTime.tm_min  = nMin;
	tmSrcTime.tm_hour = nHour;
	tmSrcTime.tm_mday = nDay;
	tmSrcTime.tm_mon  = nMon  - 1;
	tmSrcTime.tm_year = nYear - 1900;

	char sTZ_Env[50]="",sDestTZ_Env[50]="";
	sprintf(sTZ_Env , "TZ=PST%+03d:%02d" , -nTz_Hour , nTz_Min );
	//putenv("TZ=PST-8");
	putenv( sTZ_Env );
	tzset();

	timeVal = mktime( &tmSrcTime );

	int nDestTz_Hour=8, nDestTz_Min=0;

	if ( sDestTZ.length()==0 )
	{
		putenv( "TZ=PST-08:00" );
	}
	else
	{
		nDestTz_Hour = 8 , nDestTz_Min = 0;
		sscanf(sDestTZ.c_str(),"%d:%02d", &nDestTz_Hour , &nDestTz_Min );
		sprintf(sDestTZ_Env , "TZ=PST%+03d:%02d" , -nDestTz_Hour , nDestTz_Min );
		putenv( sDestTZ_Env );
	}

	tzset();

	local = localtime( &timeVal );
	//local = gmtime(&td);
	printf("Current time = %s\n", asctime(local));

	putenv("TZ=");
	tzset();

	char sRetTimeStr[50]="";
	sprintf( sRetTimeStr , "%04d-%02d-%02d %02d:%02d:%02d" , local->tm_year + 1900 ,
		local->tm_mon + 1 ,local->tm_mday,local->tm_hour,local->tm_min,local->tm_sec );

	sDestTimeStr = sRetTimeStr;

	return ret;
}
//@modify(时区处理)end;
