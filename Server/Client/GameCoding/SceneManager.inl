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
�� T�� Scene�� (��/����) �Ļ������� ������ Ÿ�ӿ� bool�� �˷��ݴϴ�.

.inl ����
.inl ������ ��� ������ #include �˴ϴ�. �׷��� ��� ���ó(�� .cpp) �� ���ø� ���� ������ ���� ���¡��� �����ϵ˴ϴ�. (ODR ���� ������ ���� ���� inline�� ����)

���ø� �ν��Ͻ�ȭ ����
ȣ��ο��� GetCurrentScene<DevScene>()�� �� ����, �����Ϸ��� �� �������� �ڵ带 �����մϴ�.

���� �˻� ����
�� ���� �������� static_assert(std::is_base_of_v<Scene, T>)�� �򰡵˴ϴ�.
������ �������� ������ �� �ڸ����� ������ ������ ���ϴ�(��Ÿ�� X).

���� ����
static_assert/is_base_of_v�� ��(Ÿ��) �ܰ� ����̶� ��Ÿ�� ������尡 �����ϴ�.

*/