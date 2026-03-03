#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameSession.h"
#include "GameRoom.h"
#include "GameObject.h"
#include "AIQueue.h"
#include "Monster.h"
#include "Player.h"
#include "Sector.h"

extern AIQueue GAIQueue;

PacketHandlerFunc g_packet_handler[HANDLER_MAX];

bool ServerPacketHandler::HandlePacket(GameSessionRef session, BYTE* buffer, int32 length)
{
    // 1) 헤더 읽기
    PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

    // 2) id 범위 체크
    if (header->id >= HANDLER_MAX)
        return false;

    // 3) 등록된 핸들러 실행
    return g_packet_handler[header->id](session, buffer, length);
}

SendBufferRef ServerPacketHandler::Make_S_EnterGame()
{
    Protocol::S_EnterGame packet;
 
 	packet.set_success(true);
 	packet.set_accountid(0);
 
 	return MakeSendBuffer(packet);//Serialize
}

SendBufferRef ServerPacketHandler::Make_S_MyPlayer(const Protocol::ObjectInfo& info)
{
    Protocol::S_MyPlayer pkt;

	Protocol::ObjectInfo* objectInfo = pkt.mutable_info(); //message구성하는 struct pointer반환

	*objectInfo = info;

	return MakeSendBuffer(pkt);
}

SendBufferRef ServerPacketHandler::Make_S_AddObject(const Protocol::S_AddObject& pkt)
{
	return MakeSendBuffer(pkt);
}

SendBufferRef ServerPacketHandler::Make_S_RemoveObject(const Protocol::S_RemoveObject& pkt)
{
    return MakeSendBuffer(pkt);
}

SendBufferRef ServerPacketHandler::Make_S_Move(const Protocol::ObjectInfo& info)
{
    Protocol::S_Move pkt;

	Protocol::ObjectInfo* objectInfo = pkt.mutable_info();
	*objectInfo = info;

    return MakeSendBuffer(pkt);
}

SendBufferRef ServerPacketHandler::Make_S_Attack(const Protocol::S_ATTACK& pkt)
{
    return MakeSendBuffer(pkt);
}

SendBufferRef ServerPacketHandler::Make_S_Speed(const Protocol::S_SPEED& pkt)
{
    return MakeSendBuffer(pkt);
}

SendBufferRef ServerPacketHandler::Make_S_Item(const Protocol::S_ITEM& pkt)
{
    return MakeSendBuffer(pkt);
}

SendBufferRef ServerPacketHandler::Make_S_Broadcast(const Protocol::S_BROADCAST& pkt)
{
    return  MakeSendBuffer(pkt);
}


bool Handle_INVALID(GameSessionRef& session, BYTE* buffer, int32 length)
{
    //초기 함수 포인터 설정용 함수
    return false;
}

