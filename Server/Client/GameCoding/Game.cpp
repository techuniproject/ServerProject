#include "pch.h"
#include "Game.h"
#include "GameInstance.h"
#include "TimeManager.h"
#include "InputManager.h"
#include "SceneManager.h"
#include "ResourceManager.h"
#include "SoundManager.h"
#include "ClientPacketHandler.h"

HWND Game::_chathwnd=nullptr;
HWND Game::_chatInput=nullptr;
HWND Game::_chatSendBtn=nullptr;


Game::Game()
{

}

Game::~Game()
{
	// 사실 마지막에 ..
	GET_SINGLE(GameInstance)->Clear();
	//GET_SINGLE(SceneManager)->Clear();
	//GET_SINGLE(ResourceManager)->Clear();

	_CrtDumpMemoryLeaks();
}

void Game::Init(HWND hwnd,HINSTANCE hInstance)
{
	_hwnd = hwnd;
	_hInstance = hInstance;
	hdc = ::GetDC(hwnd);

	::GetClientRect(hwnd, &_rect);

	hdcBack = ::CreateCompatibleDC(hdc); // hdc와 호환되는 DC를 생성
	_bmpBack = ::CreateCompatibleBitmap(hdc, _rect.right, _rect.bottom); // hdc와 호환되는 비트맵 생성
	HBITMAP prev = (HBITMAP)::SelectObject(hdcBack, _bmpBack); // DC와 BMP를 연결
	::DeleteObject(prev);

	GET_SINGLE(GameInstance)->Init(hwnd);

	//GET_SINGLE(TimeManager)->Init();
	//GET_SINGLE(InputManager)->Init(hwnd);
	//GET_SINGLE(SceneManager)->Init();
	//GET_SINGLE(ResourceManager)->Init(hwnd, fs::path(L"C:\\Users\\서정원\\Desktop\\ServerClient\\ServerProject\\Server\\Client\\Resources"));
	//GET_SINGLE(SoundManager)->Init(hwnd);

	CreateChatUI();

	GET_SINGLE(GameInstance)->ChangeScene(SceneType::DevScene);

	//GET_SINGLE(SceneManager)->ChangeScene(SceneType::DevScene);
}

void Game::Update()
{
	GET_SINGLE(GameInstance)->Update();
	if (GetAsyncKeyState(VK_F1) & 0x0001) // 눌릴 때만
	{
		ShowChatUI();
	}
	//GET_SINGLE(TimeManager)->Update();
	//GET_SINGLE(InputManager)->Update();
	//GET_SINGLE(SceneManager)->Update();
}

void Game::Render()
{
	//GET_SINGLE(SceneManager)->Render(hdcBack);

	GET_SINGLE(GameInstance)->RenderScene(hdcBack);

	//uint32 fps = GET_SINGLE(TimeManager)->GetFps();
	//float deltaTime = GET_SINGLE(TimeManager)->GetDeltaTime();

	uint32 fps = GET_SINGLE(GameInstance)->GetFps();
	float deltaTime = GET_SINGLE(GameInstance)->GetDeltaTime();

	{
		POINT mousePos = GET_SINGLE(GameInstance)->GetMousePos();
		wstring str = std::format(L"Mouse({0}, {1})", mousePos.x, mousePos.y);
		::TextOut(hdcBack, 20, 10, str.c_str(), static_cast<int32>(str.size()));
	}
	
	{
		wstring str = std::format(L"FPS({0}), DT({1})", fps, deltaTime);
		::TextOut(hdcBack, 550, 10, str.c_str(), static_cast<int32>(str.size()));
	}

	// Double Buffering
	::BitBlt(hdc, 0, 0, _rect.right, _rect.bottom, hdcBack, 0, 0, SRCCOPY); // 비트 블릿 : 고속 복사
	::PatBlt(hdcBack, 0, 0, _rect.right, _rect.bottom, WHITENESS);
}

void Game::CreateChatUI()
{
	_chathwnd = CreateWindowExW(WS_EX_CLIENTEDGE, MSFTEDIT_CLASS, L"",
		WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
		20, 50, 400, 200, _hwnd, (HMENU)1001, _hInstance, nullptr);

	_chatInput = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
		20, 260, 300, 25, _hwnd, (HMENU)1002, nullptr, nullptr);

	_chatSendBtn = CreateWindowW(L"BUTTON", L"Send",
		WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
		330, 260, 90, 25, _hwnd, (HMENU)1003, nullptr, nullptr);
}

void Game::AppendChat(const wstring& msg,  COLORREF color)
{
	if (!_chathwnd) return;

	int len = GetWindowTextLengthW(_chathwnd);
	SendMessageW(_chathwnd, EM_SETSEL, len, len);

	CHARFORMAT2 cf;
	ZeroMemory(&cf, sizeof(cf));
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = color;
	SendMessage(_chathwnd, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);


	SendMessageW(_chathwnd, EM_REPLACESEL, FALSE, (LPARAM)msg.c_str());
	SendMessageW(_chathwnd, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");

}

COLORREF Game::GetColorFromId(int id)
{
	// 해시: id 값에서 R,G,B 추출
	unsigned int hash = std::hash<int>{}(id);

	BYTE r = (hash & 0xFF);          // 하위 8비트
	BYTE g = ((hash >> 8) & 0xFF);   // 중간 8비트
	BYTE b = ((hash >> 16) & 0xFF);  // 상위 8비트

	// 너무 어두운 색이면 밝게 보정
	int brightness = r + g + b;
	if (brightness < 100) { r += 100; g += 100; b += 100; }

	return RGB(r, g, b);
}

COLORREF Game::GetDiversedColorFromId(int id)
{
	unsigned int hash = std::hash<int>{}(id);

	// Hue 0~359도
	double hue = (hash % 360);

	// 고정된 채도, 명도
	double s = 0.7, l = 0.5;

	double c = (1 - fabs(2 * l - 1)) * s;
	double x = c * (1 - fabs(fmod(hue / 60.0, 2) - 1));
	double m = l - c / 2;

	double r = 0, g = 0, b = 0;
	if (hue < 60) { r = c; g = x; b = 0; }
	else if (hue < 120) { r = x; g = c; b = 0; }
	else if (hue < 180) { r = 0; g = c; b = x; }
	else if (hue < 240) { r = 0; g = x; b = c; }
	else if (hue < 300) { r = x; g = 0; b = c; }
	else { r = c; g = 0; b = x; }

	BYTE R = (BYTE)((r + m) * 255);
	BYTE G = (BYTE)((g + m) * 255);
	BYTE B = (BYTE)((b + m) * 255);

	return RGB(R, G, B);
}

void Game::ShowChatUI()
{
	_chatVisible = !_chatVisible;

	int cmd = _chatVisible ? SW_SHOW : SW_HIDE;

	if (_chathwnd)   ShowWindow(_chathwnd, cmd);
	if (_chatInput)  ShowWindow(_chatInput, cmd);
	if (_chatSendBtn) ShowWindow(_chatSendBtn, cmd);
}
