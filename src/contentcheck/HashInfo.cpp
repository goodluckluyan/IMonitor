/*
*@file:HashInfo.cpp
*@brief:哈希校验，第三方库，但做了些修改。
*@author: zhangmiao@oristartech.com
*@date: 2012/06/05
*/
#ifndef __HASHINFO_H__
#include "HashInfo.h"
#endif
//---zhangmiao:begin---[7/10/2012]----
//@author zhangmiao@oristartech.com
//@date [2012-07-10]
//@brief 定义、声明用于hash校验的全局变量
//@new
string g_sHashDcp_PklUuidName;        //正在校验的pkl包的UUID名
string g_sHashDcp_Checking_FileName;  //正在校验的文件名
extern int g_flag_SHA1CheckStop;
//声明互斥锁
#ifdef WIN32
#include <windows.h>
extern HANDLE hMutex;
#else
#include <pthread.h>
extern pthread_mutex_t work_mutex;
#endif
//@modify end;
//---zhangmiao:end--[7/10/2012]--

HashInfo::HashInfo()
{
}

string HashInfo::getSha1FromFile(const char *file,char *bin_buffer,
                                unsigned long long *hasCpSize,int *stopCpFlag,int flag_cpucontl)
{
    string ret = "";
    char tmpBuf[3] = {0};
    char tmp_bin_buffer[SHA1_DIGEST_SIZE] = {0};

    if(bin_buffer==NULL) bin_buffer = tmp_bin_buffer;
    //*bin_buffer =(unsigned char *)ptr_align (bin_buffer_unaligned, DIGEST_ALIGN);
    //static size_t digest_hex_bytes = DIGEST_HEX_BYTES;
	if(!digest_file(file,NULL,bin_buffer,hasCpSize,stopCpFlag,flag_cpucontl))
    {
        printf("digest_file is failed!\n");
        return ret;
    }
    else
    {
	    //for (int i=0; i<(digest_hex_bytes/2); ++i)
	    for (int i=0; i<SHA1_DIGEST_SIZE; ++i)
        {
		    sprintf(tmpBuf,"%02x",bin_buffer[i]&0xFF);
            //printf("%02x",(*bin_buffer)[i]);
            ret.append(tmpBuf);
        }
        //printf("\n");
    }
    //cout << file << "---- " << ret << endl;
    return ret;
}

string HashInfo::toBase64(const char *bin_buffer)
{
    string ret = "";
    char *outstr = NULL;
    //static size_t digest_hex_bytes = DIGEST_HEX_BYTES;
    if(bin_buffer==NULL) return ret;
    //base64_encode_alloc((char *)bin_buffer,digest_hex_bytes/2,(char **)&outstr);
    base64_encode_alloc(bin_buffer,SHA1_DIGEST_SIZE,(char **)&outstr);
    if(outstr==NULL) printf("base64 error!\n");
    else
    {
        //printf("%s\n",outstr);
        ret = outstr;
        free(outstr);
    }
    //cout << ret << endl;

    return ret;
}

bool HashInfo::digest_file(const char *filename, int *binary, char *bin_result,
        unsigned long long *hasCpSize,int *stopCpFlag,int flag_cpucontl)
{
    FILE *fp;
    int err;

    fp = fopen (filename, "rb");
    if (fp == NULL)
	{
        //error (0, errno, "%s", filename);
        printf("%s:%s\n",filename,strerror(errno));
        return false;
	}
    err = sha1_stream(fp,bin_result,hasCpSize,stopCpFlag,flag_cpucontl);
    if (err)
    {
//---zhangmiao:begin---[7/10/2012]----
//@author zhangmiao@oristartech.com
//@date [2012-07-10]
//@brief 增加取消hash校验功能
//@new
#ifdef WIN32
		WaitForSingleObject(hMutex,INFINITE);
#else
		pthread_mutex_lock(&work_mutex);
#endif
        //error (0, errno, "%s", filename);
		if ( err == CODE_SHA1CHECKCANCEL )
		{
			g_flag_SHA1CheckStop = 0;
#ifdef _TEST_
			printf("%s:%s\n",filename,"SHA1 CHECK CANCEL!");
#endif
		}
#ifdef _TEST_
		else
			printf("%s:%s\n",filename,strerror(errno));
#endif


#ifdef WIN32
		ReleaseMutex(hMutex);
#else
		pthread_mutex_unlock(&work_mutex);
#endif
//@modify end;
//---zhangmiao:end--[7/10/2012]--
        if (fp != stdin)
            fclose (fp);
        return false;
    }

  //if (!is_stdin && fclose (fp) != 0)
    if (fclose (fp) != 0)
    {
//---zhangmiao:begin---[7/10/2012]----
//@author zhangmiao@oristartech.com
//@date [2012-07-10]
//@brief 增加错误的输出信息
//@new
#ifdef _TEST_
        //error (0, errno, "%s", filename);
        printf("%s:%s\n",filename,strerror(errno));
#endif
//@modify end;
//---zhangmiao:end--[7/10/2012]--
        return false;
    }

    return true;
}

