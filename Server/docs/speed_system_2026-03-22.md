# Speed 시스템 점검 정리 (2026-03-23)

## 개요

`movespeed` / `attackspeed` 구조 전체 점검 및 버그 발견.
클라이언트-서버 간 속도 단위 불일치, 미설정 초기값, 미구현 로직 등 확인.

---

## 초기값 기준

| 값 | 초기값 | 위치 | 단위 |
|---|---|---|---|
| movespeed | 96 | `GameSession.cpp:48` | px/s |
| attackspeed | 0.5f | `GameSession.cpp:47` | 초 (애니메이션 지속 시간) |

---

## 발견된 버그

### 1. attackspeed 단위 버그 (`MyPlayer.cpp:97`)

```cpp
// 수정 전 (버그)
_nextSkillAt = now + GetAttackSpeed();
// now = ms 단위, GetAttackSpeed() = 초 단위 (0.5f)
// → 쿨타임 = 0.5ms = 사실상 쿨타임 없음
```

```cpp
// 수정 후
_nextSkillAt = now + (uint64)(GetAttackSpeed() * 1000);
```

`now`는 `GetTickCount64()` 기반 ms인데 attackspeed가 초 단위 float이라 발생.
결과적으로 클라이언트에서 공격 쿨타임이 1ms 이하로 적용되어 무한 연사 가능.

---

### 2. Monster movespeed 미설정

```cpp
// Monster.cpp 생성자에서 movespeed 설정 없음
// Protobuf 기본값 = 0
```

클라이언트 TickMove에서 `pos += movespeed * dt`를 사용하는데,
movespeed = 0이면 보간이 전혀 발생하지 않아 몬스터가 현재 위치에서
다음 위치로 순간이동(텔포트)하는 현상 발생.

서버는 500ms/타일 고정 이동이므로, 클라 보간 기준속도를 `movespeed = 96 (px/s)`으로 설정 필요.

---

## 기획 결정 필요 사항

### 3. 이동 캔슬 처리 방식

현재 클라이언트에서 MOVE 상태 중 새 입력이 무시됨 (`TickIdle`에서만 `TickInput` 호출).

| 옵션 | 방식 | 특징 |
|---|---|---|
| A | 타일 이동 완료 후 다음 입력 처리 (현재) | 반응성 낮음, 서버와 동기화 쉬움 |
| B | TickMove에서도 TickInput → 즉시 destPos 재설정 | 반응성 좋음, 서버 판정과 불일치 가능 |

---

### 4. 서버 판정 쿨타임 연동

`GameObject.h`에 `_attackReadyAt`, `_stateExitAt`이 선언되어 있으나 미사용.
`Player::UpdateSkill`의 `_stateExitAt` 체크가 주석처리 상태.

```cpp
// 현재 (주석 처리 중)
//if (now < _stateExitAt) return;
```

활성화 방향:
```cpp
// SKILL 진입 시
_stateExitAt = now + (uint64)(attackspeed * 1000);
// UpdateSkill에서
if (now < _stateExitAt) return;
```

→ 이 부분은 이후 화살 85% 딜레이 구조(`_waitUntil = attackspeed * 850`)로 대체 해결됨.
   (`2026-03-24` 화살 시스템 작업 참조)

---

### 5. 화살 속도와 공격속도 연동 분리 검토

당시 구조:
```cpp
arrowSpeed = attackspeed * 600  // 공격 쿨타임 단축 → 화살도 빨라짐
```

아이템으로 attackspeed가 낮아지면 화살 비행속도도 연동되어 변하는 문제.
공격 빠름 = 화살 느려짐 or 빨라짐인지 기획 의도 확인 필요.

→ 이후 `2026-03-24` 작업에서 아래 공식으로 분리:
```cpp
speed = 480 + 96 * (1 - attackspeed * 2);
// attackspeed 낮아질수록 화살 속도 증가 (공격속도 빠름 = 화살 빠름)
```

---

## 구조 메모

### Player::UpdateIdle의 _waitUntil 용도

서버에서 플레이어 실제 이동 제한은 `Handle_C_Move`의 `minIntervalMs` 검증으로 처리.
`UpdateIdle`의 `_waitUntil` 설정은 별도 용도(당시 미활용 상태).

→ `2026-03-24`에 `_waitUntil = attackspeed * 850`으로 화살 발사 딜레이 용도로 전용.

### _waitUntil 겸용 구조 관련

`Player::UpdateSkill`에서 `_waitUntil` 체크로 BOW 딜레이를 구현하는데,
이 값이 `UpdateIdle`에서 설정되는 이동 대기값과 같은 변수를 공유.

더 명확하게는 `_attackWaitUntil`로 전용 변수 분리가 적절하나,
현재 구조에서 UpdateIdle이 매 틱 갱신하므로 실질적으로 동작상 문제없음.

---

## 다음 작업 (→ 2026-03-24 이어서 진행)

- [x] 서버 판정 쿨타임 (화살 85% 딜레이 구조로 해결)
- [x] 화살 속도 분리 (별도 공식으로 분리)
- [ ] Monster movespeed 초기값 설정 확인
- [ ] 이동 캔슬 방식 기획 결정
