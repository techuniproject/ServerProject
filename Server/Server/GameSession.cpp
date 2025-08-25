#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ServerPacketHandler.h"
#include "GameRoom.h"

void GameSession::OnConnected()
{
	GSessionManager.Add(static_pointer_cast<GameSession>(shared_from_this()));

	Send(ServerPacketHandler::Make_S_EnterGame());//환영 패킷

	//게임 입장
	GRoom->EnterRoom(GetSessionRef());
}

void GameSession::OnDisconnected()
{
	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));

	//게임 나가기
	GRoom->LeaveRoom(GetSessionRef());
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	/*PacketHeader header = *((PacketHeader*)buffer);
	cout << "Packet ID : " << header.id << "Size : " << header.size << endl;
	*/
	ServerPacketHandler::HandlePacket(static_pointer_cast<GameSession>(shared_from_this()), buffer, len);
}

//int32 GameSession::OnRecv(BYTE* buffer, int32 len)
//{
//	// Echo
//	cout << "OnRecv Len = " << len << endl;
//
//	SendBufferRef sendBuffer = make_shared<SendBuffer>(4096);
//	sendBuffer->CopyData(buffer, len);
//
//	GSessionManager.Broadcast(sendBuffer);
//
//	return len;
//}

void GameSession::OnSend(int32 len)
{
	//cout << "OnSend Len = " << len << endl;
}