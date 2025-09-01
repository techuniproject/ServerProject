#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
#include "GameRoom.h"
#include "Player.h"
#include "Monster.h"

/*
원래 메인스레드는 게임 로직 관련 수치를 담당하고,
워커스레드는 I/O처리를 하고, 패킷을 다시 뿌려주는 역할을 하는게 일반적인데,
수치를 변경함과 동시에 broadcast를 통해 알리는 과정에서 broadcast가 먼저 일어나버리면,
클라가 서버에서 게임로직 처리 전에 미리 받아 처리하는 문제가 생김.
그러므로 메인스레드가 게임로직 처리하고나서 broadcast를 하도록 선택

io작업이 생각보다 무거워 메인스레드가 해당 작업까지 처리하면 비용이 클 수 있어
io작업을 다시 워커스레드가 처리하도록 처리하는 io작업큐를 워커스레드간 경합(락걸어서)을 통해
처리하는게 나을 수도 있음.
대신 메인스레드가 패킷만드는(make시리즈)를 처리하는것은 분명함(데이터 일관성때문) 이후, send/broadcast등을 워커한테 넘김
*/

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
		//room->PushSendJob(session, sendBuf); // 워커가 io처리하도록
		session->Send(sendBuf);//메인스레드가 처리하는 경우
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
			//room->PushSendJob(session, sendBuf); // 워커가 io처리하도록
		}
		{
			//다른 모든 클라에게 추가된 플레이어 정보 전달
			Protocol::S_AddObject AddedPlayer;
			*AddedPlayer.add_objects() = curPlayer->info;

			SendBufferRef sendBuf = ServerPacketHandler::Make_S_AddObject(AddedPlayer);
			//room->PushBroadcastJob(sendBuf); //워커가 처리하도록
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
		//room->PushBroadcastJob(sendBuf);
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