#include "rgl_local.h"

static world_t r_worldData;
static byte *fileBase;

static void R_LoadLights( const lump_t *lights )
{
	uint32_t count;
	maplight_t *in, *out;

	in = (maplight_t *)(fileBase + lights->fileofs);
	if ( !lights->length ) { // not strictly required
		return;
	}
	if (lights->length % sizeof(*in))
		ri.Error(ERR_DROP, "RE_LoadWorldMap: funny lump size (lights) in %s", r_worldData.name);
	
	count = lights->length / sizeof(*in);
	out = ri.Hunk_Alloc( sizeof(*out) * count, h_low );

	r_worldData.lights = out;
	r_worldData.numLights = count;

	memcpy(out, in, count*sizeof(*out));
}

static void R_LoadTiles( const lump_t *tiles )
{
	uint32_t count;
	maptile_t *in, *out;

	in = (maptile_t *)(fileBase + tiles->fileofs);
	if (tiles->length % sizeof(*in))
		ri.Error(ERR_DROP, "RE_LoadWorldMap: funny lump size (tiles) in %s", r_worldData.name);
	
	count = tiles->length / sizeof(*in);
	out = ri.Hunk_Alloc( sizeof(*out) * count, h_low );

	r_worldData.tiles = out;
	r_worldData.numTiles = count;

	memcpy(out, in, count*sizeof(*out));
}

static void R_LoadTileset( const lump_t *sprites, const tile2d_header_t *theader )
{
	uint32_t count, i;
	spriteCoord_t *in,*out;

	in = (spriteCoord_t *)(fileBase + sprites->fileofs);
	if (sprites->length % sizeof(*out))
		ri.Error(ERR_DROP, "RE_LoadWorldMap: funny lump size (tileset) in %s", r_worldData.name);
	
	count = sprites->length / sizeof(*in);
	out = ri.Hunk_Alloc( sizeof(*out) * count, h_low );

	r_worldData.sprites = out;
	r_worldData.numSprites = count;

	memcpy( out, in, count * sizeof( *out ) );
}

static void R_CalcSpriteTextureCoords( uint32_t x, uint32_t y, uint32_t spriteWidth, uint32_t spriteHeight,
	uint32_t sheetWidth, uint32_t sheetHeight, spriteCoord_t *texCoords )
{
	const vec2_t min = { (((float)x + 1) * (float)spriteWidth) / (float)sheetWidth,
		(((float)y + 1) * (float)spriteHeight) / (float)sheetHeight };
	const vec2_t max = { ((float)x * (float)spriteWidth) / (float)sheetWidth,
		((float)y * (float)spriteHeight) / (float)sheetHeight };

	(*texCoords)[0][0] = min[0];
	(*texCoords)[0][1] = max[1];

	(*texCoords)[1][0] = min[0];
	(*texCoords)[1][1] = min[1];

	(*texCoords)[2][0] = max[0];
	(*texCoords)[2][1] = min[1];
	
	(*texCoords)[3][0] = max[0];
	(*texCoords)[3][1] = max[1];
}

