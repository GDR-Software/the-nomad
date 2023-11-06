#include "../engine/n_shared.h"
#include "../engine/n_common.h"
#include "../game/g_game.h"
#include "sys_unix.h"
#include <execinfo.h>

#define SYS_BACKTRACE_MAX 1024

extern int dll_error_count;

static field_t tty_con;
static int stdin_flags;
static struct termios tty_tc;
static qboolean stdin_active;
static qboolean ttycon_active;
static int ttycon_hide;
static int tty_erase;
static int tty_eof;
static qboolean ttycon_on = qfalse;
static qboolean ttycon_color_on = qfalse;
static cvar_t *ttycon;
static cvar_t *ttycon_ansicolor;
extern file_t logfile;

typedef enum {
	TTY_ENABLED,
	TTY_DISABLED,
	TTY_ERROR
} tty_err;

void Sys_ShutdownConsole(void);
tty_err Sys_InitConsole( void );

/*
Forcing an inline because the backtrace traces the call to the backtrace... (irony)
*/
#define BACKTRACE(amount) \
{ \
    if (amount > SYS_BACKTRACE_MAX) { \
        Con_Printf(COLOR_RED "Attempted to stacktrace > %i, aborting\n", SYS_BACKTRACE_MAX); \
    } \
    else if (!FS_Initialized()) {} \
    else { \
        void **arr; \
        char *buffer; \
        FILE *tempfp; \
        const char *ospath; \
        int size; \
        uint64_t fileLength; \
        { \
            arr = (void **)alloca(sizeof(void *) * amount); \
            size = backtrace(arr, amount); \
            ospath = FS_BuildOSPath(Cvar_VariableString("fs_basepath"), NULL, "backtrace.dat"); \
            tempfp = Sys_FOpen(ospath, "w+"); \
            if (!tempfp) { \
                FS_Printf(logfile, "ERROR: Failed to open a backtrace file" GDR_NEWLINE); \
                fprintf(stderr, "ERROR: Failed to open a backtrace file\n"); \
                Com_Shutdown(); \
                exit(-1); \
            } \
            /* write the backtrace */ \
            backtrace_symbols_fd(arr, size, fileno(tempfp)); \
        } \
        fseek(tempfp, 0L, SEEK_END); \
        fileLength = ftell(tempfp); \
        fseek(tempfp, 0L, SEEK_SET); \
        buffer = (char *)alloca(fileLength); \
        fread(buffer, fileLength, 1, tempfp); \
        Con_Printf("Successfully obtained %i stack frames\n", size); \
        Con_Printf("Stack List:\n%s\n", buffer); \
        fclose(tempfp); \
    } \
}

// flush stdin, I suspect some terminals are sending a LOT of shit
static void tty_FlushIn(void)
{
    tcflush(STDIN_FILENO, TCIFLUSH);
}

// do a backspace
// TTimo NOTE: it seems on some terminals just sending '\b' is not enough
//   so for now, in any case we send "\b \b" .. yeah well ..
//   (there may be a way to find out if '\b' alone would work though)
static void tty_Back( void )
{
	write( STDOUT_FILENO, "\b \b", 3 );
}

// clear the display of the line currently edited
// bring cursor back to beginning of line
void tty_Hide( void )
{
	int i;

	if ( !ttycon_on )
		return;

	if ( ttycon_hide )
	{
		ttycon_hide++;
		return;
	}

	if ( tty_con.cursor > 0 )
	{
		for ( i = 0; i < tty_con.cursor; i++ )
		{
			tty_Back();
		}
	}
	tty_Back(); // delete "]" ? -EC-
	ttycon_hide++;
}


// show the current line
// FIXME TTimo need to position the cursor if needed??
void tty_Show( void )
{
	if ( !ttycon_on )
		return;

	if ( ttycon_hide > 0 )
	{
		ttycon_hide--;
		if ( ttycon_hide == 0 )
		{
			write( STDOUT_FILENO, "]", 1 ); // -EC-

			if ( tty_con.cursor > 0 )
			{
				write( STDOUT_FILENO, tty_con.buffer, tty_con.cursor );
			}
		}
	}
}

