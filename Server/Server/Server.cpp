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



/*
[Overlapped IOCP 방식]

[1] CreateIoCompletionPort -> IOCP 큐 생성 및 소켓 등록
[2] WSARecv / WSASend -> 비동기 I/O 요청 시작
[3] GetQueuedCompletionStatus -> I/O 완료 통지 대기
[4] 콜백 아님 - 워커 스레드가 완료 정보 직접 처리


[전체 흐름]
1. IOCP 준비 단계 서버 초기화

	HANDLE iocpHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

- IOCP 큐 생성
- 이후 소켓들과 워커 스레드를 이 큐에 연결해서 통합 I/O 처리

2. 워커 스레드 풀 생성

	for (int i = 0; i < numThreads; ++i)
		GThreadManager->Launch([=]() { WorkerThreadMain(iocpHandle); });

- **GetQueuedCompletionStatus()**로 큐 대기하는 워커 스레드 N개 생성
- 각 워커 스레드는 I/O 완료 이벤트가 큐에 쌓일 때까지 블로킹(waiting) 상태
- 이 대기는 CPU를 소모하지 않음 → 매우 효율적

3. 클라이언트 소켓 연결 및 IOCP 등록

	CreateIoCompletionPort((HANDLE)clientSocket, iocpHandle, (ULONG_PTR)session, 0);

- 새로운 클라이언트 소켓을 IOCP 큐에 등록
- completionKey로 Session*을 등록 → I/O 완료 시 식별 용도

4. 비동기 I/O 요청 - WSARecv

	WSARecv(session->socket, ..., &overlappedEx->overlapped, NULL);

- 비동기 recv() 요청 시작
- **overlapped 구조체(OverlappedEx)**에 I/O 작업 정보를 포함해서 넘김
- 콜백은 등록하지 않음(null) → IOCP 방식은 콜백 대신 큐 기반 통지

5. I/O 완료 → 커널이 IOCP 큐에 완료 정보 삽입

- 소켓의 recv/send 등 I/O 작업이 커널에서 완료되면
- 커널은 IOCP 큐에 다음 정보를 등록함:
- completionKey: Session*
- OVERLAPPED*: OverlappedEx*
- bytesTransferred: 전송된 바이트 수
- 이건 커널 레벨에서 자동으로 발생

6. 워커 스레드가 큐에서 완료 꺼내서 처리

	GetQueuedCompletionStatus(iocpHandle, &bytesTransferred, (PULONG_PTR)&session, (LPOVERLAPPED*)&overlapped, INFINITE);

- 워커 스레드가 완료된 작업을 큐에서 꺼냄
- session, overlapped 정보를 이용해 처리 진행
- 처리 후 필요 시 다음 I/O 요청(WSARecv 등)을 다시 등록함

7. 반복 구조

- I/O 요청 (WSARecv)
- 완료되면 커널이 IOCP 큐에 결과 등록
- 워커 스레드가 꺼내서 처리
- 다시 I/O 요청 등록
- 이 구조를 통해 논리적으로는 "연속된 I/O"처럼,
물리적으로는 비동기 이벤트 기반으로 고성능 처리 가능



1) 스레드 깨우기 방식

- 커널이 I/O 완료 정보를 IOCP 큐에 넣음
- 큐를 대기하던 워커 스레드가 깨서 처리
- N개의 워커 스레드 -> M개의 I/O처리 가능 (저번 방식은 한번에 메인스레드만 콜백 받는 형태)

2) 스레드 재사용 구조

- 여러 워커 스레드가 GQCS 대기
- 워커 스레드에서 직접 분기 처리
- 우리가 직접 컨트롤 (스레드 재사용, 구조 확장 이유)

3) 메모리 구조 통합 및 캐스팅 비용

- OVERLAPPED* 를 reinterpret_cast하여 구조체 복원
- IOCP로 추가적인 completionKey를 받을 수 있어 유연함
- GetQueuedCompletionStatus()에서 바로 session을 받아서 접근 가능

4) 컨텍스트 스위칭 비용

- 워커 스레드가 이미 실행 중이므로 단순 함수 호출로 부하가 적음
- 불필요한 커널 <-> 사용자 영역 전환이 줄어듦 콜백, 이벤트 방식에 비해


[핵심 요점 7줄 정리]
IOCP 큐는 비동기 작업 완료 통지를 커널이 넣고, 우리가 꺼내는 구조다.

WSARecv/WSASend 등은 OVERLAPPED만 넘기면 커널이 완료 시 IOCP에 등록해준다.

완료 처리는 콜백이 아닌, 워커 스레드가 GetQueuedCompletionStatus()로 직접 받는다.

completionKey와 OVERLAPPED*를 통해 필요한 모든 정보를 되돌릴 수 있다.

워커 스레드는 작업 없을 땐 CPU를 전혀 소모하지 않고 대기한다.

수천 개의 소켓도 워커 스레드 4~8개로 효율적으로 처리 가능하다.

IOCP는 고성능 서버, MMO, 고속 통신에 최적화된 I/O 모델이다.

*/







