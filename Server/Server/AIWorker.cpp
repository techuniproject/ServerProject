#include "pch.h"
#include "AIWorker.h"
#include <winhttp.h>
#include "json.hpp"
#include "ServerPacketHandler.h"
#include "GameRoom.h"
// AIWorker.cpp

#include "pch.h"
#include "AIWorker.h"
#include <winhttp.h>
#include "json.hpp"
#include "ServerPacketHandler.h"
#include "GameRoom.h"

#include <algorithm> // std::min
#include <cstdio>    // fopen_s

#pragma comment(lib, "winhttp.lib")
using json = nlohmann::json;

static const wchar_t* kHost = L"api.openai.com";
static const wchar_t* kPath = L"/v1/responses";
static const char* kModel = "gpt-4.1-mini";

// --- 간단 로거: VS Output + ai.log ---
static void AILog(const std::string& s) {
    ::OutputDebugStringA((s + "\n").c_str());
    FILE* f = nullptr;
    if (fopen_s(&f, "ai.log", "a") == 0 && f) { fwrite(s.data(), 1, s.size(), f); fputc('\n', f); fclose(f); }
}

// ---------- UTF-8 유틸 ----------
static std::string EnsureUtf8(const std::string& s) {
    if (s.empty()) return s;
    int w = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), (int)s.size(), nullptr, 0);
    if (w > 0) return s;
    int w2 = ::MultiByteToWideChar(CP_ACP, 0, s.data(), (int)s.size(), nullptr, 0);
    if (w2 <= 0) return s;
    std::wstring ws(w2, L'\0');
    ::MultiByteToWideChar(CP_ACP, 0, s.data(), (int)s.size(), &ws[0], w2);
    int u8 = ::WideCharToMultiByte(CP_UTF8, 0, ws.data(), w2, nullptr, 0, nullptr, nullptr);
    std::string out(u8, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, ws.data(), w2, &out[0], u8, nullptr, nullptr);
    return out;
}

static std::wstring U8ToW(const std::string& s) {
    if (s.empty()) return {};
    int w = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out; out.resize(w ? w - 1 : 0);
    if (w) ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], w);
    return out;
}

// data: 라인에서 payload 추출
static bool ExtractDataLine(const std::string& line, std::string& out) {
    if (line.compare(0, 5, "data:") != 0) return false;
    size_t i = 5;
    while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
    out.assign(line.data() + i, line.size() - i);
    return true;
}

// data payload(JSON) 처리 → 델타는 누적, 완료에서 한 번만 방송
static void ProcessDataJson(const std::string& payload, const AIRequest& req, std::string& accum) {
    if (payload == "[DONE]" || payload == "[DONE]\n" || payload == "[DONE]\r\n") {
        AILog("[AI] DONE");
        return;
    }

    if (!nlohmann::json::accept(payload)) { AILog("[AI] json::accept=false"); return; }

    try {
        nlohmann::json j = nlohmann::json::parse(payload);

        // 최신 Responses SSE
        if (j.contains("type") && j["type"].is_string()) {
            std::string t = j["type"].get<std::string>();

            // 델타 → 누적만
            if ((t == "response.output_text.delta" || t == "response.output.delta") && j.contains("delta") && j["delta"].is_string()) {
                accum += j["delta"].get<std::string>();
                return;
            }

            // 완료 → 지금까지 누적본 한 번에 방송
            if (t == "response.completed") {
                if (!accum.empty()) {
                    if (auto room = req.room.lock()) {
                        std::string text = accum; accum.clear();
                        room->PushJob([room, text]() {
                            Protocol::S_CHAT pkt; pkt.set_msg(text); pkt.set_playerid(999);
                            if (auto sb = ServerPacketHandler::MakeSendBuffer(pkt)) room->Broadcast(sb);
                            });
                    }
                }
                return;
            }

            // 에러 이벤트 → 유저에게 안내
            if (t == "error") {
                std::string msg = "AI 에러";
                if (j.contains("error") && j["error"].contains("type")) msg += " (" + j["error"]["type"].get<std::string>() + ")";
                if (auto room = req.room.lock()) {
                    room->PushJob([room, text = msg]() {
                        Protocol::S_CHAT pkt; pkt.set_msg(text); pkt.set_playerid(999);
                        if (auto sb = ServerPacketHandler::MakeSendBuffer(pkt)) room->Broadcast(sb);
                        });
                }
                return;
            }
        }

        // 비스트리밍/호환 경로: 한 번에 온 텍스트 즉시 방송
        std::string once;
        if (j.contains("output_text")) once += j["output_text"].get<std::string>();
        if (j.contains("message") && j["message"].contains("content"))
            for (auto& c : j["message"]["content"]) if (c.contains("text")) once += c["text"].get<std::string>();
        if (j.contains("output"))
            for (auto& it : j["output"])
                if (it.contains("content"))
                    for (auto& c : it["content"])
                        if (c.contains("text")) once += c["text"].get<std::string>();

        if (!once.empty()) {
            if (auto room = req.room.lock()) {
                std::string text = once;
                room->PushJob([room, text]() {
                    Protocol::S_CHAT pkt; pkt.set_msg(text); pkt.set_playerid(999);
                    if (auto sb = ServerPacketHandler::MakeSendBuffer(pkt)) room->Broadcast(sb);
                    });
            }
        }
    }
    catch (const nlohmann::json::exception& e) {
        AILog(std::string("[AI] json ex: ") + e.what());
    }
}

