#pragma once

#include "CorePch.h"

using GameSessionRef = shared_ptr<class GameSession>;

#include "Enum.pb.h"

using ObjectState = Protocol::OBJECT_STATE_TYPE;
using Dir = Protocol::DIR_TYPE;

#define DIR_DOWN Protocol::DIR_TYPE_DOWN
#define DIR_UP Protocol::DIR_TYPE_UP
#define DIR_RIGHT Protocol::DIR_TYPE_RIGHT
#define DIR_LEFT Protocol::DIR_TYPE_LEFT

#define IDLE Protocol::OBJECT_STATE_TYPE_IDLE
#define MOVE Protocol::OBJECT_STATE_TYPE_MOVE
#define SKILL Protocol::OBJECT_STATE_TYPE_SKILL

#include "Protocol.pb.h"
#include "Struct.pb.h"

#ifdef _DEBUG
#pragma comment(lib, "ServerCore\\Debug\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Debug\\libprotobufd.lib")
#else
#pragma comment(lib, "ServerCore\\Release\\ServerCore.lib")
#pragma comment(lib, "Protobuf\\Release\\libprotobuf.lib")
#endif

#include "ServerPacketHandler.h"


struct VectorInt
{
	VectorInt() {}
	VectorInt(int32 x, int32 y) : x(x), y(y) {}
	VectorInt(POINT pt) : x(pt.x), y(pt.y) {}

	VectorInt operator+(const VectorInt& other)
	{
		VectorInt ret;
		ret.x = x + other.x;
		ret.y = y + other.y;
		return ret;
	}

	VectorInt operator-(const VectorInt& other)
	{
		VectorInt ret;
		ret.x = x - other.x;
		ret.y = y - other.y;
		return ret;
	}

	VectorInt operator*(int32 value)
	{
		VectorInt ret;
		ret.x = x * value;
		ret.y = y * value;
		return ret;
	}

	bool operator<(const VectorInt& other) const
	{
		if (x != other.x)
			return x < other.x;

		return y < other.y;
	}

	bool operator>(const VectorInt& other) const
	{
		if (x != other.x)
			return x > other.x;

		return y > other.y;
	}

	bool operator==(const VectorInt& other)const
	{
		return x == other.x && y == other.y;
	}

	void operator+=(const VectorInt& other)
	{
		x += other.x;
		y += other.y;
	}

	void operator-=(const VectorInt& other)
	{
		x -= other.x;
		y -= other.y;
	}

	int32 LengthSquared()
	{
		return x * x + y * y;
	}

	float Length()
	{
		return (float)::sqrt(LengthSquared());
	}

	int32 Dot(VectorInt other)
	{
		return x * other.x + y * other.y;
	}

	int32 Cross(VectorInt other)
	{
		return x * other.y - y * other.x;
	}

	int32 x = 0;
	int32 y = 0;
};

using Vec2Int = VectorInt;