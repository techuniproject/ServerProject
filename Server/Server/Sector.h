#pragma once
struct Sector
{
	// 3024 x 2064

	//각 타일 48x48 -> 63 x 43 개의 칸으로 나뉨 
	//
	//화면은 800x600

	//480x480 정도로 하나의 섹터를 해서 가로로 7 세로로 5  마지막 칸은 3360 2400

	// Sector은 그냥 참고용으로 편의성 및 효율성 측면에서 각 객체를 선별하는것이지
	// 수명주기를 직접 제어하면 문제생기기 때문에 스마트포인터로 관리 x

	vector<class Player*>_sectorPlayers;
	vector<class Monster*>_sectorMonsters;
	vector<class Arrow*>_sectorArrows;
};
