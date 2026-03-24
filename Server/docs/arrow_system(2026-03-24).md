# 화살(Arrow) 시스템 정리

## 1. 전체 흐름도

```
[클라이언트]                          [서버]
    |                                    |
    | C_Move(state=SKILL, pos=P0)        |
    |--------------------------------->  |
    |                                    | Handle_C_Move()
    |                                    |   player.state = SKILL
    |                                    |   player.pos = P0
    |                                    |
    |                          [매 tick] Player::UpdateSkill()
    |                                    |   _waitUntil > now? → return (대기)
    |                                    |   ... attackspeed * 850ms 경과 후 ...
    |                                    |   Arrow 생성 (pos = P0)
    |                                    |   gameRoom->Enter(arrow)
    |                                    |
    |                          [같은 tick] Arrow::UpdateIdle() (첫 이동)
    |                                    |   CanGoBySector(P1)?
    |                                    |   YES → SetCellPos(P1)
    |                                    |         S_AddObject(pos=P1) 전송  <--
    |                                    |         _waitUntil = now + 48/movespeed*1000ms
    |                                    |         SetState(MOVE)
    |                                    |   NO  → S_RemoveObject 전송 (즉시 소멸)
    |                                    |
    | S_AddObject(pos=P1) 수신           |
    | dead reckoning 시작 (P1 기준)      |
    |                                    |
    |                          [100ms 후] Arrow::UpdateMove()
    |                                    |   _waitUntil 경과 → SetState(IDLE)
    |                                    |
    |                          [다음 tick] Arrow::UpdateIdle()
    |                                    |   CanGoBySector(P2)?
    |                                    |   YES → 이동 반복
    |                                    |   NO  → 충돌 처리
    |                                    |         Monster/Player 피격 판정
    |                                    |         S_RemoveObject 전송
    |                                    |         AddDeleteProjectiletoList()
    |                                    |
    | S_RemoveObject 수신                |
    | 화살 제거                          |
    |                          [tick 말미] DeleteProjectiles() → Leave()
```

---

## 2. 화살 생성 타이밍 (85% 딜레이)

### 설계 의도
활 쏘는 애니메이션이 끝나는 시점에 화살이 나타나야 자연스러움.
`attackspeed`가 클라이언트 애니메이션 지속시간(초 단위)으로 연동되어 있으므로,
그 85%가 지난 후 서버에서 화살을 생성하고 S_AddObject를 보내면
RTT를 감안해도 애니메이션 종료 시점에 화살이 렌더링됨.

### 구현 위치: `Player::UpdateIdle()`
```cpp
void Player::UpdateIdle()
{
    _waitUntil = GetTickCount64() + info.attackspeed() * 850;
    // attackspeed * 1000ms = 애니메이션 전체 길이
    // * 850 = 85% 지점에서 화살 생성
}
```

### 구현 위치: `Player::UpdateSkill()` (BOW)
```cpp
else if (info.weapontype() == Protocol::WEAPON_TYPE_BOW)
{
    if (_waitUntil > now)
        return;  // 아직 대기 중 → SKILL 상태 유지, SetState(IDLE) 도달 안 함

    // 85% 경과 후: 화살 생성 + BroadcastBySector(S_AddObject)
    // ...
    SetState(IDLE);
}
```

### 주의사항
- `return`으로 빠져나오기 때문에 `SetState(IDLE)`이 호출되지 않아 SKILL 상태 유지됨
- `_waitUntil`은 UpdateIdle에서 매 틱 갱신 → SKILL 진입 시점의 값이 유효한 딜레이로 동작
- SWORD는 딜레이 없이 즉시 판정 (if-else 구조로 분리)

---

## 3. S_AddObject 전송 위치 (P1 기준)

### 문제 (수정 전)
```
UpdateSkill(): arrow 생성(P0) → S_AddObject(P0) 전송
같은 tick: Arrow::UpdateIdle() → P0→P1 즉시 이동
→ 클라는 P0 기준으로 dead reckoning 시작하지만 서버는 이미 P1
→ 1타일(100ms) 오차 발생
```

### 수정 후
S_AddObject를 `Arrow::UpdateIdle()` 내부에서 첫 이동 이후에 전송.

```cpp
void Arrow::UpdateIdle()
{
    if (CanGoBySector(nextPos))
    {
        SetCellPos(nextPos);          // P1으로 이동

        if (!_spawned) {
            _spawned = true;
            Protocol::S_AddObject pkt;
            *pkt.add_objects() = info; // pos = P1 기준
            // BroadcastBySector...
        }

        _waitUntil = GetTickCount64() + moveTick;
        SetState(MOVE);
    }
    // ...
}
```

클라이언트가 P1에서 dead reckoning 시작 → 서버 타일 타이밍과 동기화.

---

## 4. 클라이언트 Dead Reckoning 속도

### 수식
```
client_arrow_speed = (movespeed - 100) * dt
```

