#include "pch.h"
#include "CoreTLS.h"

thread_local uint32 LThreadId = 0;
//현재 실행 스레드의 tls를 사용.