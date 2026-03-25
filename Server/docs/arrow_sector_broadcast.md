# 화살 섹터 브로드캐스트 구조 정리

## 최종 구조 요약

화살은 수명이 짧고 직선 이동하는 결정론적 객체이므로,
일반 객체(플레이어/몬스터)와 다른 단순한 생애주기 관리를 채택한다.

```
[생성]  서버 Arrow::UpdateIdle() (_ifSpawned)
         → BroadcastBySector(S_AddObject, 스폰 위치 기준 3×3)
         → InsertAtSector로 섹터 컨테이너 등록

[이동 중] 화살이 섹터 경계 넘을 때
         → InsertAtSector로 _sectorArrows 갱신
         → DoSomethingCrossingSectors로 경계 플레이어에게 S_AddObject / S_RemoveObject

[플레이어 섹터 이동 시] Handle_C_Move
         → 플레이어/몬스터: delta 3섹터 방식 (DoSomethingCrossingSectors)
         → 화살: 새 시야 범위 9섹터 전수 스캔
             add: 새 3×3 내 모든 _sectorArrows → S_BROADCAST addobjects
             remove: 구 3×3에만 있던 섹터의 _sectorArrows → S_BROADCAST removeobjects

[소멸]  벽 충돌 or 타이머
         → BroadcastBySector(S_RemoveObject, 소멸 위치 기준 3×3)
         → DeleteProjectiles()에서 Leave() → DeleteFromSector
```

---

## 버그 원인 분석

### Bug 1: InsertAtSector 미호출 (근본 원인)

**증상:** 화살이 섹터를 이동해도 `_sectorArrows` 컨테이너가 갱신되지 않음.
스폰 위치 섹터에 화살이 영구 등록된 상태로 남음.

**원인:** `Arrow::UpdateIdle()`에서 `SetCellPos(nextPos)` 후 `InsertAtSector`를 호출하지 않음.
- `_ifSpawned` 경로: 첫 이동 후 S_AddObject만 보내고 섹터 갱신 없음
- 섹터 경계 이동 경로: `DoSomethingCrossingSectors`로 알림만 보내고 섹터 갱신 없음

**수정:** `SetCellPos(nextPos)` 직후 항상 `InsertAtSector` 호출.
같은 섹터면 early return이라 비용 없음.

---

### Bug 2: 동일 틱 내 순서 경쟁 (핵심 버그)

**증상:** 플레이어가 왔다갔다 할 때 화살이 간헐적으로 안 보임.

**원인:** 메인 스레드 한 틱 안에서 순서가 고정되어 있음.

```
FlushJobs()  → C_Move 처리 (플레이어 섹터 이동) → 이 시점 화살 위치 기준으로 delta 3섹터 체크
Update()     → Arrow::UpdateIdle() (화살 섹터 이동) → 이 시점 플레이어 위치 기준으로 알림
```

**재현 시나리오:**
```
화살: 섹터 (2,3) → 이번 틱에 (1,3)으로 이동 예정
플레이어: 섹터 (3,3) → (2,3)으로 이동

[FlushJobs] 플레이어 이동 처리:
  delta add 대상 = 섹터 (1,x)
  화살은 아직 (2,3) → add 대상 아님 → 플레이어에게 화살 S_AddObject 미전송

[Update] 화살 이동:
  delta remove 대상 = 섹터 (3,x)
  플레이어는 이미 (2,3) → remove 대상 아님 → 제거 패킷도 미전송

결과: 플레이어 (2,3), 화살 (1,3) → 시야 범위 내인데 클라에서 화살 없음
```

**수정:** 플레이어 섹터 이동 시 화살에 한해 delta 방식 대신 **새 시야 범위 9섹터 전수 스캔**으로 전환.
타이밍과 무관하게 현재 `_sectorArrows` 상태를 기준으로 가시성을 결정.

---

### Bug 3: 클라 Handle_S_BROADCAST 화살 케이스 누락

**증상:** 서버가 S_BROADCAST에 화살 정보를 담아 보내도 클라에서 무시됨.

**원인:** `Handle_S_BROADCAST`의 addobjects 처리 루프에
`OBJECT_TYPE_PROJECTILE` else if 케이스가 없었음.

화살의 주 알림 경로가 `S_AddObject` 직접 전송이라 대부분의 경우 동작하고 있었고,
"플레이어 섹터 이동 시 이미 그 섹터에 화살이 있는 경우"만 누락되어 증상이 간헐적으로만 나타남.

**수정:** `Handle_S_BROADCAST` addobjects 루프에 `OBJECT_TYPE_PROJECTILE` 케이스 추가.
`GetGameObject(id) == nullptr` 중복 체크로 이미 알고 있는 화살은 재생성 안 함.

---

## 트레이드오프

### 화살을 섹터 컨테이너로 관리하는 것 자체의 트레이드오프

| | 섹터 관리 O (현재) | 섹터 관리 X (전역 리스트) |
|---|---|---|
| 플레이어 이동 시 화살 탐색 비용 | O(9섹터 내 화살 수) | O(전체 화살 수) |
| 화살 이동 시 갱신 비용 | InsertAtSector O(1) | 없음 |
| 구현 복잡도 | 높음 | 낮음 |
| 맵이 커질수록 | 유리 | 불리 |

현재 맵 규모(5×7 섹터)에서는 어느 쪽이든 부하 차이가 작지만,
섹터 관리가 이후 맵 확장 시 확장성이 좋음.

---

### 화살에 delta 방식 대신 9섹터 전수 스캔을 쓰는 트레이드오프

**장점:**
- FlushJobs/Update 순서 경쟁 문제를 구조적으로 차단
- 구현이 단순하고 예외 케이스 없음

**단점:**
- 플레이어가 섹터를 넘을 때마다 최대 9섹터 × 화살 수만큼 순회
- 이미 알고 있는 화살도 S_BROADCAST에 포함되어 전송됨 (중복 패킷)
- 클라에서 `GetGameObject` 중복 체크로 재생성은 막히지만 패킷 자체는 소비됨

**허용 가능한 이유:**
- 화살 수가 많지 않음 (플레이어 수 × 공격 빈도)
- 플레이어 섹터 이동은 타일 단위 입력이므로 빈도가 높지 않음
- 수명이 짧아 잉여 화살이 컨테이너에 오래 남지 않음

---

### 화살 소멸 시 S_RemoveObject 범위 문제 (미해결 잠재 이슈)

현재 소멸 패킷은 **소멸 위치 기준 BroadcastBySector(3×3)** 으로만 전송됨.
화살을 생성 시점에 알게 된 플레이어가 이후 다른 섹터로 이동한 경우,
소멸 위치의 3×3 밖에 있으면 제거 패킷을 못 받을 수 있음.

실용적 완화책:
- 클라에서 화살에 최대 비행 타이머를 두어 서버 소멸 패킷 없이도 자체 제거
- 또는 소멸 시 `Broadcast` (전체 전송)로 전환 — 화살 소멸은 드문 이벤트라 부하 미미

---

## 수정 파일 목록

| 파일 | 변경 내용 |
|---|---|
| `Server/Arrow.cpp` | `UpdateIdle()` — `SetCellPos` 직후 `InsertAtSector` 호출 추가 |
| `Server/ServerPacketHandler.cpp` | `Handle_C_Move` — 화살을 delta 람다에서 분리, 9섹터 전수 스캔으로 교체 |
| `Client/ClientPacketHandler.cpp` | `Handle_S_BROADCAST` — addobjects 루프에 `OBJECT_TYPE_PROJECTILE` 케이스 추가, 중복 체크 유지 |
