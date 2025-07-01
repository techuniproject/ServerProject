//============2025-07-01==========
#include <iostream>
#include <thread>
#include <chrono>

//using namespace std;

//========================  [ ������ ���� ] ======================================//
																																																			/*
 �ϳ��� ���μ��� ������ ����Ǵ� �������� ���� �帧
 
 [��Ƽ ������ �� �޸� ����]
 
 {���� ����} 
  1. �ڵ� ���� 
  2. ������ ����
  3. �� ����

 {���� ����} 
  1. ���� ����
 
 [���� ����] 
  1. �ڵ� ���� - �����ص� ����
  2. ���� ���� - ������ �� ���� ����

 [�Ӱ� ����] 
  1. ������ ���� - ���� �������� �����ڿ��� �ش�
  2. �� ���� - ���� �޸� ���� ����Ű�Ƿ� �����ڿ��� �ش�
																																													*/


//======================== [ std::thread �⺻ ���� ] ======================================//
// [�ʿ� ǥ�� ��� ����] #include <thread>  - C++11~
		
void do_work() {
	std::cout << "Thread Create Method\n";
																																																						/*
	 1. void do_work() 
																			
	  Ÿ�� : void (*)() �Լ� ������ callable
																																																*/
}

int main() {
	std::thread t(do_work); 
	t.join();
																																																									/*
	  2. std::thread t(do_work) 

		������ : do work �Լ��� �����ϴ� ���ο� OS-Level ������ ����
				-> do_work �ּ�(�Լ� �����ͷ�)�� std::thread �����ڷ� ���� - ������ ������ �ڵ� �ּҰ� ����
																																																			*/

}
		
//======================== [ std::thread �ٽ� �޼��� ] ======================================//

	std::thread t1(do_work);
	
	t1.join();
																																													/*
	1. [ join() ]

	- �ش� �����尡 ����� ������ ���� �����尡 ��ٸ�

	- �ݵ�� ȣ���ؾ� �� (�� �ϸ� ���α׷� ���� �� std::terminate() �߻�)
		
	- ���� �����尡 ���� ���� ��, terminate()	
																																				*/

	std::thread t1(do_work);

	t1.detach();
																																						/*
	2. [ detach() ] - �ϰ��� ���� �Ұ��ϹǷ� �ذ� ���

	- �����带 ���������� �����Ű�� �������� ����

	- ������ ���� ���θ� �� �� ����

	- ����: detach() �� �ش� �����尡 ���� ��ü�� �����ϸ� undefined behavior

====== [detach�� ���� ����] ======
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
		main �����忡�� threadCaller() �Լ� ȣ���� ���� main�Լ� ���� �����ӿ� t1 thread�� ����
		����, detach�� ���� fn()�� �����ϴ� ��ü�� t1 ������� ���� �Ұ� ���°� ��.
		threadCaller() �Լ��� ���� �� ���������� ��ȯ�� ���� ���� t1�� join�Ǿ� ������ ���ᰡ ������������
		�ش� ��� ���������� ����Ǵ� �������̹Ƿ� �Լ� ���� ���Ŀ��� ����.
		������, ���̻� ������ ����� ���� ���� X
																																													*/


		std::this_thread::sleep_for(std::chrono::seconds(3));
																																																				/*
		���� ������ ����Ǳ� ��(���μ��� ���� ��)�� t1�� fn�� ������ �ð��� �����ֱ�. 
		t1�� detach�� ������ �Ұ� OS������ �����尡 �Ǿ� ��׶��忡�� ���������� ����
		���� t1�� fn ���� �� ���� ������ ���� �� t1�� terminate���� �ʰ� �׳� ������ �ƹ� ��¾��� ����
																																																	*/
	}
																																								/*
	======[detach�� ���� ���� ��ü ���] ======
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
		- gThread�� �������� �����Ǹ� �������� ������
		- join�� ���� �������� ���� ���Ḧ ��Ȯ�� ��ٸ�
		- main�Լ� ���� ������ ������ ������� ����
																																								*/

		std::this_thread::sleep_for(std::chrono::seconds(3));

		gThread.join();
	}

//--------------------------------------------------------------

	std::thread t1(do_work);

	t1.joinable();
																																										/*
	3. [ joinable() ] 

	- �ش� �����尡 join�̳� detach ���� ���� �������� Ȯ��

	- �����ϰ� join()�� �� �ִ��� �˻��� �� �ʼ�
																																		*/


	std::thread t1(do_work);

	t1.get_id();
																																	/*
	4. [ get_id() ]

	- �ش� �������� ���� ID ��ȯ (��/�ؽ� ����)

	- std::this_thread::get_id()�� ���� ���� ���� �������� ID�� Ȯ�� ����

	
																																	*/
	std::thread t1(do_work), t2(do_work);

	t1.swap(t2);
	t1 = t2;
																																								/*
	5. [ swap() / operator= ]

	- �� ������ ��ü�� ���� �ڵ��� ��ȯ

	- ����� �Ұ������� �̵�(move) �� ����


																																*/

//======================== [ std::thread ���� ���� ] ======================================//

																																															/*
	[ �Ϲ� �Լ� + �� ���� ]
																																															*/	
	void print(int a, char c) {
		std::cout << a << " " << c << "\n";
	}

	std::thread t(print, 10, 'X'); // OK

																																																/*
	[ ���� �Լ� ]
																																																	*/
	std::thread t([](int x) {
		std::cout << "x = " << x << "\n";
		}, 5);

																																															/*
	[ ������ ���� ���� (����: �ݵ�� std::ref) ]
																																																	*/
	void update(int& x) {
		x += 1;
	}

	int val = 10;
	std::thread t(update, std::ref(val)); // std::ref ������ �����!

																																																/*
	[ ��� �Լ� ȣ�� ]
																																																	*/
	class Worker {
	public:
		void run(int id) {
			std::cout << "Worker " << id << "\n";
		}
	};

	Worker w;
	std::thread t(&Worker::run, &w, 1); // ��ü ������ &w ����


																																																	/*
	[ �Լ� ��ü (Functor) ]
																																																	*/
	struct Task {
		void operator()(int a) {
			std::cout << "Task with " << a << "\n";
		}
	};

	std::thread t(Task(), 42);

																																																		/*
	[ ���� ���� �Լ� ]
																																																	*/
	void print_all(const std::string& a, int b, double c) {
		std::cout << a << ", " << b << ", " << c << "\n";
	}

	std::thread t(print_all, "score", 100, 3.14);



//-----------------------------------------------------------