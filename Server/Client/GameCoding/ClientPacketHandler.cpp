#include "pch.h"
#include "GameInstance.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"
#include "DevScene.h"
#include "MyPlayer.h"
#include "Creature.h"
#include "Sprite.h"
#include "UI.h"

extern HWND g_hWnd;
PacketHandlerFunc g_packet_handler[HANDLER_MAX];

//void ClientPacketHandler::HandlePacket(ServerSessionRef session,BYTE* buffer, int32 len)
//{
//	BufferReader br(buffer, len);
//
//	PacketHeader header;
//	br >> header;
//
//	switch (header.id)
//	{
//	case S_TEST:
//		Handle_S_TEST(session,buffer, len);
//		break;
//	case S_EnterGame:
//		Handle_S_EnterGame(session,buffer, len);
//		break;
//	case S_MyPlayer:
//		Handle_S_MyPlayer(session,buffer, len);
//		break;
//	case S_AddObject:
//		Handle_S_AddObject(session,buffer, len);
//		break;
//	case S_RemoveObject:
//		Handle_S_RemoveObject(session,buffer, len);
//		break;
//	case S_Move:
//		Handle_S_Move(session,buffer,len);
//		break;
//	}
//}

bool ClientPacketHandler::HandlePacket(ServerSessionRef session, BYTE* buffer, int32 length)
{
    // 1) 헤더 읽기
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

    // 2) id 범위 체크
    if (header->id >= HANDLER_MAX)
        return false;

    // 3) 등록된 핸들러 실행
    return g_packet_handler[header->id](session, buffer, length);
}

SendBufferRef ClientPacketHandler::Make_C_Move()
{
    Protocol::C_Move pkt;

	shared_ptr<MyPlayer> myPlayer = GET_SINGLE(GameInstance)->GetMyPlayer();

	*pkt.mutable_info()=myPlayer->info;
	

	return MakeSendBuffer(pkt);
}

SendBufferRef ClientPacketHandler::Make_C_Arrow(uint64 playerId,int32 posX,int32 posY, Protocol::DIR_TYPE dir, Protocol::OBJECT_STATE_TYPE state )
{
    Protocol::C_ARROW pkt;
    pkt.set_playerid(playerId);
    pkt.set_posx(posX);
    pkt.set_posy(posY);
    pkt.set_dir(dir);
    pkt.set_state(state);


    return MakeSendBuffer(pkt);
}

SendBufferRef ClientPacketHandler::Make_C_Chat(wstring& wstr)
{
    Protocol::C_CHAT pkt;
  
    if (wstr.empty()) return nullptr;

    int len = WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.c_str(), (int)wstr.size(),
        nullptr, 0,
        nullptr, nullptr);

    std::string str(len, 0);
    WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.c_str(), (int)wstr.size(),
        str.data(), len,
        nullptr, nullptr);
    pkt.set_msg(str);
    pkt.set_playerid(GET_SINGLE(GameInstance)->GetMyPlayerId());

    return MakeSendBuffer(pkt);
}

SendBufferRef ClientPacketHandler::Make_C_Speed(float attackSpeed, float moveSpeed)
{
    Protocol::C_SPEED pkt;
    shared_ptr<MyPlayer> myPlayer = GET_SINGLE(GameInstance)->GetMyPlayer();

    pkt.set_playerid(myPlayer->info.objectid());

    pkt.set_movespeed(moveSpeed);

    return MakeSendBuffer(pkt);
}

bool Handle_INVALID(ServerSessionRef& session, BYTE* buffer, int32 length)
{
    return false;
}

bool Handle_S_EnterGame(ServerSessionRef& session, Protocol::S_EnterGame& pkt)
{	
    bool success = pkt.success();
    uint64 accountId = pkt.accountid();
    return true;
}

bool Handle_S_MyPlayer(ServerSessionRef& session, Protocol::S_MyPlayer& pkt)
{    
    const Protocol::ObjectInfo& info = pkt.info();
    
    GameInstance* gameInstance = GET_SINGLE(GameInstance);
    
    DevScene* scene = gameInstance->GetCurrentScene<DevScene>();
    if (scene)
    {
    	shared_ptr<MyPlayer> myPlayer=scene->SpawnObject<MyPlayer>(Vec2Int(info.posx(), info.posy()));
    	myPlayer->info = info;
    	gameInstance->SetMyPlayer(myPlayer);
        return true;
    }
    return false;
}

bool Handle_S_AddObject(ServerSessionRef& session, Protocol::S_AddObject& pkt)
{
	GameInstance* gameInstance = GET_SINGLE(GameInstance);

	DevScene* scene = gameInstance->GetCurrentScene<DevScene>();
	if (scene)
	{
		scene->Handle_S_AddObject(pkt);
        return true;
	}
    return false;
}