bool Handle_C_Move(GameSessionRef& session, Protocol::C_Move& pkt)
{
    weak_ptr<GameSession>wsession = session;
    shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
    if (gameRoom) {
        gameRoom->PushJob([gameRoom, pkt, wsession]() {
           
            shared_ptr<GameSession>curSession = wsession.lock();
            if (!curSession)return;
  
            shared_ptr<Player>curSessionPlayer = curSession->player.lock();
            if (!curSessionPlayer)return;

            if (curSessionPlayer->GetObjectID() != pkt.info().objectid())return;
           
            Vec2Int nextPos{ pkt.info().posx(), pkt.info().posy() };
            Vec2Int curPos{ curSessionPlayer->GetCellPos() };
            bool isMove = (nextPos != curPos);
            curSessionPlayer->info.set_state(pkt.info().state());
            curSessionPlayer->info.set_dir(pkt.info().dir());
            curSessionPlayer->info.set_weapontype(pkt.info().weapontype());
            if (nextPos == curSessionPlayer->GetCellPos()) {
                //제자리면 체크할 필요가 있나?
                //이동일때만 체크해야되는데, 이게 스킬을 사용하면 체크해버림.
                //클라의 이동속도가 빠를때 순간적으로 서버는 통과안시켰는데 또는 백업했는데,
                // 동기화과정에서 일치하지 않는 문제 생김 클라는 뚫어서 가면안되는 영역 갔는데 서버는 통과안시킴

            }
            
          

            if (isMove) {
                float speed = curSessionPlayer->info.movespeed();
                if (speed <= 0) speed = 1.0f;
                // 약간의 오차 허용 (네트워크 렉 등을 고려하여 10~20% 정도 봐줌)
                // 너무 빡빡하면 정상 유저도 렉 걸릴 때 롤백됨
                uint64 minIntervalMs = (uint64)((48.0f / speed) * 1000);
                minIntervalMs = (uint64)(minIntervalMs * 0.9f); // 오차 범위
                uint64 now = ::GetTickCount64();
                if (now - curSessionPlayer->GetLastMoveTime() < minIntervalMs)
                {
                    // [검증 실패] 너무 빠름! 이동 허용 x 스피드핵
                    curSession->Send(ServerPacketHandler::Make_S_Move(curSessionPlayer->info));
                    return;
                }
                
                int dist = abs(nextPos.y - curPos.y) + abs(nextPos.x - curPos.x);
                if (dist > 1) {
                    //일단 이동관련 좌표는 서버에서 수정 x ->이건 클라보다 핵이나 치트방지용
                    //현재 로직에서 이게 가능한건 텔포 등이므로 패킷수가 많을 수 있어 방향 및 state도 수정 x
                    //이동을 크게 한번에 텔포시키면 클라로직꼬인거 푸는건 보내줘야할수도?
                    curSession->Send(ServerPacketHandler::Make_S_Move(curSessionPlayer->info));
                    return;
                }
               
                if (curSessionPlayer->CanGoBySector(nextPos)) {
                    //TODO Validation 해킹 체킹   
                    curSessionPlayer->SetLastMoveTime(now);
                    auto optitem = gameRoom->GetItemAt(nextPos);
                    if (optitem.has_value()) {
                        //플레이어 이동할때 다음칸 아이템 있으면
                        Item& item = optitem.value();
                        if (pkt.info().objectid() == item.itemInfo.playerid()) {
                            gameRoom->DeleteItem(item.itemInfo.itemid());
                            item.itemInfo.set_isalive(false);
                            Protocol::S_ITEM itempkt;
                            Protocol::ItemInfo* iteminfo = itempkt.mutable_iteminfo(); //message구성하는 struct pointer반환
                            *iteminfo = item.itemInfo;
                            switch (iteminfo->itemtype()) {
                            case Protocol::ITEM_TYPE::ITEM_TYPE_ATTACK:
                                curSessionPlayer->info.set_attackspeed(curSessionPlayer->info.attackspeed() + 1);
                                break;
                            case Protocol::ITEM_TYPE::ITEM_TYPE_MOVE:
                                curSessionPlayer->info.set_movespeed(curSessionPlayer->info.movespeed() + 100);
                                break;
                            case Protocol::ITEM_TYPE::ITEM_TYPE_HEAL:
                                curSessionPlayer->info.set_hp(curSessionPlayer->info.hp() + 20);
                                break;
                            default:
                                break;
                            }
                            SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Item(itempkt);
                            gameRoom->Broadcast(sendBuffer);
                        }
                    }
                    curSessionPlayer->info.set_posx(pkt.info().posx());
                    curSessionPlayer->info.set_posy(pkt.info().posy());
                    // sector변경되면 갱신후 해당 정보들을 기반으로 동기화

                    if (!curSessionPlayer->isSameSector(gameRoom->GetSectorPos(pkt.info().posx(), pkt.info().posy()))) {
                        gameRoom->InsertAtSector(gameRoom->GetSectorPos(pkt.info().posx(), pkt.info().posy()), static_cast<GameObject*>(curSessionPlayer.get()));
                        curSessionPlayer->SetCurSectorPos(gameRoom->GetSectorPos(pkt.info().posx(), pkt.info().posy()));
                        
                       // const int32 dirX[9] = { 0, 1,0,0,1,-1,-1,1,-1 };
                       // const int32 dirY[9] = { 0,1,1,-1,-1,1,0,0,-1 };
                        Protocol::S_BROADCAST broadcastRoomPlayers;
                       
                        vector<Sector>_gameRoomSector = gameRoom->_gameRoomSector;

                        {
                            auto AddObjectsForNewSectors = [&](Vec2Int sectorPos) {
                                Sector* sector = gameRoom->GetSectorAt(sectorPos);
                                if (sector) {
                                    for (Player* pl : sector->_sectorPlayers) {
                                            if (pl && pl->GetObjectID() != curSessionPlayer->GetObjectID()) {
                                                *broadcastRoomPlayers.add_addobjects() = pl->info;
                                                Protocol::S_AddObject pkt;
                                                *pkt.add_objects() = curSessionPlayer->info;
                                                SendBufferRef AddMeToOtherBuffer = ServerPacketHandler::Make_S_AddObject(pkt);
                                                pl->session->Send(AddMeToOtherBuffer);
                                            }
                                    }
                                    for (Monster* mon : sector->_sectorMonsters) {
                                            if (mon) {
                                                *broadcastRoomPlayers.add_addobjects() = mon->info;
                                            } 
                                    }
                                }
                                };

                            auto RemoveObjectsFromLastSectors = [&](Vec2Int sectorPos) {
                                Sector* sector = gameRoom->GetSectorAt(sectorPos);
                                if (sector) {
                                    for (Player* pl : sector->_sectorPlayers) {
                                            if (pl && pl->GetObjectID() != curSessionPlayer->GetObjectID()) {
                                                *broadcastRoomPlayers.add_removeobjects() = pl->info;
                                                Protocol::S_RemoveObject pkt;
                                                pkt.add_ids(curSessionPlayer->GetObjectID());
                                                SendBufferRef RemoveMeToOtherBuffer = ServerPacketHandler::Make_S_RemoveObject(pkt);
                                                pl->session->Send(RemoveMeToOtherBuffer);
                                            }
                                    }
                                    for (Monster* mon : sector->_sectorMonsters) {                       
                                            if (mon) {
                                                *broadcastRoomPlayers.add_removeobjects() = mon->info;
                                            }                   
                                    }
                                }
                              };
                            gameRoom->DoSomethingCrossingSectors(nextPos, curPos, AddObjectsForNewSectors, RemoveObjectsFromLastSectors);              
                        }                 
                       //Broadcast는 현재 클라가 새로운 섹터 진입 시 다른 오브젝트들 추가/삭제 여부 알기 위함
                        SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Broadcast(broadcastRoomPlayers);
                        curSession->Send(sendBuffer);
                    }
                   
                }
            }
                                                              

            SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Move(curSessionPlayer->info);
            gameRoom->BroadcastBySector(sendBuffer,curSessionPlayer->GetCurSectorPos());
           
            });
        return true;
    }
    return false;


}

