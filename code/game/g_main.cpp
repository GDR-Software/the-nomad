#define STB_SPRINTF_IMPLEMENTATION
#include "n_shared.h"
#include "g_game.h"

#ifdef __unix__
#include <signal.h>
#endif

#ifdef SIGINT
void SIG_INTERRUPT(int signum)
{
    Con_Printf("SIGINT recieved, exit not unnatural, freeing all memory");
    Game::Get()->~Game();
    exit(EXIT_SUCCESS);
}
#endif

#ifdef SIGSEGV
void SIG_SEGV(int signum)
{
    Con_Error("Segmentation Violation recieved, freeing all memory");
    Game::Get()->~Game();
    exit(EXIT_FAILURE);
}
#endif

#ifdef SIGABRT
void SIG_ABORT(int signum)
{
    Con_Error("SIGABRT recieved, freeing all memory");
    Game::Get()->~Game();
    exit(EXIT_FAILURE);
}
#endif

#ifdef SIGBUS
void SIG_BUS(int signum)
{
    Con_Error("SIBGUS recieved, attempted to write/read to an invalid address, freeing all memory");
    Game::Get()->~Game();
    exit(EXIT_FAILURE);
}
#endif

#ifdef SIGFPE
void SIG_FPE(int signum)
{
    Con_Error("SIGFPE recieved, rounding or divide-by-0 error, freeing all memory");
    Game::Get()->~Game();
    exit(EXIT_FAILURE);
}
#endif

int main(int argc, char** argv)
{
    I_NomadInit(argc, argv);
    return 0;
}