typedef struct
{
    uint32_t id;
    qboolean safe;
    const char *str;
} exittype_t;

static const exittype_t signals[] = {
    {SIGSEGV,  qfalse, "segmentation violation"},
    {SIGBUS,   qfalse, "bus error"},
    {SIGABRT,  qfalse, "abnormal program termination"},
    {SIGSTOP,  qtrue,  "pausing program"},
    {SIGTERM,  qtrue,  "program termination"},
    {SIGILL,   qtrue,  "illegal instruction"},
    {SIGTRAP,  qtrue,  "debug breakpoint"},
    {0,        qtrue,  "No System Error"}
};

static const exittype_t *exit_type;

void GDR_NORETURN GDR_ATTRIBUTE((format(printf, 1, 2))) GDR_DECL Sys_Error(const char *fmt, ...)
{
    va_list argptr;
	char text[MAXPRINTMSG];
    const char *msg;

	// change stdin to non blocking
	// NOTE TTimo not sure how well that goes with tty console mode
	if ( stdin_active )
	{
//		fcntl( STDIN_FILENO, F_SETFL, fcntl( STDIN_FILENO, F_GETFL, 0) & ~FNDELAY );
		fcntl( STDIN_FILENO, F_SETFL, stdin_flags );
	}

	// don't bother do a show on this one heh
	if ( ttycon_on )
	{
		tty_Hide();
	}

	va_start( argptr, fmt );
	N_vsnprintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

    msg = va("Sys_Error: %s\n", text);
    write(STDERR_FILENO, msg, strlen(msg));
    
    // fprintf COULD call malloc
//	fprintf( stderr, "Sys_Error: %s\n", text );

	Sys_Exit( -1 ); // bk010104 - use single exit point.
}

void GDR_NORETURN Sys_Exit(int code)
{
    const char *err;
#ifdef _NOMAD_DEBUG
    const bool debug = true;
#else
    const bool debug = false;
#endif

    if (code == -1) {
        if (exit_type)
            err = exit_type->str;
        else
            err = "No System Error";
        if (N_stricmp("No System Error", err) != 0)
            fprintf(stderr, "Exiting With System Error: %s\n", err);
        else
            fprintf(stderr, "Exiting With Engine Error\n");
    }
    if (dll_error_count) {
        fprintf(stderr, "Sys_Error: dll_error_count > 0, possible dlerror(): %s\n", dlerror());
    }
    Sys_ShutdownConsole();

    if (code == -1)
        exit(EXIT_FAILURE);
    
    exit(EXIT_SUCCESS);
}


void fpe_exception_handler(int signum)
{
    signal(SIGFPE, fpe_exception_handler);
}

void Catch_Signal(int signum)
{
    for (uint32_t i = 0; i < arraylen(signals); i++) {
        if (signals[i].id == signum)
            exit_type = &signals[i];
    }
    if (!exit_type)
        exit_type = &signals[arraylen(signals) - 1];
    
    Sys_Exit(-1);
}

static const struct Q3ToAnsiColorTable_s
{
	const char Q3color;
	const char *ANSIcolor;
} tty_colorTable[ ] = {
	{ S_COLOR_BLACK,    "30" },
	{ S_COLOR_RED,      "31" },
	{ S_COLOR_GREEN,    "32" },
	{ S_COLOR_YELLOW,   "33" },
	{ S_COLOR_BLUE,     "34" },
	{ S_COLOR_CYAN,     "36" },
	{ S_COLOR_MAGENTA,  "35" },
	{ S_COLOR_WHITE,    "0" }
};

static const char *getANSIcolor( char Q3color ) {
	int i;
	for ( i = 0; i < arraylen( tty_colorTable ); i++ ) {
		if ( Q3color == tty_colorTable[ i ].Q3color ) {
			return tty_colorTable[ i ].ANSIcolor;
		}
	}
	return NULL;
}