bool Handle_S_RemoveObject(ServerSessionRef& session, Protocol::S_RemoveObject& pkt)
{
    GameInstance* gameInstance = GET_SINGLE(GameInstance);

	DevScene* scene = gameInstance->GetCurrentScene<DevScene>();
	if (scene)
	{
        
		scene->Handle_S_RemoveObject(pkt);
        return true;
	}
    return false;
}

bool Handle_S_Move(ServerSessionRef& session, Protocol::S_Move& pkt)
{
    const Protocol::ObjectInfo& info = pkt.info();
 	DevScene* scene = GET_SINGLE(GameInstance)->GetCurrentScene<DevScene>();
 	if (scene)
 	{
 		uint64 myPlayerId = GET_SINGLE(GameInstance)->GetMyPlayerId();
        if (myPlayerId == info.objectid())
        {
            GET_SINGLE(GameInstance)->GetMyPlayer()->SetMaxHp(info.maxhp());
            GET_SINGLE(GameInstance)->GetMyPlayer()->SetDefence(info.defence());
            return false;
        }
 			
 
 		shared_ptr<GameObject> gameObject=scene->GetGameObject(info.objectid());
 		if (gameObject) {
            gameObject->SetState(info.state());
 			gameObject->SetDir(info.dir());
 			gameObject->SetCellPos(Vec2Int(info.posx(), info.posy()));
            gameObject->SetMaxHp(info.maxhp());
            gameObject->SetDefence(info.defence());
            gameObject->SetName(info.name());
            auto ifplayer = dynamic_pointer_cast<Player>(gameObject);
            if (ifplayer) {

                ifplayer->SetWeaponType(info.weapontype());
            }
            return true;
 		}
 	    
 	}
    return false;
}

bool Handle_S_CHAT(ServerSessionRef& session, Protocol::S_CHAT& pkt)
{
    
    int wlen = MultiByteToWideChar(CP_UTF8, 0, pkt.msg().c_str(), -1, NULL, 0);
    std::wstring* wmsg = new std::wstring(wlen-1, 0);
    MultiByteToWideChar(CP_UTF8, 0, pkt.msg().c_str(), -1, &(*wmsg)[0], wlen);
    std::wstring formatted = std::format(L"Player {} : {}", pkt.playerid(), *wmsg);

    // 메인 윈도우에 메시지 전달
    PostMessage(g_hWnd, WM_CHATMSG, (WPARAM)pkt.playerid(), (LPARAM)new std::wstring(formatted));
    return true;
}

bool Handle_S_ATTACK(ServerSessionRef& session, Protocol::S_ATTACK& pkt)
{
    DevScene* scene = GET_SINGLE(GameInstance)->GetCurrentScene<DevScene>();
    if (scene) {
        shared_ptr<GameObject> gameObject = scene->GetGameObject(pkt.attackedid());
        if (gameObject) {
            gameObject->SetHp(pkt.hp());        
        }
    }
    return false;
}

bool Handle_S_SPEED(ServerSessionRef& session, Protocol::S_SPEED& pkt)
{
    DevScene* scene = GET_SINGLE(GameInstance)->GetCurrentScene<DevScene>();
    if (scene) {
        shared_ptr<GameObject> gameObject = scene->GetGameObject(pkt.playerid());
        auto ifplayer = dynamic_pointer_cast<Player>(gameObject);
        if (ifplayer) {
            ifplayer->SetMoveSpeed(pkt.movespeed());
            ifplayer->SetFlipbookSpeed(pkt.movespeed() / 100 - 1);
        }
    }
    return false;
}

bool Handle_S_ITEM(ServerSessionRef& session, Protocol::S_ITEM& pkt)
{
    DevScene* scene = GET_SINGLE(GameInstance)->GetCurrentScene<DevScene>();
    shared_ptr<UI> itemUI = make_shared<UI>();
    Vec2 cameraPos = GET_SINGLE(GameInstance)->GetCameraPos();
   // const int screenCx = pkt.posx() - (cameraPos.x - GWinSizeX / 2);
    const int screenCx = (pkt.posx()*-50 + cameraPos.x) ;
    const int screenCy = (pkt.posy()*-50 + cameraPos.y);
    
    itemUI->SetPos(Vec2(400, 300));
    itemUI->SetSize(Vec2Int(50, 50));
    if (scene) {
        switch (pkt.itemtype()) {
        case Protocol::ITEM_TYPE_ATTACK:
            itemUI->SetCurrentSprite(GET_SINGLE(GameInstance)->GetSprite(L"AttackSpeed"));
            break;
        case Protocol::ITEM_TYPE_MOVE:
            itemUI->SetCurrentSprite(GET_SINGLE(GameInstance)->GetSprite(L"MoveSpeed"));
            break;
        default:
            break;
        }
        wstring sen = to_wstring(pkt.playerid());
        scene->AddUI(sen, itemUI);
    }
    return false;
}



