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

// Overlapped I/O (비동기 + 논블로킹)
// Overlapped + Event 방식
// I/O 요청 관점에선(WSARecv) 비동기/ 논블로킹
// I/O 완료 대기 관점 (WSAWaitForMultipleEvents) 관점에서는 블로킹

/*
추가 공부

시스템 호출 시
-> I/O 작업을 요청하는 시스템 호출 시(사용자가 커널 영역 사용) 상황에 따라 블록 상태로 바꾸고 컨텍스트 스위칭
	또는 논 블로킹후 계속 CPU점유(타이머 인터럽트 전)

- 일반 소켓 + recv() -> 블로킹 되는 시스템 콜
- 논블로킹 소켓 + recv -> 논블로킹 되는 시스템 콜
- Overlapeed소켓 + WSARecv() -> 논 블로킹되는 시스템 콜

인터럽트
-> I/O 장치로부터 I/O완료가 되어 인터럽트 발생하여 해당 스레드(블록된걸)를 준비 큐로 옮기거나
	우선순위(스케줄링)에 따라 컨텍스트 스위칭
-> 타임 슬라이스 다 사용 시, 타이머 인터럽트 발생하여 준비 리스트에 넣고 컨텍스트 스위칭

*/


const int32 BUF_SIZE = 1000;

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUF_SIZE] = {};
	int32 recvBytes = 0;
	// Overlapped 추가
	WSAOVERLAPPED overlapped = {};
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


	// Overlapped 함수를 건다 (WSARecv, WSASend)
	// Overlapped 함수가 성공했는지 확인
	// - 성공했으면 결과 얻어서 처리
	// - 실패했으면 사유를 확인

	// [비동기] I/O 작업 자체도 백그라운드로 수행, I/O 요청 후 결과가 나중에 알아서 오는 구조
	// 어떤 작업을 요청한 후, 그 작업이 완료될 때까지 기다리지 않고, 다른 작업을 계속 수행할 수 있는 방식
	//[I/O관점 비동기] : I/O작업이 백그라운드에서 커널에 의해 처리되고, 애플리케이션은 그 작업이 끝날
	//                    때까지 기다릴 필요 없이 다른 일을 할 수 있으며, 작업 완료시 커널이 알려줌.


	// WSAEventSelect방식과 다른점
	// WSAEventSelect는 커널에 특정 소켓에서 특정 일이 생기면 알려달라는 요청
	// 커널은 네트워크 이벤트가 생겼는지만 알려줌, 실제 I/O는 여전히 recv()/send() 호출로 직접 읽고 씀
	// Event 방식이지만 event 감지를 커널을 통해 전달받고 그 이후 직접 i/o처리를 하므로
	// 커널은 데이터 준비된것만 알려주고 애플리케이션이 직접 데이터 송수신하므로 완전한 비동기는 아님
	// 비동기같은 면모가 있어 이벤트 기반 비동기로도 봄 / 데이터 송수신 관점에선 비동기 아님

	// Overlapped + WSARecv , WSASend 방식은 I/O 자체를 커널에 미리 요청
	// 커널이 완료되면 알아서 통보
	// WSARecv나 WSASend 호출 후 함수는 즉시 리턴됨 / 데이터 송수신은 OS 커널이 담당
	// I/O가 끝나면 커널이 이벤트 객체 또는 IOCP 또는 Callback으로 완료 통보


	while (true) {

		SOCKADDR_IN clientAddr;
		int32 addrLen = sizeof(clientAddr);

		SOCKET clientSocket;
		while (true) {
			clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);

			if (clientSocket != INVALID_SOCKET)
				break;

			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;

			return 0;
		}

		//Overlapped 회신을 이벤트 방식
		Session session = Session{ clientSocket };
		WSAEVENT wsaEvent = ::WSACreateEvent();
		session.overlapped.hEvent = wsaEvent;
		// session.overlapped는 WSARecv/WSASend와 같은 비동기 I/O 요청시,
		// 해당 요청의 진행 상태, 결과, 이벤트 등을 추적하는 역할을 함
		// hEvent는 I/O완료 시 signal로 바꿔줄 이벤트 핸들
		// I/O 요청마다 고유한 OVERLAPPED가 필요함
		cout << "Client Connected" << endl;

		while (true)
		{
			WSABUF wsaBuf;
			wsaBuf.buf = session.recvBuffer;
			wsaBuf.len = BUF_SIZE;

			DWORD recvLen = 0;
			DWORD flags = 0;

			// WSARecv 두번째 인자 wsaBuf는 우리가 버퍼의 주소와 그 크기만큼 유효한 메모리를 채워넣을
			// 것이라고 하는 것이므로 중간에 그 메모리가 오염되면 안됨.

			//WSARecv(시스템 콜) 반환값 0이면 I/O즉시 완료됨, ERROR이면 무조건 실패는 아님-> 오류 코드를 확인해야함
			if (::WSARecv(clientSocket, &wsaBuf, 1,/*Out*/&recvLen,/*Out*/ &flags, &session.overlapped, nullptr) == SOCKET_ERROR)
			{//이 Recv를 예약한 후 완료된 시점을 어떻게 알아올것인지가 관건
				if (::WSAGetLastError() == WSA_IO_PENDING)
				{// WSAGetLastError은 가장 최근에 호출된 Winsock함수 하나의 오류 상태 코드 반환( 반환 값은 TLS값 스레드 고유 저장)
				 // 보통 바로는 여기로 들어옴 , 상대가 보내야 통과하므로 바로 보내서 통과하면 여기로 안들어옴
				 //그러므로 여기서 다른거 할걸 하고 있으면 됨.


				 //TODO
				 //...
					//1) 커널이 I/O 완료해서 이벤트가 signaled 되었는지 확인
					//  Event signaled까지 기다림
					// 이 방식도 결국 기다리는 동안 cpu할당이 안된 상태로 블록되어 사용자가 블록을 시키는 것
					// recv와 같이 자동으로 블록되는 구조는 아니지만 결국 블록이 되긴함
					// 만약 WSA_INFINITE가 아닌 타임을 정하면 타이머 큐에 등록된 타이머만료에 의해
					// 커널이 깨어나도록 스케줄링 해줌
					::WSAWaitForMultipleEvents(1, &wsaEvent, TRUE, WSA_INFINITE, FALSE);

					//2) signal이 오면 실제 데이터 수신 여부, 바이트 수 확인
					::WSAGetOverlappedResult(session.socket, &session.overlapped, &recvLen, FALSE, &flags);
					//wsarecv에 이미 recv받는 버퍼 위치를 지정했으므로 여기서 이벤트 체크로 입출력 완료를 체크, 데이터는 이미 꽂힘
					// 전에 WSAEventSelect방식에서는 우리가 IO가 준비됨을 감지했지만, 
					// 이 방식은 이미 알아서 커널이 입출력을 완료하므로 우린 I/O가 완료되었는지를 이벤트로 알아오는 것
				}
				else
				{// 여기로 들어오면 문제가 있는 것
					//TODO
					break;
				}
			}
			//우린 커널이 이벤트 완료함을 알고 버퍼(데이터)를 써야 안전함이 보장됨
			//알아서 비동기적으로 커널이 입출력 처리를 하지만 그렇다고 그냥 쓰는게 아닌 실제 완료되었는지 확인 후 사용
			cout << "Data Recv = " << session.recvBuffer << endl;
		}
		::WSACloseEvent(wsaEvent);
	}

	SocketUtils::Clear();
}