void Sys_ShutdownConsole(void)
{
    if (ttycon_on) {
        tty_Back();
        tcsetattr(STDIN_FILENO, TCSADRAIN, &tty_tc);
    }

    // restore stdin blocking mode
    if (stdin_active) {
        fcntl(STDIN_FILENO, F_SETFL, stdin_flags);
    }

    memset(&tty_con, 0, sizeof(tty_con));

    stdin_active = qfalse;
    ttycon_on = qfalse;

    ttycon_hide = 0;
}

void Sys_SigCont(int signum)
{
    Sys_InitConsole();
}

void Sys_SigTStp(int signum)
{
    sigset_t mask;

    tty_FlushIn();
    Sys_ShutdownConsole();

    sigemptyset(&mask);
    sigaddset(&mask, SIGTSTP);
    sigprocmask(SIG_UNBLOCK, &mask, NULL);

    signal(SIGTSTP, SIG_DFL);

    kill(getpid(), SIGTSTP);
}

static qboolean printableChar( char c ) {
	if ( ( c >= ' ' && c <= '~' ) || c == '\n' || c == '\r' || c == '\t' )
		return qtrue;
	else
		return qfalse;
}

void Sys_ANSIColorMsg(const char *msg, char *buffer, uint64_t bufSize)
{
    uint64_t msgLength;
    uint64_t i;
    char tmpbuf[8];
    const char *ANSIcolor;

    if (!msg || !buffer)
        return;

    msgLength = strlen(msg);
    i = 0;
    buffer[0] = '\0';

    while (i < msgLength) {
        if (msg[i] == '\n') {
            snprintf(tmpbuf, sizeof(tmpbuf), "%c[0m\n", 0x1B);
            strncat(buffer, tmpbuf, bufSize - 1);
            i += 1;
        }
        else if (msg[i] == Q_COLOR_ESCAPE && (ANSIcolor = getANSIcolor(msg[i+1])) != NULL) {
            snprintf(tmpbuf, sizeof(tmpbuf), "%c[%sm", 0x1B, ANSIcolor);
            strncat(buffer, tmpbuf, bufSize - 1);
            i += 2;
        }
        else {
            if (printableChar(msg[i])) {
                snprintf(tmpbuf, sizeof(tmpbuf), "%c", msg[i]);
                strncat(buffer, tmpbuf, bufSize - 1);
            }
            i += 1;
        }
    }
}

void Sys_Print(const char *msg)
{
    char printmsg[MAXPRINTMSG];
    size_t len;

    memset(printmsg, 0, sizeof(printmsg));

    if (ttycon_on) {
        tty_Hide();
    }
    if (ttycon_color_on) {
        Sys_ANSIColorMsg(msg, printmsg, sizeof(printmsg));
        len = strlen(printmsg);
    }
    else {
        char *out = printmsg;
        while (*msg != '\0' && out < printmsg + sizeof(printmsg)) {
            if (printableChar(*msg))
                *out++ = *msg;
            msg++;
        }
        len = out - printmsg;
    }

    write(STDERR_FILENO, printmsg, len);
    if (ttycon_on) {
        tty_Show();
    }
}

