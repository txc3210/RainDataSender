// WaterMeter.cpp: 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

extern "C" int ReadWaterMeter(unsigned char addr, float *dat)
{
	switch (addr) 
	{
	case 1: *dat = 2501.0f; break; //自来水补水表
	case 2: *dat = 188.0f; break; //蓝球场西边草坪水表
	case 3: *dat = 467.0f; break; //蓝球场东边竹林水表
	case 4: *dat = 400.0f; break; //蓝球场南边草坪水表，没看见，估计值 
	case 5: *dat = 170.0f; break; //旗杆下绿化水表
	case 6: *dat = 321.0f; break; //双层停车架南侧水表
	case 7: *dat = 1200.0f; break; //喷泉水表，没安装，估计值 
	}	
	return 0;
}


