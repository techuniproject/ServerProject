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



int main()
{
	SocketUtils::Init();
	//// 1) 소켓 생성
	//WSADATA wsaData;
	//if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	//	return 0;
	

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
		return 0;//클라는 socket열고 connect인데, 서버 listen전 connect하면 클라 소켓 오류

	//클라가 connect 하고 서버가 accept나중에하면, 클라 커널이 3-way 핸드셰이크 후 커널의 완료 큐에서 대기
	// 서버가 먼저 accept 하고 클라가 connect안하면 커널의 완료 큐가 비었기 때문에 block 상태

	while (true) {
		//Non-Blocking 소켓의 비동기 accept 방식 (비동기: 함수 호출 후 곧바로 리턴, 완료는 나중/기다리지 않고 처리 후 완성을 나중에)
		SOCKADDR_IN clientAddr;
		int32 addrLen = sizeof(clientAddr);
		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		//여기선 원래 클라가 연결하길 기다리지만, 연결이 없으면 accept()는 INVALID_SOCKET을 리턴
		if (clientSocket == INVALID_SOCKET) {
			//원래 블로킹이 되어야 하는데, 논블로킹으로 설정했잖아
			// WSAGetLastError호출하면 WSAEWOULDBLOCK 오류 코드 나옴 이상황에서, 지금은 연결이 없으니 나중에 다시 시도하라는 의미.
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue; //현재 클라가 연결을 안했으면 이 오류가 뜸 논블로킹 소켓일 경우
		
		// 하지만 이 방식도 continue로 계속 cpu 타임슬라이스 동안 할당받아 수행하므로, 완전한 비동기는 아니고
	    // 타임슬라이스 끝날 때까지 계속 루프 / 효율 안좋음 / 스레드 스케줄링 계속 ready 상태
		// 블록 소켓은 블록한번되면 입출력 처리가 되기전까진 cpu할당 못받으니 절대적으로 cpu할당 계속 받는 
		// 논블로킹 루프형이 컨텍스트 스위칭 횟수가 많음
		}
		
		cout << "Client Connected!" << endl;

		//Recv
		while (true) {

			char recvBuffer[100];
			int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0); //최대 recvBuffer만큼 받을수있는
			//recvLen은 얼마만큼 커널 recvBuf에서 읽어왔는지
			
			if (recvLen == SOCKET_ERROR) {
				
				if(::WSAGetLastError()==WSAEWOULDBLOCK)
				continue;
				//뭔가 잘못되어 -1 반환되었으면 마찬가지로 원랜 블록이지만
				//아직 상대가 send를 하지 않아 대기하는게 아닌 그냥 넘어가게함
				
				//TODO
				break;
			}
			cout << "Recv Data : " << recvBuffer << endl;
			cout << "Recv Len : " << recvLen << endl;
		}
	}

	SocketUtils::Clear();
	//::closesocket(listenSocket);
	//::WSACleanup(); //WSAStartUp 호출 시 호출해야 하는 함수
}