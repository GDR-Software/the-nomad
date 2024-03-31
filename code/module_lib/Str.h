/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __STR_H__
#define __STR_H__

#include "module_public.h"

/*
===============================================================================

	Character string

===============================================================================
*/

// make idStr a multiple of 16 bytes long
// don't make too large to keep memory requirements to a minimum
const int STR_ALLOC_BASE			= 20;
const int STR_ALLOC_GRAN			= 32;

typedef enum {
	MEASURE_SIZE = 0,
	MEASURE_BANDWIDTH
} Measure_t;

class idStr {

public:
						idStr( void );
						idStr( const idStr &text );
						idStr( const idStr &text, int start, int end );
						idStr( const char *text );
						idStr( const char *text, int start, int end );
						explicit idStr( const bool b );
						explicit idStr( const char c );
						explicit idStr( const int i );
						explicit idStr( const unsigned u );
						explicit idStr( const float f );
						~idStr( void );

	size_t				Size( void ) const;
	const char *		c_str( void ) const;
	operator			const char *( void ) const;
	operator			const char *( void );

	char				operator[]( int index ) const;
	char &				operator[]( int index );

	void				operator=( const idStr &text );
	void				operator=( const char *text );

	friend idStr		operator+( const idStr &a, const idStr &b );
	friend idStr		operator+( const idStr &a, const char *b );
	friend idStr		operator+( const char *a, const idStr &b );

	friend idStr		operator+( const idStr &a, const float b );
	friend idStr		operator+( const idStr &a, const int b );
	friend idStr		operator+( const idStr &a, const unsigned b );
	friend idStr		operator+( const idStr &a, const bool b );
	friend idStr		operator+( const idStr &a, const char b );

	idStr &				operator+=( const idStr &a );
	idStr &				operator+=( const char *a );
	idStr &				operator+=( const float a );
	idStr &				operator+=( const char a );
	idStr &				operator+=( const int a );
	idStr &				operator+=( const unsigned a );
	idStr &				operator+=( const bool a );

						// case sensitive compare
	friend bool			operator==( const idStr &a, const idStr &b );
	friend bool			operator==( const idStr &a, const char *b );
	friend bool			operator==( const char *a, const idStr &b );

						// case sensitive compare
	friend bool			operator!=( const idStr &a, const idStr &b );
	friend bool			operator!=( const idStr &a, const char *b );
	friend bool			operator!=( const char *a, const idStr &b );

						// case sensitive compare
	int					Cmp( const char *text ) const;
	int					Cmpn( const char *text, int n ) const;
	int					CmpPrefix( const char *text ) const;

						// case insensitive compare
	int					Icmp( const char *text ) const;
	int					Icmpn( const char *text, int n ) const;
	int					IcmpPrefix( const char *text ) const;

						// case insensitive compare ignoring color
	int					IcmpNoColor( const char *text ) const;

						// compares paths and makes sure folders come first
	int					IcmpPath( const char *text ) const;
	int					IcmpnPath( const char *text, int n ) const;
	int					IcmpPrefixPath( const char *text ) const;

	int					Length( void ) const;
	int					Allocated( void ) const;
	void				Empty( void );
	bool				IsEmpty( void ) const;
	void				Clear( void );
	void				Append( const char a );
	void				Append( const idStr &text );
	void				Append( const char *text );
	void				Append( const char *text, int len );
	void				Insert( const char a, int index );
	void				Insert( const char *text, int index );
	void				ToLower( void );
	void				ToUpper( void );
	bool				IsNumeric( void ) const;
	bool				IsColor( void ) const;
	bool				HasLower( void ) const;
	bool				HasUpper( void ) const;
	int					LengthWithoutColors( void ) const;
	idStr &				RemoveColors( void );
	void				CapLength( int );
	void				Fill( const char ch, int newlen );

