//============2025-07-01==========
#include <iostream>
#include <thread>
#include <chrono>

//using namespace std;

//========================  [ 스레드 정의 ] ======================================//
																																																			/*
 하나의 프로세스 내에서 실행되는 독립적인 실행 흐름
 
 [멀티 스레드 간 메모리 공유]
 
 {공유 영역} 
  1. 코드 영역 
  2. 데이터 영역
  3. 힙 영역

 {고유 영역} 
  1. 스택 영역
 
 [안전 영역] 
  1. 코드 영역 - 공유해도 무관
  2. 스택 영역 - 스레드 간 개별 영역

 [임계 영역] 
  1. 데이터 영역 - 전역 공간으로 공유자원에 해당
  2. 힙 영역 - 동일 메모리 공간 가르키므로 공유자원에 해당
																																													*/


//======================== [ std::thread 기본 사용법 ] ======================================//
// [필요 표준 헤더 파일] #include <thread>  - C++11~
		
void do_work() {
	std::cout << "Thread Create Method\n";
																																																						/*
	 1. void do_work() 
																			
	  타입 : void (*)() 함수 포인터 callable
																																																*/
}

int main() {
	std::thread t(do_work); 
	t.join();
																																																									/*
	  2. std::thread t(do_work) 

		생성자 : do work 함수를 실행하는 새로운 OS-Level 스레드 생성
				-> do_work 주소(함수 포인터로)가 std::thread 생성자로 전달 - 데이터 영역의 코드 주소가 전달
																																																			*/

}
		
//======================== [ std::thread 핵심 메서드 ] ======================================//

	std::thread t1(do_work);
	
	t1.join();
																																													/*
	1. [ join() ]

	- 해당 스레드가 종료될 때까지 메인 스레드가 기다림

	- 반드시 호출해야 함 (안 하면 프로그램 종료 시 std::terminate() 발생)
		
	- 메인 스레드가 먼저 종료 시, terminate()	
																																				*/

	std::thread t1(do_work);

	t1.detach();
																																						/*
	2. [ detach() ] - 하고나면 추적 불가하므로 잊고 살기

	- 스레드를 독립적으로 실행시키고 소유권을 해제

	- 스레드 종료 여부를 알 수 없음

	- 주의: detach() 후 해당 스레드가 죽은 객체에 접근하면 undefined behavior

====== [detach에 대한 고찰] ======
																																			*/
	void fn() {
		std::cout << "fn" << std::endl;
	}

	void threadCaller() {
		std::thread t1(fn);
		t1.detach();
	}

	int main1() {
		threadCaller();
																																																	/*
		main 스레드에서 threadCaller() 함수 호출을 통해 main함수 스택 프레임에 t1 thread가 적재
		이후, detach를 통해 fn()를 실행하는 주체인 t1 스레드는 추적 불가 상태가 됨.
		threadCaller() 함수가 종료 후 스택프레임 반환과 동시 원래 t1도 join되어 스레드 종료가 정상적이지만
		해당 경우 독립적으로 실행되는 스레드이므로 함수 종료 이후에도 실행.
		하지만, 더이상 추적할 방법이 없어 권장 X
																																													*/


		std::this_thread::sleep_for(std::chrono::seconds(3));
																																																				/*
		메인 스레드 종료되기 전(프로세스 종료 전)에 t1이 fn을 실행할 시간을 벌어주기. 
		t1은 detach로 소유권 잃고 OS수준의 스레드가 되어 백그라운드에서 독립적으로 실행
		만약 t1이 fn 실행 전 메인 스레드 종료 시 t1은 terminate되진 않고 그냥 조용히 아무 출력없이 종료
																																																	*/
	}
																																								/*
	======[detach에 대한 고찰 대체 방안] ======
																																									*/
	std::thread gThread;
	
	void fn() {
		std::cout << "fn" << std::endl;
	}

	void threadCaller() {
		gThread = std::thread(fn);
	}

	int main2() {
		
		threadCaller();

																																											/*
		- gThread는 전역으로 유지되며 소유권을 유지함
		- join을 통해 스레드의 정상 종료를 명확히 기다림
		- main함수 종료 전까지 스레드 종료됨을 보장
																																								*/

		std::this_thread::sleep_for(std::chrono::seconds(3));

		gThread.join();
	}

//--------------------------------------------------------------

	std::thread t1(do_work);

	t1.joinable();
																																										/*
	3. [ joinable() ] 

	- 해당 스레드가 join이나 detach 되지 않은 상태인지 확인

	- 안전하게 join()할 수 있는지 검사할 때 필수
																																		*/


	std::thread t1(do_work);

	t1.get_id();
																																	/*
	4. [ get_id() ]

	- 해당 스레드의 고유 ID 반환 (비교/해시 가능)

	- std::this_thread::get_id()로 현재 실행 중인 스레드의 ID도 확인 가능

	
																																	*/
	std::thread t1(do_work), t2(do_work);

	t1.swap(t2);
	t1 = t2;
																																								/*
	5. [ swap() / operator= ]

	- 두 스레드 객체의 내부 핸들을 교환

	- 복사는 불가하지만 이동(move) 은 가능


																																*/

//======================== [ std::thread 생성 인자 ] ======================================//

																																															/*
	[ 일반 함수 + 값 인자 ]
																																															*/	
	void print(int a, char c) {
		std::cout << a << " " << c << "\n";
	}

	std::thread t(print, 10, 'X'); // OK

																																																/*
	[ 람다 함수 ]
																																																	*/
	std::thread t([](int x) {
		std::cout << "x = " << x << "\n";
		}, 5);

																																															/*
	[ 참조로 인자 전달 (주의: 반드시 std::ref) ]
																																																	*/
	void update(int& x) {
		x += 1;
	}

	int val = 10;
	std::thread t(update, std::ref(val)); // std::ref 없으면 복사됨!

																																																/*
	[ 멤버 함수 호출 ]
																																																	*/
	class Worker {
	public:
		void run(int id) {
			std::cout << "Worker " << id << "\n";
		}
	};

	Worker w;
	std::thread t(&Worker::run, &w, 1); // 객체 포인터 &w 전달


																																																	/*
	[ 함수 객체 (Functor) ]
																																																	*/
	struct Task {
		void operator()(int a) {
			std::cout << "Task with " << a << "\n";
		}
	};

	std::thread t(Task(), 42);

																																																		/*
	[ 가변 인자 함수 ]
																																																	*/
	void print_all(const std::string& a, int b, double c) {
		std::cout << a << ", " << b << ", " << c << "\n";
	}

	std::thread t(print_all, "score", 100, 3.14);



//-----------------------------------------------------------