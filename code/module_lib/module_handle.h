#ifndef __MODULE_HANDLE__
#define __MODULE_HANDLE__

#pragma once

#include "module_public.h"
#include "angelscript/angelscript.h"
#include "angelscript/as_compiler.h"
#include "angelscript/as_context.h"
#include "module_jit.h"

typedef uint64_t EModuleFuncId;

enum : uint64_t
{
    ModuleInit = 0,
    ModuleCommandLine,

    ModuleDrawConfiguration,
    ModuleSaveConfiguration,

    ModuleOnKeyEvent,
    ModuleOnMouseEvent,
    ModuleOnLevelStart,
    ModuleOnLevelEnd,
    ModuleOnRunTic,
    ModuleOnSaveGame,
    ModuleOnLoadGame,

    NumFuncs
};

class CModuleHandle
{
public:
    CModuleHandle( const char *pName, const UtlVector<UtlString>& sourceFiles,
		int32_t moduleVersionMajor, int32_t moduleVersionUpdate, int32_t moduleVersionPatch );
    ~CModuleHandle();

    void SaveToCache( void ) const;
	int LoadFromCache( void );

    void ClearMemory( void );
    asIScriptContext *GetContext( void );
    asIScriptModule *GetModule( void );
    const string_t& GetName( void ) const;

	const char *GetModulePath( void ) const;

    int CallFunc( EModuleFuncId nCallId, uint32_t nArgs, uint32_t *pArgList );

	inline void GetVersion( int32_t *major, int32_t *update, int32_t *patch ) const {
		*major = m_nVersionMajor;
		*update = m_nVersionUpdate;
		*patch = m_nVersionPatch;
	}
	inline bool IsValid( void ) const {
		return m_bLoaded;
	}
    inline asIScriptFunction *GetFunction( EModuleFuncId nCallId ) {
        return m_pFuncTable[nCallId];
    }
	const int32_t *VersionMajor( void ) const {
		return &m_nVersionMajor;
	}
	const int32_t *VersionUpdate( void ) const {
		return &m_nVersionUpdate;
	}
	const int32_t *VersionPatch( void ) const {
		return &m_nVersionPatch;
	}
private:
	void Build( const UtlVector<UtlString>& sourceFiles );
    bool InitCalls( void );
    void LoadSourceFile( const UtlString& filename );

    asIScriptFunction *m_pFuncTable[NumFuncs];

    string_t m_szName;
    asIScriptContext *m_pScriptContext;
    asIScriptModule *m_pScriptModule;
	qboolean m_bLoaded;

	int32_t m_nVersionMajor;
	int32_t m_nVersionUpdate;
	int32_t m_nVersionPatch;
};

class CModuleContextHandle : public asIScriptContext
{

public:
	// Memory management
	virtual int AddRef() const override;
	virtual int Release() const override;

	// Miscellaneous
	virtual asIScriptEngine *GetEngine() const = 0;

	// Execution
	virtual int             Prepare(asIScriptFunction *func) = 0;
	virtual int             Unprepare() = 0;
	virtual int             Execute() = 0;
	virtual int             Abort() = 0;
	virtual int             Suspend() = 0;
	virtual asEContextState GetState() const = 0;
	virtual int             PushState() = 0;
	virtual int             PopState() = 0;
	virtual bool            IsNested(asUINT *nestCount = 0) const = 0;

	// Object pointer for calling class methods
	virtual int   SetObject(void *obj) = 0;

	// Arguments
	virtual int   SetArgByte(asUINT arg, asBYTE value) = 0;
	virtual int   SetArgWord(asUINT arg, asWORD value) = 0;
	virtual int   SetArgDWord(asUINT arg, asDWORD value) = 0;
	virtual int   SetArgQWord(asUINT arg, asQWORD value) = 0;
	virtual int   SetArgFloat(asUINT arg, float value) = 0;
	virtual int   SetArgDouble(asUINT arg, double value) = 0;
	virtual int   SetArgAddress(asUINT arg, void *addr) = 0;
	virtual int   SetArgObject(asUINT arg, void *obj) = 0;
	virtual int   SetArgVarType(asUINT arg, void *ptr, int typeId) = 0;
	virtual void *GetAddressOfArg(asUINT arg) = 0;

	// Return value
	virtual asBYTE  GetReturnByte() = 0;
	virtual asWORD  GetReturnWord() = 0;
	virtual asDWORD GetReturnDWord() = 0;
	virtual asQWORD GetReturnQWord() = 0;
	virtual float   GetReturnFloat() = 0;
	virtual double  GetReturnDouble() = 0;
	virtual void   *GetReturnAddress() = 0;
	virtual void   *GetReturnObject() = 0;
	virtual void   *GetAddressOfReturnValue() = 0;