char *Sys_ConsoleInput( void )
{
    // we use this when sending back commands
    static char text[ sizeof(tty_con.buffer) ];
    int avail;
    char key;
    char *s;
    field_t history;

    if ( ttycon_on ) {
		avail = read( STDIN_FILENO, &key, 1 );
		if (avail != -1) {
			// we have something
			// backspace?
			// NOTE TTimo testing a lot of values .. seems it's the only way to get it to work everywhere
			if ((key == tty_erase) || (key == 127) || (key == 8)) {
				if (tty_con.cursor > 0) {
					tty_con.cursor--;
					tty_con.buffer[tty_con.cursor] = '\0';
					tty_Back();
				}
				return NULL;
			}

			// check if this is a control char
			if (key && key < ' ') {
				if (key == '\n') {
					// push it in history
					Con_SaveField( &tty_con );
					s = tty_con.buffer;
					while ( *s == '\\' || *s == '/' ) // skip leading slashes
						s++;
					N_strncpyz( text, s, sizeof( text ) );
					Field_Clear( &tty_con );
					write( STDOUT_FILENO, "\n]", 2 );
					return text;
				}

				if (key == '\t') {
					tty_Hide();
					Field_AutoComplete( &tty_con );
					tty_Show();
					return NULL;
				}

				avail = read( STDIN_FILENO, &key, 1 );
				if (avail != -1) {
					// VT 100 keys
					if (key == '[' || key == 'O') {
						avail = read( STDIN_FILENO, &key, 1 );
						if (avail != -1) {
							switch (key) {
							case 'A':
								if ( Con_HistoryGetPrev( &history ) ) {
									tty_Hide();
									tty_con = history;
									tty_Show();
								}
								tty_FlushIn();
								return NULL;
								break;
							case 'B':
								if ( Con_HistoryGetNext( &history ) ) {
									tty_Hide();
									tty_con = history;
									tty_Show();
								}
								tty_FlushIn();
								return NULL;
								break;
							case 'C': // right
							case 'D': // left
							//case 'H': // home
							//case 'F': // end
								return NULL;
							};
						}
					}
				}

				if ( key == 12 ) { // clear teaminal
					write( STDOUT_FILENO, "\ec]", 3 );
					if ( tty_con.cursor ) {
						write( STDOUT_FILENO, tty_con.buffer, tty_con.cursor );
					}
					tty_FlushIn();
					return NULL;
				}

				Con_DPrintf( "dropping ISCTL sequence: %d, tty_erase: %d\n", key, tty_erase );
				tty_FlushIn();
				return NULL;
			}
			if ( tty_con.cursor >= sizeof( text ) - 1 ) {
				return NULL;
            }

			// push regular character
			tty_con.buffer[ tty_con.cursor ] = key;
			tty_con.cursor++;
			// print the current line (this is differential)
			write( STDOUT_FILENO, &key, 1 );
		}
		return NULL;
	}
	else if ( stdin_active ) {
		int len;
		fd_set fdset;
		struct timeval timeout;

		FD_ZERO( &fdset );
		FD_SET( STDIN_FILENO, &fdset ); // stdin
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		if ( select( STDIN_FILENO + 1, &fdset, NULL, NULL, &timeout) == -1 || !FD_ISSET( STDIN_FILENO, &fdset ) ) {
			return NULL;
		}

		len = read( STDIN_FILENO, text, sizeof( text ) );
		if ( len == 0 ) { // eof!
			fcntl( STDIN_FILENO, F_SETFL, stdin_flags );
			stdin_active = qfalse;
			return NULL;
		}

		if ( len < 1 ) {
			return NULL;
        }

		text[len-1] = '\0'; // rip off the /n and terminate
		s = text;

		while ( *s == '\\' || *s == '/' ) { // skip leading slashes
			s++;
        }

		return s;
	}

	return NULL;
}

const char *Sys_GetError(void)
{
    return strerror(errno);
}

#if 0
#define PTHREAD_CHECK_FAIL(func,...) if (func(__VA_ARGS__) != 0) N_Error(ERR_FATAL, "%s: " #func " failed, error string: %s", Sys_GetError())
#define MAX_THREADS 64
typedef struct {
    qboolean running;
    pthread_t id;
    pthread_attr_t attrib;
    const char *name;
} thread_t;
thread_t threadData[MAX_THREADS];

nhandle_t Sys_RegisterThread(const char *name)
{
    thread_t *thread;
    nhandle_t hash;

    hash = (nhandle_t)Com_GenerateHashValue(name, MAX_THREADS);
    if (threadData[hash].name) {
        N_Error(ERR_FATAL, "Sys_RegisterThread: thread '%s' registered twice", name);
    }
    thread = &threadData[hash];
    memset(thread, 0, sizeof(*thread));
    thread->name = name;

    PTHREAD_CHECK_FAIL(pthread_attr_init, &thread->attrib);

    return hash;
}

void Sys_StartThread(nhandle_t id, void (*func)(void))
{
    thread_t *thread;

    if (!threadData[id].name) {
        N_Error(ERR_FATAL, "%s: bad thread id (%i)", __func__, id);
    }

    thread = &threadData[id];
    thread->running = qtrue;

    PTHREAD_CHECK_FAIL(pthread_create, &thread->id, &thread->attrib, (void *(*)(void *))func, NULL);
}

