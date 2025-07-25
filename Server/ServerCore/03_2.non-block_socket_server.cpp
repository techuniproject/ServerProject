																																																																																																																																																															/*
	1.	[Block I/O]
	  -> I/O 작업을 요청한 프로세스 및 스레드는 요청이 완료될 때 까지 Blocked됨
	  -> ex) read 시스템 콜

	  [Socket Block I/O]
	  - 서버의 소켓은 read 시스템 콜 및 recv()를 통해 상대 클라가 
	     send하기를 기다리는데, 이때 recv_buffer에 데이터가 들어올때 까지 Block이 됨.

	  - 클라의 소켓에 write할때 과도하게 전송하여 send()를 많이 호출하는 상황에서
	     write 시스템 콜을 많이 불러 send_buffer이 다 찬다면, block 상태가 됨.

	  



	2. [Non-Block I/O]

	 정의) 프로세스/스레드를 Block하지 않고 요청에 대한 현재 상태를 즉시 리턴

	 - Non-blocking I/O는 왜 필요한가?
	 - 반대로 Non-blocking은 "CPU를 내가 계속 쓸 거다."
	 - 게임 서버, 네트워크 서버 등 수천 개의 소켓을 관리할 때 필요.

	 - 하나하나 다 blocking 시키면 스레드 수가 엄청나게 필요하고, context switching 오버헤드 커짐.

	 - Non-blocking + epoll 같은 방식으로 한 스레드가 수천 개 I/O를 효율적으로 관리.

	 - Non-blocking I/O는 recv(sock, buf, size, MSG_DONTWAIT);
		 을 호출해서 "지금 데이터 준비됨?" 을 직접 계속 물어봐야 해. (Polling)
		→ 수천 개 소켓이 있다면?
		하나하나 다 반복하면서 물어보면 CPU 낭비. 비효율.
		→ 모든 소켓 순회
		→ 매번 호출, 대부분 EAGAIN
	    → 준비 안 된 소켓에 대해서도 불필요하게 syscall 반복
        → 소켓이 많아질수록 느림 (O(N))

		 [I/O 다중화: select / poll / epoll]
			→ 커널에게 "준비된 것만 알려줘"라고 부탁하는 기술.

			1️) select
			 - 오래된 방식
		     - 최대 소켓 수 제한 있음 (FD_SETSIZE)
			 - 비트 마스크 배열
			 - 효율 낮음 (매번 모든 FD 검사)
			
			2️) poll
			 - select 개선 버전
			 - 제한 없음
			 - 배열에 fd를 넣어 커널에 넘김
			 - 모든 fd 순회 O(N) (비효율은 여전)
			
			3️) epoll
			 - 리눅스 전용 (Windows는 IOCP)
			 - 커널에 "이 소켓 상태 알려줘" 등록
			 - 이벤트가 생긴 소켓만 알아서 알려줌
			 - O(1) 스케일링 (이벤트만 반환)
			 - 수천~수만 개 소켓 효율적 관리


	 흐름) 
	  - 스레드가 read 시스템 콜을 non-block상태로 호출
	  - 커널모드로 진입(아직 컨텍스트 스위칭이 일어나지않아 CPU점유상태)
	  - 이후 I/O작업이 완료되지 않아도 에러코드(EAGAIN,EWOULDBLOCK)를 반환하며 스레드가 사용자 영역의
	     코드를 재개할수 있도록 함(물론 I/O작업과 관련된 로직은 안됨) - 다른 소켓을 시도하거나, 다른 일을 하거나 등.
	  - 재개를 하는동안 입출력 작업이 완료되고 나서 다시 read 시스템콜 호출
	  - 데이터를 커널 영역에서 유저공간으로 전송

	[소켓에서 non-block I/O]

	- 서버의 소켓은 read 시스템 콜 및 recv()를 통해 상대 클라가
		 send하기를 기다리는데, 이때 recv_buffer에 데이터가 들어왔는지 체크하고 없다면
		  이를 알리고, read에 대한 시스템 콜 호출이 끝남.

	  - 클라의 소켓에 write할때 과도하게 전송하여 send()를 많이 호출하는 상황에서
		 write 시스템 콜을 많이 불러 send_buffer이 다 찬다면, 적절한 에러코드로 이를 알리고 write 시스템 콜 호출 종료.

	[Non-Block I/O 결과 확인 방법 및 처리 방식]

	1) 완료됐는지 반복적으로 확인.
	 -> 완료된 시간과 완료를 확인한 시간 사이의 갭으로 인해 처리 속도가 느려질 수 있음
	  (block 방식에 비해)
	 -> 또한 반복적으로 system call을 호출하여 커널에 요청을 보내는데, 이유는 read 시스템 콜로
	     I/O가 이뤄졌는지 확인해야 하기 때문

	2) 완료됐는지 반복적으로 확인하는 것은 CPU 낭비가 심함(busy-waiting)
	 -> 소켓 통신에서 클라가 언제 패킷을 전달할지 모르기 때문에 서버는 한없이 CPU 자원을
	     사용하며 기다리게 됨. 
	 -> 특히 서버는 소켓이 여러개이며 여러 클라의 처리를 담당하는데, 이를 CPU 타임슬라이스 
	     번갈아가며 계속 기다리기만 하면 CPU 낭비가 심함
	 -> 그렇다고, block I/O로 처리하기엔 한 클라이언트가 I/O(send())를 요청하면 서버에선 recv로 자신의 recv
	     buffer을 확인하는 I/O를 처리하는데, 보통 입출력이 들어오면 그 시점에 버퍼에 데이터 있으면
		  타임슬라이스 계속 사용하고 없으면 즉시 블록되고 다른 스레드에게 CPU양보되어 컨텍스트 스위칭
		  그러므로 블록 입출력은 컨텍스트 스위칭 비용이 큼.
	 -> 블록킹 I/O는 결국 스레드가 block되기 때문에 스레드 1개당 1소켓으로 사용하고,
	    Non-blocking + epoll 방식 쓰면 1개로 수천 소켓을 다뤄 준비된 소켓만 알림받아 처리.
	 -> 아예 블록킹으로 스레드 1개당 여러 소켓으로 하면 입출력을 기다리는 소켓이 생기는 순간 블록되어 아예
	     스레드는 다른 소켓들을 처리못함.

	[I/O multiplexing] - 다중 입출력
	-> 관심있는 I/O 작업들을 동시에 모니터링하고 그 중에 완료된 I/O 작업들을 한번에 알려줌
	-> 네트워크 통신에 많이 사용
	 종류)
	  1. select
	  2. poll
	  3. epoll (linux)
	  4. kqueue (macos)
	  5. IOCP(I/O completion port) (windows)




















































			
			
			*/

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

