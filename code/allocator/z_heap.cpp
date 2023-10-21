#include "../engine/n_shared.h"

#define DEFAULT_HEAP_SIZE (1800*1024*1024)
#define MIN_HEAP_SIZE (1000*1024*1024)
#define DEFAULT_ZONE_SIZE (400*1024*1024)

byte *hunkbase = NULL;
void *hunkorig = NULL;
uint64_t hunksize = 0;

extern uint64_t hunk_low_used;
extern uint64_t hunk_high_used;
extern boost::recursive_mutex hunkLock, allocLock;

void Mem_Init(void);
void Z_Shutdown(void);

void Memory_Shutdown(void)
{
	Con_Printf("Memory_Shutdown: deallocating allocation daemons\n");
	hunkLock.unlock();
	allocLock.unlock();
	if (hunkorig)
		free(hunkorig);
	
	Z_Shutdown();
}

void Zone_Stats(void);
void Hunk_Stats(void);

static void Com_Meminfo_f(void)
{
	Hunk_Stats();
	Zone_Stats();
}

uint64_t Com_TouchMemory(void)
{
	uint64_t i, j, sum, start, end;

	Z_CheckHeap();

	start = Sys_Milliseconds();
	sum = 0;

	j = hunk_low_used >> 2;
	for ( i = 0 ; i < j ; i+=64 ) {			// only need to touch each page
		sum += ((uint32_t *)hunkbase)[i];
	}

	i = ( hunksize - hunk_high_used ) >> 2;
	j = hunk_high_used >> 2;
	for (  ; i < j ; i+=64 ) {			// only need to touch each page
		sum += ((uint32_t *)hunkbase)[i];
	}

	Z_TouchMemory(&sum);

	end = Sys_Milliseconds();

	Con_Printf("Com_TouchMemory: %lu msec\n", end - start);

	return sum;
}

static void Com_TouchMemory_f(void)
{
	Com_TouchMemory();
}

static cvar_t *z_hunkMegs;

void Hunk_InitMemory(void)
{
	int i;
	cvar_t *cv;

	// make sure the file system has allocated and "not" freed any temp blocks
	// this allows the config and resource files ( journal files too ) to be loaded
	// by the file system without redundant routines in the file system utilizing different
	// memory systems
	if (FS_LoadStack() != 0) {
		N_Error(ERR_FATAL, "Hunk initialization failed. File system load stack not zero");
	}

	// allocate stack based hunk allocator
	Com_StartupVariable("hunkRAM");
	cv = Cvar_Get("z_hunkMegs", "1024", CVAR_PRIVATE | CVAR_ROM | CVAR_INIT | CVAR_PROTECTED);
	if ((cv->i * 1024 * 1024) != hunksize) {
		hunksize = cv->i * 1024 * 1024;
	}
	Cvar_SetDescription(cv, "The size of the hunk heap memory segment.");

	hunkbase = (byte *)malloc(hunksize + com_cacheLine);
    if (!hunkbase) {
        N_Error(ERR_FATAL, "Hunk_InitMemory: malloc() failed on %lu bytes when allocating the hunk", hunksize);
	}
	memset(hunkbase, 0, hunksize + com_cacheLine);
	hunkorig = hunkbase;

	// cacheline align
	hunkbase = PADP(hunkbase, com_cacheLine);
	
	// initialize it all
	Hunk_Clear();

	Con_Printf("Initialized hunk heap from %p -> %p (%lu MiB)\n", (void *)hunkbase, (void *)(hunkbase + hunksize), hunksize >> 20);

	Cmd_AddCommand("meminfo", Com_Meminfo_f);
	Cmd_AddCommand("touchmemory", Com_TouchMemory_f);
	Cmd_AddCommand("hunklogsmall", Hunk_SmallLog);
	Cmd_AddCommand("hunklog", Hunk_Log);
}

void Mem_Info(void)
{
	Com_Meminfo_f();
    Z_Print(true);
}