# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This is a Visual Studio C++ solution (`Server.sln`). Build with Visual Studio 2022 targeting x64.

- **Open solution**: `Server.sln` in the repo root
- **Build all**: Visual Studio → Build → Build Solution (Ctrl+Shift+B)
- **Configurations**: Debug / Release, platform x64
- **Run server**: Set `Server` project as startup project, then F5

No CMake, no command-line build script — Visual Studio is the only supported build method.

## Code Generation (Protobuf + Packet Handlers)

Proto definitions live in `Common/protoc-21.12-win64/bin/` (`Protocol.proto`, `Enum.proto`, `Struct.proto`).

**To regenerate all `.pb.cc/.pb.h` files AND packet handler headers for all projects:**
```
Common/protoc-21.12-win64/bin/GenPackets.bat
```
This runs `protoc.exe` then a custom `GenPackets.exe`, then XCOPYs outputs to `Server/`, `DummyClient/`, and `Client/GameCoding/`.

**To regenerate only packet handler headers (Python alternative):**
```
cd Tools/PacketGenerator
MakeExe.bat
```
Requires Python + `jinja2`. Outputs to `Server/ServerPacketHandler.h` and `Client/GameCoding/ClientPacketHandler.h`.

Packet handler convention: `C_` prefix = client-sent (server receives), `S_` prefix = server-sent (client receives).

## Project Structure

```
Server.sln
├── ServerCore/        — IOCP networking engine (static lib)
├── Server/            — Game server application (exe)
├── DummyClient/       — Load-testing client (exe)
├── Client/GameCoding/ — Game client (separate Visual Studio project)
├── Common/            — Shared protoc toolchain and .proto files
├── Tools/PacketGenerator/ — Python/Jinja2 packet handler code generator
└── Libraries/         — Prebuilt libs (Protobuf, ServerCore output)
```

## Architecture

### Two-Layer Design

**ServerCore** is a reusable IOCP engine. It knows nothing about game logic:
- `IocpCore` — wraps `CreateIoCompletionPort` / `GetQueuedCompletionStatus`
- `IocpObject` — base class for anything that can be dispatched by IOCP (Session, Listener)
- `Session` / `PacketSession` — async TCP session with Recv/Send queue; `PacketSession` handles framing (`PacketHeader { uint16 size; uint16 id }` + Protobuf body)
- `Service` / `ServerService` / `ClientService` — owns the IOCP handle, session set, and Listener
- `Listener` — pre-posts multiple `AcceptEx` calls; recycles accept events
- `ThreadManager` — launches/joins `std::thread`s with TLS init/destroy

**Server** is the game content layer:
- `GameSession : PacketSession` — bridges network and game; holds `weak_ptr<GameRoom>` and `weak_ptr<Player>`
- `ServerPacketHandler` — static dispatch table (`g_packet_handler[id]`); deserializes Protobuf and calls `Handle_C_*` functions; provides `Make_S_*` / `MakeSendBuffer` helpers
- `GameRoom` — authoritative game state singleton (`GRoom`); owns `_players`, `_monsters`, `_arrows`, `_items`; runs pathfinding (A\* / BFS via `FindPath`/`MyFindPath`); drives `Update()` and `TickMonsterSpawn()`
- `GameObject → Creature → Player / Monster / Arrow` — entity hierarchy; state machine (`IDLE/MOVE/SKILL/HIT` from `Enum.proto`)
- `Sector` — spatial grid (5 rows × 7 cols, flat `vector<Sector>`, index = `y * SECTOR_WIDTH + x`); `BroadcastBySector` sends only to sectors adjacent to a position

### Thread Model

| Thread | Count | Role |
|---|---|---|
| IOCP worker | 5 | `IocpCore::Dispatch()` loop — processes I/O completions, parses packets, pushes `LambdaJob`s onto `GRoom->_jobs` |
| Main thread | 1 | `GRoom->FlushJobs()` + `GRoom->Update()` loop — executes jobs serially, runs game logic, broadcasts state |
| AI worker | 2 | `AIWorkerPool` — pops `AIRequest` from `AIQueue`, calls OpenAI Responses API via WinHTTP (SSE streaming), pushes result back as a job on `GRoom` |

**Concurrency rule**: Worker threads must not touch game state directly — they must `PushJob(lambda)` onto `GRoom->_jobs` and let the main thread execute them.

### Job System

```cpp
// Push work to be executed on the main thread:
GRoom->PushJob([=]() { /* safe to read/write game state here */ });

// Main thread drains the queue:
GRoom->FlushJobs();  // then GRoom->Update();
```

`JobQueue` is guarded by `USE_LOCK` / `WRITE_LOCK` (aliases for `std::mutex` + `std::lock_guard`).

### AI / LLM Integration

Enabled by `#define FEATURE_LLM_CHAT 1` in `Server/pch.h` (set to 0 to disable).
- API key read from environment variable `OPENAI_API_KEY` at runtime.
- Model: `gpt-4.1-mini` (constant `kModel` in `AIWorker.cpp`).
- AI responses are broadcast as `S_CHAT` with `playerId = 999` to distinguish NPC speech.
- Debug log written to `Server/x64/Debug/ai.log`.

### Packet Flow (Server → Client)

1. Create Protobuf message, call `ServerPacketHandler::MakeSendBuffer(pkt)` → returns `SendBufferRef`
2. Call `GRoom->Broadcast(sendBuffer)` or `GRoom->BroadcastBySector(sendBuffer, sectorPos)` to send to relevant sessions
3. Each `GameSession::Send(sendBuffer)` enqueues it and registers a WSASend via IOCP

### Key Types (ServerCore/Types.h + Server/pch.h)

- Integer aliases: `int8/16/32/64`, `uint8/16/32/64`
- Smart pointer aliases: `SendBufferRef`, `SessionRef`, `GameSessionRef`, `ServerServiceRef`, etc.
- `Vec2Int` / `VectorInt` — 2D integer vector for tile positions
- `ObjectState` = `Protocol::OBJECT_STATE_TYPE`, `Dir` = `Protocol::DIR_TYPE`

### Tilemap

`GameRoom::Init()` loads the tilemap from a **hardcoded absolute path**:
```cpp
_tilemap.LoadFile(L"C:\\Users\\서정원\\Desktop\\...\\Tilemap_01.txt");
```
If running on a different machine, update this path in `Server/GameRoom.cpp`.

## Known Behavior Notes

- **Wall-collision packet spam**: When the client player is pushed against a wall, `SetDir()` fires a dirty flag every tick even for the same direction, causing `SyncToServer()` to send packets every frame. Fix: guard `SetDir` to only set dirty when direction actually changes.
- **Sector broadcast**: `BroadcastBySector` sends to the 3×3 neighborhood of adjacent sectors around a position; clients outside that range will not receive the update.
- **Deferred session destruction**: Sessions are kept alive until all pending IOCP I/O completions return (UAF-safe via `owner = shared_from_this()` on each `IocpEvent`).
