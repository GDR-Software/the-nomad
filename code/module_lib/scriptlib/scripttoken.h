#ifndef __SCRIPT_TOKEN__
#define __SCRIPT_TOKEN__

#pragma once

#include "../module_public.h"

/*
===============================================================================

	CScriptToken is a token read from a file or memory with idLexer or idParser

===============================================================================
*/

// token types
#define TT_STRING					1		// string
#define TT_LITERAL					2		// literal
#define TT_NUMBER					3		// number
#define TT_NAME						4		// name
#define TT_PUNCTUATION				5		// punctuation

// number sub types
#define TT_INTEGER					0x00001		// integer
#define TT_DECIMAL					0x00002		// decimal number
#define TT_HEX						0x00004		// hexadecimal number
#define TT_OCTAL					0x00008		// octal number
#define TT_BINARY					0x00010		// binary number
#define TT_LONG						0x00020		// long int
#define TT_UNSIGNED					0x00040		// unsigned int
#define TT_FLOAT					0x00080		// floating point number
#define TT_SINGLE_PRECISION			0x00100		// float
#define TT_DOUBLE_PRECISION			0x00200		// double
#define TT_EXTENDED_PRECISION		0x00400		// long double
#define TT_INFINITE					0x00800		// infinite 1.#INF
#define TT_INDEFINITE				0x01000		// indefinite 1.#IND
#define TT_NAN						0x02000		// NaN
#define TT_IPADDRESS				0x04000		// ip address
#define TT_IPPORT					0x08000		// ip port
#define TT_VALUESVALID				0x10000		// set if intvalue and floatvalue are valid

// string sub type is the length of the string
// literal sub type is the ASCII code
// punctuation sub type is the punctuation id
// name sub type is the length of the name

class CScriptToken : public string_t
{
    friend class CScriptParser;
    friend class CScriptLexer;
public:
	int				type;								// token type
	int				subtype;							// token sub type
	uint32_t		line;								// line in script the token was on
	uint32_t		linesCrossed;						// number of lines crossed in white space before token
    uint32_t		flags;								// token flags, used for recursive defines

public:
	CScriptToken( void );
	CScriptToken( const CScriptToken *token );
	~CScriptToken();

	void operator=( const string_t& text );
	void operator=( const char *text );

	double GetDoubleValue( void );				    // double value of TT_NUMBER
	float GetFloatValue( void );				    // float value of TT_NUMBER
	uint32_t GetUnsignedIntValue( void );		    // unsigned int value of TT_NUMBER
	int32_t GetIntValue( void );				    // int value of TT_NUMBER
    uint32_t WhiteSpaceBeforeToken( void ) const;   // returns length of whitespace before token
	void ClearTokenWhiteSpace( void );              // forget whitespace before token

	void NumberValue( void ); // calculate values for a TT_NUMBER

private:
    uint32_t intvalue;						// integer value
	double floatvalue;				        // floating point value
	const char *whiteSpaceStart_p;		    // start of white space before token, only used by idLexer
	const char *whiteSpaceEnd_p;		    // end of white space before token, only used by idLexer
	CScriptToken *next;					    // next token in chain, only used by idParser

	void			AppendDirty( const char a );		// append character without adding trailing zero
};

ID_INLINE CScriptToken::CScriptToken( void ) {
}

ID_INLINE CScriptToken::CScriptToken( const CScriptToken *token ) {
	*this = *token;
}

ID_INLINE CScriptToken::~CScriptToken( void ) {
}

ID_INLINE void CScriptToken::operator=( const char *text) {
	*static_cast<idStr *>(this) = text;
}

ID_INLINE void CScriptToken::operator=( const idStr& text ) {
	*static_cast<idStr *>(this) = text;
}

ID_INLINE double CScriptToken::GetDoubleValue( void ) {
	if ( type != TT_NUMBER ) {
		return 0.0;
	}
	if ( !(subtype & TT_VALUESVALID) ) {
		NumberValue();
	}
	return floatvalue;
}

ID_INLINE float CScriptToken::GetFloatValue( void ) {
	return (float) GetDoubleValue();
}

ID_INLINE unsigned int	CScriptToken::GetUnsignedIntValue( void ) {
	if ( type != TT_NUMBER ) {
		return 0;
	}
	if ( !(subtype & TT_VALUESVALID) ) {
		NumberValue();
	}
	return intvalue;
}

ID_INLINE int CScriptToken::GetIntValue( void ) {
	return (int) GetUnsignedIntValue();
}

ID_INLINE int CScriptToken::WhiteSpaceBeforeToken( void ) const {
	return ( whiteSpaceEnd_p > whiteSpaceStart_p );
}

ID_INLINE void CScriptToken::AppendDirty( const char a ) {
	EnsureAlloced( len + 2, true );
	data[len++] = a;
}

#endif /* !__SCRIPT_TOKEN__ */
