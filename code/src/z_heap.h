#ifndef _Z_HUNK_
#define _Z_HUNK_

#pragma once

#ifndef Q3_VM

void Memory_Init();
void Memory_Shutdown();

enum {
    h_low,
    h_high
};

enum
{
	TAG_FREE       = 0,
	TAG_STATIC     = 1,
	TAG_LEVEL      = 2,
	TAG_SFX        = 3,
	TAG_MUSIC      = 4,
	TAG_PURGELEVEL = 100,
	TAG_CACHE      = 101,
	
	NUMTAGS
};

#define TAG_SCOPE TAG_CACHE
#define TAG_LOAD TAG_CACHE

typedef struct
{
    unsigned id;
    char name[14];
    uint32_t size;
} hunk_t;
typedef struct memblock_s
{
    uint32_t size;
    void **user;
    int tag;
    int id;
    char name[14];
    struct memblock_s* next;
    struct memblock_s* prev;
} memblock_t;

typedef struct memzone_s
{
	uint64_t size;
    
	// start/end cap for linked list
	memblock_t blocklist;

	// the roving block for general operations
	memblock_t* rover;
} memzone_t;

class Heap
{
private:
	static void* Hunk_AllocHigh(uint32_t size, const char *name);
	static void* Hunk_AllocLow(uint32_t size, const char *name);
	static void Z_MergeNB(memblock_t *block);
	static void Z_MergePB(memblock_t *block);
	static void Z_ScanForBlock(void *start, void *end);

	static Heap heap;
public:
	Heap() = default;
	~Heap();

	friend void Memory_Init();
	friend void Memory_Shutdown();

	static void* Hunk_Alloc(uint32_t size, const char *name, int where);
	static void Hunk_Check(void);
	static void Hunk_Print(void);
	static void Hunk_FreeToLowMark(uint64_t mark);
	static void Hunk_FreeToHighMark(uint64_t mark);
	static void Hunk_Clear(void);
	static void* Hunk_TempAlloc(uint32_t size);
	static uint64_t Hunk_LowMark(void);
	static uint64_t Hunk_HighMark(void);

	static void* Z_AlignedAlloc(uint32_t alignment, uint32_t size, int tag, void *user, const char* name);
	static void* Z_Malloc(uint32_t size, int tag, void *user, const char* name);
	static void* Z_Calloc(void *user, uint32_t size, int tag, const char* name);
	static void* Z_Realloc(void *ptr, uint32_t nsize, void *user, int tag, const char* name);

	static void Z_Free(void *ptr);
	static void Z_FreeTags(int lowtag, int hightag);
	static void Z_ChangeTag(void* user, int tag);
	static void Z_ChangeUser(void* olduser, void* newuser);
	static void Z_ChangeName(void* user, const char* name);
	static void Z_CleanCache(void);
	static void Z_CheckHeap(void);
	static void Z_ClearZone(void);
	static void Z_Print(bool all);
	static void Z_Init(void);
	static uint64_t Z_FreeMemory(void);
	static void* Z_ZoneBegin(void);
	static void* Z_ZoneEnd(void);
	static uint32_t Z_NumBlocks(int tag);

	static void Mem_Info(void);

	GDR_INLINE static Heap& Get(void) { return heap; }
};

#define Hunk_HighAlloc Heap::Hunk_HighAlloc
#define Hunk_LowAlloc Heap::Hunk_LowAlloc
#define Hunk_Alloc Heap::Hunk_Alloc
#define Hunk_Check Heap::Hunk_Check
#define Hunk_Print Heap::Hunk_Print
#define Hunk_FreeToLowMark Heap::Hunk_FreeToLowMark
#define Hunk_FreeToHighMark Heap::Hunk_FreeToHighMark
#define Hunk_Clear Heap::Hunk_Clear
#define Hunk_TempAlloc Heap::Hunk_TempAlloc
#define Hunk_LowMark Heap::Hunk_LowMark
#define Hunk_highmark Heap::Hunk_HighMark

#define Z_ScanForBlock Heap::Z_ScanForBlock
#define Z_MergePB Heap::Z_MergePB
#define Z_MergeNB Heap::Z_MergeNB
#define Z_AlignedAlloc Heap::Z_AlignedAlloc
#define Z_Malloc Heap::Z_Malloc
#define Z_Calloc Heap::Z_Calloc
#define Z_Realloc Heap::Z_Realloc
#define Z_Free Heap::Z_Free
#define Z_FreeTags Heap::Z_FreeTags
#define Z_ChangeTag Heap::Z_ChangeTag
#define Z_ChangeUser Heap::Z_ChangeUser
#define Z_ChangeName Heap::Z_ChangeName
#define Z_ClearCache Heap::Z_ClearCache
#define Z_CheckHeap Heap::Z_CheckHeap
#define Z_ClearZone Heap::Z_ClearZone
#define Z_Print Heap::Z_Print
#define Z_FreeMemory Heap::Z_FreeMemory
#define Z_ZoneBegin Heap::Z_ZoneBegin
#define Z_ZoneEnd Heap::Z_ZoneEnd
#define Z_NumBlocks Heap::Z_NumBlocks
#define Mem_Info Heap::Mem_Info

