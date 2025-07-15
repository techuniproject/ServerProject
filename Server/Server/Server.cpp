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

// [빅 엔디언 vs 리틀 엔디언] 엔디언 - 멀티바이트 데이터를 메모리에 어떻게 저장할 것인지
// [빅 엔디언 CPU] - 상위 바이트(MSB)를 낮은 주소에 저장 (사람이 읽는 숫자 순서와 동일) 
//        0x12345678 - 낮은 주소부터 0x00 (MSB) 12 34 56 78 (LSB) 0x03
// CPU는 MSB->LSB순서로 조립
// --------------------------
// [리틀 엔디언 CPU] - 하위 바이트(LSB)를 낮은 주소에 저장 
//        0x12345678 - 낮은 주소부터 0x00 (LSB) 78 56 34 12 (MSB) 0x03
// CPU는 LSB->MSB로 읽음
// 
// LSB(Least Significant Byte) : 가장 하위 자리수 0x78(1의자리)
// MSB(Most Significant Byte) : 가장 상위 자리수 0x12 (16^6의자리)
// 
// 스택은 높은주소에서 낮은주소로 쌓임 -> 그 사이사이 각 데이터 바이트 간 리틀/빅 엔디언 적용
// 
// -------------
// 
// [네트워크 바이트 오더 - 통신 프로토콜 표준은 Big Endian]
// 하지만, CPU 및 C++코드(windows)는 기본적으로 x64, x86 모두 Little Endian이므로 변환해줘야함
// htons (Host to Network Short - 2bytes)
// htonl (Host to Network Long - 4bytes)
// ntohs (Network to Host Short)
// ntohl (Network to Host Long)
// 
// 숫자(int, short) - 필요 / 문자열 (1바이트라 필요 X) / 
// 
// 사용자 정의 자료형(엔디언 변환 직접해야함) - 표준이 없음(순서, 크기 OS모름) , 패딩&정렬 차이
// ex) PlayerInfo info= {100,200,300};
//       info.id =htonl(info.id); info.hp=htons(info.hp); info.mp=htons(info.mp);
//       send(sockedt,reinterpret_cast<char*>(&info),sizeof(info));
// float 및 double은 float->long 으로 변환(memcpy)후 htonl사용/ double은  _byteswap_uint64 사용

// [네트워크 연결과 인터넷은 별개]
	// 1) 같은 pc 내 통신 : 네트워크 장비(공유기,스위치) 거치지 않음, 해당 컴퓨터 안에서만 사용 가능
	//            -> 운체 안에 가상적인 네트워크 인터페이스를 통해 처리
	//           ex) 클라(127.0.0.1) -> OS 루프백 -> 서버 (127.0.0.1)
	// 2) 다른 PC와 통신(같은 네트워크) - IP 192.168.x.x / 10.x.x.x(사설IP)
	//      -> 같은 공유기나 스위치 아래에 있음
	//      -> 물리적으로 LAN으로 연결 (공유기, 케이블, 와이파이)
	//      -> 인터넷 필요 없음
	// 3) 인터넷 거치는 경우 (다른 네트워크)
	//     -> 공인 ISP가 부여한 IP필요
	//     -> 서로 다른 네트워크 (다른 공유기, 다른회사)
	//     -> 인터넷이라는 공공망필요
	//     -> 내 PC -> ISP ->다른 ISP ->상대 PC
	// 

//[서버 클라 통신 과정]
// [서버 통신 순서]
// socket() : 서버 소켓 생성 
// bind() : IP+Port에 소켓 연결
// listen() : 연결 요청 받을 준비
// accept() : 클라이언트 연결 수락, 새 소켓 생성 (실제 연결)
// recv() / send() : 데이터 송수신 (실제 통신)
// closesocket() : 소켓 종료 (연결 해제)

// [클라 통신 순서]
// socket() : 클라이언트 소켓 생성
// connect() : 서버의 IP + Port에 연결 요청
// recv() / send() : 데이터 송수신 (실제 통신)
// closesocket() : 소켓 종료 (연결 해제)

//   클라이언트                                 서버
//  ----------                                 ----------
//  socket()                                   socket()
//                                              bind()
//                                             listen() (준비 완료)
// connect()  ----------------------------->    (요청시도) -> listen대기열에 요청 추가
//                                             accept() -  대기열 찰때까지 서버는 대기 상태
//                                          -> (OS가 대기열에서 요청 꺼내 연결 수락) 
//                                          -> 새 소켓 반환되며(이 소켓이 클라와 직접통신)
//  send()     ----------------------------->   recv()
//  recv() < ---------------------------- -     send()
//  closesocket()                            closesocket()

