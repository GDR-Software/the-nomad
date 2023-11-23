#include "sg_lib.h"

#ifdef Q3VM_LIB_NO_INTRINSICS
size_t strlen(const char *str)
{
    const char *s = str;
    while (*s) {
        s++;
    }
    return (size_t)(s - str);
}

char *strcat( char *strDestination, const char *strSource )
{
	char	*s;

	s = strDestination;
	while ( *s ) {
		s++;
	}
	while ( *strSource ) {
		*s++ = *strSource++;
	}
	*s = 0;
	return strDestination;
}


char* strchr(const char* string, int c)
{
    while ( *string ) {
		if ( *string == c ) {
			return ( char * )string;
		}
		string++;
	}
	return (char *)0;
}

char *strrchr(const char *string, int32_t c)
{
    const char *found, *p;

    c = (unsigned char)c;

    if (c == '\0')
        return strchr(string, '\0');
    
    found = NULL;
    while ((p = strchr(string, c)) != NULL) {
        found = p;
        string = p + 1;
    }
    return (char *)found;
}

char *strstr( const char *string, const char *strCharSet )
{
	while ( *string ) {
		uint32_t i;

		for ( i = 0 ; strCharSet[i] ; i++ ) {
			if ( string[i] != strCharSet[i] ) {
				break;
			}
		}
		if ( !strCharSet[i] ) {
			return (char *)string;
		}
		string++;
	}
	return (char *)0;
}

int strcmp(const char* string1, const char* string2)
{
    while ( *string1 == *string2 && *string1 && *string2 ) {
		string1++;
		string2++;
	}
	return *string1 - *string2;
}

char* strcpy(char *dst, const char *src)
{
    char *d;

    d = dst;
    while (*src) {
        *d++ = *src++;
    }
    *d = 0;
    return dst;
}

static char* med3(char*, char*, char*, cmp_t*);
static void  swapfunc(char*, char*, int, int);

/*
 * Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".
 */
#define swapcode(TYPE, parmi, parmj, n)                                        \
    {                                                                          \
        long           i  = (n) / sizeof(TYPE);                                \
        register TYPE* pi = (TYPE*)(parmi);                                    \
        register TYPE* pj = (TYPE*)(parmj);                                    \
        do                                                                     \
        {                                                                      \
            register TYPE t = *pi;                                             \
            *pi++           = *pj;                                             \
            *pj++           = t;                                               \
        } while (--i > 0);                                                     \
    }

#define SWAPINIT(a, es)                                                        \
    swaptype = ((char*)a - (char*)0) % sizeof(long) || es % sizeof(long)       \
                   ? 2                                                         \
                   : es == sizeof(long) ? 0 : 1;

static void swapfunc(a, b, n, swaptype) char *a, *b;
int32_t         n, swaptype;
{
    if (swaptype <= 1)
        swapcode(long, a, b, n) else swapcode(char, a, b, n)
}

#define swap(a, b)                                                             \
    if (swaptype == 0)                                                         \
    {                                                                          \
        long t      = *(long*)(a);                                             \
        *(long*)(a) = *(long*)(b);                                             \
        *(long*)(b) = t;                                                       \
    }                                                                          \
    else                                                                       \
        swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n)                                                       \
    if ((n) > 0)                                                               \
    swapfunc(a, b, n, swaptype)

static char *med3(a, b, c, cmp) char *a, *b, *c;
cmp_t*       cmp;
{
    return cmp(a, b) < 0 ? (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a))
                         : (cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c));
}

