// ����
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

int main()
{
	// 1) ���ο� ���� ���� (socket)				- ex) ���� �ڸ� ��ġ
	// 2) ���Ͽ� �ּ�/��Ʈ ��ȣ ���� (bind)		- ex) ���� ����
	// 3) ���� �� ��Ű�� (listen)				- ex) ���� �ٹ� ����(���� ����)
	// 4) �մ� ���� (accept)					- ex) �մ� ����
	// 5) Ŭ��� ���							- ex) ���� ����

	// ����
	// ��Ʈ��ũ ����� �����ִ� �ڵ���
	
	// ��ε�ĳ����
	// ������ ������ �ִ� ĳ���� �ϳ������� ��Ŷ�� ���ִ� ��

	WSADATA wsaData;
	// == 0x202

	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // �ʱ�ȭ
		return 0;

	// MAKEWORD
	// 1����Ʈ 1����Ʈ�� ���̿� �ο�� ��� �ϳ��� WORD�� ����.

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	// ipv4 �������� TCP ������� ����
	// � ������ ����� ������ ������.
	// 
	// ���� ��(SOCKET): Ŀ�ο��� �����ϴ� ���ҽ��� ���� ��ȣ�� �ο��� ��
	// �ش� ������ ������� ���𰡸� ��û�� �� �ش� ������ ����Ѵٴ� ����

	// 1) ���� ����
	// ad : Address Family (AF_INET = IPv4, AF_INET6 = IPv6)
	// type : TCP(SOCK_STREAM) vs UDP(SOCK_DGRAM)
	// protocol: 0
	// return : descriptor
	// int32 errorCode = ::WSAGetLastError();
	
	// 2) �ּ�/��Ʈ ��ȣ ���� (bind)
	// ���� == �ڵ���(��Ʈ��ũ ����� �����ִ�)
	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY); // � IP �ּҸ� �����ϰ� ������(INADDR_ANY: ���X)
	serverAddr.sin_port = ::htons(7777); // 80 : HTTP, ������� ���� ���ڷ� ����

	// ��Ʋ ����� - ���� ȯ�濡�� ǥ��
	// �� ����� - ��Ʈ��ũ �󿡼� ǥ��

	// ex) 0x12345678
	// low [0x78][0x56][0x34][0x12] high << little
	// low [0x12][0x34][0x56][0x78] high << big
	=> htonl(host to network long), htons(host to network short) ��� - ������ ȯ������ �����ֱ� ���� ���
	// ��Ʈ��ũ ǥ�ؿ� �´� ������� �ٲ��ִ� �Լ�

	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;
	// ��� �ּҸ� ���ε�(����) - IP�ּҿ� ��Ʈ ��ȣ ����

	// 3) ���� ���� (listen)
	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		return 0;
	// SOMAXCONN: ����� �� �ִ� ���� ���� �ɼ� - �ִ�ġ

	// 4)
	while (true)
	{
		SOCKADDR_IN clientAddr;
		::memset(&clientAddr, 0, sizeof(clientAddr));
		int32 addrLen = sizeof(clientAddr);

		// ���� �� ����
		// accept�� ������ �մ��� ������ ����
		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
			return 0;

		char ip[16];
		::inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
		cout << "Client Connected! IP = " << ip << endl;

		while (true)
		{
			// ��Ŷ
			char recvBuffer[100];

			// recv: �޼����� �޴� �Լ�
			int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
			if (recvLen <= 0)
				return 0;

			cout << "Recv Data : " << recvBuffer << endl;
			cout << "Recv Data Len: " << recvLen << endl;

			// ���� ���� - ������ ������ �޼����� �״�� �佺 (�ſ�)
			// ���� ���� �ٽ� send
			int32 resultCode = ::send(clientSocket, recvBuffer, recvLen, 0);
			if (resultCode == SOCKET_ERROR)
				return 0;
		}
	}

	::WSACleanup();
	// �ڵ尡 ������ ��Ī������ ȣ�����־�� ��.
}

// ���� ����ϴ� ����� �����͸� ������ �ʴ´ٸ� ������ ���߰� �ȴ�.


// Ŭ�� - ���� �� ¦�� ���� ���� ���
// send <-> recv
// ex) �������� send�� �ϰ� Ŭ���̾�Ʈ���� recv�� ���� ���� ���
// => ���: send�� ����!

// Ŀ�� ���ο��� �� ������ ���������� recv, send Buffer�� �����ϰ� ����.
// �ڽ��� ���ۿ��ٰ� �ش� ������ �����ϴ� ���� �����̶�� �ν�, �� ���Ĵ� �ü���� ó����.
// Send ���� == Send Buffer�� �����͸� �����ϴ� ��
// Receive ���� == Receive Buffer�� �����Ͱ� ������ �����ؼ� ������ ���� ��
// Send ���� == ���� ���۰� ���� (ex) Receive�� ���� �ʾƼ�)