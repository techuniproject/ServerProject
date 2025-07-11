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
			delete this; // virtual �Ҹ����̹Ƿ� �ش� ������ Knight*�� ����Ű�� Knight �Ҹ��� ���� ȣ��Ǿ� ��ü ��ü �ı�
		return refCount;
	}
protected:
	atomic<int> _refCount=1; // ���� ������ �� ������ ������ ���� atomic���� ���
};


template<typename T>
class TSharedPtr
{
public:
	TSharedPtr(){}
	TSharedPtr(T* ptr) { Set(ptr); }

	//����
	TSharedPtr(const TSharedPtr& other) { Set(other._ptr); }
	//�̵�
	TSharedPtr(TSharedPtr&& other)noexcept { _ptr = other._ptr; other._ptr=nullptr }

	//��� ���� ����
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
	knight->ReleaseRef();// �����ڿ��� ī��Ʈ 1���۽� 2�� �����Ǿ� ���Ƿ� 1���� 1-based�� ����
							//ǥ���� �˾Ƽ� 1���� �����ϵ��ϰ���
	//����Ʈ ������ ��� �� ���� ������ refcount�� �˾Ƽ� ������ RAII��Ģ�� ���� �ǹǷ�
	// ������ �������ϴ�.


}