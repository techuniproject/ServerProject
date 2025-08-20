#include "pch.h"
#include <iostream>
#include <thread>
#include <vector>
using namespace std;
#include <atomic>
#include <mutex>
#include "ThreadManager.h"
#include "SocketUtils.h"
#include "Listener.h"
#include "Service.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"


//class GameSession : public Session
//{//애플리케이션 레벨에서 한명의 클라이언트와 주고받는 데이터 처리 로직을 담는 객체
//public:
//	~GameSession()
//	{
//		cout << "~GameSession" << endl;
//	}
//
//	virtual int32 OnRecv(BYTE* buffer, int32 len) override
//	{
//		// Echo
//		cout << "OnRecv Len = " << len << endl;
//		Send();
//		return len;
//	}
//
//	virtual void OnSend(int32 len) override
//	{
//		cout << "OnSend Len = " << len << endl;
//	}
//};

/*
1) Accept  대기 (Listener)
- acceptEvent(owner=Listener) 여려 개 생성
- RegisterAccept(acceptEvent)에서 새 Session 생성 (service->CreateSession())
- AcceptEx(listenSocket, sessionSocket,...,acceptEvent) 등록

2) Accept완료 -> Listener이 처리
- 커널이 완료 넣음 -> 워커 스레드가 GQCS로 acceptEvent 꺼냄
- Listener::Dispatch(AcceptEvent) -> ProcessAccept()
- SO_UPDATE_ACCEPT_CONTEXT(sessionSocket, listenSocket)
- getpeername(sessionSocket)로 원격 주소 채움
- session->ProcessConnect() 호출 (세션 활성화)
- RegisterAccept(acceptEvent)로 다음 Accept 다시 걸어둠

3) 세션 활성화 & 첫 Recv 예약 (Session)
- Session::ProcessConnect()
- _connected = true, Service::AddSession(this), OnConnected()
- RegisterRecv() 호출 → 첫 WSARecv(sessionSocket, ... , &_recvEvent) 등록
- _recvEvent.owner = Session (완료는 세션으로 들어오게 태깅)

4) 클라가 send → Recv 완료 → 세션이 처리
- 커널이 _recvEvent 완료 → IOCP 큐
- 워커가 꺼냄 → Session::Dispatch(_recvEvent, bytes) → ProcessRecv(bytes)
- bytes==0 → 원격 종료 → Disconnect()
- bytes>0 → OnRecv(buf, bytes) 처리 후 다시 RegisterRecv() (수신 루프)

*/


int main()
{
	SocketUtils::Init();
	// WSAStartUp 호출, ConnectEx/DisconnectEx/AcceptEx 함수 포인터를 런타임 바인딩

	ServerServiceRef service = make_shared<ServerService>(
		NetAddress(L"127.0.0.1", 7777),
		make_shared<IocpCore>(),
		[]() { return make_shared<GameSession>(); }, // TODO : SessionManager 등
		100);
	//	ServerService(NetAddress targetAddress, IocpCoreRef core, SessionFactory factory, int32 maxSessionCount = 1);
	//  TCP: 127.0.0.1:7777 (sin.addr, sin.port) IP addr와 포트번호 입력
	//  IocpCore 인스턴스 생성(내부에 IOCP 핸들 CreateIoCompletionPort 생성)
	//  SessionFactory에 람다로 GameSession 만들어주는 함수 꽂아줌 -> 접속마다 GameSession 새로 만들어줌

	assert(service->Start());

	for (int32 i = 0; i < 5; i++)
	{
		GThreadManager->Launch([=]()
			{
				while (true)
				{
					service->GetIocpCore()->Dispatch();
				}
			});
	}

	//while (true)//컨텐츠 들어갈곳
	//{
	//	vector<BuffData> buffs{ BuffData{100,1.5f},BuffData{200,2.3f},BuffData{300,0.7f} };

	//	SendBufferRef sendBuffer = ServerPacketHandler::Make_S_TEST(1001, 100, 10, buffs);
	//	GSessionManager.Broadcast(sendBuffer);

	//	this_thread::sleep_for(250ms);
	//}



	GThreadManager->Join();

	// 윈속 종료
	SocketUtils::Clear();
}