// ---------- 워커 풀 ----------
void AIWorkerPool::Start(int workers, AIQueue* q) {
    if (_running.exchange(true)) return;
    _queue = q;
    for (int i = 0; i < workers; ++i) _threads.emplace_back(&AIWorkerPool::WorkerMain, this);
}

void AIWorkerPool::Stop() {
    if (!_running.exchange(false)) return;
    for (auto& t : _threads) if (t.joinable()) t.join();
    _threads.clear();
}

void AIWorkerPool::WorkerMain() {
    try {
        wchar_t key[1024]{}; ::GetEnvironmentVariableW(L"OPENAI_API_KEY", key, 1024);

        HINTERNET hSession = ::WinHttpOpen(
            L"GameServer-AI/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

        while (_running) {
            HINTERNET hc = nullptr, hr = nullptr;
            try {
                AIRequest req;
                if (!_queue->TryPop(req)) { ::Sleep(1); continue; }

                auto close_req = [&]() {
                    if (hr) { ::WinHttpCloseHandle(hr); hr = nullptr; }
                    if (hc) { ::WinHttpCloseHandle(hc); hc = nullptr; }
                    };

                // ----- 요청 본문 구성 (단일 문자열 input) -----
                std::string sys = EnsureUtf8(req.systemPrompt);
                std::string user = EnsureUtf8(req.userText);
                std::string ctx = EnsureUtf8(req.contextJson);

                std::string prompt = sys + "\n\nUser: " + user + "\n\n[CONTEXT] " + ctx;

                json body = {
                    {"model", kModel},
                    {"input", prompt},
                    {"temperature", 0.6},
                    {"stream", true}
                };
                std::string bodyStr = body.dump();
                AILog("[AI] REQ BODY PREVIEW: " + bodyStr.substr(0, 256));

                // ----- HTTP -----
                hc = ::WinHttpConnect(hSession, kHost, INTERNET_DEFAULT_HTTPS_PORT, 0);
                hr = ::WinHttpOpenRequest(hc, L"POST", kPath, nullptr,
                    WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                    WINHTTP_FLAG_SECURE);

                std::wstring headers = L"Content-Type: application/json\r\n"
                    L"Accept: text/event-stream\r\n"
                    L"Authorization: Bearer ";
                headers += key; headers += L"\r\n";

                if (::WinHttpSendRequest(hr, headers.c_str(), (DWORD)-1L,
                    (LPVOID)bodyStr.data(), (DWORD)bodyStr.size(),
                    (DWORD)bodyStr.size(), 0)
                    && ::WinHttpReceiveResponse(hr, nullptr)) {

                    // ----- 상태코드/에러바디 로깅 -----
                    DWORD status = 0; DWORD sz = sizeof(status);
                    if (::WinHttpQueryHeaders(hr,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz, WINHTTP_NO_HEADER_INDEX))
                    {
                        char sbuf[64]; _snprintf_s(sbuf, _TRUNCATE, "[AI] HTTP %lu\n", status);
                        ::OutputDebugStringA(sbuf);
                    }

                    if (status != 200) {
                        std::string errBody; DWORD avail2 = 0;
                        while (::WinHttpQueryDataAvailable(hr, &avail2) && avail2) {
                            std::vector<char> eb(avail2); DWORD rd2 = 0;
                            if (!::WinHttpReadData(hr, eb.data(), avail2, &rd2) || rd2 == 0) break;
                            errBody.append(eb.data(), rd2);
                        }
                        AILog("[AI] ERROR BODY:\n" + errBody);
                        close_req();
                        continue;
                    }

                    // ----- SSE 수신 -----
                    std::string chunk; chunk.reserve(16 * 1024);
                    std::string accum; // ← 누적 버퍼

                    for (;;) {
                        DWORD avail = 0;
                        if (!::WinHttpQueryDataAvailable(hr, &avail)) {
                            AILog("[AI] QueryDataAvailable failed: " + std::to_string(GetLastError()));
                            break;
                        }
                        if (avail == 0) { ::Sleep(20); continue; }

                        std::vector<char> buf(avail);
                        DWORD rd = 0;
                        if (!::WinHttpReadData(hr, buf.data(), avail, &rd) || rd == 0) {
                            AILog("[AI] ReadData ended");
                            break;
                        }

                        chunk.append(buf.data(), rd);

                        size_t pos = 0;
                        while (true) {
                            size_t nl = chunk.find('\n', pos);
                            if (nl == std::string::npos) { chunk.erase(0, pos); break; }

                            std::string line = chunk.substr(pos, nl - pos);
                            if (!line.empty() && line.back() == '\r') line.pop_back();
                            pos = nl + 1;

                            if (line.empty()) continue; // 이벤트 경계

                            std::string payload;
                            if (ExtractDataLine(line, payload)) {
                                AILog("[AI] data: " + payload.substr(0, std::min<size_t>(payload.size(), 80)));
                                ProcessDataJson(payload, req, accum); // ← 누적 처리
                            }
                            else {
                                AILog("[AI] line: " + line.substr(0, std::min<size_t>(line.size(), 80)));
                            }
                        }
                    }

                    // 스트림이 비정상 종료돼도 남은 누적분이 있으면 한 번 더 방송
                    if (!accum.empty()) {
                        if (auto room = req.room.lock()) {
                            std::string text = accum; accum.clear();
                            room->PushJob([room, text]() {
                                Protocol::S_CHAT pkt; pkt.set_msg(text); pkt.set_playerid(999);
                                if (auto sb = ServerPacketHandler::MakeSendBuffer(pkt)) room->Broadcast(sb);
                                });
                        }
                    }
                }

                close_req();
            }
            catch (const std::exception& e) {
                ::OutputDebugStringA((std::string("[AI] WorkerMain inner ex: ") + e.what() + "\n").c_str());
                if (hr) { ::WinHttpCloseHandle(hr); hr = nullptr; }
                if (hc) { ::WinHttpCloseHandle(hc); hc = nullptr; }
            }
            catch (...) {
                ::OutputDebugStringA("[AI] WorkerMain inner ex: unknown\n");
                if (hr) { ::WinHttpCloseHandle(hr); hr = nullptr; }
                if (hc) { ::WinHttpCloseHandle(hc); hc = nullptr; }
            }
        }

        if (hSession) ::WinHttpCloseHandle(hSession);
    }
    catch (const std::exception& e) {
        std::string msg = "[AI] WorkerMain outer ex: "; msg += e.what(); msg += "\n";
        ::OutputDebugStringA(msg.c_str());
    }
    catch (...) {
        ::OutputDebugStringA("[AI] WorkerMain outer ex: unknown\n");
    }
}