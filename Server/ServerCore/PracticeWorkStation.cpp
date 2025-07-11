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
	atomic<int> _refCount=1; // 증감 연산자 등 원자적 연산을 위해 atomic으로 사용
};


template<typename T>
class TSharedPtr
{
public:
	TSharedPtr(){}
	TSharedPtr(T* ptr) { Set(ptr); }

	//복사
	TSharedPtr(const TSharedPtr& other) { Set(other._ptr); }
	//이동
	TSharedPtr(TSharedPtr&& other)noexcept { _ptr = other._ptr; other._ptr=nullptr }

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
	operator T* () const { return _ptr;}
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


}