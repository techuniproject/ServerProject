#include "pch.h"
#include <iostream>
#include <thread>
#include <vector>
using namespace std;
#include <atomic>
#include <mutex>
#include <windows.h>
#include "TestMain.h"
#include "ThreadManager.h"

//UDP ����
// 1) ���ο� ���� ����
// 2) ���Ͽ� IP�ּ�/ ��Ʈ��ȣ ����(bind)
// -------
// 3) Ŭ��� ���

int main()
{
	// 1) ���� ����
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;


	// ipv4 ����, TCP ���
	SOCKET listenSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (listenSocket == INVALID_SOCKET) // ���� ��ȯ���� �ƴ϶�� ���
		return 0;



	// 2) �ּ�/��Ʈ ��ȣ ���� (bind)
	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr)); //0���� �о� �ʱ�ȭ
	serverAddr.sin_family = AF_INET; //���� �ּ��� �ּ� �йи�(ü��) : �׻� AF_INET(IPv4)���� ����
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY); // 0.0.0.0 ������ ������ �ִ� ��� IP�ּҿ����� ��û�� �޴� ��
	serverAddr.sin_port = ::htons(7777); // 80������ HTTP�� ��� �� - ����� ��Ʈ ��ȣ�� ���� -7777

	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;


	//// 3) ���� ���� (listen)
	//if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	//	return 0;
	//TCP���� Ŭ���� listening ������ ������ ��ٸ�����, UDP�� ����

	while (true)
	{
		SOCKADDR_IN clientAddr;
		::memset(&clientAddr, 0, sizeof(clientAddr));
		int32 addrLen = sizeof(clientAddr); //��� Ŭ���� �ּҸ� �����ϱ� ����

		//SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		//if (clientSocket == INVALID_SOCKET)
		//	return 0;

		////----������� ���� ��� Ŭ��� ���� ����� ���� ������� ��� Ŭ��� ��� ���� ����

		//char ip[16];
		//::inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip)); // ip���� �Լ�
		//cout << "Client Connected! IP = " << ip << endl;
		//TCP���� connect ��� �δ� ���� UDP���� ����


		// ��Ŷ
		char recvBuffer[100];


		int32 recvLen = ::recvfrom(listenSocket, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&clientAddr, &addrLen);
		if (recvLen <= 0)
			return 0;

		//int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
		//if (recvLen <= 0)
		//	return 0; //TCP����

		cout << "Recv Data : " << recvBuffer << endl;
		cout << "Recv Data Len: " << recvLen << endl;

		this_thread::sleep_for(chrono::seconds(1));

	}

	::closesocket(listenSocket);
	::WSACleanup(); //WSAStartUp ȣ�� �� ȣ���ؾ� �ϴ� �Լ�
}