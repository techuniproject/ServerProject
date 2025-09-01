#pragma once

class Game
{
public:
	Game();
	~Game();

public:
	void Init(HWND hwnd, HINSTANCE hInstance);
	void Update();
	void Render();

private:
	HWND _hwnd = {};
	HDC hdc = {};
	HINSTANCE _hInstance = {};
public:
	//채팅창 핸들
	static HWND _chathwnd;
	static HWND _chatInput;
	static HWND _chatSendBtn;
public:
	void CreateChatUI();
	static void AppendChat(const wstring& msg, COLORREF color);
	static COLORREF GetColorFromId(int id); //해시 사용
	static COLORREF GetDiversedColorFromId(int id); //해시 사용
	void ShowChatUI();
private:
	// Double Buffering
	RECT _rect;
	HDC hdcBack = {};
	HBITMAP _bmpBack = {};
	bool _chatVisible = true;
};

