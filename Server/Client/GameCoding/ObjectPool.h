#pragma once

// ============================================================
// [1] MemoryPool  : 크기별 slab 할당자 (힙 단편화 방지)
//     오브젝트의 raw 메모리를 공급하는 역할
// ============================================================
class MemoryPool
{
public:
	MemoryPool(size_t objsize, size_t chunckcnt) :
		objectSize(objsize), chunckCnt(chunckcnt) {
		assert(objectSize >= sizeof(void*));
		Expand();
	}

	~MemoryPool()
	{
		for (void* block : chuncks) {
			::operator delete(block);
		}
	}

	void* Allocate()
	{
		if (freelist == nullptr) {
			Expand();
		}

		void* ptr = freelist;
		freelist = *reinterpret_cast<void**>(ptr);
		return ptr;
	}

	void Free(void* ptr)
	{
		*reinterpret_cast<void**>(ptr) = freelist;
		freelist = ptr;
	}

private:
	void Expand() {
		size_t blockSize = objectSize * chunckCnt;
		void* block = ::operator new(blockSize);
		chuncks.push_back(block);

		char* start = static_cast<char*>(block);
		for (size_t i = 0; i < chunckCnt; ++i) {
			void* current = start + (i * objectSize);
			*reinterpret_cast<void**>(current) = freelist;
			freelist = current;
		}
	}

private:
	void* freelist = nullptr;
	vector<void*> chuncks;
	size_t objectSize;
	size_t chunckCnt;
};

// ============================================================
// [2] PoolManager : 크기별 MemoryPool 묶음 (중앙 메모리 관리자)
// ============================================================
class PoolManager
{
	struct MemoryHeader
	{
		size_t allocSize;
	};

public:
	static PoolManager& Instance() {
		static PoolManager instance;
		return instance;
	}

	void* Allocate(size_t size)
	{
		size_t totalSize = size + sizeof(MemoryHeader);

		void* ptr = nullptr;
		size_t allocSize = 0;

		if (totalSize <= 32)		{ ptr = _pool32.Allocate();  allocSize = 32;  }
		else if (totalSize <= 64)	{ ptr = _pool64.Allocate();  allocSize = 64;  }
		else if (totalSize <= 128)	{ ptr = _pool128.Allocate(); allocSize = 128; }
		else if (totalSize <= 256)	{ ptr = _pool256.Allocate(); allocSize = 256; }
		else if (totalSize <= 512)	{ ptr = _pool512.Allocate(); allocSize = 512; }
		else
		{
			ptr = ::operator new(totalSize);
			allocSize = totalSize;
		}

		MemoryHeader* header = static_cast<MemoryHeader*>(ptr);
		header->allocSize = allocSize;
		return static_cast<char*>(ptr) + sizeof(MemoryHeader);
	}

	void Deallocate(void* ptr)
	{
		char* headerPos = static_cast<char*>(ptr) - sizeof(MemoryHeader);
		MemoryHeader* header = reinterpret_cast<MemoryHeader*>(headerPos);
		size_t allocSize = header->allocSize;

		if (allocSize == 32)		_pool32.Free(headerPos);
		else if (allocSize == 64)	_pool64.Free(headerPos);
		else if (allocSize == 128)	_pool128.Free(headerPos);
		else if (allocSize == 256)	_pool256.Free(headerPos);
		else if (allocSize == 512)	_pool512.Free(headerPos);
		else						::operator delete(headerPos);
	}

private:
	MemoryPool _pool32	{ 32,  2000 };
	MemoryPool _pool64	{ 64,  1000 };
	MemoryPool _pool128	{ 128, 500  };
	MemoryPool _pool256	{ 256, 100  };
	MemoryPool _pool512	{ 512, 50   };
};

// ============================================================
// [3] ObjectPool<T> : 진짜 오브젝트 풀
//
//  기존 방식 (Memory Pool만):
//    Pop()  → MemoryPool 할당 + ctor 호출
//    dtor   → 소멸자 호출 + MemoryPool 반납
//    → 매번 ctor/dtor 비용 발생
//
//  개선된 방식 (True Object Pool):
//    Warmup() → ctor 1회 호출 후 freeList 보관
//    Pop()    → freeList에서 꺼내 PoolReset() (가벼운 상태 초기화만)
//    Return() → 소멸자 호출 없이 freeList에 반납
//    → ctor/dtor는 오브젝트 생명주기 동안 딱 1번만 호출
// ============================================================
template<typename T>
class ObjectPool
{
public:
	// DevScene::Init()에서 리소스 로딩 완료 후 호출
	// Monster/Player/Arrow ctor가 GameInstance 의존하므로
	// Warmup은 반드시 해당 Flipbook 로딩 이후에 호출할 것
	static void Warmup(size_t count)
	{
		for (size_t i = 0; i < count; ++i)
		{
			void* mem = PoolManager::Instance().Allocate(sizeof(T));
			_freeList.push_back(new(mem) T()); // ctor 1회 호출
		}
	}

	// freeList에서 오브젝트를 꺼낸다
	// freeList가 비어있으면 새로 생성 (ctor 1회)
	// shared_ptr deleter가 Return()을 호출 → dtor 미호출
	static shared_ptr<T> Pop()
	{
		T* obj = nullptr;

		if (!_freeList.empty())
		{
			obj = _freeList.back();
			_freeList.pop_back();
			obj->PoolReset(); // 소멸자 없이 상태만 초기화
		}
		else
		{
			// 풀 소진 시 새 오브젝트 생성
			void* mem = PoolManager::Instance().Allocate(sizeof(T));
			obj = new(mem) T();
		}

		// 소멸 시 Return() 호출 (dtor 없이 freeList 반납)
		return shared_ptr<T>(obj, [](T* p)
			{
				ObjectPool<T>::Return(p);
			});
	}

	// freeList에 반납 (소멸자 호출 없음)
	static void Return(T* p)
	{
		_freeList.push_back(p);
	}

	static size_t FreeCount() { return _freeList.size(); }

private:
	static vector<T*> _freeList;
};

template<typename T>
vector<T*> ObjectPool<T>::_freeList;