template<class T>
struct nomad_allocator
{
	nomad_allocator() noexcept { }
	nomad_allocator(const char* name = "zallocator") noexcept { }

	typedef T value_type;
	template<class U>
	constexpr nomad_allocator(const nomad_allocator<U>&) noexcept { }

	constexpr GDR_INLINE bool operator!=(const eastl::allocator) { return true; }
	constexpr GDR_INLINE bool operator!=(const nomad_allocator) { return false; }
	constexpr GDR_INLINE bool operator==(const eastl::allocator) { return false; }
	constexpr GDR_INLINE bool operator==(const nomad_allocator) { return true; }

	GDR_INLINE void* allocate(size_t n) const {
		return Z_Malloc(n, TAG_STATIC, NULL, "zallocator");
	}
	GDR_INLINE void* allocate(size_t& n, size_t& alignment, size_t& offset) const {
		return Z_Malloc(n, TAG_STATIC, NULL, "zallocator");
	}
	GDR_INLINE void* allocate(size_t n, size_t alignment, size_t alignmentOffset, int flags) const {
		return Z_Malloc(n, TAG_STATIC, NULL, "zallocator");
	}
	GDR_INLINE void deallocate(void *p, size_t) const noexcept {
		Z_ChangeTag(p, TAG_PURGELEVEL);
	}
};

typedef struct
{
	int num;
	int minSize;
	int maxSize;
	int totalSize;
} memoryStats_t;

void Mem_Init( void );
void Mem_Shutdown( void );
void Mem_EnableLeakTest( const char *name );
void Mem_ClearFrameStats( void );
void Mem_GetFrameStats( memoryStats_t& allocs, memoryStats_t& frees );
void Mem_GetStats( memoryStats_t& stats );
void Mem_AllocDefragBlock( void );

#ifndef DEBUG_MEMORY

void* Mem_Alloc(const uint32_t size);
void* Mem_ClearedAlloc(const uint32_t size);
void Mem_Free(void *ptr);
char* Mem_CopyString(const char *in);
void* Mem_Alloc16(const uint32_t size);
void Mem_Free16(void *ptr);

#else

void* Mem_Alloc( const uint32_t size, const char *fileName, const int lineNumber );
void* Mem_ClearedAlloc( const uint32_t size, const char *fileName, const int lineNumber );
void Mem_Free( void *ptr, const char *fileName, const int lineNumber );
char* Mem_CopyString( const char *in, const char *fileName, const int lineNumber );
void* Mem_Alloc16( const uint32_t size, const char *fileName, const int lineNumber );
void Mem_Free16( void *ptr, const char *fileName, const int lineNumber );

#define Mem_Alloc(size) Mem_Alloc(size, __FILE__, __LINE__)
#define Mem_ClearedAlloc(size) Mem_ClearedAlloc(size, __FILE__, __LINE__)
#define Mem_Free(ptr) Mem_Free(ptr, __FILE__, __LINE__)
#define Mem_CopyString(s) Mem_CopyString(s, __FILE__, __LINE__)
#define Mem_Alloc16(size) Mem_Alloc16(size, __FILE__, __LINE__)
#define Mem_Free16(ptr) Mem_Free16(ptr, __FILE__, __LINE__)

#endif

template<class T>
struct id_allocator
{
	id_allocator() noexcept { }
	id_allocator(const char* name = "zallocator") noexcept { }

	typedef T value_type;
	template<class U>
	constexpr id_allocator(const id_allocator<U>&) noexcept { }

	constexpr GDR_INLINE bool operator!=(const eastl::allocator) { return true; }
	constexpr GDR_INLINE bool operator!=(const id_allocator) { return false; }
	constexpr GDR_INLINE bool operator==(const eastl::allocator) { return false; }
	constexpr GDR_INLINE bool operator==(const id_allocator) { return true; }

	GDR_INLINE void* allocate(size_t n) const {
		return Mem_Alloc(n);
	}
	GDR_INLINE void* allocate(size_t& n, size_t& alignment, size_t& offset) const {
		n = (n + (alignment - 1)) & ~(alignment - 1);
		return Mem_Alloc(n);
	}
	GDR_INLINE void* allocate(size_t n, size_t alignment, size_t alignmentOffset, int flags) const {
		n = (n + (alignment - 1)) & ~(alignment - 1);
		return Mem_Alloc(n);
	}
	GDR_INLINE void deallocate(void *p, size_t) const noexcept {
		Mem_Free(p);
	}
};


