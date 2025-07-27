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
// 저번과 다른 점 : select방식으로 다수의 소켓(클라)을 한 스레드에서 감지하고 순차적으로 처리
//                   가능함.
// select() 방식과 WSAEventSelect() 방식
/*
		항목			|		select()			|			WSAEventSelect()
	감시 방식			|	fd_set 소켓 집합 감시	|	WSAEvent 객체 기반 이벤트 감시
 감시할 수 있는 이벤트  |	읽기/쓰기/예외 (한정적)	|	다양한 이벤트 지정 가능 (접속/수신/닫힘 등)
	소켓 수 제한		|	64개 제한 (FD_SETSIZE)	|	제한 없음 (Windows 제한 내에서는 사실상 무제한)
	이벤트 감지 대상	|	소켓 중심				|	WSAEvent 중심 (소켓 ↔ 이벤트 매핑)
	성능				|	소켓 수 많아지면 느림	|	효율은 괜찮지만 복잡
	적합한 환경			|	간단한 서버				|	Windows에서의 확장성 있는 구조 필요 시

*/

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

/*
항목	설명
✅ 장점
구현이 단순하고 직관적, 테스트와 학습에 적합
❌ 단점
매 루프마다 fd_set 초기화 & 등록 필요 → 성능 저하
소켓 수가 많아지면 순회 성능 하락
소켓 수 제한 (FD_SETSIZE)
🧩 대안
epoll (Linux), kqueue (BSD), IOCP (Windows) 등 고성능 이벤트 기반 API 사용

1. listenSocket을 fd_set(reads)에 등록
connect() 요청이 오면 listenSocket이 readable로 바뀜

FD_ISSET() 검사 후 accept() 호출 가능

2. accept()를 통해 클라이언트 소켓 획득
새로운 clientSocket을 Session 등으로 벡터에 저장

이 시점부터 클라이언트 감시 시작

3. 다음 루프에서 클라이언트 소켓을 reads에 등록
FD_SET(clientSocket, &reads);

클라이언트가 데이터를 보내면 → select가 그것을 감지

FD_ISSET()을 통해 recv할 준비가 되었음을 확인

4. 순차적으로 recv()
select 결과에서 읽을 수 있는 소켓만 추출된 상태

순회하면서 해당 소켓들에 대해 recv() 호출

→ 순차적 recv 구조

*/

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

	//fd_set은 소켓(SOCKET)의 집합을 표현하는 구조체 (소켓 수, 소켓배열)
	fd_set reads;
	// reads는 소켓에 읽을 수 있는 데이터가 있는지 감시하는 집합
	// 서버의 listening 소켓에 클라 연결 시, 클라이언트 소켓이 send로 데이터 보낼 시 서버가 recv호출 시 데이터가 있을 때
	// 클라이언트가 연결을 끊는 경우 recv호출 시 0바이트 반환 -> 연결이 정상적으로 종료됨을 의미.
	fd_set writes;
	// writes는 소켓에 데이터를 안전하게 send()할 수 있는지 확인하기 위해
	// 논블로킹 소켓에서 send()호출시, 상대 버퍼 가득 차거나, 네트워크 혼잡, 내 전송 버퍼 꽉찰 경우 WSAEWOULDBLOCK오류 반환
	//  또는 보낼 데이터가 있을 때만 감시 대상에 등록할때, 출력 버퍼 자체 관리


	vector<Session>sessions;
	sessions.reserve(100);


	while (true) {

		// 소켓 셋 초기화
		FD_ZERO(&reads);
		//map<int,int>m; m.clear 과 유사 

		//listen소켓을 집합에 넣어 해당 소켓을 감시하도록 등록 (관찰대상으로)
		FD_SET(listenSocket, &reads);
		//listenSocket이 readable하다는 건 새 클라이언트가 접속 요청(connect())을 보냈다는 것 

		// 소켓 등록
		// 새로운 클라이언트 소켓은 accept()을 한 뒤에야 우리가 알 수 있어 
		// 리스닝 소켓에 연결이 들어오고 우리가 그 클라 정보 따오면 벡터에 넣어 관리하다
		// 데이터를 recv하게 됨
		for (Session& s : sessions)
			FD_SET(s.socket, &reads);

		//[옵션] 마지막 timeout인지 설정 가능
		int32 retVal = ::select(0, &reads, nullptr, nullptr, nullptr);
		if (retVal == SOCKET_ERROR)
			break;
		// select는 reads에 있는 소켓을 모두 검사, 읽을 수 있는 소켓만 reads에 남기고 나머지는 제거됨
		// 커널에 요청해서 등록된 소켓들 중 어떤게 이벤트 발생했는지 확인.
		// 리스닝 소켓은 클라가 connect요청을 보내면 readable이 되어 reads에 남게됨.

		if (FD_ISSET(listenSocket, &reads))
		{ //listenSocket이 reads에 남아있는지 묻는 함수
		 // listenSocket에서 read는 사실상 accept이므로, 이게 남아있다면 
		 // select에 의해 이미 readable로 인정되어 넘어왔다는것
			SOCKADDR_IN clientAddr;
			int32 addrLen = sizeof(clientAddr);
			SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
			//이미 FD_ISSET으로 reads에서 읽었는지 확인했으므로 accept의 반환이 보장됨

			if (clientSocket != INVALID_SOCKET)//하지만 다시 체킹과정
			{
				if (::WSAGetLastError() == WSAEWOULDBLOCK)
					continue;

				cout << "Client Connected" << endl;
				sessions.push_back(Session{ clientSocket });

			}
		}

		// 나머지 소켓 체크

		for (Session& s : sessions) {
			if (FD_ISSET(s.socket, &reads))
			{
				int32 recvLen = ::recv(s.socket, s.recvBuffer, BUF_SIZE, 0);
				if (recvLen <= 0)
				{
					continue;
				}
				cout << "RecvData = " << s.recvBuffer << endl;
				cout << "RecvLen = " << recvLen << endl;
			}
		}

	}

	SocketUtils::Clear();
}