const int32 BUF_SIZE = 1000;

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUF_SIZE] = {};
	int32 recvBytes = 0;
};

enum IO_TYPE
{
	READ,
	WRITE,
	ACCEPT,
	CONNECT,
};

struct OverlappedEx
{
	// 이 기법으로 핸들(포인터) 하나로 정보를 통합할 수 있음 (외부 자료구조 없이)
	// 캐시 효율이 좋음 overlapped의 주소하나로 buffer, socket등 인접한 메모리 접근하므로
	// reinterpret_cast는 단순 타입 해석이라 비용이 없음
	// 메모리 크기 차원에선 이미 할당해놓고 WSAOVERLAPPED의 주소로 해당 정보 단위를 해석하므로
	// 메모리 크기 비용에선 큰 의미가없지만, 이런 정보 단위를 해석하는 강점으로 다른 자료구조 없어 메모리 절약
	WSAOVERLAPPED overlapped = {};
	int32 type = 0;
	//TODO

};

/*
WSAOVERLAPPED를 상속으로 관리 안하는 이유
-> C++에서는 구조체의 첫번째 멤버의 주소가 구조체 자체의 주소와 동일하다고 보장함. 하지만 상속은 다름

Base가 있고 Derived가 있으면 Base영역과 Derived 영역의 메모리 레이아웃 순서는 컴파일러가 결정
항상 Base가 앞에 온다는 보장이 없다.
따라서 Base* -> Derived*로 무작정 reinterpret_cast해서 Derived 멤버를 접근하면 Base영역을 포함하지 않는 위험한 영역 침범 가능

단일 상속일땐 보통 드물지만, 다중 상속일 경우 흔할 수도 있음.

메모리 레이아웃을 바꾸더라도 멤버 접근에는 문제 없는 이유는
컴파일러가 모든 타입의 레이아웃과 오프셋을 알고있고, 상속 시 자동으로 오프셋을 조정해줌
하지만 reinterpret_cast는 우리가 직접 주소 건들여서 사용하니까 오프셋 조정이 없음
*/

/*
	Session은 클라이언트 단위로 고정됨
	-그래서 메인스레드에서 새로 동적할당함 클라와 연결될때마다

	Overlapped는 I/O요청 단위로 생성되고 재사용 가능




*/


