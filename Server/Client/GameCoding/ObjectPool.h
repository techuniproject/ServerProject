#pragma once
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
			::operator delete(block);//소멸자 호출 x 메모리 반환만
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
		void* block = ::operator new(blockSize);//메모리 할당만, 생성자 호출x
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

// [2] 크기별 풀을 관리하는 중앙 매니저 (Singleton)
class PoolManager
{
	// 메모리 앞에 붙일 헤더 (반납할 때 크기를 알기 위해)
	struct MemoryHeader
	{
		size_t allocSize; // 내가 어느 풀 출신인지 기록
	};

public:
	static PoolManager& Instance() {
		static PoolManager instance;
		return instance;
	}

	// 크기에 맞는 풀을 찾아서 할당
	void* Allocate(size_t size)
	{
		// 헤더 크기만큼 더해서 할당해야 함
		size_t totalSize = size + sizeof(MemoryHeader);

		void* ptr = nullptr;
		size_t allocSize = 0;

		// 크기별 분기 (원하는 단위로 쪼개세요)
		if (totalSize <= 32) { ptr = _pool32.Allocate(); allocSize = 32; }
		else if (totalSize <= 64) { ptr = _pool64.Allocate(); allocSize = 64; }
		else if (totalSize <= 128) { ptr = _pool128.Allocate(); allocSize = 128; }
		else if (totalSize <= 256) { ptr = _pool256.Allocate(); allocSize = 256; }
		else
		{
			// 풀 범위를 벗어나면 그냥 시스템 할당 (Fallback)
			ptr = ::operator new(totalSize);
			allocSize = totalSize;
		}

		// 헤더에 크기 정보 기록
		MemoryHeader* header = static_cast<MemoryHeader*>(ptr);
		header->allocSize = allocSize;

		// 실제 유저가 쓸 메모리 주소는 헤더 뒤쪽
		return static_cast<char*>(ptr) + sizeof(MemoryHeader);
	}

	// 메모리 반납 (크기를 몰라도 헤더 보고 알아서 반납)
	void Deallocate(void* ptr)
	{
		// 유저 포인터에서 헤더 위치로 백트래킹
		char* headerPos = static_cast<char*>(ptr) - sizeof(MemoryHeader);
		MemoryHeader* header = reinterpret_cast<MemoryHeader*>(headerPos);
		size_t allocSize = header->allocSize;

		if (allocSize == 32)		_pool32.Free(headerPos);
		else if (allocSize == 64)	_pool64.Free(headerPos);
		else if (allocSize == 128)	_pool128.Free(headerPos);
		else if (allocSize == 256)	_pool256.Free(headerPos);
		else						::operator delete(headerPos);
	}

private:
	// 생성자에서 미리 할당하지 않고 Lazy Init 하거나, 여기서 사이즈 지정
	MemoryPool _pool32{ 32, 2000 };
	MemoryPool _pool64{ 64, 1000 };
	MemoryPool _pool128{ 128, 500 };
	MemoryPool _pool256{ 256, 100 };
};


template<typename T>
class ObjectPool
{
public:
	// shared_ptr 반환 (커스텀 삭제자 포함)
	static std::shared_ptr<T> Pop()
	{
		// 1. 메모리 공간 가져오기 (O(1))
		void* ptr = PoolManager::Instance().Allocate(sizeof(T));

		// 2. 생성자 호출 (Placement New)
		T* obj = new(ptr) T();

		// 3. shared_ptr 생성 시 '반납 로직(Deleter)' 등록
		return std::shared_ptr<T>(obj, [](T* p)
			{
				// 소멸자 호출 -> 메모리 반납
				p->~T();
				PoolManager::Instance().Deallocate(p);
			});
	}

};