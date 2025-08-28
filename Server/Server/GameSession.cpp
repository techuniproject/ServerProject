#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
#include "GameRoom.h"
#include "Player.h"
#include "Monster.h"

void GameSession::OnConnected()
{
	GSessionManager.Add(GetSessionRef());

	//Send(ServerPacketHandler::Make_S_EnterGame());//환영 패킷
	//게임 입장
	//GRoom->EnterRoom(GetSessionRef());
	auto room = GRoom;
	auto session = GetSessionRef();
	session->gameRoom = GRoom;

	//메인스레드가 Room에 Player등록하여 게임관련 관리
	room->PushJob([room, session]() {

		shared_ptr<Player> curPlayer = GameObject::CreatePlayer();
		session->player = curPlayer;
		curPlayer->session = session;

		curPlayer->info.set_posx(5);
		curPlayer->info.set_posy(5);

		room->Enter(curPlayer);

		{
		//현재 클라 GameSession에 현재 만든 플레이어 정보 전달
		//원래 다 GameRoom에서 처리

		SendBufferRef sendBuf = ServerPacketHandler::Make_S_MyPlayer(curPlayer->info);
		session->Send(sendBuf);
		}
		//현재 Gameroom에 있는 게임오브젝트 정보 접속 클라에 전달
		{
			Protocol::S_AddObject GameRoomObjects;
			//메인스레드가 Job처리하므로, 직접 Room의 map참고(thread_safe 워커만 안건들이게하면)
			for (auto& info : room->GetPlayersForJob()) {
				if (info.second == session->player.lock())
					continue;//전에 보낸거 중복 방지
				*GameRoomObjects.add_objects() = info.second->info;
			}
			for (auto& info : room->GetMonstersForJob())
				*GameRoomObjects.add_objects() = info.second->info;

			SendBufferRef sendBuf = ServerPacketHandler::Make_S_AddObject(GameRoomObjects);
			session->Send(sendBuf);
		}
		{
			//다른 모든 클라에게 추가된 플레이어 정보 전달
			Protocol::S_AddObject AddedPlayer;
			*AddedPlayer.add_objects() = curPlayer->info;

			SendBufferRef sendBuf = ServerPacketHandler::Make_S_AddObject(AddedPlayer);
			room->Broadcast(sendBuf);
		}		
		
		});
	
	
}

void GameSession::OnDisconnected()
{
	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));

	//게임 나가기
	// GRoom->LeaveRoom(GetSessionRef());

	auto room = GRoom;
	auto session = GetSessionRef();

	if (room == nullptr || session == nullptr)return;

	room->PushJob([room, session]() {

		auto player = session->player.lock();

		if (player == nullptr)return;

		uint64 id = player->info.objectid();
		room->Leave(id);

		Protocol::S_RemoveObject pkt;
		pkt.add_ids(id);
		SendBufferRef sendBuf = ServerPacketHandler::Make_S_RemoveObject(pkt);
		room->Broadcast(sendBuf);

		});
	
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	/*PacketHeader header = *((PacketHeader*)buffer);
	cout << "Packet ID : " << header.id << "Size : " << header.size << endl;
	*/
	ServerPacketHandler::HandlePacket(static_pointer_cast<GameSession>(shared_from_this()), buffer, len);
}


void GameSession::OnSend(int32 len)
{
	//cout << "OnSend Len = " << len << endl;
}