/********************************************* 
Copyright (C), 2014-2038, ORISTARTECH Co., Ltd.
ModuleName ：  循环队列
FileName   ： CircularQueue.h 
Author     ： xiaozhengxiu@oristartech.com
Date       ： 2014-12-18
Version    ： v1.0
Description： 
Others     ：
History    ： 
      < Author >    			< Date >             < Modification >
   xiaozhengxiu@oristartech.com        2014-12-18            Add comment
********************************************/  
#ifndef CIRCULAR_QUEUE_H_
#define CIRCULAR_QUEUE_H_

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>  

#define QUEUEDATABSIZE 1024*80 /*8192 */

struct element_Info
{
	unsigned int datalen;
	char* data;//数据
};

//template<typename T>

class CircularQueue 
{
public:
//	CircularQueue(const size_t& size) : m_size(size), m_read_index(0), m_write_index(0) {
	CircularQueue(const size_t& size){
		 m_size = size; m_read_index = 0; m_write_index = 0;
		 m_data = new element_Info[m_size];
		for (unsigned int i = 0; i < m_size; i++)//一次性分配所有内存
		{
			m_data[i].datalen = 0;
			m_data[i].data = new char[QUEUEDATABSIZE];
			memset(m_data[i].data,0,QUEUEDATABSIZE);
		}
//       if (!m_data) {// "存储空间分配失败，应用程序将终止！"
//			exit(1);
//        }
		m_elementNum = 0;
	}

	~CircularQueue(){
		for (unsigned int i = 0; i < m_size; i++)//一次性分配所有内存
		{
			if (m_data[i].data != NULL)
				delete []m_data[i].data;
		}
		if (m_data != NULL)
			delete []m_data;
	  m_read_index = m_write_index = 0;
	  m_elementNum = 0;
	}

	//判断队列是否满
	//(m_write_index + 1) % (m_size) 可实现循环自加 
	//循环队列核心方法
	bool IsFull(){  
		if((m_write_index + 1) % (m_size) == m_read_index){  
			return true;  
		} else {  
			return false;  
		}  
	}  

	//入队
	bool en_queue(const element_Info& element) {
	  if (IsFull()){  
			printf("en_queue err:queque is full\n");  
		  return false;
		} else {
			memcpy(m_data[m_write_index].data,element.data,element.datalen);
			m_data[m_write_index].datalen = element.datalen;
		    m_write_index = (m_write_index + 1) % m_size;
		    m_elementNum++;
		    return true;
		}
	  }

	//判断队列是不是为空.如果为空.就不能出队  
	bool IsEmpty(){  
		if(m_read_index == m_write_index){  
			return true;  
		} else {  
			return false;  
		}  
	}  

	//出队
	bool out_queue(element_Info* element) {
		if (IsEmpty()){  
			printf("out_queue err:queque is empty\n");  
		  return false;
		} else {
			memcpy(element->data,m_data[m_read_index].data,m_data[m_read_index].datalen);
			element->datalen = m_data[m_read_index].datalen;
			memset(m_data[m_read_index].data,0,QUEUEDATABSIZE);
		    m_read_index = (m_read_index + 1) % (m_size);
		    m_elementNum--;
		    return true;
		}
	  }

	// 清空队列
	void clear_queue() {
	  m_read_index = m_write_index = 0;
	  m_elementNum = 0;
	}

	//队列元素个数
	int GetElementNum(){
		 return m_elementNum;
	}

	//队列大小
	size_t getQueueSize(void) const{ 
		 return m_size - 1; 
	}

