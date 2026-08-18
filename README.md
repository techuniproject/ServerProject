# Server-Client Project

> 1인 개인 프로젝트 (2025.10 ~ 2026.01)  
> Windows IOCP / WinAPI 기반 멀티플레이어 2D 게임

[![YouTube Demo](https://img.shields.io/badge/YouTube-Demo-red?logo=youtube)](https://www.youtube.com/watch?v=ServerClientProject)

---

## 1. 프로젝트 소개

단일 클라이언트 2D 게임을 멀티플레이어로 전환하면서 발생하는 **동기화 문제**를 직접 설계하고 해결한 프로젝트입니다.

**응답성은 클라이언트, 무결성은 서버**가 담당하는 역할 분리 구조를 기반으로,  
Sector(섹터) 기반 AOI(Area of Interest) 시스템을 설계하여 네트워크 트래픽과 CPU 연산 비용을 대폭 절감했습니다.

| 항목 | 내용 |
|------|------|
| 개발 기간 | 2025.10 ~ 2026.01 (4개월) |
| 인원 | 1인 개인 프로젝트 |
| 소속 | 개인 프로젝트 |

---

## 2. 기술 스택

| 분류 | 사용 기술 |
|------|-----------|
| Language | C++17 |
| Client | WinAPI (2D 렌더링, 상태머신, 애니메이션, 카메라, UI) |
| Server | IOCP 비동기 TCP |
| 직렬화 | Protobuf (Google Protocol Buffers) |
| IDE | Visual Studio 2022 |
| Platform | Windows 10 / 11 |

---

## 3. 시스템 구조

```
[Client — WinAPI]          [Server — IOCP]
  MyPlayer                   GameSession (per client)
   ├── 키 입력 감지             ├── 패킷 수신 (비동기 I/O)
   ├── 클라 예측 이동           ├── ServerPacketHandler
   └── 서버 보정 수신           └── GameRoom (Job Queue)
                                     ├── Player / Monster / Arrow
                                     ├── Sector[7x5] (AOI)
                                     └── BroadcastBySector()
```

### 패킷 구조 (Protobuf 직렬화)

```
C_Move   → 클라이언트 이동 의도 전송
S_Move   → 서버 검증 후 위치 브로드캐스트
C_ARROW  → 화살 발사 의도 전송
S_AddObject / S_RemoveObject → 섹터 경계 이동 시 오브젝트 동기화
S_BROADCAST → 섹터 진입/이탈 시 월드 스냅샷 전송
```

---

## 4. 핵심 구현

### 4-1. Sector 기반 AOI (Area of Interest)

#### 도입 배경

전체 Broadcast 방식은 클라이언트/서버 모두에 비효율적입니다.

- **서버**: 몬스터 N마리 × 플레이어 M명 → O(N×M) 패킷 전송
- **클라이언트**: 화면에 없는 오브젝트까지 메모리에 상주, 렌더링 비용 낭비

이를 해결하기 위해 맵을 **480×480px 크기의 섹터로 분할**하고,  
플레이어 주변 **3×3 영향권 9개 섹터**에 대해서만 동기화를 수행합니다.

#### 섹터 설계

```
맵 크기: 3024 x 2064 px
타일 크기: 48 x 48 px  →  셀 좌표: 63 x 43
섹터 크기: 480 x 480 px (타일 10x10)
섹터 개수: 7 x 5 = 35섹터
클라이언트 카메라: 800 x 600 px
3x3 영향권: 1440 x 1440 px  >  카메라(800x600) → 팝인 방지
```

섹터 좌표 변환은 단순 나눗셈으로 처리합니다.

```cpp
sectorX = cellPos.x / 10;
sectorY = cellPos.y / 10;
```

#### 섹터 컨테이너 — 1차원 배열 + Raw Pointer

```cpp
// GameRoom.h
vector<Sector> _gameRoomSector; // 1차원 배열 — 캐시 지역성 확보
// 접근: _gameRoomSector[sectorPos.y * SECTOR_WIDTH + sectorPos.x]

// Sector.h
struct Sector {
    vector<Player*>  _sectorPlayers;   // Raw pointer — 수명 관리 분리
    vector<Monster*> _sectorMonsters;
    vector<Arrow*>   _sectorArrows;
};
```

- **1차원 배열**: 메모리 연속성 확보로 캐시 지역성 극대화
- **Raw Pointer 사용**: Sector는 공간 인덱싱 역할만 수행. 수명 관리는 `GameRoom`의 `shared_ptr` 컨테이너가 담당하여 역할 분리
- `weak_ptr`의 `lock()` 원자적 연산 비용을 매 틱마다 지불하지 않기 위한 의도적 선택

#### O(1) 삭제 — Index 캐싱 + swap-pop

```cpp
// 기존: find() 후 erase() → O(N) 시프트
// 개선: 객체가 자신의 배열 인덱스를 캐싱 → swap-pop으로 O(1) 삭제
players[idx] = players.back();
players[idx]->SetCurSectorIndex(idx); // 교체된 원소 인덱스 갱신
players.pop_back();
```

#### BroadcastBySector — 전체 → 인접 9섹터 한정

```cpp
// GameRoom.cpp
void GameRoom::BroadcastBySector(SendBufferRef sendBuffer, Vec2Int SectorPos) {
    if (SectorPos == Vec2Int(-1, -1)) return;

    for (int i = 0; i < 9; ++i) {
        Vec2Int nextFindSectorPos{ SectorPos.x + dirX[i], SectorPos.y + dirY[i] };
        if (CheckValidSectorPos(nextFindSectorPos)) {
            Sector* nextFindSector = GetSectorAt(nextFindSectorPos);
            if (nextFindSector) {
                for (Player* pl : nextFindSector->_sectorPlayers)
                    pl->session->Send(sendBuffer);
            }
        }
    }
}
```

**성능 개선 결과 (몬스터 500마리 기준)**

| 방식 | FPS | Delta Time |
|------|-----|------------|
| 전체 Broadcast | 122 | 0.0086527 |
| Sector Broadcast | 278 | 0.0041603 |
| Sector + 렌더링 컬링 | 518 | 0.001944 |

---

### 4-2. 섹터 경계 이동 — 차집합 브로드캐스트

섹터 이동 시 단순히 새 9섹터 전체를 동기화하면 이미 알고 있는 오브젝트까지 중복 전송됩니다.  
**교집합(이미 동기화된 6섹터)을 제외한 차집합(새로 진입/이탈한 3섹터)에 대해서만** Add/Remove 브로드캐스트를 수행합니다.

```
이동 전 9섹터  →  이동 후 9섹터 (오른쪽 1칸)

[교집합 6섹터]: 이미 동기화 완료 → 제외
[차집합 Add  3섹터]: 신규 진입 섹터 월드 정보 전송
[차집합 Remove 3섹터]: 이탈 섹터 오브젝트 제거 패킷 전송
```

```cpp
// GameRoom.cpp
void GameRoom::DoSomethingCrossingSectors(
    Vec2Int curCellPos, Vec2Int lastCellPos,
    function<void(Vec2Int)> add,
    function<void(Vec2Int)> remove)
```

**Add Edge Case 처리**: 두 클라이언트가 동시에 서로 반대 방향의 섹터 경계를 교차할 때 Spawn 패킷이 중복 수신될 수 있습니다.  
서버 objectId 기반 조회로 중복 생성을 방지합니다.

```cpp
if (GetGameObject(info.objectid()) == nullptr) { /* 생성 */ }
```

---

### 4-3. 섹터 경계 Flickering 방지

섹터 경계 근처에서 좌우로 반복 이동 시, 매 전환마다 Add/Remove 브로드캐스트가 반복 발생합니다.

**해결**: 진입 방향에 따라 섹터 전환 경계선을 **1칸(48px) 연장**합니다.

```
카메라 시야 반지름(400px) < 신규 전환 경계선(472px) < 섹터 폭(480px)
```

이 조건을 만족하면 다음 섹터가 시야에 들어오기 전에 미리 동기화가 완료되어 팝인 현상이 차단됩니다.

```
기존: Cell 9 ↔ 0 전환 시 즉시 섹터 이동 판정 → 경계 1칸 진동 시 반복 브로드캐스트
신규: 우측 이동은 0→1 도달 시 / 좌측 이동은 9→8 도달 시 전환
```

---

### 4-4. 클라이언트 예측 이동 + 서버 4단계 검증

네트워크 레이턴시로 인한 입력 지연을 숨기기 위해 클라이언트가 먼저 이동하고, 서버가 검증합니다.

#### 클라이언트 흐름

```
[1] 키 입력 → 다음 셀 이동 가능 여부 확인
[2] 이동 패킷 서버 전송 (목표 좌표 + 방향)
[3] 즉시 클라 예측 이동 시작 (이동 속도 비례 보간)
[4] 서버 검증 실패 시 → 서버 좌표로 강제 보정
```

#### 서버 4단계 검증

```cpp
// [1] 세션 플레이어 ID 일치 확인 — 다른 플레이어 조작 차단
if (curSessionPlayer->GetObjectID() != pkt.info().objectid()) return;

// [2] 인접 1칸만 허용 — Cell 1칸씩 이동하기 때문
int dist = abs(nextPos.y - curPos.y) + abs(nextPos.x - curPos.x);
// 초과 시 서버 최신 좌표로 강제 동기화

// [3] 스피드 핵 방어 — 네트워크 오차 10% 허용
uint64 TimeByMoveSpeed = (uint64)((48.0f / speed) * 1000);
TimeByMoveSpeed = (uint64)(TimeByMoveSpeed * 0.9f);
if (now - curSessionPlayer->GetLastMoveTime() < TimeByMoveSpeed) return;

// [4] 충돌 검사 — 벽 및 다른 객체와의 충돌 최종 판정
```

---

### 4-5. Arrow 동기화 — 3단계 발전

| 단계 | 방식 | 문제 |
|------|------|------|
| 1단계 | 클라 권위 — 클라가 화살 생성 후 위치를 서버로 전달 | 검증 없음, 전체 Broadcast |
| 2단계 | 서버 권위 + Sector — 서버가 화살 생성, 매 틱 섹터 기반 이동 패킷 전송 | 다수 화살 시 서버 부하 |
| **3단계** | **서버 권위 + Sector + 클라 예측** | 틱당 패킷 호출 제거 |

**최종 방식 (3단계)**:
- 클라이언트는 공격 의도만 전송 → 서버가 화살 생성 패킷(속도, 방향, 위치) 1회 전송
- 클라이언트는 수신 데이터 + DeltaTime으로 매 프레임 화살 위치를 자체 계산
- **섹터 경계 이동 / 다른 플레이어의 영향권 진입 시에만** Add/Remove 동기화

→ 화살 이동 중 틱당 패킷 전송을 사실상 제거하여 트래픽 부하 절감

---

## 5. AI 활용 방식

코드를 AI에 직접 제공하지 않고, **설계 의도와 문제를 설명한 뒤 토론**하는 방식으로 활용했습니다.

1. 문제 파악 및 구현 목표 정의 (본인)
2. 설계 방식에 대해 AI와 트레이드오프 토론
3. 종합적 요소를 검토하여 최종 판단 후 본인이 직접 구현
4. 구현 코드를 기반으로 버그 지점 탐색 및 디버깅

---

## 6. 빌드 방법

### 요구 환경

| 항목 | 버전 |
|------|------|
| OS | Windows 10 / 11 (64-bit) |
| IDE | Visual Studio 2022 |
| 추가 | Protobuf 설치 필요 |

### 빌드 순서

```
1. Server/Server.sln 열기
2. 빌드 구성: x64 / Debug 또는 Release 선택
3. Server 프로젝트 빌드 후 실행
4. Client/GameCoding.sln 열기
5. GameCoding 프로젝트 빌드 후 실행 (서버 먼저 실행 필요)
```
