#include <iostream>
#include <windows.h>
#include <atomic>
#include <mutex>
#include <queue>
#include <map>
using namespace std;

class RefCountable
{
public:
	RefCountable() {}
	virtual ~RefCountable() {}

	int GetRefCount() { return _refCount; }
	int AddRef() { return ++_refCount; }
	int ReleaseRef() {
		int refCount = --_refCount;
		if (refCount == 0)
			delete this; // virtual 소멸자이므로 해당 문법은 Knight*를 가리키면 Knight 소멸자 까지 호출되어 전체 객체 파괴
		return refCount;
	}
protected:
	atomic<int> _refCount = 1; // 증감 연산자 등 원자적 연산을 위해 atomic으로 사용
};

class Knight : public RefCountable
{
public:

};

map<int, Knight*> _k;


int main() {
	Knight* k = new Knight();

	_k[100] = k;
	k->AddRef();
	k->ReleaseRef();

	_k.erase(100);
	k->ReleaseRef();
	k = nullptr;
}



//===========================================================================================================================================


#include <iostream>
#include <windows.h>
#include <atomic>
#include <mutex>
#include <queue>
#include <map>
using namespace std;

class RefCountable
{
public:
	RefCountable() {}
	virtual ~RefCountable() {}

	int GetRefCount() { return _refCount; }
	int AddRef() { return ++_refCount; }
	int ReleaseRef() {
		int refCount = --_refCount;
		if (refCount == 0)
			delete this; // virtual 소멸자이므로 해당 문법은 Knight*를 가리키면 Knight 소멸자 까지 호출되어 전체 객체 파괴
		return refCount;
	}
protected:
	atomic<int> _refCount = 1; // 증감 연산자 등 원자적 연산을 위해 atomic으로 사용
};

class Knight : public RefCountable
{
public:

};

map<int, Knight*> _k;

void Test(Knight* knight) {
	_k[100] = knight;
	knight->AddRef();
}
// 멀티스레드에서 이 코드가 유효하지 않은 이유
// atomic변수더라도 지금은 공유 자원의 연산 문제가 아닌 로직간 관계에서 생기므로 
// 이 부분도 동기화가 필요함.
//-> 매개변수로 이미 knight(힙영역 공유 데이터) 를 가져올때 RefCount가 늘어나면서
// 받아와야 동시다발적으로 다른데서 refcount를 줄이더라도 문제가 없지만,
// 받아오고 저 AddRef를 실행하기 전에 다른 스레드가 ReleaseRef까지 실행해버리면 
// 유효하지 않은 메모리를 접근

// 그러므로 수동 RefCount관리가 어려우며 위험함

int main() {
	Knight* k = new Knight();

	thread t(Test, k);

	k->ReleaseRef();

	t.join();

}

//===========================================================================================================================================


#include <iostream>
#include <windows.h>
#include <atomic>
#include <mutex>
#include <queue>
#include <map>
using namespace std;

class RefCountable
{
public:
	RefCountable() {}
	virtual ~RefCountable() {}

	int GetRefCount() { return _refCount; }
	int AddRef() { return ++_refCount; }
	int ReleaseRef() {
		int refCount = --_refCount;
		if (refCount == 0)
			delete this; // virtual 소멸자이므로 해당 문법은 Knight*를 가리키면 Knight 소멸자 까지 호출되어 전체 객체 파괴
		return refCount;
	}
protected:
	atomic<int> _refCount = 1; // 증감 연산자 등 원자적 연산을 위해 atomic으로 사용
};


template<typename T>
class TSharedPtr
{
public:
	TSharedPtr() {}
	TSharedPtr(T* ptr) { Set(ptr); }

	//복사
	TSharedPtr(const TSharedPtr& other) { Set(other._ptr); }
	//이동
	TSharedPtr(TSharedPtr&& other)noexcept { _ptr = other._ptr; other._ptr = nullptr }

	//상속 관계 복사
	template<typename U>
	TSharedPtr(const TSharedPtr<U>& rhs) { Set(static_cast<T*>(rhs._ptr)); }


	~TSharedPtr() { Release(); }
public:
	TSharedPtr& operator=(const TSharedPtr& rhs) {
		if (_ptr != rhs._ptr) {
			Release();
			Set(rhs._ptr);
		}
		return *this;
	}


	TSharedPtr& operator=(TSharedPtr&& rhs) {
		Release();
		_ptr = rhs._ptr;
		rhs._ptr = nullptr;
		return *this;
	}

	bool operator==(const TSharedPtr& rhs)const { return _ptr == rhs._ptr; }
	bool operator ==(T* ptr)const { return _ptr == ptr; }
	bool operator!=(const TSharedPtr& rhs)const { return _ptr != rhs._ptr; }
	bool operator!=(T* ptr)const { return _ptr != ptr; }
	bool operator<(const TSharedPtr& rhs)const { return _ptr < rhs._ptr; }
	//????// T* operator*() { return _ptr; }
	//????// const T* operator*()const { return _ptr; }
	operator T* () const { return _ptr; }
	T* operator->() { return _ptr; }
	const T* operator->()const { return _ptr; }

	bool IsNull() { return _ptr == nullptr; }
private:
	void Set(T* ptr) {
		_ptr = ptr;
		if (ptr)
			ptr->AddRef();
	}

	void Release()
	{
		if (_ptr != nullptr) {
			_ptr->ReleaseRef();
			_ptr = nullptr;
		}
	}
protected:
	T* _ptr = nullptr;
};

class Knight : public RefCountable
{
public:

};

map<int, Knight*> _k;
using KnightRef = TSharedPtr<Knight>;

void Test(Knight* knight) {
	_k[100] = knight;
	knight->AddRef();
}


int main() {

	KnightRef knight(new Knight);
	knight->ReleaseRef();// 생성자에서 카운트 1시작시 2로 증가되어 임의로 1낮춰 1-based로 관리
	//표준은 알아서 1부터 시작하도록관리
//스마트 포인터 사용 시 복사 등으로 refcount가 알아서 관리가 RAII원칙에 의해 되므로
// 스레드 세이프하다.

	 TSharedPtr<Knight>& l=knight;
		//->형태는 refCount를 늘리지않으므로 멤버 변수로 가지면 위험
		// 또한 함수에서 받을 때 임시로 사용 시엔 유용
	 //함수에서 매개변수로 const shared_ptr<T>&로 받으면, 읽기전용으로 받아 참조횟수
	 // 관리가 불필요할 때 + 원자적 연산 감소(추적안하므로)
	 // 또한 해당 함수에서 shared_ptr의 소유권을 요구하지 않는다는 의미 내포

	
}