void WorkerThreadMain(HANDLE iocpHandle)
{
	while (true)
	{
		//TODO

		//GQCS
		DWORD bytesTransferred = 0;
		Session* session = nullptr;
		OverlappedEx* overlappedEx = nullptr;
		// 워커 스레드는 I/O완료가 큐에 없으면 블로킹 되어 CPU소모안하고
		// sleep상태에 가까운 효율적인 상태로 대기하며 CPU 리소스 거의 소모 안함
		// 이벤트 기반 wait함수이므로 스레드는 wait상태로 전환되고 해당 IOCP 큐에 작업 생기면 커널이 이벤트 발생시켜 깨운다.
		bool ret=::GetQueuedCompletionStatus(iocpHandle, &bytesTransferred, (ULONG_PTR*)&session, (LPOVERLAPPED*)&overlappedEx, INFINITE);
		//WSARecv가 완료되면 이 코드가 성공적으로 진행될것임
		// 사용자 레벨에서 key와 overlapped 정보를 넣어줄 수 있어 이를 잘 활용하여야 함
		// CreateIoCompletionPort에서 넣어준 key와 WSARecv에 넣어준 overlapped포인터 값으로 
		// GetQueuedCompletionStatus에서 다시 ULONG_PTR* session과 LPOVERLAPPEd* overlappedEx로 반환해서 구분용으로 사용
		// Overlapped에 새로운 구조체로 우리가 원하는 정보를 추가하여 그걸 반환받았을 때 다양한 정보창구로 사용
		// 이 함수 덕에 IOCP 구조는 I/O 이벤트가 많든, 워커 스레드가 많든 관계없이 스레드 세이프함이 보장됩니다.
		
		// SESSION에 CreateIoCompletionPort 때 넘겨준 key값 그대로 반환
		// overlappedEx -> WSARecv()에 넘긴 포인터 그대로 반환
		// bytesTransferred -> 클라이언트가 보낸 실제 바이트 수
		// 큐에서 해당 이벤트 제거 및 이 시점부턴 이미 I/O완료된 상태
		// 그러므로 우리는 이 session 및 overlapped에 대해 다시 I/O요청을 보낼 수 있음.
		if (ret == false || bytesTransferred == 0)
			continue;
		

		cout << "Recv Data Len = " << bytesTransferred << endl;
		cout << "Recv Data IOCP = " << session->recvBuffer << endl;

		//Thread Safe한가?
		// Recv하는거에 대해선 우리가 WSARecv를 한번 예약을 하였으므로
		// 입출력 요청이 하나 들어감이 보장되어 하나의 스레드만 처리하는 것이 보장되어 스레드 세이프하다
		// 이후 해당 처리 완료를 한 스레드가 다시 I/O요청을 하여 무한 반복하는 구조이다.
		// GetQueuedCompletionStatus가 I/O 완료 알림 1건당 단 하나의 스레드만 깨워 처리하기 때문에 Safe함

		// 그리고 여러 클라이언트가 동시에 접속하면 메인스레드에서 이미 그 클라에 대한
		// 개별 세션 버퍼와 overlapped를 활용하여 재사용하며 계속 io예약하므로 병렬적으로
		// 여러 클라에 대한 처리가 가능함.
		
		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUF_SIZE;


		// 이 코드는 왜 있을까?
		// 우리가 지속적으로 스레드가 I/O작업을 클라로부터 처리하기 위해서는
		// 다시 I/O 요청 예약을 보내야만 하기 때문임 (반복을 위해)
		DWORD recvLen = 0;
		DWORD flags = 0;
		::WSARecv(session->socket, &wsaBuf, 1, &recvLen, &flags,/*중요!*/&overlappedEx->overlapped, NULL);

	
	
	}
}