void Sys_JoinThread(nhandle_t id)
{
    thread_t *thread;

    if (!threadData[id].name) {
        N_Error(ERR_FATAL, "%s: bad thread id (%i)", __func__, id);
    }
    if (!threadData[id].running) {
        Con_DPrintf("%s(%i), not running\n", __func__, id);
    }

    thread = &threadData[id];
    PTHREAD_CHECK_FAIL(pthread_join, thread->id, (void **)NULL);
    thread->running = qfalse;
}

typedef struct {
    qboolean locked;
    pthread_mutex_t mutex;
    pthread_mutexattr_t attrib;
    pthread_rwlock_t rwlock;
    uint32_t type;
} mutex_t;

mutex_t *Sys_CreateMutex(int type)
{
    mutex_t *m;

    m = (mutex_t *)Z_Malloc(sizeof(*m), TAG_STATIC);
    memset(m, 0, sizeof(*m));

    PTHREAD_CHECK_FAIL(pthread_mutexattr_init, &m->attrib);

    switch (type) {
    case MUTEX_TYPE_RECURSIVE:
        PTHREAD_CHECK_FAIL(pthread_mutexattr_settype, &m->attrib, PTHREAD_MUTEX_RECURSIVE);
        break;
    case MUTEX_TYPE_SHARED:
        PTHREAD_CHECK_FAIL(pthread_rwlock_init, &m->rwlock, NULL);
        break;
    };

    PTHREAD_CHECK_FAIL(pthread_mutex_init, &m->mutex, &m->attrib);
    m->type = type;

    return m;
}

void Sys_LockMutex(mutex_t *m)
{
    if (m->locked) {
        N_Error(ERR_FATAL, "Sys_LockMutex on a locked mutex");
    }
}
#endif

tty_err Sys_InitConsole(void)
{
    struct termios tc;
    const char *term;

    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);

    Con_Printf("Sys_InitConsole: initializing logging\n");

    // if SIGCONT is recieved, reinitialize the console
    signal(SIGCONT, Sys_SigCont);

    if (signal(SIGTSTP, SIG_IGN) == SIG_DFL) {
        signal(SIGTSTP, Sys_SigTStp);
    }

    stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (stdin_flags == -1) {
        stdin_active = qfalse;
        return TTY_ERROR;
    }

    // set non-blocking mode
    fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);
    stdin_active = qtrue;

    ttycon_color_on = qtrue;

    term = getenv("TERM");
    if (isatty(STDIN_FILENO) != -1 || !term || !strcmp(term, "dumb") || !strcmp(term, "raw")) {
        ttycon_on = qfalse;
        return TTY_DISABLED;
    }

    Field_Clear(&tty_con);
    tcgetattr(STDIN_FILENO, &tty_tc);
    tty_erase = tty_tc.c_cc[VERASE];
    tty_eof = tty_tc.c_cc[VEOF];
    tc = tty_tc;

    tc.c_lflag &= ~(ECHO | ICANON);
    tc.c_iflag &= ~(ISTRIP | INPCK);
    tc.c_cc[VMIN] = 1;
    tc.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSADRAIN, &tc);

    ttycon_color_on = qtrue;
    ttycon_on = qtrue;

    tty_Hide();
    tty_Show();
    
    return TTY_ENABLED;
}

void Sys_PrintBinVersion( const char* name )
{
	const char *date = __DATE__;
	const char *time = __TIME__;
	const char *sep = "==============================================================";

	fprintf( stdout, "\n\n%s\n", sep );
	fprintf( stdout, "Linux GLNomad Full Executable  [%s %s]\n", date, time );
	fprintf( stdout, " local install: %s\n", name );
	fprintf( stdout, "%s\n\n", sep );
}