void   qsort(a, n, es, cmp) void* a;
size_t n, es;
cmp_t* cmp;
{
    char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
    int32_t   d, r, swaptype, swap_cnt;

loop:
    SWAPINIT(a, es);
    swap_cnt = 0;
    if (n < 7)
    {
        for (pm = (char*)a + es; pm < (char*)a + n * es; pm += es)
            for (pl = pm; pl > (char*)a && cmp(pl - es, pl) > 0; pl -= es)
                swap(pl, pl - es);
        return;
    }
    pm = (char*)a + (n / 2) * es;
    if (n > 7)
    {
        pl = a;
        pn = (char*)a + (n - 1) * es;
        if (n > 40)
        {
            d  = (n / 8) * es;
            pl = med3(pl, pl + d, pl + 2 * d, cmp);
            pm = med3(pm - d, pm, pm + d, cmp);
            pn = med3(pn - 2 * d, pn - d, pn, cmp);
        }
        pm = med3(pl, pm, pn, cmp);
    }
    swap(a, pm);
    pa = pb = (char*)a + es;

    pc = pd = (char*)a + (n - 1) * es;
    for (;;)
    {
        while (pb <= pc && (r = cmp(pb, a)) <= 0)
        {
            if (r == 0)
            {
                swap_cnt = 1;
                swap(pa, pb);
                pa += es;
            }
            pb += es;
        }
        while (pb <= pc && (r = cmp(pc, a)) >= 0)
        {
            if (r == 0)
            {
                swap_cnt = 1;
                swap(pc, pd);
                pd -= es;
            }
            pc -= es;
        }
        if (pb > pc)
            break;
        swap(pb, pc);
        swap_cnt = 1;
        pb += es;
        pc -= es;
    }
    if (swap_cnt == 0)
    { /* Switch to insertion sort */
        for (pm = (char*)a + es; pm < (char*)a + n * es; pm += es)
            for (pl = pm; pl > (char*)a && cmp(pl - es, pl) > 0; pl -= es)
                swap(pl, pl - es);
        return;
    }

    pn = (char*)a + n * es;
    r  = min(pa - (char*)a, pb - pa);
    vecswap(a, pb - r, r);
    r = min(pd - pc, pn - pd - es);
    vecswap(pb, pn - r, r);
    if ((r = pb - pa) > es)
        qsort(a, r / es, es, cmp);
    if ((r = pd - pc) > es)
    {
        /* Iterate rather than recurse to save stack space */
        a = pn - r;
        n = r / es;
        goto loop;
    }
    /*      qsort(pn - r, r / es, es, cmp);*/
}
#endif

int32_t tolower(int32_t c)
{
    if (c >= 'A' && c <= 'Z') {
        c += 'a' - 'A';
    }
    return c;
}

int32_t toupper(int32_t c)
{
    if (c >= 'a' && c <= 'z') {
        c += 'A' - 'a';
    }
    return c;
}


double atof(const char* string)
{
    float sign;
    float value;
    int32_t   c;

    // skip whitespace
    while (*string <= ' ')
    {
        if (!*string)
        {
            return 0;
        }
        string++;
    }

    // check sign
    switch (*string)
    {
    case '+':
        string++;
        sign = 1;
        break;
    case '-':
        string++;
        sign = -1;
        break;
    default:
        sign = 1;
        break;
    }

    // read digits
    value = 0;
    c     = string[0];
    if (c != '.')
    {
        do
        {
            c = *string++;
            if (c < '0' || c > '9')
            {
                break;
            }
            c -= '0';
            value = value * 10 + c;
        } while (1);
    }
    else
    {
        string++;
    }

    // check for decimal point
    if (c == '.')
    {
        double fraction;

        fraction = 0.1;
        do
        {
            c = *string++;
            if (c < '0' || c > '9')
            {
                break;
            }
            c -= '0';
            value += c * fraction;
            fraction *= 0.1;
        } while (1);
    }

    // not handling 10e10 notation...

    return value * sign;
}

double _atof(const char** stringPtr)
{
    const char* string;
    float       sign;
    float       value;
    int32_t         c = '0'; // bk001211 - uninitialized use possible

    string = *stringPtr;

    // skip whitespace
    while (*string <= ' ')
    {
        if (!*string)
        {
            *stringPtr = string;
            return 0;
        }
        string++;
    }

    // check sign
    switch (*string)
    {
    case '+':
        string++;
        sign = 1;
        break;
    case '-':
        string++;
        sign = -1;
        break;
    default:
        sign = 1;
        break;
    }

    // read digits
    value = 0;
    if (string[0] != '.')
    {
        do
        {
            c = *string++;
            if (c < '0' || c > '9')
            {
                break;
            }
            c -= '0';
            value = value * 10 + c;
        } while (1);
    }

    // check for decimal point
    if (c == '.')
    {
        double fraction;

        fraction = 0.1;
        do
        {
            c = *string++;
            if (c < '0' || c > '9')
            {
                break;
            }
            c -= '0';
            value += c * fraction;
            fraction *= 0.1;
        } while (1);
    }

    // not handling 10e10 notation...
    *stringPtr = string;

    return value * sign;
}