//void ClientPacketHandler::Handle_S_TEST(ServerSessionRef session,BYTE* buffer, int32 len)
//{
//	PacketHeader* header = (PacketHeader*)buffer;
//	//uint16 id = header->id;
//	uint16 size = header->size;
//
//	Protocol::S_TEST pkt;
//	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));
//
//	uint64 id = pkt.id();
//	uint32 hp = pkt.hp();
//	uint16 attack = pkt.attack();
//
//	cout << "ID: " << id << " HP : " << hp << " ATT : " << attack << endl;
//
//	for (int32 i = 0; i < pkt.buffs_size(); i++)
//	{
//		const Protocol::BuffData& data = pkt.buffs(i);
//		cout << "BuffInfo : " << data.buffid() << " " << data.remaintime() << endl;
//	}
//}





//void ClientPacketHandler::Handle_S_EnterGame(ServerSessionRef session, BYTE* buffer, int32 len)
//{
//	PacketHeader* header = (PacketHeader*)buffer;
//	//uint16 id = header->id;
//	uint16 size = header->size;
//
//	Protocol::S_EnterGame pkt;
//	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));
//
//	bool success = pkt.success();
//	uint64 accountId = pkt.accountid();
//	
//}
//
//void ClientPacketHandler::Handle_S_MyPlayer(ServerSessionRef session, BYTE* buffer, int32 len)
//{
//	PacketHeader* header = (PacketHeader*)buffer;
//	//uint16 id = header->id;
//	uint16 size = header->size;
//
//	Protocol::S_MyPlayer pkt;
//	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));
//
//	const Protocol::ObjectInfo& info = pkt.info();
//
//	GameInstance* gameInstance = GET_SINGLE(GameInstance);
//
//	DevScene* scene = gameInstance->GetCurrentScene<DevScene>();
//	if (scene)
//	{
//		shared_ptr<MyPlayer> myPlayer=scene->SpawnObject<MyPlayer>(Vec2Int(info.posx(), info.posy()));
//		myPlayer->info = info;
//		gameInstance->SetMyPlayer(myPlayer);
//	}
//}
//
//void ClientPacketHandler::Handle_S_AddObject(ServerSessionRef session, BYTE* buffer, int32 len)
//{
//	PacketHeader* header = (PacketHeader*)buffer;
//	//uint16 id = header->id;
//	uint16 size = header->size;
//
//	Protocol::S_AddObject pkt;
//	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));
//
//	GameInstance* gameInstance = GET_SINGLE(GameInstance);
//
//	DevScene* scene = gameInstance->GetCurrentScene<DevScene>();
//	if (scene)
//	{
//		scene->Handle_S_AddObject(pkt);
//	}
//}
//
//void ClientPacketHandler::Handle_S_RemoveObject(ServerSessionRef session, BYTE* buffer, int32 len)
//{
//	PacketHeader* header = (PacketHeader*)buffer;
//	//uint16 id = header->id;
//	uint16 size = header->size;
//
//	Protocol::S_RemoveObject pkt;
//	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));
//
//	GameInstance* gameInstance = GET_SINGLE(GameInstance);
//
//	DevScene* scene = gameInstance->GetCurrentScene<DevScene>();
//	if (scene)
//	{
//		scene->Handle_S_RemoveObject(pkt);
//	}
//}
//
//void ClientPacketHandler::Handle_S_Move(ServerSessionRef session, BYTE* buffer, int32 len)
//{
//
//	PacketHeader* header = (PacketHeader*)buffer;
//	//uint16 id = header->id;
//	uint16 size = header->size;
//
//	Protocol::S_Move pkt;
//	pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));
//
//
//	const Protocol::ObjectInfo& info = pkt.info();
//	DevScene* scene = GET_SINGLE(GameInstance)->GetCurrentScene<DevScene>();
//	if (scene)
//	{
//		uint64 myPlayerId = GET_SINGLE(GameInstance)->GetMyPlayerId();
//		if (myPlayerId == info.objectid())
//			return;
//
//		shared_ptr<GameObject> gameObject=scene->GetGameObject(info.objectid());
//		if (gameObject) {
//			gameObject->SetDir(info.dir());
//			gameObject->SetState(info.state());
//			gameObject->SetCellPos(Vec2Int(info.posx(), info.posy()));
//		}
//	
//	}
//
//
//}
//
//SendBufferRef ClientPacketHandler::Make_C_Move()
//{
//	Protocol::C_Move pkt;
//
//	shared_ptr<MyPlayer> myPlayer = GET_SINGLE(GameInstance)->GetMyPlayer();
//
//	*pkt.mutable_info()=myPlayer->info;
//	
//
//	return MakeSendBuffer(pkt,C_Move);
//}