	int					Find( const char c, int start = 0, int end = -1 ) const;
	int					Find( const char *text, bool casesensitive = true, int start = 0, int end = -1 ) const;
	bool				Filter( const char *filter, bool casesensitive ) const;
	int					Last( const char c ) const;						// return the index to the last occurance of 'c', returns -1 if not found
	const char *		Left( int len, idStr &result ) const;			// store the leftmost 'len' characters in the result
	const char *		Right( int len, idStr &result ) const;			// store the rightmost 'len' characters in the result
	const char *		Mid( int start, int len, idStr &result ) const;	// store 'len' characters starting at 'start' in result
	idStr				Left( int len ) const;							// return the leftmost 'len' characters
	idStr				Right( int len ) const;							// return the rightmost 'len' characters
	idStr				Mid( int start, int len ) const;				// return 'len' characters starting at 'start'
	void				StripLeading( const char c );					// strip char from front as many times as the char occurs
	void				StripLeading( const char *string );				// strip string from front as many times as the string occurs
	bool				StripLeadingOnce( const char *string );			// strip string from front just once if it occurs
	void				StripTrailing( const char c );					// strip char from end as many times as the char occurs
	void				StripTrailing( const char *string );			// strip string from end as many times as the string occurs
	bool				StripTrailingOnce( const char *string );		// strip string from end just once if it occurs
	void				Strip( const char c );							// strip char from front and end as many times as the char occurs
	void				Strip( const char *string );					// strip string from front and end as many times as the string occurs
	void				StripTrailingWhitespace( void );				// strip trailing white space characters
	idStr &				StripQuotes( void );							// strip quotes around string
	void				Replace( const char *old, const char *nw );

	// file name methods
	int					FileNameHash( void ) const;						// hash key for the filename (skips extension)
	idStr &				BackSlashesToSlashes( void );					// convert slashes
	idStr &				SetFileExtension( const char *extension );		// set the given file extension
	idStr &				StripFileExtension( void );						// remove any file extension
	idStr &				StripAbsoluteFileExtension( void );				// remove any file extension looking from front (useful if there are multiple .'s)
	idStr &				DefaultFileExtension( const char *extension );	// if there's no file extension use the default
	idStr &				DefaultPath( const char *basepath );			// if there's no path use the default
	void				AppendPath( const char *text );					// append a partial path
	idStr &				StripFilename( void );							// remove the filename from a path
	idStr &				StripPath( void );								// remove the path from the filename
	void				ExtractFilePath( idStr &dest ) const;			// copy the file path to another string
	void				ExtractFileName( idStr &dest ) const;			// copy the filename to another string
	void				ExtractFileBase( idStr &dest ) const;			// copy the filename minus the extension to another string
	void				ExtractFileExtension( idStr &dest ) const;		// copy the file extension to another string
	bool				CheckExtension( const char *ext );

	// char * methods to replace library functions
	static int			Length( const char *s );
	static char *		ToLower( char *s );
	static char *		ToUpper( char *s );
	static bool			IsNumeric( const char *s );
	static bool			IsColor( const char *s );
	static bool			HasLower( const char *s );
	static bool			HasUpper( const char *s );
	static int			LengthWithoutColors( const char *s );
	static char *		RemoveColors( char *s );
	static int			Cmp( const char *s1, const char *s2 );
	static int			Cmpn( const char *s1, const char *s2, int n );
	static int			Icmp( const char *s1, const char *s2 );
	static int			Icmpn( const char *s1, const char *s2, int n );
	static int			IcmpNoColor( const char *s1, const char *s2 );
	static int			IcmpPath( const char *s1, const char *s2 );			// compares paths and makes sure folders come first
	static int			IcmpnPath( const char *s1, const char *s2, int n );	// compares paths and makes sure folders come first
	static void			Append( char *dest, int size, const char *src );
	static void			Copynz( char *dest, const char *src, int destsize );
	static int			snPrintf( char *dest, int size, const char *fmt, ... ) GDR_ATTRIBUTE((format(printf,3,4)));
	static int			vsnPrintf( char *dest, int size, const char *fmt, va_list argptr );
	static int			FindChar( const char *str, const char c, int start = 0, int end = -1 );
	static int			FindText( const char *str, const char *text, bool casesensitive = true, int start = 0, int end = -1 );
	static bool			Filter( const char *filter, const char *name, bool casesensitive );
	static void			StripMediaName( const char *name, idStr &mediaName );
	static bool			CheckExtension( const char *name, const char *ext );
	static const char *	FloatArrayToString( const float *array, const int length, const int precision );

