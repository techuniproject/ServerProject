#include <iostream>
#include <windows.h>
#include <atomic>
#include <mutex>
#include <queue>
using namespace std;

mutex m;
queue<int> q;
HANDLE hEvent; //커널 오브젝트 가리키는 핸들 정수
condition_variable cv; // 커널 오브젝트가 아닌 유저레벨 오브젝트

void Producer()
{
	while (true) {

		// CV 사용 시,
		// 1) Lock 잡기
		// 2) 공유 변수 값 수정
		// 3) Lock 을 풀고 
		// 4) CV 통해서 통지
		{
			unique_lock<mutex> lock(m);
			q.push(100);
			//여기서 notify_one넣을시 락이 풀리기 전의 시간동안 반대편에서 wait함
			// cv.notify_one();
			// 여기서 호출하면 깨운 스레드는 mutex획득을 위해 다시 대기할 수 있음. lock상태이므로

		}//락을 풀고 CV통지를 위해 범위 지정
		cv.notify_one(); //대기 타고 있는 애들 중 하나만 깨움, notifyall은 다 깨움


		//event방식
	//	unique_lock<mutex> lock(m);
	//	q.push(100);

	//	::SetEvent(hEvent); //Signal (파란불로 바꿈)
	//
	//	this_thread::sleep_for(100ms); // 현재 스레드를 재워 양보하여 컨텍스트 스위칭 시키고
									   // blocked상태 로 바꾸고 100ms 이후 ready 상태 큐로 넣음



	}
}

void Consumer()
{
	while (true) {

		// ::WaitForSingleObject(hEvent, INFINITE); // 현재 스레드를 커널이 Blocked 상태로 전환 - CPU 반환 + 현재 스레드 잠듦
												 // SetEvent가 호출되어 Signaled상태되면 OS가 대기 중인 스레드를 Ready상태로 꺠움
												 // Event가 파란불될때까지 
												 // CPU 검사를 통한 무한정 대기하는 것이 아닌 잠들어있다가 깨움

		// ::ResetEvent(hEvent); //Manual reset을 수동으로 하면 이 함수로 초기상태 값으로 초기화

		//이벤트 방식은 결국 통과를 하더라도 조건에 의해 의미없는 CPU 타임슬라이스 소모 시, 자원낭비가 됨
		// 그러므로 condition_variable로 조건을 동시에 체킹할 수 있는 형태로 하면 좋음.

		//-> cv방식으로
		unique_lock<mutex> lock(m);
		cv.wait(lock, []() {return q.empty() == false; });
		// 1) Lock을 잡으려고 시도 (이미 잡혔으면 skip)
		// 2) 조건을 확인 (unique_lock을 한 이유 : 중간에 lock을 걸고 풀고를 가능하게 하기 위해)
		//  -> 만족 시 : 바로 이어나가 코드 진행 (처리할 데이터가 있다는 것이므로 lock유지한 채 실행)
		//  -> 불만족 시 : Lock을 풀고 대기 상태로 전환


		//event방식 	unique_lock<mutex> lock(m);
		//if (q.empty() == false) 
		{
			int data = q.front();
			q.pop();
			cout << data << '\n';
		}
	}
}

int main() {

	// 커널 오브젝트
	// - Usage Count
	// - Signal (파란불) / Non-Signal (빨간불)

	//bool -> Auto/Manual
	hEvent = ::CreateEvent(NULL/*보안속성*/, FALSE/*bManualReset*/, FALSE/*초기상태*/, NULL);


	thread t1(Producer);
	thread t2(Consumer);

	t1.join();
	t2.join();

	::CloseHandle(hEvent);
}