int main()
{
	SocketUtils::Init();

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return 0;

	// iocp 방식으로 소켓 등록되면 자동으로 논블로킹으로 되어 따로 논블로킹 설정 안해도됨

	SocketUtils::SetReuseAddress(listenSocket, true);

	if (SocketUtils::BindAnyAddress(listenSocket, 7777) == false)
		return 0;

	if (SocketUtils::Listen(listenSocket) == false)
		return 0;

	HANDLE iocpHandle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL,0,0);
	// 작업들 큐를 만들어주는것 - CP

	//WorkerThreads
	for (int32 i = 0; i < 5; ++i)
		GThreadManager->Launch([=]() {WorkerThreadMain(iocpHandle); });

	vector<Session*> sessionManager;

	//MainThread - accept 담당
	// 내가 현재 이해한 흐름)
	// 클라는 네트워크 통신을 위해 connect를 작성해야하므로 서버는 accept를 통해 클라의 정보를 따옴
	// 그 시간 차는 블로킹 방식으로 서버가 대기할 수 있음
	// 이후, 클라의 요청이 들어오면 해당 클라에 맞춤으로 1:1로 가져가야하는 세션을 만들고 그 정보에 클라 소켓을 등록
	// 이후 그걸 Iocp에 등록하고 구별 용 key를 등록하여 해당 소켓을 예약
	// 이후, 하나의 입출력 작업에 비례하여 필요한 overlapped를 만들고, 입출력 요청 예약을함
	// 각 워커스레드가 이후엔 해당 반환 세션을 기반으로 다시 해당 클라의 io작업을 다루고,
	// overlapped는 io작업이 완료된 이후 재사용 가능하므로 지속적으로 처리하며 모든 클라의
	// 통신을 무한루프를 돌며 connect요청과 비례한 바퀴수 돌면 그 이후엔 병렬적으로 워크 스레드를 통해
	// 동시다발적으로 처리가능.
	// 
	while (true)
	{
		SOCKADDR_IN clientAddr;
		int32 addrLen = sizeof(clientAddr);

		// 아직 블로킹 방식 함수 accept로 하지만, 나중에 바꿀것임
		// 클라 connect까지 대기하므로 무한루프 진행 안됨
		SOCKET clientSocket=::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
			return 0;

		Session* session = new Session(); //새로 들어오는 클라에 대한 정보를 담는 Session
		session->socket = clientSocket;
		sessionManager.push_back(session);

		cout << "Client Connected" << endl;

		//[KEY 사용]!!!!
		::CreateIoCompletionPort(HANDLE(clientSocket), iocpHandle,/*key*/(ULONG_PTR)session, 0);
		//key로 고유한 session의 주소값 넣음

		WSABUF wsaBuf;
		wsaBuf.buf = session->recvBuffer;
		wsaBuf.len = BUF_SIZE;

		OverlappedEx* overlappedEx = new OverlappedEx();
		overlappedEx->type = IO_TYPE::READ;

		DWORD recvLen = 0;
		DWORD flags = 0;
		// IO 예약을 완료, 클라가 데이터 보낼떄까지 실질적으론 아무 일도 안일어남
		::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags,/*중요!*/&overlappedEx->overlapped, NULL);
		// 앞으로 우리가 clientSocket을 큐에 등록하였으므로 IO가 완료되면 큐에 들어가 우리가 사용할 수 있음
		// 큐에 들어간 것을 인지하기 위해선 원래 이벤트나 콜백으로 알아왔지만, 이젠 워커스레드를 배치하여 
		// 그 스레드에서 처리하도록함 WorkerThreadMain함수에서 (각 워커 스레드가 실행하는것)

		/*
		[같은 Session에 중복 WSARecv 요청 문제]

		// 첫 번째 요청
		WSARecv(s->socket, &buf, 1, &recvLen, &flags, &s->overlapped, NULL);

		// 아직 완료되지 않았는데...

		// 두 번째 요청 (같은 Session, 같은 버퍼, 같은 overlapped로!)
		WSARecv(s->socket, &buf, 1, &recvLen, &flags, &s->overlapped, NULL);
		즉, 같은 소켓(Session)에 대해 아직 완료되지 않은 I/O가 진행 중인데
		다시 WSARecv()를 호출해서 중복 요청을 날리는 상황입니다.
		-> 동일한 recvBuffer	두 개의 I/O가 같은 메모리에 데이터를 동시에 쓰게 됨
		-> 동일한 OVERLAPPED 구조체	커널이 동일한 구조체 주소에 두 개의 결과를 억지로 채우려다 충돌 발생
		-> 완료 통지도 꼬일 수 있음	어느 WSARecv가 어떤 결과를 보낸 건지 구분 불가
	
		[방지]
		-> 각 WSARecv() 요청은 반드시 고유한 OVERLAPPED 구조체를 사용해야 함
			-일반적으로는 new OverlappedEx()로 새로 만들고, 완료 후 delete or pool로 관리

		-> Session마다 recvPending 상태를 bool로 기록하여 중복 호출 방지
			if (session->recvPending)
				return; // 이미 요청 보냈음
			
			session->recvPending = true;
			WSARecv(...);
		-> 완료된 워커 스레드에서만 다음 요청을 보내도록 제한
	*/


		/*
		 [포인터 메모리 유효 문제]
		 session과 overlapped 모두는 개발자가 직접 수명 관리해야 하며,
		하나라도 먼저 삭제되면 나머지는 dangling pointer가 되어 매우 위험합니다.
		우리는 결국 메인 스레드에서 생성 후, 지금 다른 스레드에게 넘겨주기때문에 계속해서 유효한 주소여야하는데,
		특정 스레드에서 해당 세션에 대해 종료함을 처리하여 해당 메모리를 건들이면 위험해진다
		그러므로 해당 메모리 공간만큼은 보장되어야 하므로 smartpointer 등으로 관리해야함.
		*/
	}



	SocketUtils::Clear();
}