//UDP 서버
// 1) 새로운 소켓 생성
// 2) 소켓에 IP주소/ 포트번호 설정(bind)
// -------
// 3) 클라와 통신

int main()
{
	// 1) 소켓 생성
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;


	// ipv4 버전, TCP 방식
	SOCKET listenSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (listenSocket == INVALID_SOCKET) // 오류 반환값이 아니라면 통과
		return 0;


	///

	// setsockopt 소켓의 동작 방식을 커널에 지시하기 위한 함수
	// int setsockopt(int socket, int level, int option_name,
	//                 const void* option_value, socklen_t option_len);
	// 1) level (SOL_SOCKET, IPPROTO_IP, IPPROTO_TCP) : 옵션의 범위 (SOL_SOCKET, IPPROTO_TCP, 등)
	// 2) opt_name : 수정하고 싶은 옵션 
	// 3) opt_value : 수정하고 싶은 옵션값
	// 4) opt_len : value 크기


	//setsockopt는 listen() 호출 전에 해야 효과가 있음 (특히 SO_REUSEADDR)
	//int 값 넘길 때 포인터로 넘겨야 함

	// SO_KEEPALIVE :[TCP기반] 주기적으로 연결 상태확인하여 일정시간 무응답이면 커넥션 끊음
	bool enable = true;
	::setsockopt(listenSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&enable, sizeof(enable));

	// SO_LINGER : 지연하다 (TCP소켓이 close될때 남은 데이터를 보낼지 여부 설정)
	// SO_SNDBUF : 송신 버퍼 크기 설정
	// SO_RCVBUF : 수신 버퍼 크기 설정

	int32 sendBufferSize;
	int32 optionLen = sizeof(sendBufferSize);
	::getsockopt(listenSocket, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufferSize, &optionLen);
	cout << "송신 버퍼 크기 : " << sendBufferSize << endl;


	//SO_RESUSEADDR
	{
		bool enable = true;
		::setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(enable));
		// 서버 문제 생겨 다시 서버 실행 시, 소켓 회수가 완전히 안될때 사용했던 포트번호 등 
		// 이미 사용중인 상태로 인식하여 해당 자원들을 재활용하겠다는 취지
	}

	// IPPROTO_TCP
	// TCP_NNDELAY = Nagle 알고리즘 작동 여부
	// 작은 패킷(send 호출마다 바로 보내는 것)을 줄이기 위해 나온 최적화 방식
	// [작은 크기의 TCP 패킷들을 하나로 묶어 보내는 최적화 기법]
	// TCP는 기본적으로 send()호출마다 바로 패킷을 전송할 수 있음.
	// 하지만 너무 작은 데이터를 자주 보내면, 각 계층 헤더 오버헤드 증가, 네트워크 병목 발생, CPU/메모리 낭비
	// 이걸 피하기 위해 이전 패킷의 ACK를 받기 전까진 새로운 작은 패킷 전송을 지연시킴
	// 대신 여러개의 작은 데이터들을 하나로 합쳐서 한번에 전송함
	// 게임은 매 순간 중요한 정보가 넘어올 수 있기 때문에 Nagle 알고리즘 꺼두는 편
	// 우린 컨텐츠 단위에서 관리하여 뭉쳐서 보내는 것으로 타협


	// 2) 주소/포트 번호 설정 (bind)
	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr)); //0으로 밀어 초기화
	serverAddr.sin_family = AF_INET; //전송 주소의 주소 패밀리(체계) : 항상 AF_INET(IPv4)으로 설정
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY); // 0.0.0.0 서버가 가지고 있는 모든 IP주소에서의 요청을 받는 뜻
	serverAddr.sin_port = ::htons(7777); // 80까지는 HTTP가 사용 중 - 사용할 포트 번호를 지정 -7777

	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;


	//// 3) 업무 개시 (listen)
	//if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
	//	return 0;
	//TCP에선 클라의 listening 소켓이 연결을 기다리지만, UDP는 없음

	while (true)
	{
		SOCKADDR_IN clientAddr;
		::memset(&clientAddr, 0, sizeof(clientAddr));
		int32 addrLen = sizeof(clientAddr); //상대 클라의 주소를 저장하기 위함

		//SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		//if (clientSocket == INVALID_SOCKET)
		//	return 0;

		////----여기까진 이제 상대 클라와 직접 통신할 소켓 만들어져 상대 클라와 통신 가능 상태

		//char ip[16];
		//::inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip)); // ip추출 함수
		//cout << "Client Connected! IP = " << ip << endl;
		//TCP에서 connect 기반 맺는 과정 UDP에선 없음


		// 패킷
		char recvBuffer[100];


		int32 recvLen = ::recvfrom(listenSocket, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&clientAddr, &addrLen);
		if (recvLen <= 0)
			return 0;

		//int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
		//if (recvLen <= 0)
		//	return 0; //TCP형식

		cout << "Recv Data : " << recvBuffer << endl;
		cout << "Recv Data Len: " << recvLen << endl;

		this_thread::sleep_for(chrono::seconds(1));

	}

	::closesocket(listenSocket);
	::WSACleanup(); //WSAStartUp 호출 시 호출해야 하는 함수
}
// Non-Blocking socket 설명 전  (강의기준)
//======================================================================================================