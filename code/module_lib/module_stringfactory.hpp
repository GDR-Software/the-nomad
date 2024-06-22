#ifndef __MODULE_STRINGFACTORY__
#define __MODULE_STRINGFACTORY__

#pragma once

#include "module_public.h"
#include "../engine/n_threads.h"
#include <EASTL/unordered_set.h>
#include <EASTL/bonus/lru_cache.h>

// FIXME: this is an abomination
//using CStringCache = eastl::lru_cache<string_t, int32_t, eastl::allocator, eastl::list<string_t, eastl::allocator>, 
//	eastl::unordered_map<string_t, eastl::pair<int32_t, eastl::list<string_t, eastl::allocator>::iterator>, eastl::hash<string_t>,
//	eastl::equal_to<string_t>, eastl::allocator, true>>;
using CStringCache = eastl::unordered_map<string_t, int32_t, eastl::hash<string_t>,
	eastl::equal_to<string_t>, eastl::allocator, true>;
using string_hash_t = string_t;

class CModuleStringFactory : public asIStringFactory
{
public:
	CModuleStringFactory( void ) {
		m_StringCache.reserve( 8192 );
	}
	~CModuleStringFactory() {
		m_StringCache.clear();
	}
	
	const void *GetStringConstant( const char *pData, asUINT nLength ) {
		PROFILE_FUNCTION();

		CThreadAutoLock<CThreadMutex> lock( m_hLock );

		const string_hash_t str( pData, nLength );
		auto it = m_StringCache.find( str );
		if ( it != m_StringCache.end() ) {
			it->second++;
		} else {
			it = m_StringCache.insert( CStringCache::value_type( str, 1 ) ).first;
		}
		return (const void *)&it->first;
	}
	int ReleaseStringConstant( const void *pStr ) {
		PROFILE_FUNCTION();

		int ret;
		
		if ( !pStr ) {
			Assert( pStr );
			return asERROR;
		}
		
		ret = asSUCCESS;
		
		CThreadAutoLock<CThreadMutex> lock( m_hLock );

		const string_hash_t& data = *(const string_t *)pStr;

		auto it = m_StringCache.find( data );
		if ( it == m_StringCache.end() ) {
			Con_Printf( COLOR_RED "[ERROR] ReleaseStringConstant: invalid string '%s'\n", data.c_str() );
			ret = asERROR;
		} else {
			it->second--;
			if ( !it->second ) {
				m_StringCache.erase( it );
			}
		}
		
		return ret;
	}
	int GetRawStringData( const void *pStr, char *pData, asUINT *nLength ) const {
		PROFILE_FUNCTION();

		if ( !pStr ) {
			Assert( pStr );
			return asERROR;
		}

		CThreadAutoLock<CThreadMutex> lock( *const_cast<CThreadMutex *>( &m_hLock ) );
		const string_t *data = (const string_t *)pStr;
		if ( nLength ) {
			*nLength = data->size();
		}
		if ( pData ) {
			memcpy( pData, data->data(), data->size() );
		}
		return asSUCCESS;
	}
	
	CThreadMutex m_hLock;
	CStringCache m_StringCache;
};

extern CModuleStringFactory *g_pStringFactory;

#endif