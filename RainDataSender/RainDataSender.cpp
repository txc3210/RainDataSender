// RainDataSender.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <stdio.h>
#include <WinSock2.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")


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
		
	SOCKET sockClient = socket(AF_INET, SOCK_STREAM, 0);
    return 0;
}

