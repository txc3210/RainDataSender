// WaterMeter.cpp: 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"

extern "C" int ReadWaterMeter(unsigned char addr, float *dat)
{
	*dat = 100.0f;
	return 0;
}


