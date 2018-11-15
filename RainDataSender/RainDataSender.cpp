// RainDataSender.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include <WinSock2.h>
#include <iostream>
#include <WS2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

/*****************************************
//服务器IP地址
#define SERVER_IP		"192.168.0.55"
#define SERVER_PORT		8800
*****************************************/

///*****************************************
//本人电脑IP地址
#define SERVER_IP		"192.168.108.2"
#define SERVER_PORT		18860
//*****************************************/

struct RainDataStruct {
	float MeterIn;
	float MeterOut[6];
}RainDataStruct;

int keep_connect()
{
	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
	SOCKADDR_IN addrServer;
	//addrServer.sin_addr.S_un.S_addr = inet_addr( SERVER_IP );
	struct in_addr s;	
	
	inet_pton(AF_INET, SERVER_IP, &(addrServer.sin_addr.S_un));
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(SERVER_PORT);

	int ret = connect(sockClient, reinterpret_cast<SOCKADDR*>(&addrServer), sizeof(addrServer));
	if (ret != 0)
	{
		std::cout << "Connect server failed!" << std::endl;
		return -1;
	}

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
	while (1)
	{
		keep_connect();
		Sleep(5000);//连接失败后，5秒钟重新连接一次服务器
	}
    return 0;
}

