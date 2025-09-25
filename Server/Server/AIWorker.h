#pragma once
#include "AIQueue.h"

class AIWorkerPool {
public:
    void Start(int workers, AIQueue* q);
    void Stop();

private:
    void WorkerMain();

private:
    std::vector<std::thread> _threads;
    std::atomic<bool> _running{ false };
    AIQueue* _queue = nullptr;
};
