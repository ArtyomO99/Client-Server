#define WIN32_LEAN_AND_MEAN
#define DEFAULT_BUFLET 1024

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <iostream>
#include <stdio.h>
#include <fstream>
#include <string>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

mutex mtx;

void char_to_string(char* ch, string str){
	str = string(ch);
}

void string_to_char(string str, char* ch)
{
	for (int i = 0; i < str.size(); i++)
		ch[i] = str[i];
	ch[str.size()] = '\0';
}

DWORD WINAPI recv_file_buf(LPVOID sock) {
	
	SOCKET ClientSock;
	ClientSock = ((SOCKET*)sock)[0];

	ofstream file_buf;
	string way = "/C++/Buffer/buffer.txt";
	file_buf.open(way, ios::app);

	if (!file_buf.good()) {
		cout << "Error open file" << endl;
		exit(1);
	}

	int result = 0;
	string str;
	char recvbuf[DEFAULT_BUFLET] = { 0 };


	while (true) {
		result = recv(ClientSock, &recvbuf[0], sizeof(recvbuf), 0);
		file_buf << recvbuf;
		break;
	}

	file_buf.close();

	mtx.lock();
	send(ClientSock, "Good bay", sizeof("Good bay"), 0);
	mtx.unlock();
	return 0;
}

int main() {

	WSADATA wsaData;
	ADDRINFO hints;
	ADDRINFO* addrResult = NULL;
	SOCKET ClientSock = INVALID_SOCKET;
	SOCKET ServerSock = INVALID_SOCKET;

	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, "22000", &hints, &addrResult);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	ServerSock = socket(addrResult->ai_family, addrResult->ai_socktype, addrResult->ai_protocol);
	if (ServerSock == INVALID_SOCKET) {
		cout << "socked failed" << endl;
		freeaddrinfo(addrResult);
		WSACleanup();
		return 1;
	}

	iResult = bind(ServerSock, addrResult->ai_addr, (int)addrResult->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed" << endl;
		closesocket(ServerSock);
		ServerSock = INVALID_SOCKET;
		freeaddrinfo(addrResult);
		WSACleanup();
		return 1;
	}

	iResult = listen(ServerSock, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		cout << "listen failed" << endl;
		closesocket(ServerSock);
		freeaddrinfo(addrResult);
		WSACleanup();
		return 1;
	}
	
	
	while (true) {
		ClientSock = accept(ServerSock, NULL, NULL);
		if (ClientSock == INVALID_SOCKET) {

			send(ClientSock, "accept failed\n", sizeof("accept failed\n"), 0);
			
		}
		
		DWORD threadID;
		CreateThread(NULL, NULL, recv_file_buf, &ClientSock, NULL, &threadID);
		
	}
	
	/*
	char recvbuf[DEFAULT_BUFLET];
	int recvbuflen = DEFAULT_BUFLET;
	int iSendResult = NULL;

	do {
		iResult = recv(ClientSock, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			cout << "recv succesful" << endl;
			cout << recvbuf << endl;

			iSendResult = send(ClientSock, recvbuf, iResult, 0);
			if (iSendResult == SOCKET_ERROR) {
				cout << "send fail" << endl;
				closesocket(ClientSock);
				freeaddrinfo(addrResult);
				WSACleanup();
				return 1;
			}
			cout << "Byte send: " << iSendResult << endl;

		}
		else if (iResult == 0)
			cout << "Connection closing..." << endl;
		else {
			cout << "recv fail" << endl;
			closesocket(ClientSock);
			freeaddrinfo(addrResult);
			WSACleanup();
			return 1;
		}

	} while (iResult > 0);

	closesocket(ClientSock);
	WSACleanup();
	*/


	closesocket(ServerSock);
	WSACleanup();
	return 0;
}