template<class type, uint32_t blockSize>
class BlockAlloc
{
public:
	BlockAlloc()
		: blocks(NULL), free(NULL), total(0), active(0) { }
	~BlockAlloc()
	{ Shutdown(); }

	void Shutdown(void)
	{
		while(blocks) {
			block_t *block = blocks;
			blocks = blocks->next;
			delete block;
		}
		blocks = NULL;
		free = NULL;
		total = active = 0;
	}
	
	type* Alloc(void)
	{
		if (!free) {
			block_t *block = (block_t *)Mem_Alloc(sizeof(block_t));
			block->next = blocks;
			blocks = block;
			for (uint32_t i = 0; i < blockSize; i++) {
				block->elements[i].next = free;
				free = &block->elements[i];
			}
			total += blockSize;
		}
		active++;
		element_t *element = free;
		free = free->next;
		element->next = NULL;
		return &element->t;
	}
	void Free(type *t)
	{
		element_t *element = (element_t *)t;
		element->next = free;
		free = element;
		active--;
	}

	GDR_INLINE uint32_t GetTotalCount(void) const { return total; }
	GDR_INLINE uint32_t GetAllocCount(void) const { return active; }
	GDR_INLINE uint32_t GetFreeCount(void) const { return total - active; }
private:
	typedef struct element_s
	{
		type t;
		struct element_s* next;
	} element_t;

	typedef struct block_s
	{
		element_t elements[blockSize];
		struct block_s* next;
	} block_t;

	block_t* blocks;
	element_t* free;
	uint32_t total;
	uint32_t active;
};

#if 0
template<typename T>
class linked_list
{
public:
	class linked_list_node
	{
	public:
		T val;
		linked_list_node *next;
		linked_list_node *prev;
	public:
		linked_list_node() = default;
		linked_list_node(const linked_list_node &) = delete;
		linked_list_node(linked_list_node &&) = default;
		~linked_list_node() = default;
		inline linked_list_node& operator++(int) noexcept
		{
			next->next->prev = this;
			next->prev = prev;
			next->next = this;
			prev->next = next;
			return *this;
		}
		inline linked_list_node& operator++() noexcept
		{
			next->next->prev = this;
			next->prev = prev;
			next->next = this;
			prev->next = next;
			return *this;
		}
	};
	
	typedef linked_list_node* node;
	typedef linked_list_node list_node;
	typedef linked_list_node* iterator;
	typedef const linked_list_node* const_iterator;
	typedef std::size_t size_type;
	typedef T value_type;
private:
	linked_list<T>::node ptr_list = NULL;
	std::size_t _size = 0;
	
	linked_list<T>::node alloc_node(void) {
		linked_list<T>::node ptr = (linked_list<T>::node)Z_Malloc(
			sizeof(linked_list<T>::list_node), TAG_STATIC, &ptr, "listnode");
		if (ptr == NULL)
			N_Error("linked_list::alloc_node: memory allocation failed");
		
		return ptr;
	}
	void dealloc_node(linked_list<T>::node ptr) noexcept {
		if (ptr == NULL)
			return;
		Z_Free((void *)ptr);
	}
public:
	linked_list() noexcept = default;
	~linked_list() noexcept
	{
		if (ptr_list == NULL || begin() == NULL)
			return;

		for (linked_list<T>::iterator it = begin(); it != end(); it = it->next)
			dealloc_node(it);
	}
	linked_list(linked_list &&) = default;
	linked_list(const linked_list &) = default;
	
	// memory management
	inline void clear(void) noexcept
	{
		if (ptr_list == NULL)
			return;
		
		for (linked_list<T>::iterator it = begin(); it != end(); it = it->next)
			dealloc_node(it);
	}
	inline void free_node(linked_list<T>::node ptr) noexcept
	{
		if (ptr == NULL)
			return;
		
		ptr->prev->next = ptr->next;
		ptr->next->prev = ptr->prev;
		dealloc_node(ptr);
		--_size;
	}
	inline void pop_back() noexcept
	{
		if (ptr_list == NULL)
			return;
		
		linked_list<T>::node endptr = back_node();
		endptr->prev->next = NULL;
		dealloc_node(endptr);
		--_size;
	}
	inline void erase(linked_list<T>::node ptr) noexcept
	{
		if (ptr == NULL || ptr_list == NULL)
			return;
		
		if (ptr == front_node())
			pop_front();
		else if (ptr == back_node())
			pop_back();
		else
			free_node(ptr);
	}
	inline void pop_front() noexcept
	{
		linked_list<T>::node frontptr = front_node();
		if (frontptr->next != NULL) {
			frontptr->next->prev = NULL;
			frontptr = frontptr->next;
			ptr_list = frontptr;
		}
		dealloc_node(ptr_list);
		--_size;
	}
	inline linked_list<T>::node push_back(void) noexcept
	{
		linked_list<T>::node ptr;
		if (ptr_list == NULL) {
			ptr_list = alloc_node();
			ptr = ptr_list;
			ptr_list->prev = NULL;
		}
		else {
			ptr = alloc_node();
			linked_list<T>::node endptr = back_node();
			endptr->next = ptr;
			ptr->prev = endptr;
		}
		ptr->next = NULL;
		++_size;
		return ptr;
	}
	inline linked_list<T>::node emplace_back(void) noexcept
	{ return push_back(); }
	