	//打印队列里的值，只支持整型值输出，只做测试  
	void print_queue(){  
		size_t i = m_read_index;  
		while(i != m_write_index){  
			printf("%s ", m_data[i].data);  
			i = (i + 1) % m_size;  
		}  
		printf("\n");
	}  

//获取文件大小 
unsigned long long get_file_size(const char *path)  
{  
    unsigned long long filesize = -1;      
    struct stat statbuff;  
    if(stat(path, &statbuff) < 0){  
        return filesize;  
    }else{  
        filesize = statbuff.st_size;  
    }  
    return filesize;  
}  

private:
	size_t m_read_index;//头指针，即读指针
	size_t m_write_index;//尾指针，即写指针
	size_t m_size;//队列大小	
	element_Info* m_data;//数据
	unsigned int m_elementNum;//队列元素个数
};
#endif

/*
//测试循环队列函数
int main(int argc, char** argv) {
  CircularQueue circular_queue(6);
  int inum = 0;
  std::string v = "";
  int vlen = 0;
  int i=0;

  v = "a"; 
  element_Info einfo;
  einfo.data = (char*)v.c_str();
  einfo.datalen = (unsigned int)v.length();
  circular_queue.en_queue(einfo);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  v = "ab";
  einfo.data = (char*)v.c_str();
  einfo.datalen = (unsigned int)v.length();
  circular_queue.en_queue(einfo);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  v = "abc";
  einfo.data = (char*)v.c_str();
  einfo.datalen = (unsigned int)v.length();
  circular_queue.en_queue(einfo);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  v = "abcd";
  einfo.data = (char*)v.c_str();
  einfo.datalen = (unsigned int)v.length();
  circular_queue.en_queue(einfo);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  v = "abcde";
  einfo.data = (char*)v.c_str();
  einfo.datalen = (unsigned int)v.length();
  circular_queue.en_queue(einfo);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  v = "abcdef";
  einfo.data = (char*)v.c_str();
  einfo.datalen = (unsigned int)v.length();
  circular_queue.en_queue(einfo);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  v = "abcdefg";
  einfo.data = (char*)v.c_str();
  einfo.datalen = (unsigned int)v.length();
  circular_queue.en_queue(einfo);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  element_Info value;
  value.datalen = 0;
  value.data = new char[QUEUEDATABSIZE];
  memset(value.data,0,QUEUEDATABSIZE);

  circular_queue.out_queue(&value);
  printf("出队：--%s--%d--\n", value.data,value.datalen);
  memset(value.data,0,QUEUEDATABSIZE);
  inum = circular_queue.GetElementNum();
   printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

   circular_queue.out_queue(&value);
  printf("出队：--%s--%d--\n", value.data,value.datalen);
  memset(value.data,0,QUEUEDATABSIZE);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  circular_queue.out_queue(&value);
  printf("出队：--%s--%d--\n", value.data,value.datalen);
  memset(value.data,0,QUEUEDATABSIZE);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  v = "abcdefgh";
  einfo.data = (char*)v.c_str();
  einfo.datalen = (unsigned int)v.length();
  circular_queue.en_queue(einfo);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  v = "abcdefghi";
  einfo.data = (char*)v.c_str();
  einfo.datalen = (unsigned int)v.length();
  circular_queue.en_queue(einfo);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  v = "abcdefghij";
  einfo.data = (char*)v.c_str();
  einfo.datalen = (unsigned int)v.length();
  circular_queue.en_queue(einfo);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  v = "abcdefghijk";
  einfo.data = (char*)v.c_str();
  einfo.datalen = (unsigned int)v.length();
  circular_queue.en_queue(einfo);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  circular_queue.out_queue(&value);
  printf("出队：--%s--%d--\n", value.data,value.datalen);
  memset(value.data,0,QUEUEDATABSIZE);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  v = "abcdefghijkl";
  einfo.data = (char*)v.c_str();
  einfo.datalen = (unsigned int)v.length();
  circular_queue.en_queue(einfo);
  inum = circular_queue.GetElementNum();
  printf("队列元素个数：%d\n",inum);
  circular_queue.print_queue();

  printf("队列大小：%d\n",circular_queue.getQueueSize());

  if (value.data != NULL)
	 delete []value.data;

  return 0;
}*/