static void R_GenerateTexCoords( tile2d_info_t *info )
{
	uint32_t y, x;
	uint32_t i;
	uint32_t sheetWidth, sheetHeight;
	const texture_t *image;
	char texture[MAX_NPATH];
	vec3_t tmp1, tmp2;

	COM_StripExtension( info->texture, texture, sizeof( texture ) );
	if ( texture[ strlen( texture ) - 1 ] == '.' ) {
		texture[ strlen( texture ) - 1 ] = '\0';
	}

	R_FindImageFile( info->texture, IMGTYPE_COLORALPHA, IMGFLAG_NO_COMPRESSION | IMGFLAG_CLAMPTOEDGE );
	r_worldData.shader = R_GetShaderByHandle( RE_RegisterShader( texture ) );
	if ( r_worldData.shader == rg.defaultShader ) {
		ri.Error( ERR_DROP, "RE_LoadWorldMap: failed to load shader for '%s'", r_worldData.name );
	}

	image = r_worldData.shader->stages[0]->bundle[0].image[0];

	info->tileCountX = image->width / info->tileWidth;
	info->tileCountY = image->height / info->tileHeight;

	sheetWidth = info->tileCountX * info->tileWidth;
	sheetHeight = info->tileCountY * info->tileHeight;

	ri.Printf( PRINT_DEVELOPER, "Generating worldData tileset %ux%u:%ux%u, %u sprites\n", sheetWidth, sheetHeight,
		info->tileWidth, info->tileHeight, info->numTiles );

	for ( y = 0; y < info->tileCountY; y++ ) {
		for ( x = 0; x < info->tileCountX; x++ ) {
			R_CalcSpriteTextureCoords( x, y, info->tileWidth, info->tileHeight, sheetWidth, sheetHeight,
				&r_worldData.sprites[ y * info->tileCountX + x ] );
		}
	}

	for ( y = 0; y < r_worldData.height; y++ ) {
		for ( x = 0; x < r_worldData.width; x++ ) {
			VectorCopy2( r_worldData.tiles[ y * r_worldData.width + x ].texcoords[0],
				r_worldData.sprites[ r_worldData.tiles[ y * r_worldData.width + x ].index ][0] );
			VectorCopy2( r_worldData.tiles[ y * r_worldData.width + x ].texcoords[1],
				r_worldData.sprites[ r_worldData.tiles[ y * r_worldData.width + x ].index ][1] );
			VectorCopy2( r_worldData.tiles[ y * r_worldData.width + x ].texcoords[2],
				r_worldData.sprites[ r_worldData.tiles[ y * r_worldData.width + x ].index ][2] );
			VectorCopy2( r_worldData.tiles[ y * r_worldData.width + x ].texcoords[3],
				r_worldData.sprites[ r_worldData.tiles[ y * r_worldData.width + x ].index ][0] );
//            memcpy( r_worldData.tiles[ y * r_worldData.width + x ].texcoords,
//                r_worldData.sprites[ r_worldData.tiles[ y * r_worldData.width + x ].index ], sizeof( spriteCoord_t ) );

			VectorCopy2( r_worldData.vertices[ ( y * r_worldData.width + x ) + 0].uv,
				r_worldData.tiles[y * r_worldData.width + x].texcoords[0] );
			VectorCopy2( r_worldData.vertices[( y * r_worldData.width + x ) + 1].uv,
				r_worldData.tiles[y * r_worldData.width + x].texcoords[1] );
			VectorCopy2( r_worldData.vertices[( y * r_worldData.width + x ) + 2].uv,
				r_worldData.tiles[y * r_worldData.width + x].texcoords[2] );
			VectorCopy2( r_worldData.vertices[( y * r_worldData.width + x ) + 3].uv,
				r_worldData.tiles[y * r_worldData.width + x].texcoords[3] );
		}
	}
}

float stsvco_valenceScore( const int numTris ) {
    return 2*powf( numTris, -.5f );
}

