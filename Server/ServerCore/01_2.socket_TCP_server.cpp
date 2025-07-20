// 서버
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

//--------------------------------------------------------
/*[구별 목적]작성:서정원*/
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
//--------------------------------------------------------

int main()
{
	// 1) 새로운 소켓 생성 (socket)				- ex) 직원 자리 배치
	// 2) 소켓에 주소/포트 번호 설정 (bind)		- ex) 직원 교육
	// 3) 소켓 일 시키기 (listen)				- ex) 직원 근무 시작(영업 개시)
	// 4) 손님 접속 (accept)					- ex) 손님 입장
	// 5) 클라와 통신							- ex) 서비스 시작

	// 소켓
	// 네트워크 통신을 도와주는 핸드폰
	
	// 브로드캐스팅
	// 인접한 지역에 있는 캐릭터 하나씩에게 패킷을 쏴주는 것

	WSADATA wsaData;
	// == 0x202

	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) // 초기화
		return 0;
	//--------------------------------------------------------
	/*작성:서정원*/
	// WSAStartup : 프로세스에서 winsock dll 사용을 시작하는 함수
	// 첫인자 : [in] wVersionRequired - 호출자가 사용할 수 있는 가장 높은 버전의 Windows 소켓 사양
	//                                    상위바이트 : 부 버전번호, 낮은 바이트 : 주 버전번호
	//			MAKEWORD(2,2) ->0x202  :2byte word에 high부분에 2, low부분에 2를 만드는 함수
	// 두번째 인자 : Windows 소켓 구현의 세부 정보를 수신하는 WSADATA 데이터 구조에 대한 포인터
	// 
	// 반환값 : 성공하면 0을 반환함, 실패시 오류코드 반환
	//--------------------------------------------------------

	// MAKEWORD
	// 1바이트 1바이트로 하이와 로우로 묶어서 하나의 WORD를 만듦.

	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	// ipv4 버전으로 TCP 방식으로 만듬
	// 어떤 식으로 통신을 할지를 정해줌.
	// 
	// 리턴 값(SOCKET): 커널에서 관리하는 리소스에 정수 번호를 부여한 값
	// 해당 소켓을 대상으로 무언가를 요청할 때 해당 정수를 사용한다는 개념

	//--------------------------------------------------------
	/*작성:서정원*/
	// 1) 소켓 생성
	// ad : Address Family (AF_INET = IPv4, AF_INET6 = IPv6)
	// type : TCP(SOCK_STREAM) vs UDP(SOCK_DGRAM)
	// protocol: 0
	// return : descriptor
	// int32 errorCode = ::WSAGetLastError();

	// socket([in] int af, [in] int type, [in] int protocol)
	// 첫인자 [af] : 주소 패밀리 사양 -> Winsock2.h에 사용할 수 있는 값 정의 : AF_INET(ipv4), AF_INET6(ipv6) 
	// 두번째 인자 [type] : 새 소켓에 대한 형식 사양 -> 마찬가지로 Winsock2.h에 정의
	// SOCKE_STREAM : OOB 데이터 전송 메커니즘을 사용하여 순차적이고 신뢰할 수 있는 양방향 연결 기반 바이트 스트림을 제공하는 소켓 유형
	//                 이 방식은 인터넷 주소 패밀리(AF_INET or AF_INET6)에 대해 TCP를 사용
	// SOCK_DGRAM : UDP 방식
	// 세번째 인자 [protocol] : 사용할 프로토콜, 0이면 다음을 수행하지 않는다.
	// 반환값 : 오류가 발생하지 않으면 새 소켓을 참조하는 설명자를 반환.
	// 오류 확인 함수 : int32 errorcode = ::WSAGetLastError();
	//--------------------------------------------------------
	
	// 2) 주소/포트 번호 설정 (bind)
	// 소켓 == 핸드폰(네트워크 통신을 도와주는)
	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));//0으로 밀어 초기화
	serverAddr.sin_family = AF_INET;//전송 주소의 주소 패밀리(체계) : 항상 AF_INET(IPv4)으로 설정
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY); // 0.0.0.0 서버가 가지고 있는 모든 IP주소에서의 요청을 받는 뜻
	serverAddr.sin_port = ::htons(7777);  // 80은 HTTP가 사용 중 - 사용할 포트 번호를 지정 -7777
	//--------------------------------------------------------
	/*작성:서정원*/
	// bind : 이 소켓이 IP/포트를 소유함을 OS에 알리는
	// 클라가 bind 안하는 이유는 : os가 임시 ip와 포트 자동 할당함 (찾아가는 곳이 아니므로 할당받은 클라 ip따오면 서로 통신가능)
	// 첫 인자 [in] s : 바인딩되지 않은 소켓을 식별하는 설명자
	// 두번째 인자 addr : 바인딩된 소켓에 할당할 로컬 주소의 구조체에 대한 포인터
	// 세번째 인자 [in] namelen : addr이 가리키는 값의 길이(바이트)

	// 여기까진 해당 서버가 0.0.0.0:7777(Ipv4, 모든 네트워크 인터페이스, 포트 7777)
	//  에서 오는 TCP연결 요청을 받을 준비를 마친역할
	//--------------------------------------------------------
	

	// 리틀 엔디언 - 현재 환경에서 표준
	// 빅 엔디언 - 네트워크 상에서 표준

	// ex) 0x12345678
	// low [0x78][0x56][0x34][0x12] high << little
	// low [0x12][0x34][0x56][0x78] high << big
	//=> htonl(host to network long), htons(host to network short) 사용 - 동일한 환경으로 맞춰주기 위해 사용
	// 네트워크 표준에 맞는 방식으로 바꿔주는 함수

	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;
	// 어떠한 주소를 바인딩(맵핑) - IP주소와 포트 번호 연결

	// 3) 업무 개시 (listen)
	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		return 0;
	// block 안됨, 그냥 대기열 생성
	// SOMAXCONN: 대기할 수 있는 숫자 관련 옵션 - 최대치

	//-> 이제부터 이 소켓은 연결 요청을 기다릴 준비가 됐다
	// 클라가 connect()요청을 보내면, OS가 이 요청을 받아 대기열(SOMAXCONN은 대기열 최대 길이로 지정)에 넣는다.
	// 아직 연결 수락은 안 한 상태(accept()필요) - 연결 요청을 받을 준비가 됐고 대기열이 생긴 상태


	// 4)
	while (true)
	{
		SOCKADDR_IN clientAddr;
		::memset(&clientAddr, 0, sizeof(clientAddr));
		int32 addrLen = sizeof(clientAddr);

		// 상대방 쪽 소켓
		// accept는 입장한 손님(connect)이 없으면 멈춤(blocked)
		
		SOCKET clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
			return 0;

		//--------------------------------------------------------
		/*작성 서정원*/
		// accept함수는 blocking함수의 일종(결과 나올때까지 리턴하지 않고 기다리는)
		// 클라의 connect요청 보낸 소켓의 정보(상대 클라 IP & Port)가 clientAddr에 들어오고, 
		// accept요청이 수락되면, 함수의 반환값으로 새로운 통신용 소켓을 만듦(상대 클라와 통신하는)
		// 클라가 많아질수록 clientSocket의 수가 증가해야함( 클라 소켓과 리스닝 소켓은 IP주소와 포트넘버 동일)
		// listenSocket은 계속 유지하여 다른 연결 대기 가능
		// TCP상의 connection의 고유성을 보장하기 위해 이런방식 채택
		//----여기까진 이제 상대 클라와 직접 통신할 소켓 만들어져 상대 클라와 통신 가능 상태
		//--------------------------------------------------------
		/*추가 보충
		 1) 서버와 클라에서 TCP가 유니크하게 식별되는 과정!!
	    서버 <50.50.50.50, 8080> / 클라 <77.77.77.77, 49999>

	    1. connection 맺는 요청을 기다리는 listening socket이 존재 ->listen()함수
		2. connection이 맺어지면(3-way handshake로) listening socket이 또 다른 socket을 만듦 -> 클라(connect())로 connection하면
		3. accept()함수를 호출함으로써 그 다르게 만든 소켓과 클라의 소켓이 connection을 형성
		4. 그렇게 추가적으로 만들어진 소켓은 IP주소와 포트넘버가 모두 동일
		5. 그러면 어떻게 소켓을 식별하지?
		[connection 연결 여부로]
		 - connection전 연결 요청시(connection 없으면) - listening socket<IP,Port>로 구별하여 listening socket으로 전달 
		 - connection 성립 이후 <src IP, src port, dest IP, dest port>로 리스닝이 아닌 보낸 소켓과 다른 소켓 정보로 식별
		 																
		*/
		char ip[16];
		::inet_ntop(AF_INET, &clientAddr.sin_addr, ip, sizeof(ip));
		cout << "Client Connected! IP = " << ip << endl;

		while (true)
		{
			// 패킷
			char recvBuffer[100];

			// recv: 메세지를 받는 함수
			int32 recvLen = ::recv(clientSocket, recvBuffer, sizeof(recvBuffer), 0);
			if (recvLen <= 0)
				return 0;
			//recv는 받을 데이터 없으면 블록
			/*작성 서정원*/
			// [중요!!] 소켓에는 커널영역에 커널 RecvBuffer와 SendBuffer이 존재함.
			// send를 하는것은 SendBuffer에 데이터를 복사하는 것
			// 이후 OS가 상대의 recvbuffer에 전달하는(TCP등을 이용)것을 시도
			// recv는 저렇게 커널 recvbuffer에 있는 내용을 자기의 recvbuffer에 복사해오는것
			// 클라가 send를 안하면 서버는 여기 recv에서 잠수타게 됨(recvbuffer가 비어있으므로)


			cout << "Recv Data : " << recvBuffer << endl;
			cout << "Recv Data Len: " << recvLen << endl;

			// 에코 서버 - 상대방이 보내준 메세지를 그대로 토스 (거울)
			// 받은 것을 다시 send
			// send는 버퍼가 꽉 차면 블록
			int32 resultCode = ::send(clientSocket, recvBuffer, recvLen, 0);
			if (resultCode == SOCKET_ERROR)
				return 0;
		}
	}

	::WSACleanup();
	// 코드가 끝나면 대칭적으로 호출해주어야 함.
}

// 현재 사용하는 방식은 데이터를 보내지 않는다면 서버는 멈추게 된다.


// 클라 - 서버 간 짝이 맞지 않을 경우
// send <-> recv
// ex) 서버에서 send를 하고 클라이언트에서 recv로 받지 않을 경우
// => 결과: send는 성공!

// 커널 내부에서 각 소켓은 내부적으로 recv, send Buffer를 소유하고 있음.
// 자신의 버퍼에다가 해당 내용을 복사하는 것을 성공이라고 인식, 그 이후는 운영체제가 처리함.
// Send 성공 == Send Buffer에 데이터를 복사하는 것
// Receive 성공 == Receive Buffer에 데이터가 있으면 복사해서 가지고 오는 것
// Send 실패 == 샌드 버퍼가 꽉참 (ex) Receive를 하지 않아서)