int32_t atoi(const char* string)
{
    int32_t sign;
    int32_t value;
    int32_t c;

    // skip whitespace
    while (*string <= ' ')
    {
        if (!*string)
        {
            return 0;
        }
        string++;
    }

    // check sign
    switch (*string)
    {
    case '+':
        string++;
        sign = 1;
        break;
    case '-':
        string++;
        sign = -1;
        break;
    default:
        sign = 1;
        break;
    }

    // read digits
    value = 0;
    do
    {
        c = *string++;
        if (c < '0' || c > '9')
        {
            break;
        }
        c -= '0';
        value = value * 10 + c;
    } while (1);

    // not handling 10e10 notation...

    return value * sign;
}

int32_t _atoi(const char** stringPtr)
{
    int32_t         sign;
    int32_t         value;
    int32_t         c;
    const char* string;

    string = *stringPtr;

    // skip whitespace
    while (*string <= ' ')
    {
        if (!*string)
        {
            return 0;
        }
        string++;
    }

    // check sign
    switch (*string)
    {
    case '+':
        string++;
        sign = 1;
        break;
    case '-':
        string++;
        sign = -1;
        break;
    default:
        sign = 1;
        break;
    }

    // read digits
    value = 0;
    do
    {
        c = *string++;
        if (c < '0' || c > '9')
        {
            break;
        }
        c -= '0';
        value = value * 10 + c;
    } while (1);

    // not handling 10e10 notation...

    *stringPtr = string;

    return value * sign;
}

double tan(double x)
{
    return sin(x) / cos(x);
}

static int32_t randSeed = 0;

void srand(unsigned seed)
{
    randSeed = seed;
}

int32_t rand(void)
{
    randSeed = (69069 * randSeed + 1);
    return randSeed & 0x7fff;
}


int32_t abs(int32_t n)
{
    return n < 0 ? -n : n;
}

double fabs(double x)
{
    return x < 0 ? -x : x;
}

#define ALT 0x00000001       /* alternate form */
#define HEXPREFIX 0x00000002 /* add 0x or 0X prefix */
#define LADJUST 0x00000004   /* left adjustment */
#define LONGDBL 0x00000008   /* long double */
#define LONGINT 0x00000010   /* long integer */
#define QUADINT 0x00000020   /* quad integer */
#define SHORTINT 0x00000040  /* short integer */
#define ZEROPAD 0x00000080   /* zero (as opposed to blank) pad */
#define FPT 0x00000100       /* floating point32_t number */

#define to_digit(c) ((c) - '0')
#define is_digit(c) ((unsigned)to_digit(c) <= 9)
#define to_char(n) ((n) + '0')

void AddInt(char** buf_p, int32_t val, int32_t width, int32_t flags)
{
    char  text[32];
    int32_t   digits;
    int32_t   signedVal;
    char* buf;

    digits    = 0;
    signedVal = val;
    if (val < 0)
    {
        val = -val;
    }
    do
    {
        text[digits++] = '0' + val % 10;
        val /= 10;
    } while (val);

    if (signedVal < 0)
    {
        text[digits++] = '-';
    }

    buf = *buf_p;

    if (!(flags & LADJUST))
    {
        while (digits < width)
        {
            *buf++ = (flags & ZEROPAD) ? '0' : ' ';
            width--;
        }
    }

    while (digits--)
    {
        *buf++ = text[digits];
        width--;
    }

    if (flags & LADJUST)
    {
        while (width--)
        {
            *buf++ = (flags & ZEROPAD) ? '0' : ' ';
        }
    }

    *buf_p = buf;
}

