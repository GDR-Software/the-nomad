#include "../module_public.h"
#include "scriptstdstring.h"

// This function takes an input string and splits it into parts by looking
// for a specified delimiter. Example:
//
// string str = "A|B||D";
// Array<string>@ array = str.split("|");
//
// The resulting array has the following elements:
//
// {"A", "B", "", "D"}
//
// AngelScript signature:
// Array<string>@ string::split(const string_t& in delim) const
static CScriptArray *StringSplit(const string_t& delim, const string_t& str)
{
	// Obtain a pointer to the engine
	asIScriptContext *ctx = asGetActiveContext();
	asIScriptEngine *engine = ctx->GetEngine();

	// Create the array object
	CScriptArray *array = CScriptArray::Create( g_pModuleLib->GetScriptEngine()->GetTypeInfoByName( "string" ) );

	// Find the existence of the delimiter in the input string
	size_t pos = 0, prev = 0;
	asUINT count = 0;
	while( (pos = str.find(delim, prev)) != string_t::npos )
	{
		// Add the part to the array
		array->Resize(array->GetSize()+1);
		( (string_t *)array->At( count ) )->assign( &str[prev], pos - prev );

		// Find the next part
		count++;
		prev = pos + delim.length();
	}

	// Add the remaining part
	array->Resize(array->GetSize()+1);
	( (string_t *)array->At( count ) )->assign(&str[prev]);

	return array;
}

static void StringSplit_Generic(asIScriptGeneric *gen)
{
	// Get the arguments
	const string_t *str   = (string_t *)gen->GetObjectData();
	const string_t *delim = *(string_t **)gen->GetAddressOfArg(0);

	// Return the array by handle
	*(CScriptArray **)gen->GetAddressOfReturnLocation() = StringSplit(*delim, *str);
}



// This function takes as input an array of string handles as well as a
// delimiter and concatenates the array elements into one delimited string.
// Example:
//
// Array<string> array = {"A", "B", "", "D"};
// string str = join(array, "|");
//
// The resulting string is:
//
// "A|B||D"
//
// AngelScript signature:
// string join(const Array<string> &in array, const string_t& in delim)
static string_t StringJoin(const CScriptArray &array, const string_t& delim)
{
	// Create the new string
	string_t str = "";
	if( array.GetSize() )
	{
		int n;
		for( n = 0; n < (int)array.GetSize() - 1; n++ )
		{
			str += *(string_t *)array.At(n);
			str += delim;
		}

		// Add the last part
		str += *(string_t *)array.At(n);
	}

	return str;
}

static void StringJoin_Generic(asIScriptGeneric *gen)
{
	// Get the arguments
	CScriptArray  *array = *(CScriptArray **)gen->GetAddressOfArg(0);
	string_t *delim = *(string_t **)gen->GetAddressOfArg(1);

	// Return the string
	new(gen->GetAddressOfReturnLocation()) string_t(StringJoin(*array, *delim));
}

// This is where the utility functions are registered.
// The string type must have been registered first.
void RegisterStdStringUtils(asIScriptEngine *engine)
{
	int r;

	if( strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY") )
	{
		r = engine->RegisterObjectMethod("string", "array<string>@ split(const string& in) const", asFUNCTION(StringSplit_Generic), asCALL_GENERIC); assert(r >= 0);
		r = engine->RegisterGlobalFunction("string join(const array<string> &in, const string& in)", asFUNCTION(StringJoin_Generic), asCALL_GENERIC); assert(r >= 0);
	}
	else
	{
		r = engine->RegisterObjectMethod("string", "array<string>@ split(const string in) const", asFUNCTION(StringSplit), asCALL_CDECL_OBJLAST); assert(r >= 0);
		r = engine->RegisterGlobalFunction("string join(const array<string> &in, const string& in)", asFUNCTION(StringJoin), asCALL_CDECL); assert(r >= 0);
	}
}

END_AS_NAMESPACE
