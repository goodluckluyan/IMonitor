/*
*@file:PKLInfo.cpp
*@brief:pkl info ½á¹¹
*@author: zhangmiao@oristartech.com
*@date: 2012/06/05
*/
#ifndef __PKLINFO_H__
#include "PKLInfo.h"
#endif

int PKLInfo::num = 0;
PKLInfo::PKLInfo()
{
    num++;
    //std::cout << "contrator:num=" << num << std::endl; 
    id.clear();
    path.clear();
}
PKLInfo::~PKLInfo()
{
    num--;
    //std::cout << "decontractor:num=" << num << std::endl; 
}