/*
=================
Sys_BinName

This resolves any symlinks to the binary. It's disabled for debug
builds because there are situations where you are likely to want
to symlink to binaries and /not/ have the links resolved.
=================
*/
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
const char *Sys_BinName( const char *arg0 )
{
	static char dst[ PATH_MAX ];

#ifdef _NOMAD_DEBUG

#if defined (__linux__)
	int n = readlink( "/proc/self/exe", dst, PATH_MAX - 1 );

	if ( n >= 0 && n < PATH_MAX )
		dst[ n ] = '\0';
	else
		N_strncpyz( dst, arg0, PATH_MAX );
#elif defined (__APPLE__)
	uint32_t bufsize = sizeof( dst );

	if ( _NSGetExecutablePath( dst, &bufsize ) == -1 )
	{
		N_strncpyz( dst, arg0, PATH_MAX );
	}
#else

#warning Sys_BinName not implemented
	N_strncpyz( dst, arg0, PATH_MAX );
#endif

#else // DEBUG
	N_strncpyz( dst, arg0, PATH_MAX );
#endif
	return dst;
}

int Sys_ParseArgs(int argc, const char *argv[])
{
    if (argc == 2) {
        if ((!strcmp(argv[1], "--version")) || (!strcmp(argv[1], "-v"))) {
            Sys_PrintBinVersion(Sys_BinName(argv[0]));
            return 1;
        }
    }
    return 0;
}

static void SignalHandle(int signum)
{
    if (signum == SIGSEGV) {
        exit_type = &signals[0];
        Sys_Error("recieved SIGSEGV");
    } else if (signum == SIGABRT) {
        exit_type = &signals[2];
        Sys_Error("recieved SIGABRT");
    } else if (signum == SIGTERM) {
        signal(SIGTERM, SignalHandle);
    } else if (signum == SIGBUS) {
        exit_type = &signals[1];
        Sys_Error("recieved SIGBUS");
    } else if (signum == SIGILL) {
        exit_type = &signals[5];
        Sys_Error("recieved SIGILL");
    } else if (signum == SIGFPE) {
        Con_DPrintf("recieved SIGFPE, floating point exception, don't really care... continuing...\n");
        BACKTRACE(128); // do a stack dump
        signal(SIGFPE, SignalHandle);
    } else if (signum == SIGTRAP) {
        Con_DPrintf("DebugBreak Triggered...\n");
        BACKTRACE(128); // do a stack dump
        signal(SIGTRAP, SignalHandle);
    } else {
        Con_DPrintf("Unknown signal (%i)... Wtf?\n", signum);
    }
}

void Sys_Init(void)
{
    signal(SIGTERM, SignalHandle);
    signal(SIGSEGV, SignalHandle);
    signal(SIGFPE, SignalHandle);
    signal(SIGABRT, SignalHandle);
    signal(SIGBUS, SignalHandle);
    signal(SIGTRAP, SignalHandle);
    signal(SIGILL, SignalHandle);
}

int main(int argc, char **argv)
{
    char con_title[MAX_CVAR_VALUE];
    int xpos, ypos;
    char *cmdline;
    int len, i;
    tty_err err;

#ifdef __APPLE__
	// This is passed if we are launched by double-clicking
	if ( argc >= 2 && N_strncmp( argv[1], "-psn", 4 ) == 0 )
		argc = 1;
#endif

    // merge the command line, this is kinda silly
	for ( len = 1, i = 1; i < argc; i++ )
		len += strlen( argv[i] ) + 1;

	cmdline = (char *)malloc( len );
    if (!cmdline) { // oh shit
        write(STDERR_FILENO, "malloc() failed, out of memory\n", strlen("malloc() failed, out of memory\n"));
        _Exit(EXIT_FAILURE);
    }
	*cmdline = '\0';
	for ( i = 1; i < argc; i++ ) {
		if ( i > 1 )
			strcat( cmdline, " " );
        
		strcat( cmdline, argv[i] );
	}

	// get the initial time base
	Sys_Milliseconds();
    Sys_Init();

    err = Sys_InitConsole();
	if ( err == TTY_ENABLED ) {
		Con_Printf( "Started tty console (use +set ttycon 0 to disable)\n" );
	}
	else {
		if ( err == TTY_ERROR ) {
			Con_Printf( "stdin is not a tty, tty console mode failed\n" );
            ttycon_on = qfalse;
		}
	}
    
    Com_Init(cmdline);

	while (1) {
		// run the game
		Com_Frame(qfalse);
	}
	// never gets here
	return 0;
}