	// Exception handling
	virtual int                SetException(const char *info, bool allowCatch = true) = 0;
	virtual int                GetExceptionLineNumber(int *column = 0, const char **sectionName = 0) = 0;
	virtual asIScriptFunction *GetExceptionFunction() = 0;
	virtual const char *       GetExceptionString() = 0;
	virtual bool               WillExceptionBeCaught() = 0;
	virtual int                SetExceptionCallback(asSFuncPtr callback, void *obj, int callConv) = 0;
	virtual void               ClearExceptionCallback() = 0;

	// Debugging
	virtual int                SetLineCallback(asSFuncPtr callback, void *obj, int callConv) = 0;
	virtual void               ClearLineCallback() = 0;
	virtual asUINT             GetCallstackSize() const = 0;
	virtual asIScriptFunction *GetFunction(asUINT stackLevel = 0) = 0;
	virtual int                GetLineNumber(asUINT stackLevel = 0, int *column = 0, const char **sectionName = 0) = 0;
	virtual int                GetVarCount(asUINT stackLevel = 0) = 0;
	virtual int                GetVar(asUINT varIndex, asUINT stackLevel, const char** name, int* typeId = 0, asETypeModifiers* typeModifiers = 0, bool* isVarOnHeap = 0, int* stackOffset = 0) = 0;
#ifdef AS_DEPRECATED
	// deprecated since 2022-05-04, 2.36.0
	virtual const char        *GetVarName(asUINT varIndex, asUINT stackLevel = 0) = 0;
#endif
	virtual const char        *GetVarDeclaration(asUINT varIndex, asUINT stackLevel = 0, bool includeNamespace = false) = 0;
#ifdef AS_DEPRECATED
	// deprecated since 2022-05-04, 2.36.0
	virtual int                GetVarTypeId(asUINT varIndex, asUINT stackLevel = 0) = 0;
#endif
	virtual void              *GetAddressOfVar(asUINT varIndex, asUINT stackLevel = 0, bool dontDereference = false, bool returnAddressOfUnitializedObjects = false) = 0;
	virtual bool               IsVarInScope(asUINT varIndex, asUINT stackLevel = 0) = 0;
	virtual int                GetThisTypeId(asUINT stackLevel = 0) = 0;
	virtual void              *GetThisPointer(asUINT stackLevel = 0) = 0;
	virtual asIScriptFunction *GetSystemFunction() = 0;

	// User data
	virtual void *SetUserData(void *data, asPWORD type = 0) = 0;
	virtual void *GetUserData(asPWORD type = 0) const = 0;

	// Serialization
	virtual int StartDeserialization() = 0;
	virtual int FinishDeserialization() = 0;
	virtual int PushFunction(asIScriptFunction *func, void *object) = 0;
	virtual int GetStateRegisters(asUINT stackLevel, asIScriptFunction **callingSystemFunction, asIScriptFunction **initialFunction, asDWORD *origStackPointer, asDWORD *argumentsSize, asQWORD *valueRegister, void **objectRegister, asITypeInfo **objectTypeRegister) = 0;
	virtual int GetCallStateRegisters(asUINT stackLevel, asDWORD *stackFramePointer, asIScriptFunction **currentFunction, asDWORD *programPointer, asDWORD *stackPointer, asDWORD *stackIndex) = 0;
	virtual int SetStateRegisters(asUINT stackLevel, asIScriptFunction *callingSystemFunction, asIScriptFunction *initialFunction, asDWORD origStackPointer, asDWORD argumentsSize, asQWORD valueRegister, void *objectRegister, asITypeInfo *objectTypeRegister) = 0;
	virtual int SetCallStateRegisters(asUINT stackLevel, asDWORD stackFramePointer, asIScriptFunction *currentFunction, asDWORD programPointer, asDWORD stackPointer, asDWORD stackIndex) = 0;
	virtual int GetArgsOnStackCount(asUINT stackLevel) = 0;
	virtual int GetArgOnStack(asUINT stackLevel, asUINT arg, int* typeId, asUINT *flags, void** address) = 0;
protected:
	virtual ~CModuleContextHandle() override;

    int32_t m_nRefCount;
};

class CModuleCacheHandle : public asIBinaryStream
{
public:
	CModuleCacheHandle( const char *path, fileMode_t mode );

	virtual int Read( void *pBuffer, asUINT nBytes ) override;
	virtual int Write( const void *pBuffer, asUINT nBytes ) override;

	virtual ~CModuleCacheHandle() override;
private:
	fileHandle_t m_hFile;
};

typedef struct {
    const char *name;
    uint64_t callId;
    uint32_t expectedArgs;
    qboolean required;
} moduleFunc_t;

extern const moduleFunc_t funcDefs[NumFuncs];

#endif