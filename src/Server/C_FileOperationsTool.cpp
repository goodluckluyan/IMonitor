#include <unistd.h>
#include <sys/stat.h> 
#include <stdio.h>
#include<dirent.h> 
#include <string.h>
#include <stdlib.h>
#include "C_FileOperationsTool.h"

CFileOperationsTool::CFileOperationsTool()
{

}

CFileOperationsTool::CFileOperationsTool(const CFileOperationsTool& obj)
{

}

CFileOperationsTool& CFileOperationsTool::operator=(const CFileOperationsTool& obj )
{
	return *this;
}

CFileOperationsTool* CFileOperationsTool::Instance()
{
	static CFileOperationsTool* pInst =NULL;
	if (!pInst )
	{
		pInst =new CFileOperationsTool;
	}
	return pInst;
}

bool CFileOperationsTool::FileExist(const std::string& path )
{
	int erno =access(path.c_str(),F_OK );
	return !erno;
}

bool CFileOperationsTool::RemoveDirectoryPathIncFolder(const std::string& path )
{
	int erno =0;
	bool flag =true;
	struct stat sb;
	if (!stat(path.c_str(), &sb) )
	{
		if (S_ISDIR(sb.st_mode ))
		{
			DIR *dp =NULL;
			if ((dp =opendir(path.c_str() ) ) )
			{
				dirent *dirp=NULL;
				while((dirp =readdir(dp))!= NULL ) 
				{
					std::string name =dirp->d_name;
					if (name!="."&&name!="..")
					{
						name =path+name;
						if (!stat(name.c_str(), &sb) )
						{
							if (S_ISDIR(sb.st_mode ))
							{
								name +="/";
							}
						}
						flag &=RemoveDirectoryPathIncFolder(name);
					}
				}
				erno =remove(path.c_str() );
				flag &=(!erno);
			} 	
		}else
		{
			erno =remove(path.c_str() );
			flag &=(!erno);
		}
	}
	return flag;
}

bool CFileOperationsTool::RemoveDirectoryPath(const std::string& path )
{
	 DIR *dp =NULL;
	 bool flag =true;
	 if ((dp =opendir(path.c_str() ) ) )
	 {
		int erno =0;
		dirent *dirp=NULL;
		struct stat sb;
		while((dirp =readdir(dp))!= NULL ) 
		{
			std::string name =dirp->d_name;
			if (name!="."&&name!="..")
			{
				name =path+name;
				if (!stat(name.c_str(), &sb) )
				{
					if (S_ISDIR(sb.st_mode ))
					{
						name +="/";
					}
				}
				flag &=RemoveDirectoryPathIncFolder(name);
			}
		}
	 } 
	 return flag;
}

bool CFileOperationsTool::RemoveFile(const std::string& path )
{
	int erno =remove(path.c_str() );
	return !erno;
}

bool CFileOperationsTool::CreateDirectoryPath(const std::string& path )
{
	char dir[500];  
	strcpy(dir,path.c_str() );  
	int len = strlen(dir);  
	if(dir[len-1]!='/')
	{
		strcat(dir,"/");
	}
	len = strlen(dir);
	bool flag =true;
	for(int i=1; i<len; i++ )  
	{  
		if(dir[i]=='/')  
		{  
			dir[i]='\0';
			std::string dirpath =dir;
			dirpath +="/";
			if(access(dirpath.c_str(), F_OK )!=0   )  
			{  
				if(mkdir(dirpath.c_str(),S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH )==-1)  
				{     
					flag =false;
					break;
				}
			}  
			dir[i] ='/';  
		}  
	}
	return flag;
}

bool CFileOperationsTool::ChangeFilePermissions(const std::string& path )
{
	std::string cmdOwn ="chown -R admin:admin "+path;
	std::string cmdMod ="chmod -R 755 "+path;
	int reno1 =system(cmdOwn.c_str() );
	int reno2 =system(cmdMod.c_str() );
	return( (!reno1)&&(!reno2) );
}
