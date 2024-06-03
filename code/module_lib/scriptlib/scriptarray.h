#ifndef __SCRIPTARRAY_H__
#define __SCRIPTARRAY_H__

#include "../module_public.h"
#include "../../engine/n_threads.h"

struct SArrayBuffer {
	asDWORD size;
	asDWORD capacity;
	byte data[1];
};

struct SArrayCache {
	asIScriptFunction *cmpFunc;
	asIScriptFunction *eqFunc;
	int cmpFuncReturnCode; // To allow better error message in case of multiple matches
	int eqFuncReturnCode;
};

class CScriptArray
{
public:
	// Set the memory functions that should be used by all CScriptArrays
	static void SetMemoryFunctions( asALLOCFUNC_t allocFunc, asFREEFUNC_t freeFunc );

	// Factory functions
	static CScriptArray *Create( asITypeInfo *ot );
	static CScriptArray *Create( asITypeInfo *ot, asUINT length );
	static CScriptArray *Create( asITypeInfo *ot, asUINT length, void *defaultValue );
	static CScriptArray *Create( asITypeInfo *ot, void *listBuffer );

	// Memory management
	void AddRef( void ) const;
	void Release( void ) const;

	// Type information
	asITypeInfo *GetArrayObjectType() const;
	int          GetArrayTypeId() const;
	int          GetElementTypeId() const;

	// Get the current size
	asUINT GetSize( void ) const;

	// Returns true if the array is empty
	bool   IsEmpty( void ) const;

	// Pre-allocates memory for elements
	void   Reserve( asUINT maxElements );

	// Resize the array
	void   Resize( asUINT numElements );

	// Get a pointer to an element. Returns 0 if out of bounds
	void       *At( asUINT index );
	const void *At( asUINT index ) const;

	// Set value of an element. 
	// The value arg should be a pointer to the value that will be copied to the element.
	// Remember, if the array holds handles the value parameter should be the 
	// address of the handle. The refCount of the object will also be incremented
	void  SetValue( asUINT index, void *value );

	// Copy the contents of one array to another (only if the types are the same)
	CScriptArray& operator=( const CScriptArray& );

	// Compare two arrays
	bool operator==( const CScriptArray& ) const;

	// Array manipulation
	void InsertAt( asUINT index, void *value );
	void InsertAt( asUINT index, const CScriptArray& arr );
	void InsertLast( void *value );
	void RemoveAt( asUINT index );
	void RemoveLast( void );
	void RemoveRange( asUINT start, asUINT count );
	void SortAsc( void );
	void SortDesc( void );
	void SortAsc( asUINT startAt, asUINT count );
	void SortDesc( asUINT startAt, asUINT count );
	void Sort( asUINT startAt, asUINT count, bool asc );
	void Sort( asIScriptFunction *less, asUINT startAt, asUINT count );
	void Reverse( void );
	int Find( void *value ) const;
	int Find( asUINT startAt, void *value ) const;
	int FindByRef( void *ref ) const;
	int FindByRef( asUINT startAt, void *ref ) const;

	// Return the address of internal buffer for direct manipulation of elements
	void *GetBuffer( void );
	const void *GetBuffer( void ) const;

	void Clear( void );

	// GC methods
	int  GetRefCount( void );
	void SetFlag( void );
	bool GetFlag( void );
	void EnumReferences( asIScriptEngine *pEngine );
	void ReleaseAllHandles( asIScriptEngine *pEngine );
protected:
	mutable CThreadAtomic<int32_t> refCount;
	mutable bool    gcFlag;
	asITypeInfo    *objType;
	asIScriptFunction *subTypeHandleAssignFunc;
	uint32_t         elementSize;
	int32_t         subTypeId;
//	idDynamicAlloc<byte, 4096, 1024> arrayCacheAllocator;

	SArrayBuffer *buffer;

	// Constructors
	CScriptArray( asITypeInfo *ot, void *initBuf ); // Called from script when initialized with list
	CScriptArray( asUINT length, asITypeInfo *ot );
	CScriptArray( asUINT length, void *defVal, asITypeInfo *ot );
	CScriptArray( const CScriptArray& other );
	virtual ~CScriptArray();

	void DeleteBuffer( SArrayBuffer *buffer );

	void AllocBuffer( uint32_t nItems );
	void DoAllocate( int delta, uint32_t at );
	bool  Less( const void *a, const void *b, bool asc );
	void *GetArrayItemPointer(int index );
	void *GetDataPointer(void *buffer );
	void  Copy( void *dst, void *src );
	void  Swap( void *a, void *b );
	void  Precache( void );
	bool  CheckMaxSize( asUINT numElements );
	void  CreateBuffer( SArrayBuffer **buffer, asUINT numElements );
	void  CopyBuffer( SArrayBuffer *dst, SArrayBuffer *src );
	void  Construct( SArrayBuffer *buf, asUINT start, asUINT end );
	void  Destruct( SArrayBuffer *buf, asUINT start, asUINT end );
	bool  Equals( const void *a, const void *b, asIScriptContext *ctx, SArrayCache *cache ) const;
};

void RegisterScriptArray(asIScriptEngine *engine);

#endif
