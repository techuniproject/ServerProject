# C++ IOCP 멀티플레이 게임 서버

Windows IOCP(I/O Completion Port) 기반의 고성능 멀티플레이 게임 서버입니다.
네트워킹 엔진부터 게임 로직, AI NPC까지 직접 구현하였으며, 2계층 구조(엔진 / 컨텐츠)로 설계했습니다.

---

## 기술 스택

| 분류 | 기술 |
|---|---|
| 언어 | C++17 |
| 네트워크 | Windows IOCP (Winsock2, AcceptEx / WSARecv / WSASend) |
| 직렬화 | Protocol Buffers (protobuf 3) |
| AI 연동 | OpenAI Responses API, WinHTTP, SSE 스트리밍 |
| 코드 생성 | Python + Jinja2 (패킷 핸들러 자동 생성) |
| 빌드 | Visual Studio 2022, x64 |

---

## 아키텍처 개요

```
┌──────────────────────────────────────────────────────┐
│                     Server (게임 컨텐츠)               │
│  GameRoom  ─  ServerPacketHandler  ─  GameSession     │
│  GameObject / Creature / Player / Monster / Arrow     │
│  Sector 공간분할  ─  A* 경로탐색  ─  AIWorkerPool      │
├──────────────────────────────────────────────────────┤
│                  ServerCore (네트워크 엔진)             │
│  IocpCore  ─  Listener  ─  Session/PacketSession      │
│  Service  ─  ThreadManager  ─  Send/RecvBuffer        │
└──────────────────────────────────────────────────────┘
```

### ServerCore — 재사용 가능한 IOCP 엔진

게임 로직을 전혀 모르는 순수 네트워킹 레이어입니다.

- **IocpCore**: `CreateIoCompletionPort` / `GetQueuedCompletionStatus` 래핑. 완료 이벤트를 꺼내 `IocpObject::Dispatch()`로 분배
- **Listener**: `AcceptEx`를 미리 여러 개 걸어두는 방식으로 Accept 파이프라인 유지. Accept 이벤트 재사용
- **Session / PacketSession**: 비동기 TCP 세션. `PacketSession`이 `PacketHeader(size + id)` 프레이밍 처리 후 `OnRecvPacket()`으로 완성된 패킷만 전달
- **Service**: IOCP 핸들, 세션 집합, Listener를 소유하는 서버/클라이언트 서비스 추상화
- **ThreadManager**: 스레드 생성·조인 및 TLS 초기화/정리

### Server — 게임 컨텐츠 레이어

- **GameSession**: `PacketSession`을 상속, `GameRoom`과 `Player`에 대한 약참조(weak_ptr) 보유
- **ServerPacketHandler**: 패킷 ID → 핸들러 함수 정적 테이블. Protobuf 역직렬화 후 `Handle_C_*` 함수 호출
- **GameRoom**: 게임 상태 싱글턴(`GRoom`). 플레이어·몬스터·화살·아이템 관리, 경로탐색, 몬스터 스폰

---

## 핵심 구현

### 1. 스레드 모델 및 Job Queue 패턴

```
IOCP Worker × 5  ──PushJob()──▶  GameRoom::_jobs (JobQueue)
                                         │
                                 Main Thread (FlushJobs + Update)
                                         │
AI Worker × 2    ──PushJob()──▶  GameRoom::_jobs
```

- **IOCP 워커 5개**: 무한 루프로 `IocpCore::Dispatch()` 호출. 패킷을 수신하면 게임 상태를 직접 수정하지 않고 `GRoom->PushJob(lambda)`으로 위임
- **메인 스레드**: `FlushJobs()` → `Update()` 루프. 게임 상태에 단독 접근하므로 별도 락 없이 권위적(authoritative) 로직 수행
- **AI 워커 2개**: OpenAI API 블로킹 HTTP를 별도 스레드에서 처리, 응답 도착 시 역시 `PushJob()`으로 메인 스레드에 전달

이 구조로 I/O 멀티스레딩과 게임 로직 단일 스레드를 동시에 달성합니다.

### 2. 섹터 기반 공간 분할 & 선택적 브로드캐스트