bool Handle_C_CHAT(GameSessionRef& session, Protocol::C_CHAT& pkt)
{
   // cout << pkt.msg() << endl;


    //shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
    //if (gameRoom) {
    //    gameRoom->PushJob([gameRoom, pkt]() {
    //        Protocol::S_CHAT chatfromclientpkt;
    //        chatfromclientpkt.set_msg(pkt.msg());
    //        chatfromclientpkt.set_playerid(pkt.playerid());
    //      
    //        SendBufferRef sendbuffer = ServerPacketHandler::MakeSendBuffer(chatfromclientpkt);
    //        gameRoom->Broadcast(sendbuffer);
    //        return true;
    //        });
    //
    //}
    auto gameRoom = session->gameRoom.lock();
    if (!gameRoom) return false;

    // 1) 원래 하던 그대로, 사용자 채팅을 브로드캐스트
    gameRoom->PushJob([gameRoom, pkt]() {
        Protocol::S_CHAT echo;
        echo.set_msg(pkt.msg());
        echo.set_playerid(pkt.playerid());
        SendBufferRef sb = ServerPacketHandler::MakeSendBuffer(echo);
        gameRoom->Broadcast(sb);
        });

    // 2) "/npc " 또는 "/ai " 명령이면 LLM 작업 큐에 투입
    std::string msg = pkt.msg();
    auto starts_with = [&](const char* p) { return msg.rfind(p, 0) == 0; };

    if (starts_with("/npc ") || starts_with("/ai ")) {
        AIRequest r;
        r.playerId = pkt.playerid();
        r.npcId = 0;
        r.convId = 0;
        r.userText = msg.substr(msg.find(' ') + 1); // 접두사 뒤만 LLM에 보냄
        r.room = gameRoom;                       // 답변은 이 방에 브로드캐스트
        r.systemPrompt =
            "당신은 마을 안내 NPC입니다. 정중하고 짧게 대답. 욕설/개인정보/광고 금지. 한국어.";
        r.contextJson = "{}"; // 필요하면 플레이어/퀘스트 상태 요약 넣기
        GAIQueue.Push(std::move(r));
    }
    return true;
    
}

bool Handle_C_ARROW(GameSessionRef& session, Protocol::C_ARROW& pkt)
{
    //shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
    //if (gameRoom) {
    //    gameRoom->PushJob([gameRoom, pkt]() {

    //        Vec2Int deltaXY[4] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
    //        Vec2Int nextPos{ pkt.posx() + deltaXY[pkt.dir()].x,pkt.posy() + deltaXY[pkt.dir()].y };

    //       

    //        if (auto monster = gameRoom->GetCreatureAt(nextPos))
    //        {
    //            monster->OnDamaged(dynamic_pointer_cast<Creature>(gameRoom->FindObject(pkt.playerid())));
    //            if (auto m = std::dynamic_pointer_cast<Monster>(monster)) {
    //                m->ApplyHitStun(505); //플레이어 공격 쿨타임 500이라 같이 500이면 둘다 동시에 때림
    //            }
    //        }

    //        });
    //    return true;
    //}
    //return false;
    return false;
}

bool Handle_C_SPEED(GameSessionRef& session, Protocol::C_SPEED& pkt)
{
   /* shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
    if (gameRoom) {
        gameRoom->PushJob([gameRoom, pkt]() {

            Protocol::S_SPEED sendpkt;
            sendpkt.set_playerid(pkt.playerid());
            sendpkt.set_movespeed(pkt.movespeed());
            sendpkt.set_attackspeed(pkt.attackspeed());

            SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Speed(sendpkt);
            gameRoom->Broadcast(sendBuffer);

            });
        return true;
    }
    return false;*/
    return false;
}

/*
//없앨때
 shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
    if (gameRoom) {
        gameRoom->PushJob([gameRoom, pkt]() {
            Protocol::S_RemoveObject sendpkt;
            int cnt = pkt.ids_size();
            for (int i = 0; i < cnt; ++i) {
                gameRoom->Leave(pkt.ids(i));
                sendpkt.add_ids(pkt.ids(i));
            }

            SendBufferRef sendBuffer = ServerPacketHandler::Make_S_RemoveObject(sendpkt);
            gameRoom->Broadcast(sendBuffer);

            });
        return true;
    }
*/