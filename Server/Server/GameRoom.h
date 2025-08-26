#pragma once

class GameRoom : public enable_shared_from_this<GameRoom>
{
public:
	GameRoom();
	virtual ~GameRoom();

	void Init();
	void Update();

	void EnterRoom(GameSessionRef session);
	void LeaveRoom(GameSessionRef session);
	shared_ptr<class GameObject> FindObject(uint64 id);

	shared_ptr<GameRoom> GetRoomRef() { return shared_from_this(); }
public:
	//PacketHandler
	void Handle_C_Move(Protocol::C_Move& pkt);
public:
	void AddObject(shared_ptr<class GameObject> gameObject);
	void RemoveObject(uint64 id);
	void Broadcast(SendBufferRef& sendBuffer);

private:
	map<uint64, shared_ptr<class Player>> _players;
	map<uint64, shared_ptr<class Monster>> _monsters;
};

extern shared_ptr<GameRoom> GRoom;
