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