맵을 **5 × 7 그리드**(섹터)로 나누어, 이동/공격 패킷을 전체가 아닌 **인접 3×3 섹터의 플레이어**에게만 전송합니다.

```
┌───┬───┬───┬───┬───┬───┬───┐
│ 0 │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │  ← SECTOR_WIDTH = 7
├───┼───┼───┼───┼───┼───┼───┤
│ 7 │ 8 │███│███│███│   │   │  ← 이동한 플레이어 주변 9칸에만 브로드캐스트
├───┼───┼───┼───┼───┼───┼───┤
│14 │   │███│ P │███│   │   │  P: 플레이어 현재 위치
├───┼───┼───┼───┼───┼───┼───┤
│21 │   │███│███│███│   │   │
├───┼───┼───┼───┼───┼───┼───┤
│28 │   │   │   │   │   │34 │  ← SECTOR_HEIGHT = 5
└───┴───┴───┴───┴───┴───┴───┘
```

- 각 섹터는 `vector<Player*>`, `vector<Monster*>`, `vector<Arrow*>` 보유
- 섹터 배열 내 삭제: **swap-and-pop** 패턴으로 O(1) 제거
- 크로스 섹터 이동 시 `DoSomethingCrossingSectors(add, remove)` 콜백으로 새 섹터 진입 플레이어에게 AddObject, 이탈 섹터 플레이어에게 RemoveObject 전송

### 3. 수명 관리 — UAF 방지

IOCP 비동기 I/O에서 가장 취약한 지점은 I/O 완료 전 세션 소멸입니다.

```cpp
// 비동기 I/O 등록 시 강참조로 고정
_recvEvent.owner = shared_from_this();  // shared_ptr<Session>

// 완료 후 워커가 꺼낼 때
IocpObjectRef obj = iocpEvent->owner;   // 완료 처리 동안 생존 보장
obj->Dispatch(iocpEvent, numBytes);
iocpEvent->owner = nullptr;             // 강참조 해제 → 이후 소멸 가능
```

- `IocpObject : enable_shared_from_this` 상속으로 자기 자신의 `shared_ptr`을 안전하게 생성
- 모든 보류 중인 I/O가 완료돼 `owner` 참조가 모두 풀릴 때 비로소 세션 소멸 → **지연 파괴(deferred destruction)** 패턴

### 4. 서버 권위적(Server-Authoritative) 이동·전투

```
클라이언트                        서버
─────────────────────────────────────────────────
입력 → 로컬 예측 이동             C_Move 수신
애니메이션 재생        ────▶     속도·거리·충돌 검증
                                  통과 시 위치 반영
                       ◀────     S_Move 브로드캐스트
다른 클라이언트 상태 보간
```

- 이동 속도 기반 최소 패킷 간격(minIntervalMs) 검증으로 속도핵 방어
- 1타일 초과 이동 거리 차단
- 섹터 기반 충돌 감지(`CanGoBySector`)

### 5. 몬스터 AI — A* 경로탐색

```cpp
// Monster::UpdateIdle()
_target = room->FindClosestPlayerBySector(GetCellPos());  // 섹터 탐색
room->FindPath(GetCellPos(), _target->GetCellPos(), path, maxDepth);  // A*
// 다음 타일로 이동 → 섹터 갱신 → BroadcastMoveBySector()
```

- **FindClosestPlayerBySector**: 전체 플레이어 순회 대신 인접 섹터만 탐색
- **maxDepth** 제한으로 경로탐색 비용 상한 설정
- 몬스터 수 `DESIRED_COUNT(=500)`를 유지하는 자동 스폰 시스템

### 6. 패킷 프로토콜 — Protobuf + 자동 코드 생성

```
Common/protobuf/Protocol.proto
       │
       ▼ GenPackets.bat (protoc + 커스텀 제너레이터)
       │
       ├── Server/Enum.pb.h / Protocol.pb.h / Struct.pb.h
       ├── Server/ServerPacketHandler.h  (자동 생성)
       └── Client/ClientPacketHandler.h (자동 생성)
```

패킷 구조: `PacketHeader { uint16 size; uint16 id; }` + Protobuf 직렬화 페이로드

### 7. AI NPC — LLM 연동 (OpenAI Responses API)

