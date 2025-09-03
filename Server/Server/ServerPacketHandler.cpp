#include "pch.h"
#include "ServerPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameSession.h"
#include "GameRoom.h"
#include "GameObject.h"

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


bool Handle_INVALID(GameSessionRef& session, BYTE* buffer, int32 length)
{
    //초기 함수 포인터 설정용 함수
    return false;
}

bool Handle_C_Move(GameSessionRef& session, Protocol::C_Move& pkt)//받아와서 시리얼라이즈해줌
{
   // uint64 enqueueTime = GetTickCount64();
    
    shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
    if (gameRoom) {
        gameRoom->PushJob([gameRoom, pkt/*, enqueueTime*/]() {
         /*   uint64 startTime = GetTickCount64();
            uint64 delay = startTime - enqueueTime;
          
            cout << "[JobQueue Delay] " << delay << " ms" << endl;*/
            
            shared_ptr<GameObject> object = gameRoom->FindObject(pkt.info().objectid());
            if (object == nullptr)return;
           
        	//TODO Validation 해킹 체킹
        	object->info.set_state(pkt.info().state());
        	object->info.set_dir(pkt.info().dir());
        	object->info.set_posx(pkt.info().posx());
        	object->info.set_posy(pkt.info().posy());

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


    shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
    if (gameRoom) {
        gameRoom->PushJob([gameRoom, pkt]() {
            Protocol::S_CHAT chatfromclientpkt;
            chatfromclientpkt.set_msg(pkt.msg());
            chatfromclientpkt.set_playerid(pkt.playerid());
          
            SendBufferRef sendbuffer = ServerPacketHandler::MakeSendBuffer(chatfromclientpkt);
            gameRoom->Broadcast(sendbuffer);
            return true;
            });
    
    }
   /* Protocol::S_CHAT chatfromclientpkt;
    chatfromclientpkt.set_msg(pkt.msg());
    chatfromclientpkt.set_playerid(pkt.playerid());
    SendBufferRef sendbuffer= ServerPacketHandler::MakeSendBuffer(chatfromclientpkt);

    shared_ptr<GameRoom> gameRoom = session->gameRoom.lock();
    if (gameRoom) {
        gameRoom->Broadcast(sendbuffer);
        return true;
    }*/
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