/*
0) 부팅 · 공용 초기화 (main)
윈속 초기화
SocketUtils::Init();

WSAStartup 호출, ConnectEx/DisconnectEx/AcceptEx 함수 포인터를 런타임으로 바인딩.

서비스 구성

auto service = make_shared<ServerService>(
	NetAddress(L"127.0.0.1", 7777),
	make_shared<IocpCore>(),
	[](){ return make_shared<GameSession>(); },
	100);
127.0.0.1:7777로 리슨할 서버 서비스.

IocpCore 인스턴스를 생성(내부에 IOCP 핸들 CreateIoCompletionPort).

세션 팩토리: 접속마다 GameSession을 새로 만들어줌 (컨텐츠 훅 OnRecv/OnSend 구현된 에코 세션).

서비스 시작
service->Start();

ServerService::Start() → Listener 생성 → Listener::StartAccept(service)

리스너 소켓 생성/옵션 설정/bind/listen → IOCP 등록

Accept 파이프라인 준비: maxSessionCount(=100) 만큼 IocpEvent(Accept)를 만들어 RegisterAccept() 반복 호출해 미리 걸어둠.

워커 스레드 가동

for (i = 0; i < 5; ++i)
  Launch( while(true) service->GetIocpCore()->Dispatch(); );
5개 워커가 무한 루프로 IocpCore::Dispatch() 호출 → IOCP 큐에서 완료 이벤트(OVERLAPPED)를 끊임없이 꺼내 분배.

1) Accept 사이클 (Listener 중심)
Listener::RegisterAccept(acceptEvent)

세션 팩토리로 새 GameSession 생성 → 그 소켓을 IOCP에 등록(Service::CreateSession() 경유).

acceptEvent->session = 그 세션으로 매핑.

AcceptEx(listenSocket, sessionSocket, ...) 비동기 등록 (완료 시 IOCP 큐로 이벤트가 들어감).

완료 도착 → 워커 스레드

워커 스레드 하나가 Dispatch()에서 꺼냄 → acceptEvent->owner(=Listener의 shared_ptr)로 리스너 생존 보장.

Listener::Dispatch() → ProcessAccept() 실행.

Listener::ProcessAccept()

SetUpdateAcceptSocket()로 새 소켓에 리슨 소켓 컨텍스트 적용.

getpeername()로 원격 주소 확인 → session->SetNetAddress(...).

핵심 전달: session->ProcessConnect() 호출(세션 레벨로 제어 이동).

그리고 다시 RegisterAccept(acceptEvent)를 호출해 다음 접속을 미리 걸어둠(무한 accept 루프 유지).

2) Session 수명 시작과 Recv 루프
Session::ProcessConnect()

_connected = true.

Service::AddSession(this)로 세션 집합에 등록(현재 접속 수 증가).

OnConnected()(컨텐츠 훅; 지금은 비어있음).

첫 수신 예약: RegisterRecv() 호출.

Session::RegisterRecv()

_recvEvent.Init(); _recvEvent.owner = shared_from_this();
→ 이 Recv 완료가 돌아오기 전까지 세션이 강참조로 살아있음(UAF 방지).

WSARecv(sessionSocket, &_recvBuffer, 1, ... , &_recvEvent, nullptr) 비동기 등록.

즉시 실패이고 WSA_IO_PENDING이 아니면 HandleError()(보통 Disconnect 경로).

클라가 뭔가 보냄 → 완료 도착

IOCP 큐에 Recv 완료가 들어감 → 워커 스레드 중 하나가 Dispatch()로 꺼냄.

IocpCore::Dispatch() → iocpEvent->owner(=세션의 shared_ptr) 획득 → Session::Dispatch(Recv) 호출.

Session::ProcessRecv(numBytes)

이벤트의 owner 해제(강참조 해제).

numBytes == 0이면 원격 종료 → Disconnect("Recv 0").

>0이면 컨텐츠 훅 호출: GameSession::OnRecv(buf, len).

컨텐츠: GameSession::OnRecv → Echo

cout << "OnRecv Len = " << len;

Send(buffer, len); 호출로 받은 그대로 돌려줌 → 에코 서버 동작!

다시 수신 예약

ProcessRecv() 끝에서 RegisterRecv()를 다시 호출 → 무한 Recv 루프.

3) Send 경로 (에코)
Session::Send(buffer, len)

(임시 구현) 동적 IocpEvent* sendEvent = new IocpEvent(Send) 생성.

sendEvent->owner = shared_from_this();로 완료 때까지 세션 생존 보장.

내부 버퍼에 데이터 복사 후 RegisterSend(sendEvent).

Session::RegisterSend(sendEvent)

WSASend() 비동기 등록.

즉시 실패이고 WSA_IO_PENDING이 아니면 HandleError() + 이벤트 해제.

Send 완료 도착

워커 스레드가 IocpCore::Dispatch()로 꺼냄 → Session::Dispatch(Send) → ProcessSend(sendEvent, numBytes).

이벤트의 owner 해제, delete sendEvent로 메모리 해제.

numBytes == 0이면 Disconnect("Send 0"), 아니면 OnSend(numBytes)(컨텐츠 훅: 길이 출력).

4) Disconnect(종료) 흐름
어떤 이유로든 끊길 때

수신 0 바이트 / 송신 0 바이트 / 에러 / 서버가 직접 끊기 결심.

Session::Disconnect(cause)

_connected.exchange(false)로 중복 방지.

OnDisconnected()(훅) 호출 후, Service::ReleaseSession(this)로 서비스의 세트에서 제거(카운트 감소).

RegisterDisconnect() → DisconnectEx() 비동기 등록(완료 시 ProcessDisconnect()에서 owner 해제).

실제 소멸 시점

이벤트(Recv/Send/Disconnect)마다 owner = shared_from_this()를 들고 있으므로, 모든 보류 I/O 완료가 돌아와 owner 참조가 풀릴 때 실제로 세션의 shared_ptr 카운트가 0이 됨 → 소멸자 호출.

지금 버전은 CancelIoEx()/closing 플래그/세대 토큰은 아직 없음. 그래서 “대기 중인 I/O가 있으면 그게 돌아올 때까지” 세션이 살아있는 지연 파괴 모델이야(안전하지만 리소스 오래 잡을 수 있음).

5) IOCP 분배(Dispatcher)와 워커
워커 스레드(5개)가 전부 while(true) core->Dispatch();만 수행.

IocpCore::Dispatch()는 GetQueuedCompletionStatus()로 이벤트를 받아오고, 성공/실패 상관없이 iocpEvent->owner->Dispatch(iocpEvent, numBytes)로 해당 객체(Session/Listener)에게 업무를 넘김.

key는 쓰지 않고, 이벤트 내부의 owner(shared_ptr)로 대상 객체를 안전하게 식별 + 수명 보장.

6) 요약 타임라인 (한 접속 기준)
부팅: Init → ServerService Start → Listener가 N개 AcceptEx 미리 걸어둠.

접속: Accept 완료 → Listener가 세션에 ProcessConnect() 호출 → 세션 등록 + 첫 Recv 예약.

수신: 클라 보냄 → Recv 완료 → OnRecv → Send 등록(에코) → Recv 재등록.

송신: Send 완료 → OnSend.

종료: 원격 종료/에러/수동 종료 → Disconnect → DisconnectEx 등록 → 모든 보류 I/O가 돌아오면 세션 소멸.

7) 지금 구조의 강점/개선 포인트
강점

이벤트가 owner=shared_from_this()를 들고 완료까지 생존 보장 → UAF 방지.

Listener가 Accept 이벤트를 풀처럼 재사용 → Accept 압력에 강함.

컨텐츠 훅(OnRecv/OnSend/OnConnected/OnDisconnected)으로 엔진/컨텐츠 분리.

개선

빠른 종료: CancelIoEx + closing 플래그 + generation 토큰을 넣으면 “논리적으로 종료” 후 늦게 도착한 완료는 무시 가능 → 리소스 빨리 회수.

Send 메모리/락 개선: 지금은 new/delete와 큰 락 범위. 전용 이벤트/버퍼 풀 + 전송 큐로 바꾸면 경합↓ GC 비용↓.

에러 처리: HandleError에서 원인별 로깅/상태 정리 구체화.

*/