	// hash keys
	static int			Hash( const char *string );
	static int			Hash( const char *string, int length );
	static int			IHash( const char *string );					// case insensitive
	static int			IHash( const char *string, int length );		// case insensitive

	// character methods
	static char			ToLower( char c );
	static char			ToUpper( char c );
	static bool			CharIsPrintable( int c );
	static bool			CharIsLower( int c );
	static bool			CharIsUpper( int c );
	static bool			CharIsAlpha( int c );
	static bool			CharIsNumeric( int c );
	static bool			CharIsNewLine( char c );
	static bool			CharIsTab( char c );

	friend int			sprintf( idStr &dest, const char *fmt, ... );
	friend int			vsprintf( idStr &dest, const char *fmt, va_list ap );

	void				ReAllocate( int amount, bool keepold );				// reallocate string data buffer
	void				FreeData( void );									// free allocated string memory

						// format value in the given measurement with the best unit, returns the best unit
	int					BestUnit( const char *format, float value, Measure_t measure );
						// format value in the requested unit and measurement
	void				SetUnit( const char *format, float value, int unit, Measure_t measure );

	static void			InitMemory( void );
	static void			ShutdownMemory( void );
	static void			PurgeMemory( void );
	static void			ShowMemoryUsage_f( const idCmdArgs &args );

	int					DynamicMemoryUsed() const;
	static idStr		FormatNumber( int number );

protected:
	int					len;
	char *				data;
	int					alloced;
	char				baseBuffer[ STR_ALLOC_BASE ];

	void				Init( void );										// initialize string using base buffer
	void				EnsureAlloced( int amount, bool keepold = true );	// ensure string data buffer is large anough
};

GDR_INLINE void idStr::EnsureAlloced( int amount, bool keepold ) {
	if ( amount > alloced ) {
		ReAllocate( amount, keepold );
	}
}

GDR_INLINE void idStr::Init( void ) {
	len = 0;
	alloced = STR_ALLOC_BASE;
	data = baseBuffer;
	data[ 0 ] = '\0';
#ifdef ID_DEBUG_UNINITIALIZED_MEMORY
	memset( baseBuffer, 0, sizeof( baseBuffer ) );
#endif
}

GDR_INLINE idStr::idStr( void ) {
	Init();
}

GDR_INLINE idStr::idStr( const idStr &text ) {
	int l;

	Init();
	l = text.Length();
	EnsureAlloced( l + 1 );
	strcpy( data, text.data );
	len = l;
}

GDR_INLINE idStr::idStr( const idStr &text, int start, int end ) {
	int i;
	int l;

	Init();
	if ( end > text.Length() ) {
		end = text.Length();
	}
	if ( start > text.Length() ) {
		start = text.Length();
	} else if ( start < 0 ) {
		start = 0;
	}

	l = end - start;
	if ( l < 0 ) {
		l = 0;
	}

	EnsureAlloced( l + 1 );

	for ( i = 0; i < l; i++ ) {
		data[ i ] = text[ start + i ];
	}

	data[ l ] = '\0';
	len = l;
}

GDR_INLINE idStr::idStr( const char *text ) {
	int l;

	Init();
	if ( text ) {
		l = strlen( text );
		EnsureAlloced( l + 1 );
		strcpy( data, text );
		len = l;
	}
}

GDR_INLINE idStr::idStr( const char *text, int start, int end ) {
	int i;
	int l = strlen( text );

	Init();
	if ( end > l ) {
		end = l;
	}
	if ( start > l ) {
		start = l;
	} else if ( start < 0 ) {
		start = 0;
	}

	l = end - start;
	if ( l < 0 ) {
		l = 0;
	}

	EnsureAlloced( l + 1 );

	for ( i = 0; i < l; i++ ) {
		data[ i ] = text[ start + i ];
	}

	data[ l ] = '\0';
	len = l;
}