static void R_OptimizeVertexCache( void )
{
	glIndex_t *pIndices;
	drawVert_t *pVerts;
	uint64_t i, j;

	typedef struct {
		int numAdjecentTris;
		int numTrisLeft;
		int triListIndex;
		int cacheIndex;
	} vertex_t;

	typedef struct {
		int vertices[3];
		qboolean drawn;
	} triangle_t;

	vertex_t *vertices;
	triangle_t *triangles;
	uint32_t *vertToTri;

	const int numTriangles = r_worldData.numIndices / 3;

	pIndices = r_worldData.indices;
	pVerts = r_worldData.vertices;

	vertices = ri.Hunk_AllocateTempMemory( r_worldData.numVertices * sizeof( *vertices ) );
	triangles = ri.Hunk_AllocateTempMemory( numTriangles * sizeof( *triangles ) );
	
	for ( i = 0; i < r_worldData.numVertices; ++i ) {
		vertices[i].numAdjecentTris = 0;
		vertices[i].numTrisLeft = 0;
		vertices[i].triListIndex = 0;
		vertices[i].cacheIndex = -1;
	}
	
	for ( i = 0; i < numTriangles; ++i ) {
		for ( j = 0; j < 3; ++j ) {
			triangles[i].vertices[j] = pIndices[ i * 3 + j ];
			++vertices[triangles[i].vertices[j]].numAdjecentTris;
		}
		
		triangles[i].drawn = false;
	}
	
	// Loop through and find index for the tri list for vertex->tri
	for( i = 1; i < r_worldData.numVertices; ++i ) {
		vertices[i].triListIndex = vertices[ i - 1 ].triListIndex+vertices[ i - 1 ].numAdjecentTris;
	}
	
	const int numVertToTri = vertices[ r_worldData.numVertices - 1 ].triListIndex+vertices[ r_worldData.numVertices - 1 ].numAdjecentTris;
	vertToTri = ri.Hunk_AllocateTempMemory( numVertToTri * sizeof( *vertToTri ) );
	
	for ( i = 0; i < numTriangles; ++i ) {
		for ( j = 0; j < 3; ++j ) {
			const int index = triangles[ i ].vertices[ j ];
			const int triListIndex = vertices[ index ].triListIndex + vertices[index].numTrisLeft;
			vertToTri[ triListIndex ] = i;
			++vertices[index].numTrisLeft;
		}
	}
	
	// Make LRU cache
	const int   LRUCacheSize = 64;
	int         *LRUCache = ri.Hunk_AllocateTempMemory( LRUCacheSize * sizeof( int ) );
	float       *scoring = ri.Hunk_AllocateTempMemory( LRUCacheSize * sizeof(float ) );
	
	for( i = 0; i < LRUCacheSize; ++i ) {
		LRUCache[i] = -1;
		scoring[i] = -1.0f;
	}
	
	int numIndicesDone = 0;
	while ( numIndicesDone != r_worldData.numIndices ) {
		// update vertex scoring
		for ( i = 0; i < LRUCacheSize && LRUCache[i] >= 0; ++i ) {
			int vertexIndex = LRUCache[i];
			if ( vertexIndex != -1 ) {
				// Do scoring based on cache position
				if( i < 3 ) {
					scoring[i] = 0.75f;
				} else {
					const float scaler = 1.0f / ( LRUCacheSize - 3 );
					const float scoreBase = 1.0f - ( i - 3 ) * scaler;
					scoring[i] = powf ( scoreBase, 1.5f );
				};
				// Add score based on tris left for vertex (valence score)
				const int numTrisLeft = vertices[ vertexIndex ].numTrisLeft;
				scoring[i] += stsvco_valenceScore(numTrisLeft);
			}
		}
		// find triangle to draw based on score
		// Update score for all triangles with vertexes in cache
		int triangleToDraw = -1;
		float bestTriScore = 0.0f;
		for ( i = 0; i < LRUCacheSize && LRUCache[i] >= 0; ++i ) {
			const int vIndex = LRUCache[i];
			
			if( vertices[vIndex].numTrisLeft > 0 ) {
				for(int t = 0; t < vertices[vIndex].numAdjecentTris; ++t) {
					const int tIndex = vertToTri[ vertices[vIndex].triListIndex + t];
					if( !triangles[ tIndex ].drawn ) {
						float triScore = .0f;
						for(int v = 0; v < 3; ++v) {
							const int cacheIndex = vertices[triangles[ tIndex ].vertices[v]].cacheIndex;
							if( cacheIndex >= 0) {
								triScore += scoring[ cacheIndex ];
							}
						}
						
						if( triScore > bestTriScore ) {
							triangleToDraw = tIndex;
							bestTriScore = triScore;
						}
					}
				}
			}
		}
		
		if( triangleToDraw < 0 ) {
			// No triangle can be found by heuristic, simply choose first and best
			for(int t = 0; t < numTriangles; ++t ) {
				if( !triangles[t].drawn ) {
					//compute valence for each vertex
					float triScore = .0f;
					for(int v = 0; v < 3; ++v ) {
						const unsigned int vertexIndex = triangles[t].vertices[v];
						// Add score based on tris left for vertex (valence score)
						const int numTrisLeft = vertices[ vertexIndex ].numTrisLeft;
						triScore += stsvco_valenceScore(numTrisLeft);
					}
					if( triScore >= bestTriScore ) {
						triangleToDraw = t;
						bestTriScore = triScore;
					}
				}
			}
		}
		
		// update cache
		int cacheIndex = 3;
		int numVerticesFound = 0;
		while( LRUCache[numVerticesFound] >= 0 && numVerticesFound < 3 && cacheIndex < LRUCacheSize ) {
			bool topOfCacheInTri = false;
			// Check if index is in triangle
			for ( i = 0; i < 3; ++i ) {
				if( triangles[triangleToDraw].vertices[i] == LRUCache[numVerticesFound]) {
					++numVerticesFound;
					topOfCacheInTri = true;
					break;
				}
			}
			
			if(!topOfCacheInTri) {
				int topIndex = LRUCache[ numVerticesFound ];
				for( int j = numVerticesFound; j < 2; ++j) {
					LRUCache[j] = LRUCache[j+1];
				}
				LRUCache[2] = LRUCache[cacheIndex];
				if( LRUCache[2] >= 0) vertices[ LRUCache[2] ].cacheIndex = -1;
				
				LRUCache[cacheIndex] = topIndex;
				if(topIndex >= 0) vertices[ topIndex ].cacheIndex = cacheIndex;
				++cacheIndex;
			}
		}
		
		// Set triangle as drawn
		for(int v = 0; v < 3; ++v) {
			const int index = triangles[triangleToDraw].vertices[v];
			
			LRUCache[ v ] = index;
			vertices[ index ].cacheIndex = v;
			
			--vertices[index].numTrisLeft;
			
			pIndices[ numIndicesDone ] = index;
			++numIndicesDone;
		}
		
		
		triangles[ triangleToDraw ].drawn = true;
	}
	
	// Memory cleanup
	ri.Hunk_FreeTempMemory( scoring );
	ri.Hunk_FreeTempMemory( LRUCache );
	ri.Hunk_FreeTempMemory( vertToTri );
	ri.Hunk_FreeTempMemory( triangles );
	ri.Hunk_FreeTempMemory( vertices );
}