	// random algos
	inline void swap(linked_list& list)
	{
		if (list.ptr_list == NULL || ptr_list == NULL)
			return;
	}
	inline void swap_nodes(linked_list<T>::iterator iter1, linked_list<T>::iterator iter2) noexcept
	{
		if (ptr_list == NULL || iter1 == NULL || iter2 == NULL)
			return;
		
		if (iter1 == front_node())
			iter1->next->prev = iter2;
		else if (iter1 == back_node())
			iter1->prev->next = iter2;
		else {
			iter1->prev->next = iter2;
			iter1->next->prev = iter2;
		}
		
		if (iter2 == front_node())
			iter2->next->prev = iter1;
		else if (iter2 == back_node())
			iter2->prev->next = iter1;
		else {
			iter2->prev->next = iter1;
			iter2->next->prev = iter1;
		}
	}
	linked_list<T>::node find_node(linked_list<T>::iterator begin_it, std::size_t n,
		const T &val) noexcept
	{
		if (ptr_list == NULL || begin_it == NULL || n == 0)
			return NULL;
		
		for (linked_list<T>::iterator it = begin_it; --n; it = it->next) {
			if (it->val == val) return it;
		}
		return ptr_list;
	}
	inline void erase_n(linked_list<T>::iterator begin_it, std::size_t n) noexcept
	{
		if (begin_it == NULL || ptr_list == NULL || n == 0)
			return;
		
		for (linked_list<T>::iterator it = begin_it; --n; it = it->next)
			erase(it);
	}
	inline void erase_range(linked_list<T>::iterator begin_it, linked_list<T>::iterator end_it) noexcept
	{
		if (begin_it == NULL || end_it == NULL || ptr_list == NULL)
			return;
		
		for (linked_list<T>::iterator it = begin_it; it != end_it; it = it->next)
			erase(it);
	}

	// iterators/access
	inline T& operator[](std::size_t index) noexcept
	{
		if (ptr_list == NULL || index > _size) {
			Con_Printf("WARNING: ptr_list is NULL or index > _size in linked_list::operator[]");
			return 0;
		}
		
		linked_list<T>::iterator it = begin();
		while (--index && it != end())
			it = it->next;
		
		return it->val;
	}
	inline std::size_t size(void) const noexcept
	{ return _size; }
	inline linked_list<T>::iterator begin(void) {
		if (ptr_list == NULL)
			Con_Printf("WARNING: ptr_list is NULL in linked_list::begin");
		
		return ptr_list;
	}
	inline linked_list<T>::iterator end(void) {
		if (ptr_list == NULL) {
			Con_Printf("WARNING: ptr_list is NULL in linked_list::end");
			return NULL;
		}
		
		linked_list<T>::iterator _endptr;
		for (_endptr = begin();; _endptr = _endptr->next) {
			if (_endptr == NULL) break;
		}
		return _endptr;
	}
	inline linked_list<T>::node back_node(void) {
		if (ptr_list == NULL) {
			Con_Printf("WARNING: ptr_list is NULL in linked_list::back_node");
			return NULL;
		}
		linked_list<T>::node _endptr;
		for (_endptr = begin();; _endptr = _endptr->next) {
			if (_endptr->next == NULL) break;
		}
		return _endptr;
	}
	inline linked_list<T>::node front_node(void) {
		if (ptr_list == NULL)
			Con_Printf("WARNING: ptr_list is NULL in linked_list::front_node");
		
		return ptr_list;
	}
	inline T& back(void) noexcept {
		if (ptr_list == NULL)
			Con_Printf("WARNING: ptr_list is NULL in linked_list::back");
		
		return back_node()->val;
    }
	inline T& front(void) noexcept {
		if (ptr_list == NULL)
			Con_Printf("WARNING: ptr_list is NULL in linked_list::front");

		return front_node()->val;
	}
};
#endif

#endif

#endif