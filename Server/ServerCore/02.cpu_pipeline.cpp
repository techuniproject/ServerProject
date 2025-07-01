//============2025-07-01==========
#include <iostream>
#include <thread>
#include <chrono>



//========================  [ 스레드 정의 ] ======================================//
																																																																										/*
		[==================[강의 내용]=====================]
																																																																											*/																																												
									
int x = 0;
int y = 0;
int r1 = 0;
int r2 = 0;

bool ready = false;
// volatile bool ready = false - volatile은 컴파일러 최적화(Store/Load)가 불가하도록 - 휘발성(바뀔 수 있으니 최적화 X) 보장
// [volatile] -> 캐싱 금지 : 레지스터에 저장해두고 재사용하는 최적화 금지 /  읽기/쓰기 생략 금지 : 변수의 실제 메모리 주소에 매번 접근하도록 강제
// [volatile] -> 항상 메모리 접근 : 값을 매번 메모리에서 읽고, 쓸 때도 직접 씀

// 초기 x, y, r1, r2 모두 0
// ready는 false -> thread 두개 함수 실행 이후 true로 바꿔줌
void Thread_1() {

	while (ready == false) {}

	y = 1;  //Store y
	r1 = x; // Load x
}

void Thread_2() {

	while (ready == false) {}

	x = 1;  //Store x
	r1 = y; // Load y
}
				
int main() {
	
	int count = 0;

	while (1) {

		ready = false;
		count++;
		x = y = r1 = r2 = 0;
		std::thread t1(Thread_1);
		std::thread t2(Thread_2);

		ready = true;
		
		t1.join();
		t2.join();

		if (r1 == 0 && r2 == 0)break;
	}
	std::cout << count << std::endl;
}

					
																																																																																								/*
 ✅ 핵심 개념: Memory Reordering (메모리 재정렬)
	
	- CPU는 성능 최적화를 위해 개발자가 작성한 코드 순서와는 다르게 연산 순서를 바꿔서 실행할 수 있어.

	- 특히 store → load 순서가 load → store처럼 뒤바뀌는 게 대표적인 최적화 중 하나야.


	🧠 왜 순서를 바꾸는가?
	
	CPU는 내부적으로

	1. 파이프라인 구조: 각 명령어가 단계별로 동시에 처리됨

	2. Out-of-Order Execution: 가능한 연산부터 먼저 실행하여 유휴 자원 최소화

	3. Store Buffer / Load Buffer: 메모리 접근을 효율적으로 관리함

	[이런 이유들로 인해 CPU는 다음과 같은 일이 가능함]

	🔁 예시: store → load 재정렬
																																							*/
		x = 1;      // store
		r1 = y;     // load
																																																			/*
	개발자는 위처럼 코딩했지만, CPU는 아래처럼 실행할 수도 있음:
																																																				*/
		r1 = y;     // load
		x = 1;      // store	
																																																			/*
	✅ 단일 스레드에서는 결과가 문제 없을 수 있지만
	❗ 멀티스레드 환경에서는 상태 불일치(bug, race condition) 발생 가능	
																																																	*/

																																											/*		
	📌 실제 문제 시나리오
	🔴 예시 : 두 스레드 간의 flag + data
																																															*/
		// Thread 1
		data = 42;         // (1) store
		ready = true;      // (2) store

		// Thread 2
		if (ready) {       // (3) load
			print(data);   // (4) load
		}

																																																							/*
		개발자는(1) →(2) →(3) →(4)를 기대하지만,
			CPU는 아래 순서로 실행 가능함 :

		(2) ready = true 먼저 store됨

		(3) ready를 보고 true임

		(4) data는 아직 42로 안 바뀜 → 쓰레기 값 읽힘
			

		✅ 해결책: Memory Barrier (메모리 장벽)
		C++에서는 std::atomic과 memory_order를 통해 제어함																																																									*/

		// Thread 1
		data.store(42, std::memory_order_relaxed);
		ready.store(true, std::memory_order_release); // release barrier

		// Thread 2
		if (ready.load(std::memory_order_acquire)) {  // acquire barrier
			std::cout << data.load() << '\n';
		}

																																																				/*
		🔒 release / acquire 의미
			release : 이전의 모든 store들이 메모리에 반드시 먼저 반영된 뒤, ready = true가 수행됨

			acquire : ready == true를 읽은 후에는, 그 이후의 load들은 모두 이전의 store 값을 보장받음
																																																											*/
