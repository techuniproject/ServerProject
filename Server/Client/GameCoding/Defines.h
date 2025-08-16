#pragma once

//싱글톤 -> Meyers Singleton (참조 반환)
//전역으로 관리하는게 메모리 삭제 위험 없음 스마트포인터로 관리(힙)에 비해

#define NO_COPY_MOVE(ClassName)					\
public:											\
	ClassName (const ClassName&) = delete;		\
	ClassName& operator=(const ClassName&) = delete;  \
	ClassName (ClassName&&) = delete;	    \
	ClassName& operator=(ClassName&&) = delete;


#define DECLARE_SINGLE(ClassName)			\
private:									\
	ClassName() = default;					\
	NO_COPY_MOVE(ClassName)					\
public:										\
	static ClassName* GetInstance()			\
	{										\
		static ClassName s_instance;		\
		return &s_instance;					\
	}


#define DECLARE_DEFAULT_DESTRUCTOR(ClassName) \
public:										\
	~ClassName();

#define DEFINE_DEFAULT_DESTRUCTOR(ClassName)\
	ClassName::~ClassName()=default;

#define GET_SINGLE(ClassName)	ClassName::GetInstance()

#define SAFE_DELETE(ptr)		\
	if (ptr)					\
	{							\
		delete ptr;				\
		ptr = nullptr;			\
	}