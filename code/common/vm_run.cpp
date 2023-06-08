#include "../src/n_shared.h"
#include "../bff_file/g_bff.h"
#include "vm.h"
#include "n_vm.h"

#define MAX_VM_NAME 10

static vm_t sgame;

void VM_Init(bffscript_t *scripts)
{
    Con_Printf("VM_Init: initializing virtual bytecode interpreters");
    memset(&sgame, 0, sizeof(sgame));

    bffscript_t *script = BFF_FetchScript("NM_SGAME");
    VM_Create(&sgame, "sgame", script->bytecode, script->codelen, G_SystemCalls);
    Con_Printf("Allocated sgame.qvm");
}

void VM_Restart(void)
{
    bffscript_t *script;
    if (!BFF_FetchScript("NM_SGAME")->bytecode) {
        N_Error("VM_Restart: vm bytecode freed prematurely (sgame)");
    }
    if (sgame.callLevel) {
        N_Error("VM_Restart: called on running vm (sgame)");
    }

    // restart the memory
    //TODO: THIS SHIT
    script = BFF_FetchScript("NM_SGAME");

    VM_Free(&sgame);
    VM_Create(&sgame, "sgame", script->bytecode, script->codelen, G_SystemCalls);
}

static uint64_t active_vm = 0;
uint32_t vm_command;
uint32_t vm_args[12];

static void VM_RunMain()
{
    intptr_t result = VM_Call(NULL, vm_command,
        vm_args[0], vm_args[1], vm_args[2], vm_args[3], vm_args[4], vm_args[5],
        vm_args[6], vm_args[7], vm_args[8], vm_args[9], vm_args[10], vm_args[11]);
}


intptr_t VM_Stop(uint64_t index)
{
}

void VM_Run(uint64_t index, int command, const nomadvector<int>& vm_args)
{
    intptr_t result = VM_Call(&sgame, vm_command,
        vm_args[0], vm_args[1], vm_args[2], vm_args[3], vm_args[4], vm_args[5],
        vm_args[6], vm_args[7], vm_args[8], vm_args[9], vm_args[10], vm_args[11]);
}

void Com_free(void *p, vm_t* vm, vmMallocType_t type)
{
    (void)vm;
    (void)type;
    if (p == NULL) {
        Con_Error("null pointer (Com_free)");
        return;
    }
    p = NULL;
}
void* Com_malloc(size_t size, vm_t* vm, vmMallocType_t type)
{
    (void)vm;
    (void)type;
    return Z_Malloc(size, TAG_STATIC, NULL, "vmMem");
}
void Com_Error(vmErrorCode_t level, const char* error)
{
    (void)level;
    N_Error("%s", error);
}