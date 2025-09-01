#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
#include "GameRoom.h"
#include "Player.h"
#include "Monster.h"

/*
���� ���ν������ ���� ���� ���� ��ġ�� ����ϰ�,
��Ŀ������� I/Oó���� �ϰ�, ��Ŷ�� �ٽ� �ѷ��ִ� ������ �ϴ°� �Ϲ����ε�,
��ġ�� �����԰� ���ÿ� broadcast�� ���� �˸��� �������� broadcast�� ���� �Ͼ������,
Ŭ�� �������� ���ӷ��� ó�� ���� �̸� �޾� ó���ϴ� ������ ����.
�׷��Ƿ� ���ν����尡 ���ӷ��� ó���ϰ��� broadcast�� �ϵ��� ����

io�۾��� �������� ���ſ� ���ν����尡 �ش� �۾����� ó���ϸ� ����� Ŭ �� �־�
io�۾��� �ٽ� ��Ŀ�����尡 ó���ϵ��� ó���ϴ� io�۾�ť�� ��Ŀ�����尣 ����(���ɾ)�� ����
ó���ϴ°� ���� ���� ����.
��� ���ν����尡 ��Ŷ�����(make�ø���)�� ó���ϴ°��� �и���(������ �ϰ�������) ����, send/broadcast���� ��Ŀ���� �ѱ�
*/

void GameSession::OnConnected()
{
	GSessionManager.Add(GetSessionRef());

	//Send(ServerPacketHandler::Make_S_EnterGame());//ȯ�� ��Ŷ
	//���� ����
	//GRoom->EnterRoom(GetSessionRef());
	auto room = GRoom;
	auto session = GetSessionRef();
	session->gameRoom = GRoom;

	//���ν����尡 Room�� Player����Ͽ� ���Ӱ��� ����
	room->PushJob([room, session]() {

		shared_ptr<Player> curPlayer = GameObject::CreatePlayer();
		session->player = curPlayer;
		curPlayer->session = session;

		curPlayer->info.set_posx(5);
		curPlayer->info.set_posy(5);

		room->Enter(curPlayer);

		{
		//���� Ŭ�� GameSession�� ���� ���� �÷��̾� ���� ����
		//���� �� GameRoom���� ó��

		SendBufferRef sendBuf = ServerPacketHandler::Make_S_MyPlayer(curPlayer->info);
		//room->PushSendJob(session, sendBuf); // ��Ŀ�� ioó���ϵ���
		session->Send(sendBuf);//���ν����尡 ó���ϴ� ���
		}
		//���� Gameroom�� �ִ� ���ӿ�����Ʈ ���� ���� Ŭ�� ����
		{
			Protocol::S_AddObject GameRoomObjects;
			//���ν����尡 Jobó���ϹǷ�, ���� Room�� map����(thread_safe ��Ŀ�� �Ȱǵ��̰��ϸ�)
			for (auto& info : room->GetPlayersForJob()) {
				if (info.second == session->player.lock())
					continue;//���� ������ �ߺ� ����
				*GameRoomObjects.add_objects() = info.second->info;
			}
			for (auto& info : room->GetMonstersForJob())
				*GameRoomObjects.add_objects() = info.second->info;

			SendBufferRef sendBuf = ServerPacketHandler::Make_S_AddObject(GameRoomObjects);
			session->Send(sendBuf); 
			//room->PushSendJob(session, sendBuf); // ��Ŀ�� ioó���ϵ���
		}
		{
			//�ٸ� ��� Ŭ�󿡰� �߰��� �÷��̾� ���� ����
			Protocol::S_AddObject AddedPlayer;
			*AddedPlayer.add_objects() = curPlayer->info;

			SendBufferRef sendBuf = ServerPacketHandler::Make_S_AddObject(AddedPlayer);
			//room->PushBroadcastJob(sendBuf); //��Ŀ�� ó���ϵ���
			room->Broadcast(sendBuf);
		}		
		
		});
	
	
}

void GameSession::OnDisconnected()
{
	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));

	//���� ������
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