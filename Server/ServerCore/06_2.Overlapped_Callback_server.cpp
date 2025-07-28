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
// Overlapped + Callback 방식


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
/*========이거 다시 공부=======
static_cast<Session*>(overlapped)
-> 컴파일 타임에 타입 안전성 검사를 수행
단, 이건 overlapped가 Session에서 파생된 클래스일 때만 유효함
ex) Base* -> Derived*처럼
그러므로 이 경우 static_cast는 컴파일 에러남

reinterpret_cast<Session*>(overlapped)
-> 타입 상관없이 포인터를 강제로 변환
-> C언어 식의 캐스팅과 흡사 안전성 보장없음

*/

const int32 BUF_SIZE = 1000;

struct Session
{
	WSAOVERLAPPED overlapped = {};
	//***수정사항 무조건 Session 구조체 첫번째 인자로 배치
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUF_SIZE] = {};
	int32 recvBytes = 0;
};
/*
우리가 wsaBuf에 어디에 데이터를 채워 넣을지를 알려주고, overlapped는 작업 상태를 담고 있는 구조체 포인터를 넘김
그리고 콜백도 같이 넘겨 커널은 우리가 넘겨준 WSAOVERLAPPED포인터를 저장해놓고 있다가 반환
우리가 이 포인터를 접근하여 다른 멤버를 접근하려면 WSAOVERLAPPED가 첫 멤버변수여야 다른 멤버를 시작주소 기반으로 접근가능
만약 overlapped가 두번째 인자면 Session*로 캐스팅하면 해당 주소로부터 Session크기만큼 참조하는 포인터이므로 유효하지 않음
*/

void CALLBACK RecvCallback(DWORD error, DWORD recvLen, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	cout << "Data Recv Len Callback = " << recvLen << endl;
	//TODO

	// 여기서 recvBuffer의 데이터를 알려면 overlapped로 알아야하는데,
	// overlapped를 캐스팅을 하려면 overlapped가 첫 인자로 와야 포인터로 캐스팅 할 수 있기때문
	Session* session = (Session*)overlapped;
	cout << session->recvBuffer << endl;
	// 이렇게 하면 

}

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

		cout << "Client Connected" << endl;

		while (true)
		{
			WSABUF wsaBuf;
			wsaBuf.buf = session.recvBuffer;
			wsaBuf.len = BUF_SIZE;

			DWORD recvLen = 0;
			DWORD flags = 0;

			// 완료 통지시 함수포인터로 넣어준 함수를 호출하기를 기입한 것임
			if (::WSARecv(clientSocket, &wsaBuf, 1,/*Out*/&recvLen,/*Out*/ &flags, &session.overlapped, RecvCallback) == SOCKET_ERROR)
			{
				if (::WSAGetLastError() == WSA_IO_PENDING)
				{
					//TODO
					//...
				   // Alertable Wait
				   // 잠에 들긴하지만, 콜백함수가 호출되는 상황오면 깨어나면서 해당 함수 호출
				   // 입출력이 완료되면 커널이 인터럽트를 발생하고 스레드는 꺠어나서 등록한 콜백 함수 호출
					::SleepEx(INFINITE, TRUE);
					// 두번째 인자가 TRUE여야 Alertable 상태가 되어 IO완료시 콜백이 호출됨
				}
				else
				{// 여기로 들어오면 문제가 있는 것
					//TODO
					break;
				}
			}
			cout << "Data Recv = " << session.recvBuffer << endl;
		}

	}

	SocketUtils::Clear();
}