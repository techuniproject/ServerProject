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
#include "SocketUtils.h"

// WSAEventSelect = WSAEventSelect�� �ٽ��� �Ǵ� (Windows ����)
// ���ϰ� ���õ� ��Ʈ��ũ �̺�Ʈ�� [�̺�Ʈ ��ü]�� ���� ����.
// ���ϸ��� 1���� �̺�Ʈ ��ü ����, ���ϰ� �̺�Ʈ�� 1:1 ���� ����
// �̺�Ʈ �߻� ���δ� Ŀ�� �������� ����
// 
// Ŭ���̾�Ʈ ���� ��û : select(FD_ISSET(listenSocket)->accept()), WSAEventSelect(FD_ACCEPT�̺�Ʈ �߻�))
// Ŭ���̾�Ʈ ������ ���� : select(FD_ISSET(sock)->recv(), WSAEventSelect(FD_READ �̺�Ʈ �߻�)
// Ŭ���̾�Ʈ ���� ���� : select(recv()->0��ȯ), WSAEventSelect(FD_CLOSE �̺�Ʈ �߻�)
// 
// ���� : WSACreateEvent (���� ���� Manual-Reset + Non-Signaled ���� ����)
// ���� : WSACloseEvent 
// ��ȣ ���� ���� : WSAWaitForMultipleEvents
// ��ü���� ��Ʈ��ũ �̺�Ʈ �˾Ƴ��� : WSAEnumNetworkEvents



const int32 BUF_SIZE = 1000;

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUF_SIZE] = {};
	int32 recvBytes = 0;
};


int main()
{
	SocketUtils::Init();

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return 0;

	//�� ���ŷ
	u_long on = 1;
	if (::ioctlsocket(listenSocket, FIONBIO, &on) == INVALID_SOCKET)
		return 0;

	SocketUtils::SetReuseAddress(listenSocket, true);

	if (SocketUtils::BindAnyAddress(listenSocket, 7777) == false)
		return 0;

	if (SocketUtils::Listen(listenSocket) == false)
		return 0;


	vector<WSAEVENT> wsaEvents;
	vector<Session>sessions;
	sessions.reserve(100);


	while (true) {

		WSAEVENT listenEvent = ::WSACreateEvent();
		wsaEvents.push_back(listenEvent);
		sessions.push_back(Session{ listenSocket });
		// DummySession(listenSocket����) �� �߰�. Session�� �̺�Ʈ 1:1�� �������µ� �ε��� ���߱� ����

		if (::WSAEventSelect(listenSocket, listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
			return 0;

		while (true)
		{
			int32 index = ::WSAWaitForMultipleEvents(wsaEvents.size(), &wsaEvents[0], FALSE, WSA_INFINITE, FALSE);

			// �̺�Ʈ�� ������ �߻��ص�,�ټ��� �����ϱ� ������, �ѹ��� ȣ�⿡���� ���� ���� �߻��� �ϳ��� �̺�Ʈ�� �ε��� ��ȯ
			if (index == WSA_WAIT_FAILED)
				continue;

			index -= WSA_WAIT_EVENT_0;
			// �̰� ��ȯ index������ ���� ���� ��ġ �� �� ����.
			// �迭������ �̺�Ʈ�� ���� ��ġ

			WSANETWORKEVENTS networkEvents;
			if (::WSAEnumNetworkEvents(sessions[index].socket, wsaEvents[index], &networkEvents) == SOCKET_ERROR)
				continue;

			if (networkEvents.lNetworkEvents & FD_ACCEPT)
			{
				//Error-Check
				if (networkEvents.iErrorCode[FD_ACCEPT_BIT] != 0)
					continue;

				SOCKADDR_IN clientAddr;
				int32 addrLen = sizeof(clientAddr);
				SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

				if (clientSocket != INVALID_SOCKET)
				{
					if (::WSAGetLastError() == WSAEWOULDBLOCK)
						continue;

					cout << "Client Connected" << endl;
					WSAEVENT clientEvent = ::WSACreateEvent();
					wsaEvents.push_back(clientEvent);
					sessions.push_back(Session{ clientSocket });

					if (::WSAEventSelect(clientSocket, clientEvent, FD_READ | FD_WRITE | FD_CLOSE) == SOCKET_ERROR)
						return 0;
				}
			}
			//Client Session ���� üũ
			if (networkEvents.lNetworkEvents & FD_READ)
			{
				if (networkEvents.iErrorCode[FD_READ_BIT] != 0)
					continue;

				Session& s = sessions[index];

				//Read
				int32 recvLen = ::recv(s.socket, s.recvBuffer, BUF_SIZE, 0);
				if (recvLen == SOCKET_ERROR && ::WSAGetLastError() != WSAEWOULDBLOCK)
				{
					if (recvLen <= 0)
						continue;
				}
				cout << "RecvData = " << s.recvBuffer << endl;
				cout << "RecvLen = " << recvLen << endl;
			}
		}
		// �ϳ��� �̺�Ʈ�� �ϳ��� �������� ó���ϹǷ�, select��ĺ��� ���� �� ����


	}

	SocketUtils::Clear();
}