
/*
[멀티스레드]
프로그램의 작업을 여러 스레드로 나누어 동시에 수행하는 기법


[스레드							vs					프로세스]
 프로세스 내의 실행 단위							실행 중인 프로그램
 독립된 메모리 공간 사용							코드, 힙, 데이터 영역 공유, 스택은 독립적
 독립적 실행										프로세스 내 실행
 생성, 종료 시 높은 비용							생성, 종료 시 낮은 비용

┌───────────────┐ ← 높은 주소
│   Stack       │					◀︎ 스레드마다 따로 존재 (개별적)
├───────────────┤
│   Heap        │					◀︎ 공유
├───────────────┤
│   Data (전역/정적 변수) │		◀︎ 공유
├───────────────┤
│   Code (텍스트 영역)    │		◀︎ 공유
└───────────────┘ ← 낮은 주소

*/
// 코드 영역 공유 확인 예제
//#include <iostream>
//#include <thread>
//
//void func() {
//    std::cout << "함수 주소: " << func << "\n";
//}
//
//int main() {
//    std::thread t1(func);
//    std::thread t2(func);
//
//    t1.join();
//    t2.join();
//    return 0;
//}

// 힙 영역 공유 확인 예제
//#include <iostream>
//#include <thread>
//#include <mutex>
//
//std::mutex mutex;
//
//void task(int* sharedPtr, int id)
//{
//    std::lock_guard<std::mutex> lock(mutex);
//    (*sharedPtr)++;
//    std::cout << "Thread " << id << ": value = " << *sharedPtr << ", addr = " << sharedPtr << '\n';
//
//}
//int main() {
//    int* heapPtr = new int(0);
//
//    std::thread t1(task, heapPtr, 1);
//    std::thread t2(task, heapPtr, 2);
//
//    t1.join();
//    t2.join();
//
//    std::cout << "Final value: " << *heapPtr << '\n';
//    return 0;
//}

// 스택 영역 공유x 확인 예제
//#include <iostream>
//#include <thread>
//
//void task(int id)
//{
//    int localVar = 1;
//    std::cout << "Thread" << id << ": addr = " << &localVar << '\n';
//
//}
//int main() {
//
//    std::thread t1(task, 1);
//    std::thread t2(task, 2);
//
//    t1.join();
//    t2.join();
//
//    return 0;
//}

/*
 [CPU와 스레드 관계]
 CPU는 코어라는 물리적 유닛으로 구성되며, 각 코어는 하나의 스레드를 실행할 수 있는 단위이다.

 ┌───────────────────── CPU (중앙처리장치)  ─────────────────────────┐
│																													│
│  ┌───────── 코어 1 ────────────┐	┌───────── 코어 2 ────────────┐	│
│  │				ALU + 레지스터 등	                │  │	           ALU + 레지스터 등                    │	│
│  └─────────────────────────┘  └─────────────────────────┘	│
│							↑														↑							│	│
│						스레드 A												스레드 B						│	│
└─────────────────────────────────────────────────────────┘
*/

/*
[스택 영역 공유하지 않는 이유]
왜 스택 메모리를 공유하지 않는 것일까 ?

스택은 함수 호출 컨텍스트를 저장하는 공간
- 각 스레드는 함수를 호출하고 리턴하는 구조
- 이때 스택은 다음과 같은 정보들을 가지고 있다.
	함수의 매개변수
	리턴 주소
	지역 변수

만약 공유하게 된다면 ?
- 리턴 주소가 꼬임
- 지역 변수 덮어쓰기 가능

2. 만약 실제로 공유하게 되었을 경우, 어떤 일이 발생할까 ?
만약 스택이 여러 스레드를 공유하게 된다면

	1. 스레드 a가 함수 호출 중인 상태에서 중단
	2. 스레드 b가 같은 스택을 사용해서 다른 함수 호출->스택 내용 덮어써짐.
	3. 스레드 a가 다시 실행될 경우->이전 상태로 복원해서 시작 불가능(스택 오염)

컨텍스트 스위칭은 해당 순서로 일어나게 되는데

	1. 현재 스레드의 레지스터, pc, sp 저장
	2. 다음 스레드의 레지스터, pc, sp 복원
	3. cpu는 새로운 스택 영역을 사용하게 된다.
		(pc(program counter) : 명령어 주소 레지스터) - 명령어 주소 저장
		(sp(stack pointer) :스택 최상단 위치 레지스터) - 스택의 가장 위 주소를 가리키는 레지스터

스택을 공유하게 되면 스레드 간 함수 흐름과 데이터가 충돌하기 때문에 공유할 수 없다.
*/

// 벨로그 작성: https://velog.io/@mongjinjin/%EC%8A%A4%EB%A0%88%EB%93%9C%EB%8A%94-%EC%99%9C-%EC%8A%A4%ED%83%9D%EB%A7%8C-%EA%B3%B5%EC%9C%A0%ED%95%98%EC%A7%80-%EC%95%8A%EC%9D%84%EA%B9%8C-%EB%A9%80%ED%8B%B0%EC%8A%A4%EB%A0%88%EB%94%A9-%EB%A9%94%EB%AA%A8%EB%A6%AC-%EA%B5%AC%EC%A1%B0-%EC%89%BD%EA%B2%8C-%EC%9D%B4%ED%95%B4%ED%95%98%EA%B8%B0