#pragma once


class InputManager
{
	//DECLARE_SINGLE(InputManager);

public:
	void Init(HWND hwnd);
	void Update();

	// 누르고 있을 때
	bool GetButton(KeyType key) { return GetState(key) == KeyState::Press; }

	// 맨 처음 눌렀을 때
	bool GetButtonDown(KeyType key) { return GetState(key) == KeyState::Down; }

	// 맨 처음 눌렀다가 땔 때
	bool GetButtonUp(KeyType key) { return GetState(key) == KeyState::Up; }

	POINT GetMousePos() { return _mousePos; }

private:
	KeyState GetState(KeyType key) { return _states[static_cast<uint8>(key)]; }

private:
	HWND _hwnd = 0;
	vector<KeyState> _states;
	POINT _mousePos;
};