GDR_INLINE idStr::idStr( const bool b ) {
	Init();
	EnsureAlloced( 2 );
	data[ 0 ] = b ? '1' : '0';
	data[ 1 ] = '\0';
	len = 1;
}

GDR_INLINE idStr::idStr( const char c ) {
	Init();
	EnsureAlloced( 2 );
	data[ 0 ] = c;
	data[ 1 ] = '\0';
	len = 1;
}

GDR_INLINE idStr::idStr( const int i ) {
	char text[ 64 ];
	int l;

	Init();
	l = sprintf( text, "%d", i );
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

GDR_INLINE idStr::idStr( const unsigned u ) {
	char text[ 64 ];
	int l;

	Init();
	l = sprintf( text, "%u", u );
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

GDR_INLINE idStr::idStr( const float f ) {
	char text[ 64 ];
	int l;

	Init();
	l = idStr::snPrintf( text, sizeof( text ), "%f", f );
	while( l > 0 && text[l-1] == '0' ) text[--l] = '\0';
	while( l > 0 && text[l-1] == '.' ) text[--l] = '\0';
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

GDR_INLINE idStr::~idStr( void ) {
	FreeData();
}

GDR_INLINE size_t idStr::Size( void ) const {
	return sizeof( *this ) + Allocated();
}

GDR_INLINE const char *idStr::c_str( void ) const {
	return data;
}

GDR_INLINE idStr::operator const char *( void ) {
	return c_str();
}

GDR_INLINE idStr::operator const char *( void ) const {
	return c_str();
}

#pragma GCC diagnostic push
// shut up GCC's stupid "warning: assuming signed overflow does not occur when assuming that
// (X - c) > X is always false [-Wstrict-overflow]"
#pragma GCC diagnostic ignored "-Wstrict-overflow"
GDR_INLINE char idStr::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[ index ];
}

GDR_INLINE char &idStr::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[ index ];
}
#pragma GCC diagnostic pop

GDR_INLINE void idStr::operator=( const idStr &text ) {
	if (&text == this) {
		return;
	}

	int l;

	l = text.Length();
	EnsureAlloced( l + 1, false );
	memcpy( data, text.data, l );
	data[l] = '\0';
	len = l;
}

GDR_INLINE idStr operator+( const idStr &a, const idStr &b ) {
	idStr result( a );
	result.Append( b );
	return result;
}

GDR_INLINE idStr operator+( const idStr &a, const char *b ) {
	idStr result( a );
	result.Append( b );
	return result;
}

GDR_INLINE idStr operator+( const char *a, const idStr &b ) {
	idStr result( a );
	result.Append( b );
	return result;
}

GDR_INLINE idStr operator+( const idStr &a, const bool b ) {
	idStr result( a );
	result.Append( b ? "true" : "false" );
	return result;
}

GDR_INLINE idStr operator+( const idStr &a, const char b ) {
	idStr result( a );
	result.Append( b );
	return result;
}

GDR_INLINE idStr operator+( const idStr &a, const float b ) {
	char	text[ 64 ];
	idStr	result( a );

	sprintf( text, "%f", b );
	result.Append( text );

	return result;
}

GDR_INLINE idStr operator+( const idStr &a, const int b ) {
	char	text[ 64 ];
	idStr	result( a );

	sprintf( text, "%d", b );
	result.Append( text );

	return result;
}

GDR_INLINE idStr operator+( const idStr &a, const unsigned b ) {
	char	text[ 64 ];
	idStr	result( a );

	sprintf( text, "%u", b );
	result.Append( text );

	return result;
}

GDR_INLINE idStr &idStr::operator+=( const float a ) {
	char text[ 64 ];

	sprintf( text, "%f", a );
	Append( text );

	return *this;
}

GDR_INLINE idStr &idStr::operator+=( const int a ) {
	char text[ 64 ];

	sprintf( text, "%d", a );
	Append( text );

	return *this;
}

GDR_INLINE idStr &idStr::operator+=( const unsigned a ) {
	char text[ 64 ];

	sprintf( text, "%u", a );
	Append( text );

	return *this;
}

GDR_INLINE idStr &idStr::operator+=( const idStr &a ) {
	Append( a );
	return *this;
}

GDR_INLINE idStr &idStr::operator+=( const char *a ) {
	Append( a );
	return *this;
}

GDR_INLINE idStr &idStr::operator+=( const char a ) {
	Append( a );
	return *this;
}

GDR_INLINE idStr &idStr::operator+=( const bool a ) {
	Append( a ? "true" : "false" );
	return *this;
}

GDR_INLINE bool operator==( const idStr &a, const idStr &b ) {
	return ( !idStr::Cmp( a.data, b.data ) );
}

GDR_INLINE bool operator==( const idStr &a, const char *b ) {
	assert( b );
	return ( !idStr::Cmp( a.data, b ) );
}

GDR_INLINE bool operator==( const char *a, const idStr &b ) {
	assert( a );
	return ( !idStr::Cmp( a, b.data ) );
}

GDR_INLINE bool operator!=( const idStr &a, const idStr &b ) {
	return !( a == b );
}

GDR_INLINE bool operator!=( const idStr &a, const char *b ) {
	return !( a == b );
}

GDR_INLINE bool operator!=( const char *a, const idStr &b ) {
	return !( a == b );
}

GDR_INLINE int idStr::Cmp( const char *text ) const {
	assert( text );
	return idStr::Cmp( data, text );
}

GDR_INLINE int idStr::Cmpn( const char *text, int n ) const {
	assert( text );
	return idStr::Cmpn( data, text, n );
}

GDR_INLINE int idStr::CmpPrefix( const char *text ) const {
	assert( text );
	return idStr::Cmpn( data, text, strlen( text ) );
}

GDR_INLINE int idStr::Icmp( const char *text ) const {
	assert( text );
	return idStr::Icmp( data, text );
}

GDR_INLINE int idStr::Icmpn( const char *text, int n ) const {
	assert( text );
	return idStr::Icmpn( data, text, n );
}

GDR_INLINE int idStr::IcmpPrefix( const char *text ) const {
	assert( text );
	return idStr::Icmpn( data, text, strlen( text ) );
}

GDR_INLINE int idStr::IcmpNoColor( const char *text ) const {
	assert( text );
	return idStr::IcmpNoColor( data, text );
}

GDR_INLINE int idStr::IcmpPath( const char *text ) const {
	assert( text );
	return idStr::IcmpPath( data, text );
}

GDR_INLINE int idStr::IcmpnPath( const char *text, int n ) const {
	assert( text );
	return idStr::IcmpnPath( data, text, n );
}

GDR_INLINE int idStr::IcmpPrefixPath( const char *text ) const {
	assert( text );
	return idStr::IcmpnPath( data, text, strlen( text ) );
}

GDR_INLINE int idStr::Length( void ) const {
	return len;
}

GDR_INLINE int idStr::Allocated( void ) const {
	if ( data != baseBuffer ) {
		return alloced;
	} else {
		return 0;
	}
}

GDR_INLINE void idStr::Empty( void ) {
	EnsureAlloced( 1 );
	data[ 0 ] = '\0';
	len = 0;
}

GDR_INLINE bool idStr::IsEmpty( void ) const {
	return ( idStr::Cmp( data, "" ) == 0 );
}

GDR_INLINE void idStr::Clear( void ) {
	FreeData();
	Init();
}

GDR_INLINE void idStr::Append( const char a ) {
	EnsureAlloced( len + 2 );
	data[ len ] = a;
	len++;
	data[ len ] = '\0';
}

GDR_INLINE void idStr::Append( const idStr &text ) {
	int newLen;
	int i;

	newLen = len + text.Length();
	EnsureAlloced( newLen + 1 );
	for ( i = 0; i < text.len; i++ ) {
		data[ len + i ] = text[ i ];
	}
	len = newLen;
	data[ len ] = '\0';
}

GDR_INLINE void idStr::Append( const char *text ) {
	int newLen;
	int i;

	if ( text ) {
		newLen = len + strlen( text );
		EnsureAlloced( newLen + 1 );
		for ( i = 0; text[ i ]; i++ ) {
			data[ len + i ] = text[ i ];
		}
		len = newLen;
		data[ len ] = '\0';
	}
}

GDR_INLINE void idStr::Append( const char *text, int l ) {
	int newLen;
	int i;

	if ( text && l ) {
		newLen = len + l;
		EnsureAlloced( newLen + 1 );
		for ( i = 0; text[ i ] && i < l; i++ ) {
			data[ len + i ] = text[ i ];
		}
		len = newLen;
		data[ len ] = '\0';
	}
}

GDR_INLINE void idStr::Insert( const char a, int index ) {
	int i, l;

	if ( index < 0 ) {
		index = 0;
	} else if ( index > len ) {
		index = len;
	}

	l = 1;
	EnsureAlloced( len + l + 1 );
	for ( i = len; i >= index; i-- ) {
		data[i+l] = data[i];
	}
	data[index] = a;
	len++;
}

GDR_INLINE void idStr::Insert( const char *text, int index ) {
	int i, l;

	if ( index < 0 ) {
		index = 0;
	} else if ( index > len ) {
		index = len;
	}

	l = strlen( text );
	EnsureAlloced( len + l + 1 );
	for ( i = len; i >= index; i-- ) {
		data[i+l] = data[i];
	}
	for ( i = 0; i < l; i++ ) {
		data[index+i] = text[i];
	}
	len += l;
}

GDR_INLINE void idStr::ToLower( void ) {
	for (int i = 0; data[i]; i++ ) {
		if ( CharIsUpper( data[i] ) ) {
			data[i] += ( 'a' - 'A' );
		}
	}
}

GDR_INLINE void idStr::ToUpper( void ) {
	for (int i = 0; data[i]; i++ ) {
		if ( CharIsLower( data[i] ) ) {
			data[i] -= ( 'a' - 'A' );
		}
	}
}

GDR_INLINE bool idStr::IsNumeric( void ) const {
	return idStr::IsNumeric( data );
}

GDR_INLINE bool idStr::IsColor( void ) const {
	return idStr::IsColor( data );
}

GDR_INLINE bool idStr::HasLower( void ) const {
	return idStr::HasLower( data );
}

GDR_INLINE bool idStr::HasUpper( void ) const {
	return idStr::HasUpper( data );
}

GDR_INLINE idStr &idStr::RemoveColors( void ) {
	idStr::RemoveColors( data );
	len = Length( data );
	return *this;
}

GDR_INLINE int idStr::LengthWithoutColors( void ) const {
	return idStr::LengthWithoutColors( data );
}

GDR_INLINE void idStr::CapLength( int newlen ) {
	if ( len <= newlen ) {
		return;
	}
	data[ newlen ] = 0;
	len = newlen;
}

GDR_INLINE void idStr::Fill( const char ch, int newlen ) {
	EnsureAlloced( newlen + 1 );
	len = newlen;
	memset( data, ch, len );
	data[ len ] = 0;
}

GDR_INLINE int idStr::Find( const char c, int start, int end ) const {
	if ( end == -1 ) {
		end = len;
	}
	return idStr::FindChar( data, c, start, end );
}

GDR_INLINE int idStr::Find( const char *text, bool casesensitive, int start, int end ) const {
	if ( end == -1 ) {
		end = len;
	}
	return idStr::FindText( data, text, casesensitive, start, end );
}

GDR_INLINE bool idStr::Filter( const char *filter, bool casesensitive ) const {
	return idStr::Filter( filter, data, casesensitive );
}

GDR_INLINE const char *idStr::Left( int len, idStr &result ) const {
	return Mid( 0, len, result );
}

GDR_INLINE const char *idStr::Right( int len, idStr &result ) const {
	if ( len >= Length() ) {
		result = *this;
		return result;
	}
	return Mid( Length() - len, len, result );
}

GDR_INLINE idStr idStr::Left( int len ) const {
	return Mid( 0, len );
}

#pragma GCC diagnostic push
// shut up GCC's stupid "warning: assuming signed overflow does not occur when assuming that
// (X - c) > X is always false [-Wstrict-overflow]"
#pragma GCC diagnostic ignored "-Wstrict-overflow"
GDR_INLINE idStr idStr::Right( int len ) const {
	if ( len >= Length() ) {
		return *this;
	}
	return Mid( Length() - len, len );
}
#pragma GCC diagnostic pop

GDR_INLINE void idStr::Strip( const char c ) {
	StripLeading( c );
	StripTrailing( c );
}

GDR_INLINE void idStr::Strip( const char *string ) {
	StripLeading( string );
	StripTrailing( string );
}

GDR_INLINE bool idStr::CheckExtension( const char *ext ) {
	return idStr::CheckExtension( data, ext );
}

GDR_INLINE int idStr::Length( const char *s ) {
	int i;
	for ( i = 0; s[i]; i++ ) {}
	return i;
}

GDR_INLINE char *idStr::ToLower( char *s ) {
	for ( int i = 0; s[i]; i++ ) {
		if ( CharIsUpper( s[i] ) ) {
			s[i] += ( 'a' - 'A' );
		}
	}
	return s;
}

GDR_INLINE char *idStr::ToUpper( char *s ) {
	for ( int i = 0; s[i]; i++ ) {
		if ( CharIsLower( s[i] ) ) {
			s[i] -= ( 'a' - 'A' );
		}
	}
	return s;
}

GDR_INLINE int idStr::Hash( const char *string ) {
	int i, hash = 0;
	for ( i = 0; *string != '\0'; i++ ) {
		hash += ( *string++ ) * ( i + 119 );
	}
	return hash;
}

GDR_INLINE int idStr::Hash( const char *string, int length ) {
	int i, hash = 0;
	for ( i = 0; i < length; i++ ) {
		hash += ( *string++ ) * ( i + 119 );
	}
	return hash;
}

GDR_INLINE int idStr::IHash( const char *string ) {
	int i, hash = 0;
	for( i = 0; *string != '\0'; i++ ) {
		hash += ToLower( *string++ ) * ( i + 119 );
	}
	return hash;
}

GDR_INLINE int idStr::IHash( const char *string, int length ) {
	int i, hash = 0;
	for ( i = 0; i < length; i++ ) {
		hash += ToLower( *string++ ) * ( i + 119 );
	}
	return hash;
}

GDR_INLINE bool idStr::IsColor( const char *s ) {
	return ( s[0] == Q_COLOR_ESCAPE && s[1] != '\0' && s[1] != ' ' );
}

GDR_INLINE char idStr::ToLower( char c ) {
	if ( c <= 'Z' && c >= 'A' ) {
		return ( c + ( 'a' - 'A' ) );
	}
	return c;
}

GDR_INLINE char idStr::ToUpper( char c ) {
	if ( c >= 'a' && c <= 'z' ) {
		return ( c - ( 'a' - 'A' ) );
	}
	return c;
}

GDR_INLINE bool idStr::CharIsPrintable( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c >= 0x20 && c <= 0x7E ) || ( c >= 0xA1 && c <= 0xFF );
}

GDR_INLINE bool idStr::CharIsLower( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c >= 'a' && c <= 'z' ) || ( c >= 0xE0 && c <= 0xFF );
}

GDR_INLINE bool idStr::CharIsUpper( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c <= 'Z' && c >= 'A' ) || ( c >= 0xC0 && c <= 0xDF );
}

GDR_INLINE bool idStr::CharIsAlpha( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
			 ( c >= 0xC0 && c <= 0xFF ) );
}

GDR_INLINE bool idStr::CharIsNumeric( int c ) {
	return ( c <= '9' && c >= '0' );
}

GDR_INLINE bool idStr::CharIsNewLine( char c ) {
	return ( c == '\n' || c == '\r' || c == '\v' );
}

GDR_INLINE bool idStr::CharIsTab( char c ) {
	return ( c == '\t' );
}

GDR_INLINE int idStr::DynamicMemoryUsed() const {
	return ( data == baseBuffer ) ? 0 : alloced;
}

#endif /* !__STR_H__ */