### 이유
- 수학적으로는 `movespeed * dt`가 정확하지만, RTT로 인해 S_RemoveObject가
  늦게 도착하는 동안 화살이 벽을 살짝 뚫고 들어가는 것처럼 보임
- `(movespeed - 100)`으로 클라 속도를 약간 낮추면 RTT 지연분을 상쇄해
  화살 앞끝이 벽에 닿는 시점에 자연스럽게 제거됨
- 화살 스프라이트에 길이가 있어 중심 기준 판정과의 시각적 오프셋도 함께 보정

### 서버 판정은 변경 없음
서버의 타일 기반 충돌 판정(`48.0f / movespeed * 1000ms`)은 그대로 유지.
클라 렌더링 속도만 보정.

---

## 5. 섹터 간 브로드캐스트 버그 수정

### 문제
`DoSomethingCrossingSectors()`에 같은 섹터 내 이동 시 조기 종료 조건이 없었음.
`dirX == 0 && dirY == 0`인 경우(섹터 미변경)에도 add/remove 콜백이 모두 호출되어
같은 섹터의 플레이어들에게 매 타일마다 S_RemoveObject + S_AddObject가 동시에 전송됨
→ 클라이언트에서 화살이 매 이동마다 제거/재생성되는 것처럼 처리될 수 있음.

### 수정
```cpp
void GameRoom::DoSomethingCrossingSectors(Vec2Int curCellPos, Vec2Int LastCellPos, ...)
{
    Vec2Int CurSectorPos  = GetSectorPos(curCellPos.x, curCellPos.y);
    Vec2Int LastSectorPos = GetSectorPos(LastCellPos.x, LastCellPos.y);

    if (CurSectorPos == LastSectorPos) return; // 추가: 같은 섹터면 아무것도 안 함

    // ...기존 로직
}
```

---

## 6. 공격 속도 부동소수점 버그 수정

### 문제
```cpp
// 수정 전
if (curSessionPlayer->info.attackspeed() > 0.1f) {
    curSessionPlayer->info.set_attackspeed(curSessionPlayer->info.attackspeed() - 0.1f);
}
```
0.1f는 IEEE 754에서 정확히 표현 불가(`0.10000000149...`).
0.5 → 0.4 → 0.3 → 0.2 → 0.1 반복 뺄셈 시 누적 오차로
최솟값 도달 후 한 번 더 감소해 거의 0이 될 수 있음 (화살 속도 폭발).

### 수정
```cpp
// 수정 후
case Protocol::ITEM_TYPE::ITEM_TYPE_ATTACK:
{
    int level = (int)roundf(curSessionPlayer->info.attackspeed() * 10);
    if (level > 1) {
        curSessionPlayer->info.set_attackspeed((level - 1) * 0.1f);
    }
    break;
}
```
정수 레벨로 변환 후 비교 → 부동소수점 누적 오차 완전 제거.

---

## 7. 히트 씹힘 버그 수정 (업데이트 순서)

### 문제
```
기존 순서: Players → Monsters → Arrows

1. Monster::UpdateIdle(): IDLE→MOVE, SetCellPos(P_new) 실행
2. Arrow::UpdateIdle(): CanGoBySector(P_old) 체크
   → GetCreatureAtSector(P_old) = nullptr (몬스터 이미 이동)
   → CanGo = true → 화살 통과 → 히트 씹힘
```

### 수정
```cpp
// GameRoom::Update()
void GameRoom::Update()
{
    for (auto& item : _players)  { item.second->Update(); }
    for (auto& item : _arrows)   { item.second->Update(); } // Arrows 먼저
    for (auto& item : _monsters) { item.second->Update(); } // Monsters 나중
    for (auto& item : _items)    { item.second->Update(); }
    TickMonsterSpawn();
    DeleteProjectiles();
}
```

```
수정 후 순서: Players → Arrows → Monsters

1. Arrow::UpdateIdle(): CanGoBySector(P_old) 체크
   → GetCreatureAtSector(P_old) = 몬스터 아직 여기 있음
   → CanGo = false → 충돌 브랜치 → 히트 정상 처리
2. Monster::UpdateIdle(): IDLE→MOVE, SetCellPos(P_new)
```

### 한계
반대 케이스(몬스터가 화살 쪽으로 이동해 들어오는 순간)는 여전히 엣지케이스로 남음.
완전한 해결을 원하면 틱 시작 시 위치 스냅샷 방식을 추가로 검토.

---

## 8. 관련 파일 목록

| 파일 | 역할 |
|---|---|
| `Server/Player.cpp` | UpdateIdle (딜레이 설정), UpdateSkill (화살 생성 로직) |
| `Server/Arrow.cpp` | UpdateIdle (이동/충돌), UpdateMove (타이밍 대기) |
| `Server/Arrow.h` | `_spawned` 플래그 |
| `Server/GameRoom.cpp` | Update() 순서, DoSomethingCrossingSectors(), DeleteProjectiles() |
| `Server/ServerPacketHandler.cpp` | Handle_C_Move, 공격속도 아이템 처리 |
