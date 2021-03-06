// RainDataSender.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include <WinSock2.h>
#include <iostream>
#include <WS2tcpip.h>
#include <boost/format.hpp>
#include <time.h>
#include <thread>
#include <chrono>
#include <ctime>

#pragma comment(lib, "Ws2_32.lib")

/**********************************************
编译配置项:
1: 编译生成向服务器发送数据的正式版本
0：编译生成向个人电脑发送数据的测试版本
**********************************************/
#define SendDataToServer		1 

#if SendDataToServer
//使用服务器IP地址
#define SERVER_IP		"192.168.0.55"
#define SERVER_PORT		8800
#define SEND_DATA_INTERVAL	15 * 60 //发送数据时间间隔,单位:秒
#define RECONNECT_INTERVAL	10 //重新连接服务器时间间隔，单位：秒
#else
//使用本人电脑IP地址
#define SERVER_IP		"192.168.108.2"
#define SERVER_PORT		8800
#define SEND_DATA_INTERVAL	5 //发送数据时间间隔,单位:秒
#define RECONNECT_INTERVAL	5 //重新连接服务器时间间隔，单位：秒
#endif

//WaterMeter.dll中读取水表数据的函数
typedef int (*ReadWaterMeterFun)(unsigned char addr, float * dat);
ReadWaterMeterFun ReadWaterMeter;

//数据结构体
struct RainDataStruct {
	float MeterIn;
	float MeterOut[6];
};

//socket RAII,自动释放socket资源
class sock_guard {
public:
	sock_guard(SOCKET sock)
	{
		m_sock = sock;
	}
	~sock_guard()
	{
		shutdown(m_sock, SD_BOTH);
		closesocket(m_sock);
//		std::cout << "sock_guard close the socket" << std::endl;
	}
private:
	SOCKET m_sock;
};

/***************************************************************************************
@Function: Send data using http protocol
@Param socket: Socket connected to server
@Param body: Message want to send
@Return: Result of send()
***************************************************************************************/
int http_send(SOCKET & socket, const std::string & body)
{
	std::string header("POST / HTTP/1.1\r\nConnection: Keep-Alive\r\n");
	header += boost::str(boost::format("Content-Length: %d\r\n\r\n") % body.size());
	header += body;
	std::size_t sum_bytes = 0;//发送成功的总字节数
	int count = 0;
	const char *pbuf = header.c_str();
	std::size_t size = header.size();	
	//发送数据,一次发送不完继续发送
	while (1)
	{
		count = send(socket, pbuf + sum_bytes, size - sum_bytes, 0);
		if (count > 0)
		{
			sum_bytes += static_cast<std::size_t>(count);
			if (sum_bytes >= size)
				return sum_bytes;
			continue;
		}
		else
			return count;			
	}	
}

/**************************************************************************************
@Function: Get current time
@Return: The time string formated like "yyyy-mm-dd hh:mm:ss"
**************************************************************************************/
std::string get_time()
{
	time_t timep;
	struct tm tmp;
	time(&timep);
	//gmtime_s(&tmp, &timep);//世界标准时间
	localtime_s(&tmp, &timep);//本地时间，VS2017要求使用的新函数，
	/*
	std::string str = boost::str(boost::format("%d-%02d-%02d %02d:%02d:%02d") \
		% (tmp.tm_year + 1900) % (tmp.tm_mon + 1) % tmp.tm_mday \
		% tmp.tm_hour % tmp.tm_min % tmp.tm_sec);	//蠢办法
	*/
	char str_time[20];
	//strftime(str_time, sizeof(str_time), "%Y-%m-%d %H:%M:%S", &tmp); //好办法
	strftime(str_time, sizeof(str_time), "%F %T", &tmp); //更好的方法
	return std::string(str_time);
}

/**********************************************************************************
//Data format
DataType=RainWater*MeterIn=452.3*MeterOut0=100.1*MeterOut1=112.2*MeterOut2=113.3*\
MeterOut3=114.4*MeterOut4=115.5*MeterOut5=116.6*
**********************************************************************************/
/*********************************************************************************
@Funtion: Format the data to a string
@Param dat: Pointer to the data struct;
@Return: The formated string
*********************************************************************************/
std::string format_msg(struct RainDataStruct * dat)
{
	std::string str("DataType=RainWater*");
	str += boost::str(boost::format("MeterIn=%.1f*") % dat->MeterIn);
	str += boost::str(boost::format("MeterOut0=%.1f*MeterOut1=%.1f*MeterOut2=%.1f*") \
		%dat->MeterOut[0] % dat->MeterOut[1] % dat->MeterOut[2]);
	str += boost::str(boost::format("MeterOut3=%.1f*MeterOut4=%.1f*MeterOut5=%.1f*") \
		%dat->MeterOut[3] % dat->MeterOut[4] % dat->MeterOut[5]);
	return str;
}


