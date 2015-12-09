#include <unistd.h>
#include <sys/stat.h> 
#include <pwd.h>
#include <sys/types.h>
#include <stdio.h>
#include<dirent.h> 
#include <string.h>
#include <stdlib.h>
#include "C_FileOperationsTool.h"
#include "C_Log.h"

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
	uid_t uid = getuid();  
	if(setuid(0) ) 
	{  
		CLog::Write(Error,"set-root-uid: setuid error");  
	}   
	
	int reno1 =system(cmdOwn.c_str() );
	int reno2 =system(cmdMod.c_str() );
	if(setuid(uid)) {  
		CLog::Write(Error,"set-Normal-uid: setuid error");  
	}  
	if (reno1>=0&&reno2>=0 )
	{
		return true;
	} else if (CheckOwn(path) )
	{
		return true;
	}
	return false;
}

bool CFileOperationsTool::CheckOwn(const std::string& path )
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
						flag &=CheckOwn(name);
					}
				}
				struct stat buf;
				stat(path.c_str(),&buf );
				int self=0;
				int group =0;
				int other =0;
				int n;
				for(n=8;n>=6;n--)  
				{  
					if(buf.st_mode&(1<<n))  
					{  
						switch(n%3)  
						{  
						case 2:  
							self+=4;  
							break;  
						case 1:  
							self+=2;  
							break;  
						case 0:  
							self+=1;   
							break;  
						default:  
							break;  
						}  
					}  
				} 

				for(n=5;n>=3;n--)  
				{  
					if(buf.st_mode&(1<<n))  
					{  
						switch(n%3)  
						{  
						case 2:  
							group+=4;  
							break;  
						case 1:  
							group+=2;  
							break;  
						case 0:  
							group+=1;   
							break;  
						default:  
							break;  
						}  
					}  
				}  

				for(n=2;n>=0;n--)  
				{  
					if(buf.st_mode&(1<<n))  
					{  
						switch(n%3)  
						{  
						case 2:  
							other+=4;  
							break;  
						case 1:  
							other+=2;  
							break;  
						case 0:  
							other+=1;   
							break;  
						default:  
							break;  
						}  
					}  
				}  

				if ( !( (7==self) && (5==group) && (5==other) ) )
				{
					char strbuf[255];
					sprintf(strbuf," mod is:%d%d%d",self, group, other );
					CLog::Write(Error, "The directory "+path+strbuf );
					flag =false;
				}

				struct passwd * usrMsg =getpwnam("admin");
				if ( (usrMsg->pw_uid!=buf.st_uid)||(usrMsg->pw_gid!=buf.st_gid) )
				{
					CLog::Write(Error, "The directory "+path+" Change own failed!" );
					flag =false;
				}
			} 	
		}else
		{
			struct stat buf;
			stat(path.c_str(),&buf );
			int self=0;
			int group =0;
			int other =0;
			int n;
			for(n=8;n>=6;n--)  
			{  
				if(buf.st_mode&(1<<n))  
				{  
					switch(n%3)  
					{  
					case 2:  
						self+=4;  
						break;  
					case 1:  
						self+=2;  
						break;  
					case 0:  
						self+=1;   
						break;  
					default:  
						break;  
					}  
				}  
			} 

			for(n=5;n>=3;n--)  
			{  
				if(buf.st_mode&(1<<n))  
				{  
					switch(n%3)  
					{  
					case 2:  
						group+=4;  
						break;  
					case 1:  
						group+=2;  
						break;  
					case 0:  
						group+=1;   
						break;  
					default:  
						break;  
					}  
				}  
			}  

			for(n=2;n>=0;n--)  
			{  
				if(buf.st_mode&(1<<n))  
				{  
					switch(n%3)  
					{  
					case 2:  
						other+=4;  
						break;  
					case 1:  
						other+=2;  
						break;  
					case 0:  
						other+=1;   
						break;  
					default:  
						break;  
					}  
				}  
			}  

			if ( !( (7==self) && (5==group) && (5==other) ) )
			{
				char strbuf[255];
				sprintf(strbuf," mod is:%d%d%d",self, group, other );
				CLog::Write(Error, "The file "+path+strbuf );
				flag =false;
			}
			struct passwd * usrMsg =getpwnam("admin");
			if ( (usrMsg->pw_uid!=buf.st_uid)||(usrMsg->pw_gid!=buf.st_gid) )
			{
				CLog::Write(Error, "The file "+path+" Change own failed!" );
				flag =false;
			}
		}
	}
	return flag;
}