void AddFloat(char** buf_p, float fval, int32_t width, int32_t prec)
{
    char  text[32];
    int32_t   digits;
    float signedVal;
    char* buf;
    int32_t   val;

    // get the sign
    signedVal = fval;
    if (fval < 0)
    {
        fval = -fval;
    }

    // write the float number
    digits = 0;
    val    = (int32_t)fval;
    do
    {
        text[digits++] = '0' + val % 10;
        val /= 10;
    } while (val);

    if (signedVal < 0)
    {
        text[digits++] = '-';
    }

    buf = *buf_p;

    while (digits < width)
    {
        *buf++ = ' ';
        width--;
    }

    while (digits--)
    {
        *buf++ = text[digits];
    }

    *buf_p = buf;

    if (prec < 0)
        prec = 6;
    // write the fraction
    digits = 0;
    while (digits < prec)
    {
        fval -= (int)fval;
        fval *= 10.0;
        val            = (int)fval;
        text[digits++] = '0' + val % 10;
    }

    if (digits > 0)
    {
        buf    = *buf_p;
        *buf++ = '.';
        for (prec = 0; prec < digits; prec++)
        {
            *buf++ = text[prec];
        }
        *buf_p = buf;
    }
}

void AddString(char** buf_p, char* string, int32_t width, int32_t prec)
{
    int32_t   size;
    char* buf;

    buf = *buf_p;

    if (string == NULL)
    {
        string = "(null)";
        prec   = -1;
    }

    if (prec >= 0)
    {
        for (size = 0; size < prec; size++)
        {
            if (string[size] == '\0')
            {
                break;
            }
        }
    }
    else
    {
        size = strlen(string);
    }

    width -= size;

    while (size--)
    {
        *buf++ = *string++;
    }

    while (width-- > 0)
    {
        *buf++ = ' ';
    }

    *buf_p = buf;
}

int32_t vsprintf(char* buffer, const char* fmt, va_list argptr)
{
    int32_t*    arg;
    char*       buf_p;
    char        ch;
    int32_t     flags;
    int32_t     width;
    int32_t     prec;
    int32_t     n;
    char        sign;

    buf_p = buffer;
    arg   = (int32_t *)argptr;

    while (1)
    {
        // run through the format string until we hit a '%' or '\0'
        for (ch = *fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++)
        {
            *buf_p++ = ch;
        }
        if (ch == '\0')
        {
            goto done;
        }

        // skip over the '%'
        fmt++;

        // reset formatting state
        flags = 0;
        width = 0;
        prec  = -1;
        sign  = '\0';

    rflag:
        ch = *fmt++;
    reswitch:
        switch (ch)
        {
        case '-':
            flags |= LADJUST;
            goto rflag;
        case '.':
            n = 0;
            while (is_digit((ch = *fmt++)))
            {
                n = 10 * n + (ch - '0');
            }
            prec = n < 0 ? -1 : n;
            goto reswitch;
        case '0':
            flags |= ZEROPAD;
            goto rflag;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            n = 0;
            do
            {
                n  = 10 * n + (ch - '0');
                ch = *fmt++;
            } while (is_digit(ch));
            width = n;
            goto reswitch;
        case 'c':
            *buf_p++ = (char)*arg;
            arg++;
            break;
        case 'd':
        case 'i':
            AddInt(&buf_p, *arg, width, flags);
            arg++;
            break;
        case 'f':
            AddFloat(&buf_p, *(double*)arg, width, prec);
#ifdef __LCC__
            arg += 1; // everything is 32 bit in my compiler
#else
            arg += 2;
#endif
            break;
        case 's':
            AddString(&buf_p, (char*)*arg, width, prec);
            arg++;
            break;
        case '%':
            *buf_p++ = ch;
            break;
        default:
            *buf_p++ = (char)*arg;
            arg++;
            break;
        }
    }

done:
    *buf_p = 0;
    return buf_p - buffer;
}

/* this is really crappy */
int32_t sscanf( const char *buffer, const char *fmt, ... )
{
	int32_t	cmd;
	int32_t	**arg;
	int32_t	count;

	arg = (int32_t **)&fmt + 1;
	count = 0;

	while ( *fmt ) {
		if ( fmt[0] != '%' ) {
			fmt++;
			continue;
		}

		cmd = fmt[1];
		fmt += 2;

		switch ( cmd ) {
		case 'i':
		case 'd':
		case 'u':
			**arg = _atoi( &buffer );
			break;
		case 'f':
			*(float *)*arg = _atof( &buffer );
			break;
		}
		arg++;
	}

	return count;
}