float R_CalcCacheEfficiency( void )
{
	uint32_t numCacheMisses;
	int *cache;
	int i;
	
	cache = ri.Hunk_AllocateTempMemory( 64 * sizeof( int ) );
    
    for ( i = 0; i < 64; ++i ) {
		cache[i] = -1;
	}
    
    for ( i = 0; i < r_worldData.numIndices; ++i ) {
        const uint32_t index = r_worldData.indices[i];

        // check if vertex in cache
        qboolean foundInCache = qfalse;
        for ( int c = 0; c < 64 && cache[c] >= 0 && !foundInCache; ++c) {
            if ( cache[ c ] == index ) {
				foundInCache = qtrue;
			}
        }
        
        if ( !foundInCache ) {
            ++numCacheMisses;
            for ( int c = 64 - 1; c  >= 1; --c ) {
                cache[c] = cache[ c-1 ];
            }
            cache[0] = index;
        }
    }
    
    ri.Hunk_FreeTempMemory( cache );
    
    return (float)numCacheMisses / (float)( r_worldData.numIndices / 3 );
};

void R_InitWorldBuffer( void )
{
	maptile_t *tile;
	uint32_t numSurfs, i;
	uint32_t offset;
	vertexAttrib_t *attribs;

	r_worldData.numIndices = r_worldData.width * r_worldData.height * 6;
	r_worldData.numVertices = r_worldData.width * r_worldData.height * 4;

	r_worldData.indices = ri.Hunk_Alloc( sizeof( glIndex_t ) * r_worldData.numIndices, h_low );
	r_worldData.vertices = ri.Hunk_Alloc( sizeof( drawVert_t ) * r_worldData.numVertices, h_low );

	// cache the indices so that we aren't calculating these every frame (there could be thousands)
	for ( i = 0, offset = 0; i < r_worldData.numIndices; i += 6, offset += 4 ) {
		r_worldData.indices[i + 0] = offset + 0;
		r_worldData.indices[i + 1] = offset + 1;
		r_worldData.indices[i + 2] = offset + 2;

		r_worldData.indices[i + 3] = offset + 3;
		r_worldData.indices[i + 4] = offset + 2;
		r_worldData.indices[i + 5] = offset + 0;
	}

	ri.Printf( PRINT_INFO, "Optimizing vertex cache... (Current cache misses: %f)\n", R_CalcCacheEfficiency() );
	R_OptimizeVertexCache();
	ri.Printf( PRINT_INFO, "Optimized cache misses: %f\n", R_CalcCacheEfficiency() );

	r_worldData.buffer = R_AllocateBuffer( "worldDrawBuffer", NULL, r_worldData.numVertices * sizeof(drawVert_t), NULL,
		r_worldData.numIndices * sizeof( glIndex_t ), BUFFER_FRAME );
	
	attribs = r_worldData.buffer->attribs;

	attribs[ATTRIB_INDEX_POSITION].enabled		= qtrue;
	attribs[ATTRIB_INDEX_TEXCOORD].enabled		= qtrue;
	attribs[ATTRIB_INDEX_COLOR].enabled			= qtrue;
	attribs[ATTRIB_INDEX_NORMAL].enabled		= qtrue;
	attribs[ATTRIB_INDEX_WORLDPOS].enabled      = qtrue;
	attribs[ATTRIB_INDEX_TANGENT].enabled       = qtrue;

	attribs[ATTRIB_INDEX_POSITION].count		= 3;
	attribs[ATTRIB_INDEX_TEXCOORD].count		= 2;
	attribs[ATTRIB_INDEX_COLOR].count			= 4;
	attribs[ATTRIB_INDEX_NORMAL].count			= 4;
	attribs[ATTRIB_INDEX_WORLDPOS].count        = 3;
	attribs[ATTRIB_INDEX_TANGENT].count			= 4;

	attribs[ATTRIB_INDEX_POSITION].type			= GL_FLOAT;
	attribs[ATTRIB_INDEX_TEXCOORD].type			= GL_FLOAT;
	attribs[ATTRIB_INDEX_COLOR].type			= GL_UNSIGNED_SHORT;
	attribs[ATTRIB_INDEX_NORMAL].type			= GL_SHORT;
	attribs[ATTRIB_INDEX_WORLDPOS].type         = GL_FLOAT;
	attribs[ATTRIB_INDEX_TANGENT].type          = GL_SHORT;

	attribs[ATTRIB_INDEX_POSITION].index		= ATTRIB_INDEX_POSITION;
	attribs[ATTRIB_INDEX_TEXCOORD].index		= ATTRIB_INDEX_TEXCOORD;
	attribs[ATTRIB_INDEX_COLOR].index			= ATTRIB_INDEX_COLOR;
	attribs[ATTRIB_INDEX_NORMAL].index			= ATTRIB_INDEX_NORMAL;
	attribs[ATTRIB_INDEX_WORLDPOS].index        = ATTRIB_INDEX_WORLDPOS;
	attribs[ATTRIB_INDEX_TANGENT].index         = ATTRIB_INDEX_TANGENT;

	attribs[ATTRIB_INDEX_POSITION].normalized	= GL_FALSE;
	attribs[ATTRIB_INDEX_TEXCOORD].normalized	= GL_FALSE;
	attribs[ATTRIB_INDEX_COLOR].normalized		= GL_TRUE;
	attribs[ATTRIB_INDEX_NORMAL].normalized		= GL_TRUE;
	attribs[ATTRIB_INDEX_WORLDPOS].normalized   = GL_FALSE;
	attribs[ATTRIB_INDEX_TANGENT].normalized    = GL_TRUE;

	attribs[ATTRIB_INDEX_POSITION].offset		= offsetof( drawVert_t, xyz );
	attribs[ATTRIB_INDEX_TEXCOORD].offset		= offsetof( drawVert_t, uv );
	attribs[ATTRIB_INDEX_COLOR].offset			= offsetof( drawVert_t, color );
	attribs[ATTRIB_INDEX_NORMAL].offset			= offsetof( drawVert_t, normal );
	attribs[ATTRIB_INDEX_WORLDPOS].offset       = offsetof( drawVert_t, worldPos );
	attribs[ATTRIB_INDEX_TANGENT].offset        = offsetof( drawVert_t, tangent );

	attribs[ATTRIB_INDEX_POSITION].stride		= sizeof( drawVert_t );
	attribs[ATTRIB_INDEX_TEXCOORD].stride		= sizeof( drawVert_t );
	attribs[ATTRIB_INDEX_COLOR].stride			= sizeof( drawVert_t );
	attribs[ATTRIB_INDEX_NORMAL].stride			= sizeof( drawVert_t );
	attribs[ATTRIB_INDEX_WORLDPOS].stride       = sizeof( drawVert_t );
	attribs[ATTRIB_INDEX_WORLDPOS].stride       = sizeof( drawVert_t );
}

