#pragma once

/*----------------
	IocpObject
-----------------*/

/*
shared_ptr은 힙메모리를 관리하는 객체
내부적으로 control block을 관리하는데, 구성은 refcount, weakcount, T의 소멸자이다.
refcount가 0이되면 소멸자를 호출하게 됨.
스택메모리를 shared_ptr로 호출하는 것은 말이안됨, 스코프 벗어나면 스택 메모리는 반환되기 때문에 없는 메모리를 없애려고 시도함. 

weak_ptr은 객체의 참조를 가지는 것.
실제 refcount를 올리진않지만, weak_count를 올리게 된다.
weak_ptr로 객체를 참조하고 있는 상황에서 그 객체의 refcount가 0이 되면 해당 객체가 사라지지만,
weak_count는 0이 아니기 때문에 control block은 사라지지 않는다. 그로 인해 control block에서 관리하는
.lock() 함수를 사용하여 해당 객체의 생명주기 여부를 물어볼 수 있게 된다.
.lock() 호출시 참조 객체가 유효하면(refcount가 0이 아니라면) shared_ptr<T>형태로 반환, 유효 안하면 nullptr반환
.expired() 시 소멸 여부 boolean으로 반환
.use_count() 는 현재 살아있는 shared_ptr개수 반환

control block 소멸 조건 -> refcount==0 && weakcount==0
객체 소멸조건 -> refcount==0

*/


// 자신에 대한 shared_ptr을 shared_ptr<T> self(this) 형태로 관리 시,
// 외부에서 이 객체에 대해 이미 shared_ptr로 관리된 경우 이중 삭제 위험 있음. (refcount 두개로 관리됨)ㄴ
//enable_shared_from_this는 이 객체를 관리하는 기존 shared_ptr을 기반으로 shared_ptr만들어줌
//-> 하나의 refcount로 관리되게끔 함.
// 내부적으로는 enabled_shared_from_this는 weak_ptr<T> weak_this 멤버를 가지며,
// 이걸 상속받은 객체가 처음으로 shared_ptr로 생성시, weak_this.lock()을 호출하여 동일한 제어 블록 가지는 shared_ptr생성
// 그렇기 때문에 new로 만든 객체인데 이걸 상속받으면 예외가 생기고, 반드시 shared_ptr로 만들어야함.

class IocpObject : public enable_shared_from_this<IocpObject>
{
public:
	virtual HANDLE GetHandle() abstract;
	virtual void Dispatch(struct IocpEvent* iocpEvent, int32 numOfBytes = 0) abstract;
};

/*--------------
	IocpCore
---------------*/

class IocpCore
{
public:
	IocpCore();
	~IocpCore();

	HANDLE		GetHandle() { return _iocpHandle; }

	bool		Register(IocpObjectRef iocpObject);
	bool		Dispatch(uint32 timeoutMs = INFINITE);

private:
	HANDLE		_iocpHandle;
};