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

// Select 모델 = (select 함수)

// socket set
// 1) 읽기[] 쓰기[] 예외[] 관찰 대상
// 2) select (readSet, writeSet, exceptSet ) -> 관찰 시작
// 3) 적어도 하나의 소켓 준비되면 리턴 -> 낙오자는 알아서 제거
// 4) 남은 소켓 체크해서 진행

// windows - select()의 한계
// 1. FD_SETSIZE 제한 - Windows에서는 기본적으로 64개의 소켓만 감시 가능(소켓 수 제한은 대규모 서버에선 치명적)
// 2. 선형 탐색 방식 - select()는 소켓을 선형으로 다 뒤지며 상태 확인 O(N) 비용이 매 호출 발생
// 3. 레벨 트리거 방식 - 이벤트가 계속 유지되면 계속 FD_ISSET()에 걸림 : 반복 처리 필요, 상태 기반이라 비효율
// 4. 모든 감시 집합을 매번 다시 세팅 : FD_SET은 입출력 감시마다 초기화 및 세팅 해야함

// IOCP - 진짜 비동기, 커널 통지 기반, 많은 스레드 감시 효율, 높은 확장성, O(1), 우선순위 스레드 큐 지우너 

const int32 BUF_SIZE = 1000;
/*
int select(
	int nfds,               // 사용하지 않음 (Linux용), Windows에서는 무시됨
	fd_set* readfds,        // 읽기 감시 대상
	fd_set* writefds,       // 쓰기 감시 대상
	fd_set* exceptfds,      // 예외 감시 대상
	const timeval* timeout  // 타임아웃 (NULL이면 무한 대기)
);
*/

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

	fd_set reads;
	fd_set writes;
	//fd_set은 소켓(SOCKET)의 집합을 표현하는 구조체 (소켓 수, 소켓배열)
	vector<Session>sessions;
	sessions.reserve(100);


	while (true) {
		
		// 소켓 셋 초기화
		FD_ZERO(&reads);
		//map<int,int>m; m.clear 과 유사 

		//listen소켓을 집합에 넣어 해당 소켓을 감시하도록 등록 (관찰대상으로)
		FD_SET(listenSocket, &reads);

		// 소켓 등록
		for (Session& s : sessions)
			FD_SET(s.socket, &reads);

		//[옵션] 마지막 timeout인지 설정 가능
		int32 retVal=::select(0, &reads, nullptr, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
			break;
		// select는 reads에 있는 소켓을 모두 검사, 읽을 수 있는 소켓만 reads에 남기고 나머지는 제거됨

		if (FD_ISSET(listenSocket, &reads)) 
		{ //listenSocket이 reads에 남아있는지 묻는 함수
		 // listenSocket에서 read는 사실상 accept이므로, 이게 남아있다면 
		 //
			SOCKADDR_IN clientAddr;
			int32 addrLen = sizeof(clientAddr);
			SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

			
		}


	}

	SocketUtils::Clear();
}