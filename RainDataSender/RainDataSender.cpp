// RainDataSender.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include <WinSock2.h>
#include <iostream>
#include <WS2tcpip.h>
#include <boost/format.hpp>
#include <time.h>

#pragma comment(lib, "Ws2_32.lib")

/*****************************************
//服务器IP地址
#define SERVER_IP		"192.168.0.55"
#define SERVER_PORT		8800
*****************************************/

///*****************************************
//本人电脑IP地址
#define SERVER_IP		"192.168.108.2"
#define SERVER_PORT		18660
//*****************************************/

#define SEND_INTERVAL	5 * 1000

struct RainDataStruct {
	float MeterIn;
	float MeterOut[6];
}RainData;

int http_send(SOCKET & socket, const std::string & str)
{
	std::string header("POST / HTTP/1.1\r\nConnection: Keep-Alive\r\n");
	header += boost::str(boost::format("Content-Length: %d\r\n\r\n") % str.size());
	header += str;
	return send(socket, header.c_str(), header.size(), 0);
}

std::string get_time()
{
	time_t timep;
	struct tm *p = new tm;
	time(&timep);
	//p = gmtime(&timep);//VS2010旧写法
	//gmtime_s(p, &timep);//世界标准时间
	localtime_s(p, &timep);//本地时间，VS2017的新写法
	std::string str = boost::str(boost::format("%d-%02d-%02d %02d:%02d:%02d") \
		% (p->tm_year + 1900) % (p->tm_mon + 1) % p->tm_mday \
		% p->tm_hour % p->tm_min % p->tm_sec);
	delete p;
	return str;
}

/*
DataType=RainWater*MeterIn=452.3*MeterOut0=100.1*MeterOut1=112.2*MeterOut2=113.3*\
MeterOut3=114.4*MeterOut4=115.5*MeterOut5=116.6*
*/
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

int keep_connect()
{
	int err = 0;
	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN addrServer;
	//addrServer.sin_addr.S_un.S_addr = inet_addr( SERVER_IP );//VS2010写法	
	inet_pton(AF_INET, SERVER_IP, &(addrServer.sin_addr));//VS2017写法，要使用新函数，否刚报错	
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(SERVER_PORT);

	int ret = connect(sockClient, reinterpret_cast<SOCKADDR*>(&addrServer), sizeof(addrServer));
	if (ret == SOCKET_ERROR)
	{
		err = WSAGetLastError();
		std::cout << boost::str( boost::format("Connect server failed, err = %d") % err ) << std::endl;
		return -1;
	}
	std::cout << "Connect server (" << SERVER_IP << ":" << SERVER_PORT << ") success" << std::endl;
	
	while (1)
	{
		std::string strMsg = format_msg(&RainData);
		ret = http_send(sockClient, strMsg);
		if (ret > 0)
		{
			std::cout << "【" << get_time() << "】:Send " << ret << " bytes to server" << std::endl;
			Sleep(SEND_INTERVAL);
			continue;
		}
		else if (ret == SOCKET_ERROR)
		{
			err = WSAGetLastError();
			if (err == WSAESHUTDOWN)
				std::cout << "Socket has been shutdown" << std::endl;
		}
		return ret;
	}	
}

int main(int argc, char* argv[])
{
	RainData.MeterIn = 2570;
	RainData.MeterOut[0] = 100;
	RainData.MeterOut[1] = 112;
	RainData.MeterOut[2] = 211;
	RainData.MeterOut[3] = 333;
	RainData.MeterOut[4] = 144;
	RainData.MeterOut[5] = 55;

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
	while (1)
	{
		keep_connect();
		Sleep(5000);//连接失败后，5秒钟重新连接一次服务器
	}
    return 0;
}

