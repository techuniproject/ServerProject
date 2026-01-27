#pragma once

#include "Types.h"
#include "CoreMacro.h"
#include "CoreTLS.h"
#include "CoreGlobal.h"

#include <vector>
#include <list>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <string>
#include <mutex>
#include <optional>
#include <thread>
#include <atomic>
#include <string_view>
#include <random>
#include <numeric>
#include <functional>

#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <windows.h> // 순서 유의
#include <assert.h>
#include <cctype>
#include <iostream>
using namespace std;

#include "SocketUtils.h"
#include "SendBuffer.h"
#include "Session.h"
#pragma comment(lib, "ws2_32.lib")