/***********************************************************************
@Function: Read data;
@Param dat: Pointer to the data struct;
@Return: Result of read, 0 for success, <0 for faild of the reason
************************************************************************/
//读取雨水回收系统各个水表的实时数据
int ReadRainData(RainDataStruct * dat)
{
	ReadWaterMeter(1, &(dat->MeterIn));	
	for (int i = 0; i < 6; i++)
	{
		ReadWaterMeter(2 + i, &(dat->MeterOut[i]));
	}	
	return 0;
}

/************************************************************************************************
@Function: Connect server and send data periodically
@Return: Result of connect or send when failed
*************************************************************************************************/
int connect_server()
{
	int err = 0;
	
	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN addrServer;
	//addrServer.sin_addr.S_un.S_addr = inet_addr( SERVER_IP );//VS2010写法	
	inet_pton(AF_INET, SERVER_IP, &(addrServer.sin_addr));//VS2017写法，要使用新函数，否刚报错	
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(SERVER_PORT);

	sock_guard sg(sockClient);//使用RAII自动释放socket资源

	//连接服务器
	int ret = connect(sockClient, reinterpret_cast<SOCKADDR*>(&addrServer), sizeof(addrServer));
	if (ret == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		std::cout << boost::str( boost::format("Connect server failed, err = %d, try to reconnect server later") % err ) << std::endl;		
		return -1;
	}
	std::cout << "Connect server (" << SERVER_IP << ":" << SERVER_PORT << ") success, ";
	std::cout << boost::str(boost::format("socket = %u") % sockClient)<< std::endl;
	RainDataStruct dat;
	std::string strMsg;
 //	while (1)
//	{
	ReadRainData(&dat);//读取实时数据
	strMsg = format_msg(&dat);//格式化数据
	std::cout << "【" << get_time() << "】:";
	ret = http_send(sockClient, strMsg);//使用HTTP协议发送数据		
	if (ret > 0)
	{
		std::cout << "Send " << ret << " bytes to server" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(SEND_DATA_INTERVAL));//C++延时方法
		//Sleep(SEND_DATA_INTERVAL * 1000);
		//continue;
		return ret;//发送一次就断开连接
	}
	else if (ret == SOCKET_ERROR)
	{
		err = WSAGetLastError();			
		if (err == WSAESHUTDOWN)
			std::cout << "Send data failed, socket has been shutdown, try to reconnect server later" << std::endl;
		else
			std::cout << boost::str(boost::format("Send data failed, err = %d, try to reconnect server later") % err) << std::endl;			
	}		
		return err;
//	}	
}

bool load_dll()
{
	HMODULE hMod = ::LoadLibraryA("WaterMeter.dll");
	if (hMod == NULL)
	{
		std::cout << "Load library WaterMeter.dll faild" << std::endl;
		return false;
	}
	ReadWaterMeter = reinterpret_cast<ReadWaterMeterFun>(GetProcAddress(hMod, "ReadWaterMeter"));
	if (ReadWaterMeter == NULL)
	{
		std::cout << "GetProcAddress ReadWaterMeter from WaterMeter.dll failed" << std::endl;
		return false;
	}
	std::cout << "Load library WaterMeter.dll success" << std::endl;
	return true;
}

int main(int argc, char* argv[])
{
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD(1, 1);
	int err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0)
	{
		std::cout << "WSAStartup error, please run this program again" << std::endl;
		return -1;
	}
	if (LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
	{
		WSACleanup();
		std::cout << "Requested version is not 1.1" << std::endl;
		return -2;
	}

	if (!load_dll())
		return -3;
	//出错后周期性的尝试连接服务器
	while (1)
	{
		connect_server();
		//Sleep(RECONNECT_INTERVAL * 1000);//连接失败后，重新连接服务器
		std::this_thread::sleep_for(std::chrono::seconds(RECONNECT_INTERVAL)); //重新连接服务器
	}
    return 0;
}

