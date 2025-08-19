#pragma once
#include <type_traits>

template<typename T>
inline T* SceneManager::GetCurrentScene() noexcept {
    static_assert(std::is_base_of_v<Scene, T>);
    return dynamic_cast<T*>(_scene.get());
}

template<typename T>
inline const T* SceneManager::GetCurrentScene() const noexcept {
    static_assert(std::is_base_of_v<Scene, T>);
    return dynamic_cast<const T*>(_scene.get());
}

/*
std::is_base_of_v<Scene, T>
→ T가 Scene의 (직/간접) 파생형인지 컴파일 타임에 bool로 알려줍니다.

.inl 포함
.inl 파일은 헤더 끝에서 #include 됩니다. 그래서 모든 사용처(각 .cpp) 가 템플릿 정의 본문을 “본 상태”로 컴파일됩니다. (ODR 위반 방지를 위해 보통 inline로 정의)

템플릿 인스턴스화 시점
호출부에서 GetCurrentScene<DevScene>()를 쓴 순간, 컴파일러가 그 조합으로 코드를 생성합니다.

정적 검사 실행
그 생성 과정에서 static_assert(std::is_base_of_v<Scene, T>)가 평가됩니다.
조건을 만족하지 않으면 그 자리에서 컴파일 에러가 납니다(런타임 X).

기계어 없음
static_assert/is_base_of_v는 형(타입) 단계 계산이라 런타임 오버헤드가 없습니다.

*/