static void R_ProcessLights( void )
{
	shaderLight_t *lights;
	const maplight_t *data;
	uint32_t i;

	if ( !r_worldData.numLights ) {
		return;
	}

	ri.Printf( PRINT_DEVELOPER, "Processing %u lights\n", r_worldData.numLights );

	lights = (shaderLight_t *)rg.lightData->data;
	data = r_worldData.lights;
	for ( i = 0; i < r_worldData.numLights; i++ ) {
		VectorCopy4( lights[i].color, data[i].color );
		VectorCopy2( lights[i].origin, data[i].origin );
		lights[i].brightness = data[i].brightness;
		lights[i].range = data[i].range;
		lights[i].constant = data[i].constant;
		lights[i].linear = data[i].linear;
		lights[i].quadratic = data[i].quadratic;
		lights[i].type = data[i].type;
	}

//    memcpy( rg.lightData->data, lights, sizeof( *lights ) * r_worldData.numLights );
}

void RE_LoadWorldMap( const char *filename )
{
	bmf_t *header;
	mapheader_t *mheader;
	tile2d_header_t *theader;
	int i;
	char texture[MAX_NPATH];
	union {
		byte *b;
		void *v;
	} buffer;

	ri.Printf( PRINT_INFO, "------ RE_LoadWorldMap( %s ) ------\n", filename );

	if ( strlen( filename ) >= MAX_NPATH ) {
		ri.Error(ERR_DROP, "RE_LoadWorldMap: name '%s' too long", filename );
	}

	if ( rg.worldMapLoaded ) {
		ri.Error( ERR_DROP, "attempted to reduntantly load world map" );
	}

	// load it
	ri.FS_LoadFile( filename, &buffer.v );
	if ( !buffer.v ) {
		ri.Error( ERR_DROP, "RE_LoadWorldMap: %s not found", filename );
	}

	// clear rg.world so if the level fails to load, the next
	// try will not look at the partially loaded version
	rg.world = NULL;

	rg.worldMapLoaded = qtrue;

	memset( &r_worldData, 0, sizeof( r_worldData ) );
	N_strncpyz( r_worldData.name, filename, sizeof( r_worldData.name ) );
	N_strncpyz( r_worldData.baseName, COM_SkipPath( r_worldData.name ), sizeof( r_worldData.baseName ) );

	COM_StripExtension( r_worldData.baseName, r_worldData.baseName, sizeof( r_worldData.baseName ) );

	header = (bmf_t *)buffer.b;
	if ( LittleInt( header->version ) != LEVEL_VERSION ) {
		ri.Error( ERR_DROP, "RE_LoadWorldMap: %s has the wrong version number (%i should be %i)",
			filename, LittleInt( header->version ), LEVEL_VERSION );
	}

	fileBase = (byte *)header;

	mheader = &header->map;
	theader = &header->tileset;

	// swap all the lumps
	for ( i = 0; i < ( sizeof( bmf_t ) / 4 ); i++ ) {
		((int32_t *)header)[i] = LittleInt( ((int32_t *)header)[i] );
	}

	VectorCopy( r_worldData.ambientLightColor, mheader->ambientLightColor );

	r_worldData.width = mheader->mapWidth;
	r_worldData.height = mheader->mapHeight;
	r_worldData.numTiles = r_worldData.width * r_worldData.height;

	ri.Printf( PRINT_INFO, "Loaded map '%s', %ix%i, ambientLightColor: ( %f, %f, %f )\n", r_worldData.name,
		r_worldData.width, r_worldData.height, r_worldData.ambientLightColor[0], r_worldData.ambientLightColor[1],
		r_worldData.ambientLightColor[2] );

	r_worldData.firstLevelShader = rg.numShaders;
	r_worldData.firstLevelSpriteSheet = rg.numSpriteSheets;
	r_worldData.firstLevelTexture = rg.numTextures;
	r_worldData.levelShaders = 0;
	r_worldData.levelSpriteSheets = 0;
	r_worldData.levelTextures = 0;

	ri.Cmd_ExecuteCommand( "snd.startup_level" );

	// load into heap
	R_LoadTileset( &mheader->lumps[LUMP_SPRITES], theader );
	R_LoadTiles( &mheader->lumps[LUMP_TILES] );
	R_LoadLights( &mheader->lumps[LUMP_LIGHTS] );

	rg.world = &r_worldData;

	R_ProcessLights();
	R_InitWorldBuffer();
	R_GenerateTexCoords( &theader->info );

	ri.FS_FreeFile( buffer.v );
}
