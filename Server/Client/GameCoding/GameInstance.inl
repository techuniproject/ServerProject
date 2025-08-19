#pragma once
#include "SceneManager.h" // ⬅️ 여기서 SceneManager 템플릿 정의를 보이게 함

template<typename T>
inline T* GameInstance::GetCurrentScene() noexcept {
    return _SceneManager->template GetCurrentScene<T>();
}

template<typename T>
inline const T* GameInstance::GetCurrentScene() const noexcept {
    return _SceneManager->template GetCurrentScene<T>();
}
