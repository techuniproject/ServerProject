#include "pch.h"
#include "GameInstance.h"
#include "Creature.h"
#include "InputManager.h"
#include "TimeManager.h"
#include "ResourceManager.h"
#include "Flipbook.h"
#include "CameraComponent.h"
#include "SceneManager.h"
#include "DevScene.h"


Creature::Creature()
{
	
}

Creature::~Creature()
{

}

void Creature::BeginPlay()
{
	Super::BeginPlay();
}

void Creature::Tick()
{
	Super::Tick();

}

void Creature::Render(HDC hdc)
{
	Super::Render(hdc);
}

void Creature::OnDamaged(shared_ptr<Creature>  attacker)
{
	//if (attacker == nullptr)
	//	return;

	//Stat& attackerStat = attacker->GetStat();
	//Stat& stat = GetStat();

	//int32 damage = attackerStat.attack - stat.defence;
	//if (damage <= 0)
	//	return;

	//stat.hp = max(0, stat.hp - damage);
	//if (stat.hp == 0)
	//{
	//	/*Scene* scene = GET_SINGLE(SceneManager)->GetCurrentScene();
	//	if (scene)
	//	{
	//		scene->RemoveActor(this);
	//	}*/
	//	GET_SINGLE(GameInstance)->GetCurrentScene().RemoveActor(shared_from_this());
	//	//GET_SINGLE(SceneManager)->GetCurrentScene()->RemoveActor(this);
	//}
}



// 간단 HP바: center 기준, 타일 크기에 맞춰 폭 자동
void Creature::DrawHealthBar(HDC hdc, POINT center, int tileSize, int hp, int hpMax) {
    if (hpMax <= 0 || hp <= 0) return;
    Vec2 cameraPos = GET_SINGLE(GameInstance)->GetCameraPos();

    // 월드 중심 → 화면 중심 좌표 (TransparentBlt 식과 동일한 오프셋)
    //  * TransparentBlt는 좌상단(sx,sy)을 구했지만,
    //    HP바는 "중심"에서 상대 위치로 계산하므로 중심만 보정해주면 됨.
    const int screenCx = center.x - (cameraPos.x - GWinSizeX / 2);
    const int screenCy = center.y - (cameraPos.y - GWinSizeY / 2);

    // 비율 및 바 크기
    float t = (float)hp / (float)hpMax;
    if (t < 0.f) t = 0.f; else if (t > 1.f) t = 1.f;
    const int barW = max(28, tileSize - 8);
    const int barH = 6;

    // 스프라이트 상단 위에 표시
    RECT rc = {
        screenCx - barW / 2,
        screenCy - (tileSize / 2) - 10,
        screenCx + barW / 2,
        screenCy - (tileSize / 2) - 10 + barH
    };
    if (rc.left > rc.right)   std::swap(rc.left, rc.right);
    if (rc.top > rc.bottom)  std::swap(rc.top, rc.bottom);

    // 배경
    HBRUSH bg = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    // 채움(HP 비율)
    auto clamp255 = [](int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); };
    int r = clamp255((int)(255 * (1.f - t)) + (int)(32 * t));
    int g = clamp255((int)(64 * (1.f - t)) + (int)(255 * t));
    HBRUSH fg = CreateSolidBrush(RGB(r, g, 32));

    RECT fill = rc;
    fill.right = rc.left + (LONG)((rc.right - rc.left) * t);
    FillRect(hdc, &fill, fg);
    DeleteObject(fg);

    // 테두리
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBr = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBr);
    DeleteObject(pen);

    // (선택) 숫자 표시
    // SetBkMode(hdc, TRANSPARENT);
    // SetTextColor(hdc, RGB(220,220,220));
    // wchar_t buf[32]; swprintf(buf, 32, L"%d/%d", hp, hpMax);
    // DrawTextW(hdc, buf, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}
