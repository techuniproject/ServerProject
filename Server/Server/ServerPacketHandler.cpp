#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameSession.h"
#include "GameRoom.h"
#include "GameObject.h"
#include "AIQueue.h"
#include "Monster.h"

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


bool Handle_INVALID(GameSessionRef& session, BYTE* buffer, int32 length)
{
    //초기 함수 포인터 설정용 함수
    return false;
}

bool Handle_C_Move(GameSessionRef& session, Protocol::C_Move& pkt)
{
    
    shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
    if (gameRoom) {
        gameRoom->PushJob([gameRoom, pkt]() {
            
            shared_ptr<GameObject> object = gameRoom->FindObject(pkt.info().objectid());
            if (object == nullptr)return;
           
            Vec2Int nextPos{ pkt.info().posx(), pkt.info().posy() };
            object->info.set_state(pkt.info().state());
            object->info.set_dir(pkt.info().dir());
           
            int a = pkt.info().state();
            if (object->CanGo(nextPos)) {
                //TODO Validation 해킹 체킹                           
                object->info.set_posx(pkt.info().posx());
                object->info.set_posy(pkt.info().posy());             
            }
            object->info.set_weapontype(pkt.info().weapontype());

          /*  if (pkt.info().state() == Protocol::OBJECT_STATE_TYPE_SKILL) {
                object->info.set_weapontype(pkt.info().weapontype());
                object->_attackRequested = true;     //데미지 2중 들어감                          
            }*/
           

            SendBufferRef sendBuffer = ServerPacketHandler::Make_S_Move(pkt.info());
            gameRoom->Broadcast(sendBuffer);
           
            });
        return true;
    }
    return false;

    //shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
    //if (gameRoom) {
    //    gameRoom->Handle_C_Move(pkt);
    //    return true;
    //}
    //return false;
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
    shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
    if (gameRoom) {
        gameRoom->PushJob([gameRoom, pkt]() {

            Vec2Int deltaXY[4] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
            Vec2Int nextPos{ pkt.posx() + deltaXY[pkt.dir()].x,pkt.posy() + deltaXY[pkt.dir()].y };

           

            if (auto monster = gameRoom->GetCreatureAt(nextPos))
            {
                monster->OnDamaged(dynamic_pointer_cast<Creature>(gameRoom->FindObject(pkt.playerid())));
                if (auto m = std::dynamic_pointer_cast<Monster>(monster)) {
                    m->ApplyHitStun(505); //플레이어 공격 쿨타임 500이라 같이 500이면 둘다 동시에 때림
                }
            }

            });
        return true;
    }
    return false;

}

bool Handle_C_SPEED(GameSessionRef& session, Protocol::C_SPEED& pkt)
{
    shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
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