채팅 메시지 `/npc <텍스트>` 또는 `/ai <텍스트>` 입력 시 LLM 응답을 NPC 대사로 브로드캐스트합니다.

```
채팅 패킷 수신
    │ /npc 또는 /ai 감지
    ▼
AIQueue::Push(AIRequest)          ← 논블로킹, 즉시 반환
    │
AI Worker Thread (× 2)
    │ WinHTTP + HTTPS POST to api.openai.com
    │ SSE 스트리밍 수신 → 텍스트 조각 누적
    ▼ response.completed 이벤트
GRoom->PushJob(lambda)            ← 메인 스레드에서 안전하게 처리
    │
S_CHAT(playerId=999) 브로드캐스트 ← NPC 대사로 전체 전송
```

- **WinHTTP** 동기 HTTP 클라이언트를 별도 AI 워커 스레드에서 실행하여 게임 루프 블로킹 없음
- **SSE(Server-Sent Events)** 스트리밍 파싱: `data:` 접두사 추출 → JSON 파싱 → delta 누적
- `OPENAI_API_KEY` 환경 변수에서 API 키 로드 (코드에 하드코딩 없음)
- `#define FEATURE_LLM_CHAT 0` 으로 컴파일 타임 비활성화 가능

---

## 게임 시스템

| 시스템 | 구현 내용 |
|---|---|
| 입장/퇴장 | 접속 시 `S_EnterGame` + `S_MyPlayer` + `S_AddObject`(기존 객체 목록) 전송 |
| 이동 | 클라이언트 예측 + 서버 검증 → `S_Move` 브로드캐스트 |
| 전투 | 몬스터 근접 공격(SKILL 상태), 화살(Projectile) 시스템 |
| 아이템 | 몬스터 사망 시 드랍 (`S_ITEM`), 플레이어 이동으로 획득. 공격속도/이동속도/힐 3종 |
| 채팅 | 전체 브로드캐스트 채팅 + `/npc` 명령으로 LLM NPC 대화 |

---

## 빌드 및 실행

### 요구사항
- Windows 10/11, Visual Studio 2022
- Protobuf 3 (사전 컴파일된 라이브러리 포함)

### 빌드
```
Server.sln 를 Visual Studio로 열기
플랫폼: x64 / 구성: Debug 또는 Release
빌드: Ctrl+Shift+B
```

### 프로토콜 변경 시 재생성
```bat
Common\protoc-21.12-win64\bin\GenPackets.bat
```

### 실행
```
# AI 기능 사용 시 환경 변수 설정
set OPENAI_API_KEY=sk-...

# Server 프로젝트를 시작 프로젝트로 설정 후 F5
# 서버: 127.0.0.1:7777 리슨
```

---

## 프로젝트 구조

```
Server.sln
├── ServerCore/          # IOCP 네트워크 엔진 (정적 라이브러리)
│   ├── IocpCore.h/cpp   # IOCP 핸들 관리, 완료 이벤트 분배
│   ├── Session.h/cpp    # TCP 세션, 비동기 Recv/Send 큐
│   ├── Listener.h/cpp   # AcceptEx 파이프라인
│   ├── Service.h/cpp    # ServerService / ClientService
│   └── ThreadManager    # 스레드 생성·조인
│
├── Server/              # 게임 서버 (실행 파일)
│   ├── GameRoom         # 게임 상태 권위 관리자 (싱글턴 GRoom)
│   ├── GameObject       # 엔티티 기반 클래스 (ID, 위치, HP, 섹터)
│   ├── Creature         # 상태머신 (Idle/Move/Skill/Hit)
│   ├── Player / Monster / Arrow
│   ├── Sector           # 공간 분할 셀
│   ├── Tilemap          # 맵 타일 충돌 데이터
│   ├── ServerPacketHandler  # 패킷 디스패치 테이블
│   ├── AIQueue / AIWorker   # LLM 비동기 처리
│   └── Server.cpp       # main: 서비스 구성, 스레드 가동
│
├── Common/protoc/       # .proto 정의 파일 + protoc 실행 파일
├── Tools/PacketGenerator/  # Python/Jinja2 코드 생성기
├── DummyClient/         # 부하 테스트 클라이언트
└── Client/              # 게임 클라이언트 (별도 프로젝트)
```
