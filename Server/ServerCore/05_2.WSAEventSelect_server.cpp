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

// WSAEventSelect = WSAEventSelect가 핵심이 되는 (Windows 전용)
// 소켓과 관련된 네트워크 이벤트를 [이벤트 객체]를 통해 감지.
// 소켓마다 1개의 이벤트 객체 연결, 소켓과 이벤트는 1:1 매핑 구조
// 이벤트 발생 여부는 커널 레벨에서 감지
// 
// 클라이언트 접속 요청 : select(FD_ISSET(listenSocket)->accept()), WSAEventSelect(FD_ACCEPT이벤트 발생))
// 클라이언트 데이터 보냄 : select(FD_ISSET(sock)->recv(), WSAEventSelect(FD_READ 이벤트 발생)
// 클라이언트 연결 끊음 : select(recv()->0반환), WSAEventSelect(FD_CLOSE 이벤트 발생)
// 
// 생성 : WSACreateEvent (수동 리셋 Manual-Reset + Non-Signaled 상태 시작)
// 삭제 : WSACloseEvent 
// 신호 상태 감지 : WSAWaitForMultipleEvents
// 구체적인 네트워크 이벤트 알아내기 : WSAEnumNetworkEvents



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

	//논 블로킹
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
		// DummySession(listenSocket매핑) 을 추가. Session과 이벤트 1:1로 가져가는데 인덱스 맞추기 위함

		if (::WSAEventSelect(listenSocket, listenEvent, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR)
			return 0;

		while (true)
		{
			int32 index = ::WSAWaitForMultipleEvents(wsaEvents.size(), &wsaEvents[0], FALSE, WSA_INFINITE, FALSE);

			// 이벤트가 여러개 발생해도,다수를 감지하긴 하지만, 한번의 호출에서는 가장 먼저 발생한 하나의 이벤트의 인덱스 반환
			if (index == WSA_WAIT_FAILED)
				continue;

			index -= WSA_WAIT_EVENT_0;
			// 이걸 반환 index값에서 빼면 시작 위치 알 수 있음.
			// 배열에서의 이벤트의 시작 위치

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
			//Client Session 소켓 체크
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
		// 하나의 이벤트를 하나의 루프에서 처리하므로, select방식보다 느릴 수 있음


	}

	SocketUtils::Clear();
}