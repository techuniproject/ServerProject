#pragma once

//�̱��� -> Meyers Singleton (���� ��ȯ)
//�������� �����ϴ°� �޸� ���� ���� ���� ����Ʈ�����ͷ� ����(��)�� ����

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