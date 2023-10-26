#ifndef _SG_PUBLIC_
#define _SG_PUBLIC_

#pragma once

typedef enum
{
    SG_CVAR_UPDATE,
    SG_CVAR_REGISTER,
    SG_CVAR_SET,
    SG_CVAR_VARIABLESTRINBUFFER,

    SG_MILLISECONDS,

    SG_PRINT,
    SG_ERROR,

    SG_RE_SETCOLOR,
    SG_RE_ADDPOLYTOSCENE,
    SG_RE_ADDPOLYLISTTOSCENE,
    SG_RE_REGISTERSHADER,

    SG_GETGAMESTATE,
    
    SG_SND_REGISTERSFX,
    SG_SND_PLAYSFX,
    SG_SND_STOPSFX,
    SG_SND_ADDLOOPINGSFX,
    SG_SND_CLEARLOOPINGSFX,
    SG_SND_PLAYTRACK,

    SG_ADDCOMMAND,
    SG_REMOVECOMMAND,
    SG_ARGC,
    SG_ARGV,
    SG_ARGS,

    SG_KEY_GETCATCHER,
    SG_KEY_SETCATCHER,
    SG_KEY_ISDOWN,
    SG_KEY_GETKEY,
    SG_MEMORY_REMAINING,

    SG_FS_FOPENWRITE,
    SG_FS_FOPENREAD,
    SG_FS_FILESEEK,
    SG_FS_FILETELL,
    SG_FS_FILELENGTH,
    SG_FS_WRITE,
    SG_FS_READ,
    SG_FS_WRITEFILE,
    SG_FS_FCLOSE,

    SG_FLOOR = 107,
    SG_CEIL,
    SG_ACOS,

    NUM_SGAME_IMPORT
} sgameImport_t;

typedef enum
{
    SGAME_INIT,
    SGAME_CONSOLE_COMMAND,
    SGAME_SHUTDOWN,
    SGAME_RUNTIC,
    SGAME_DRAW,
    SGAME_STARTLEVEL,
    SGAME_ENDLEVEL,
    SGAME_FINISH_FRAME,
    SGAME_KEY_EVENT,
    SGAME_MOUSE_EVENT,
    SGAME_EVENT_NONE,
    SGAME_EVENT_HANDLING,

    NUM_SGAME_EXPORT
} sgameExport_t;


#endif