int main()
{
	// 1) 소켓 생성
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return 0;
	// WSAStartup : 프로세스에서 winsock dll 사용을 시작하는 함수
	// 첫인자 : [in] wVersionRequired - 호출자가 사용할 수 있는 가장 높은 버전의 Windows 소켓 사양
	//                                    상위바이트 : 부 버전번호, 낮은 바이트 : 주 버전번호
	//			MAKEWORD(2,2) ->0x202  :2byte word에 high부분에 2, low부분에 2를 만드는 함수
	// 두번째 인자 : Windows 소켓 구현의 세부 정보를 수신하는 WSADATA 데이터 구조에 대한 포인터
	// 
	// 반환값 : 성공하면 0을 반환함, 실패시 오류코드 반환
	

	// ipv4 버전, TCP 방식
	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET) // 오류 반환값이 아니라면 통과
		return 0;
	// socket([in] int af, [in] int type, [in] int protocol)
	// 첫인자 [af] : 주소 패밀리 사양 -> Winsock2.h에 사용할 수 있는 값 정의 : AF_INET(ipv4), AF_INET6(ipv6) 
	// 두번째 인자 [type] : 새 소켓에 대한 형식 사양 -> 마찬가지로 Winsock2.h에 정의
	// SOCKE_STREAM : OOB 데이터 전송 메커니즘을 사용하여 순차적이고 신뢰할 수 있는 양방향 연결 기반 바이트 스트림을 제공하는 소켓 유형
	//                 이 방식은 인터넷 주소 패밀리(AF_INET or AF_INET6)에 대해 TCP를 사용
	// SOCK_DGRAM : UDP 방식
	// 세번째 인자 [protocol] : 사용할 프로토콜, 0이면 다음을 수행하지 않는다.
	// 반환값 : 오류가 발생하지 않으면 새 소켓을 참조하는 설명자를 반환.
	// 오류 확인 함수 : int32 errorcode = ::WSAGetLastError();

	

	// 2) 주소/포트 번호 설정 (bind)
	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr)); //0으로 밀어 초기화
	serverAddr.sin_family = AF_INET; //전송 주소의 주소 패밀리(체계) : 항상 AF_INET(IPv4)으로 설정
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY); // 0.0.0.0 서버가 가지고 있는 모든 IP주소에서의 요청을 받는 뜻
	serverAddr.sin_port = ::htons(7777); // 80까지는 HTTP가 사용 중 - 사용할 포트 번호를 지정 -7777

	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;

	// bind : 이 소켓이 IP/포트를 소유함을 OS에 알리는
	// 클라가 bind 안하는 이유는 : os가 임시 ip와 포트 자동 할당함 (찾아가는 곳이 아니므로 할당받은 클라 ip따오면 서로 통신가능)
	// 첫 인자 [in] s : 바인딩되지 않은 소켓을 식별하는 설명자
	// 두번째 인자 addr : 바인딩된 소켓에 할당할 로컬 주소의 구조체에 대한 포인터
	// 세번째 인자 [in] namelen : addr이 가리키는 값의 길이(바이트)

	//-------->
	// 여기까진 해당 서버가 0.0.0.0:7777(Ipv4, 모든 네트워크 인터페이스, 포트 7777)
	//  에서 오는 TCP연결 요청을 받을 준비를 마친역할
	


	// 3) 업무 개시 (listen)
	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		return 0;

	//-> 이제부터 이 소켓은 연결 요청을 기다릴 준비가 됐다
	// 클라가 connect()요청을 보내면, OS가 이 요청을 받아 대기열(SOMAXCONN은 대기열 최대 길이로 지정)에 넣는다.
	// 아직 연결 수락은 안 한 상태(accept()필요) - 연결 요청을 받을 준비가 됐고 대기열이 생긴 상태

	
	while (true)
	{
		SOCKADDR_IN clientAddr;
		::memset(&clientAddr, 0, sizeof(clientAddr));
		int32 addrLen = sizeof(clientAddr); //상대 클라의 주소를 저장하기 위함

		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
			return 0;
		// accept함수는 blocking함수의 일종(결과 나올때까지 리턴하지 않고 기다리는)
		// 클라의 connect요청 보낸 소켓의 정보(상대 클라 IP & Port)가 clientAddr에 들어오고, 
		// accept요청이 수락되면, 함수의 반환값으로 새로운 통신용 소켓을 만듦(상대 클라와 통신하는)
		// 클라가 많아질수록 clientSocket의 수가 증가해야함
		// listenSocket은 계속 유지하여 다른 연결 대기 가능

		

		//----여기까진 이제 상대 클라와 직접 통신할 소켓 만들어져 상대 클라와 통신 가능 상태

		char ip[16];
		::inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip)); // ip추출 함수
		cout << "Client Connected! IP = " << ip << endl;

		while (true)
		{
			// 패킷
			char recvBuffer[100];
			int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
			if (recvLen <= 0)
				return 0;
			
			// [중요!!] 소켓에는 커널영역에 커널 RecvBuffer와 SendBuffer이 존재함.
			// send를 하는것은 SendBuffer에 데이터를 복사하는 것
			// 이후 OS가 상대의 recvbuffer에 전달하는(TCP등을 이용)것을 시도
			// recv는 저렇게 커널 recvbuffer에 있는 내용을 자기의 recvbuffer에 복사해오는것
			// 클라가 send를 안하면 서버는 여기 recv에서 잠수타게 됨(recvbuffer가 비어있으므로)
			
			
			

			cout << "Recv Data : " << recvBuffer << endl;
			cout << "Recv Data Len: " << recvLen << endl;

			//에코 서버 (상대가 보내준 데이터를 그대로 다시 토스할 용도로)
			int32 resultCode = ::send(clientSocket, recvBuffer, recvLen, 0);
			if (resultCode == SOCKET_ERROR)
				return 0;
		}
	}

	::closesocket(listenSocket);
	::WSACleanup(); //WSAStartUp 호출 시 호출해야 하는 함수
}