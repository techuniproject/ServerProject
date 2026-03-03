#pragma once

struct AIRequest {
    uint32_t playerId;                // 누가 보냈는지
    uint32_t npcId;                   // NPC 식별(없으면 0)
    uint32_t convId;                  // 대화 세션(필요 없으면 0)
    std::string userText;             // UTF-8
    std::weak_ptr<class GameRoom> room; // 방송할 방(메인 스레드에서 Broadcast)
    std::string systemPrompt;         // NPC 페르소나 규칙
    std::string contextJson;          // 요약 컨텍스트(선택)
};

class AIQueue {
public:
    void Push(AIRequest&& r) { std::lock_guard<std::mutex> lg(_m); _q.push(std::move(r)); }
    bool TryPop(AIRequest& out) {
        std::lock_guard<std::mutex> lg(_m);
        if (_q.empty()) return false;
        out = std::move(_q.front()); _q.pop(); return true;
    }
private:
    std::mutex _m;
    std::queue<AIRequest> _q;
};
