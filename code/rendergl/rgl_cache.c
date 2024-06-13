#include "rgl_local.h"

static void R_InitRingbuffer( GLenum target, uint32_t elementSize, uint32_t segmentElements, glRingbuffer_t *rb );
static uint32_t R_RotateRingbuffer( glRingbuffer_t *rb );
static void R_ShutdownRingbuffer( GLenum target, glRingbuffer_t *rb );

void R_VaoPackTangent( int16_t *out, vec4_t v )
{
	out[0] = v[0] * 32767.0f + ( v[0] > 0.0f ? 0.5f : -0.5f );
	out[1] = v[1] * 32767.0f + ( v[1] > 0.0f ? 0.5f : -0.5f );
	out[2] = v[2] * 32767.0f + ( v[2] > 0.0f ? 0.5f : -0.5f );
	out[3] = v[3] * 32767.0f + ( v[3] > 0.0f ? 0.5f : -0.5f );
}

void R_VaoPackNormal( int16_t *out, vec3_t v )
{
	out[0] = v[0] * 32767.0f + ( v[0] > 0.0f ? 0.5f : -0.5f );
	out[1] = v[1] * 32767.0f + ( v[1] > 0.0f ? 0.5f : -0.5f );
	out[2] = v[2] * 32767.0f + ( v[2] > 0.0f ? 0.5f : -0.5f );
	out[3] = 0;
}

void R_VaoPackColor( uint16_t *out, const vec4_t c )
{
	out[0] = c[0] * 65535.0f + 0.5f;
	out[1] = c[1] * 65535.0f + 0.5f;
	out[2] = c[2] * 65535.0f + 0.5f;
	out[3] = c[3] * 65535.0f + 0.5f;
}

void R_VaoUnpackTangent( vec4_t v, int16_t *pack )
{
	v[0] = pack[0] / 32767.0f;
	v[1] = pack[1] / 32767.0f;
	v[2] = pack[2] / 32767.0f;
	v[3] = pack[3] / 32767.0f;
}

void R_VaoUnpackNormal( vec3_t v, int16_t *pack )
{
	v[0] = pack[0] / 32767.0f;
	v[1] = pack[1] / 32767.0f;
	v[2] = pack[2] / 32767.0f;
}


static void R_SetVertexPointers( const vertexAttrib_t attribs[ATTRIB_INDEX_COUNT] )
{
	uint32_t attribBit, i;
	const vertexAttrib_t *vAtb;

    for ( i = 0; i < ATTRIB_INDEX_COUNT; i++ ) {
		attribBit = 1 << i;
		vAtb = &attribs[i];

        if ( vAtb->enabled ) {
            nglVertexAttribPointer( vAtb->index, vAtb->count, vAtb->type, vAtb->normalized, vAtb->stride, (const void *)vAtb->offset );
			if ( !( glState.vertexAttribsEnabled & attribBit ) ) {
				nglEnableVertexAttribArray( vAtb->index );
			}
			glState.vertexAttribsEnabled |= attribBit;
        }
        else {
			if ( ( glState.vertexAttribsEnabled & attribBit ) ) {
	            nglDisableVertexAttribArray( vAtb->index );
			}
			glState.vertexAttribsEnabled &= ~attribBit;
        }
    }
}

void VBO_SetVertexPointers( vertexBuffer_t *vbo, uint32_t attribBits )
{
	// if nothing is set, set everything
	if ( !( attribBits & ATTRIB_BITS ) ) {
		attribBits = ATTRIB_BITS;
	}

	vbo->attribs[ ATTRIB_INDEX_POSITION ].enabled = attribBits & ATTRIB_POSITION;
	vbo->attribs[ ATTRIB_INDEX_TEXCOORD ].enabled = attribBits & ATTRIB_TEXCOORD;
	vbo->attribs[ ATTRIB_INDEX_COLOR ].enabled = attribBits & ATTRIB_COLOR;
	vbo->attribs[ ATTRIB_INDEX_LIGHTCOORD ].enabled = attribBits & ATTRIB_LIGHTCOORD;
	vbo->attribs[ ATTRIB_INDEX_WORLDPOS ].enabled = attribBits & ATTRIB_WORLDPOS;

	R_SetVertexPointers( vbo->attribs );
}

/*
* R_ClearVertexPointers: clears all vertex pointers in the current GL state
*/
static void R_ClearVertexPointers( void )
{
	uint32_t attribBit, i;

    for ( i = 0; i < ATTRIB_INDEX_COUNT; i++ ) {
		attribBit = 1 << i;
		if ( ( glState.vertexAttribsEnabled & attribBit ) ) {
	        nglDisableVertexAttribArray( i );
		}
    }
	glState.vertexAttribsEnabled = 0;
}

void R_InitGPUBuffers( void )
{
	uint64_t verticesSize, indicesSize;
	uintptr_t offset;

	ri.Printf( PRINT_INFO, "---------- R_InitGPUBuffers ----------\n" );

	rg.numBuffers = 0;

	backend.drawBuffer = R_AllocateBuffer( "batchBuffer", NULL, sizeof(srfVert_t) * MAX_BATCH_VERTICES, NULL,
		sizeof(glIndex_t) * MAX_BATCH_INDICES, BUFFER_FRAME );

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].enabled		= qtrue;
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].enabled		= qtrue;
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].enabled			= qtrue;
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].enabled		= qfalse;
	backend.drawBuffer->attribs[ATTRIB_INDEX_WORLDPOS].enabled		= qtrue;
	backend.drawBuffer->attribs[ATTRIB_INDEX_LIGHTCOORD].enabled	= qfalse;

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].count		= 3;
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].count		= 2;
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].count			= 4;
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].count			= 4;
	backend.drawBuffer->attribs[ATTRIB_INDEX_WORLDPOS].count		= 3;
	backend.drawBuffer->attribs[ATTRIB_INDEX_LIGHTCOORD].count		= 4;

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].type			= GL_FLOAT;
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].type			= GL_FLOAT;
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].type			= GL_UNSIGNED_SHORT;
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].type			= GL_SHORT;
	backend.drawBuffer->attribs[ATTRIB_INDEX_WORLDPOS].type			= GL_FLOAT;
	backend.drawBuffer->attribs[ATTRIB_INDEX_LIGHTCOORD].type		= GL_FLOAT;

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].index		= ATTRIB_INDEX_POSITION;
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].index		= ATTRIB_INDEX_TEXCOORD;
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].index			= ATTRIB_INDEX_COLOR;
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].index			= ATTRIB_INDEX_NORMAL;
	backend.drawBuffer->attribs[ATTRIB_INDEX_WORLDPOS].index		= ATTRIB_INDEX_WORLDPOS;
	backend.drawBuffer->attribs[ATTRIB_INDEX_LIGHTCOORD].index		= ATTRIB_INDEX_LIGHTCOORD;

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].normalized	= GL_FALSE;
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].normalized	= GL_FALSE;
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].normalized		= GL_TRUE;
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].normalized		= GL_TRUE;
	backend.drawBuffer->attribs[ATTRIB_INDEX_WORLDPOS].normalized	= GL_FALSE;
	backend.drawBuffer->attribs[ATTRIB_INDEX_LIGHTCOORD].normalized = GL_FALSE;

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].offset		= offsetof( srfVert_t, xyz );
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].offset		= offsetof( srfVert_t, st );
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].offset			= offsetof( srfVert_t, color );
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].offset			= offsetof( srfVert_t, normal );
	backend.drawBuffer->attribs[ATTRIB_INDEX_WORLDPOS].offset		= offsetof( srfVert_t, worldPos );
	backend.drawBuffer->attribs[ATTRIB_INDEX_LIGHTCOORD].offset     = offsetof( srfVert_t, lightmap );

	backend.drawBuffer->attribs[ATTRIB_INDEX_POSITION].stride		= sizeof( srfVert_t );
	backend.drawBuffer->attribs[ATTRIB_INDEX_TEXCOORD].stride		= sizeof( srfVert_t );
	backend.drawBuffer->attribs[ATTRIB_INDEX_COLOR].stride			= sizeof( srfVert_t );
	backend.drawBuffer->attribs[ATTRIB_INDEX_NORMAL].stride			= sizeof( srfVert_t );
	backend.drawBuffer->attribs[ATTRIB_INDEX_WORLDPOS].stride		= sizeof( srfVert_t );

	VBO_BindNull();

	GL_CheckErrors();
}

void R_ShutdownGPUBuffers( void )
{
	uint64_t i;
	vertexBuffer_t *vbo;

	ri.Printf( PRINT_INFO, "---------- R_ShutdownGPUBuffers -----------\n" );

	VBO_BindNull();

	for ( i = 0; i < rg.numBuffers; i++ ) {
		vbo = rg.buffers[i];

		R_ShutdownBuffer( vbo );
	}

	memset( rg.buffers, 0, sizeof( rg.buffers ) );
	rg.numBuffers = 0;
}

#if defined( GL_ARB_buffer_storage ) && defined( GL_ARB_sync )
/*
============
R_InitRingbuffer
============
*/
static void R_InitRingbuffer( GLenum target, uint32_t elementSize, uint32_t segmentElements, glRingbuffer_t *rb ) {
	const uint32_t totalSize = elementSize * segmentElements * DYN_BUFFER_SEGMENTS;
	uint32_t i;

	nglBufferStorage( target, totalSize, NULL, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT );

	rb->baseAddr = nglMapBufferRange( target, 0, totalSize,
					 GL_MAP_WRITE_BIT |
					 GL_MAP_PERSISTENT_BIT |
					 GL_MAP_FLUSH_EXPLICIT_BIT );
	rb->elementSize = elementSize;
	rb->segmentElements = segmentElements;
	rb->activeSegment = 0;

	for ( i = 1; i < DYN_BUFFER_SEGMENTS; i++ ) {
		rb->syncs[ i ] = nglFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
	}
}

/*
============
R_RotateRingbuffer
============
*/
static uint32_t R_RotateRingbuffer( glRingbuffer_t *rb ) {
	rb->syncs[ rb->activeSegment ] = nglFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );

	rb->activeSegment++;
	if ( rb->activeSegment >= DYN_BUFFER_SEGMENTS ) {
		rb->activeSegment = 0;
	}

	// wait until next segment is ready in 1 sec intervals
	while ( nglClientWaitSync( rb->syncs[ rb->activeSegment ], GL_SYNC_FLUSH_COMMANDS_BIT, 10000000 ) == GL_TIMEOUT_EXPIRED ) {
		ri.Printf( PRINT_WARNING, "long wait for GL buffer" );
	}

	nglDeleteSync( rb->syncs[ rb->activeSegment ] );

	return rb->activeSegment * rb->segmentElements;
}

/*
============
R_ShutdownRingbuffer
============
*/
static void R_ShutdownRingbuffer( GLenum target, glRingbuffer_t *rb ) {
	uint32_t i;

	nglUnmapBuffer( target );
	rb->baseAddr = NULL;

	for ( i = 0; i < DYN_BUFFER_SEGMENTS; i++ ) {
		if ( i == rb->activeSegment ) {
			continue;
		}

		nglDeleteSync( rb->syncs[ i ] );
	}
}
#endif

static GLuint R_InitBufferStorage( GLenum target, GLsizei size, const GLvoid *data, GLenum usage, qboolean newBuffer, GLuint buffer )
{
	bufferMemType_t memType;
	GLenum err;
	GLuint bufferId = buffer;

	if ( newBuffer ) {
		if ( NGL_VERSION_ATLEAST( 4, 5 ) ) {
			nglCreateBuffers( 1, &bufferId );
		} else {
			nglGenBuffers( 1, &bufferId );
		}
	}
	if ( ( err = nglGetError() ) != GL_NO_ERROR ) {
		ri.Error( ERR_DROP, "Error generating OpenGL hardware buffer: %s", GL_ErrorString( err ) );
	}

	nglBindBufferARB( target, bufferId );
	if ( r_drawMode->i == DRAWMODE_MAPPED && NGL_VERSION_ATLEAST( 4, 5 ) ) {
		if ( !newBuffer ) {
			nglDeleteBuffers( 1, &bufferId );
			nglCreateBuffers( 1, &bufferId );
			nglBindBuffer( target, bufferId );
		}
		nglBufferStorage( target, size, data, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );
	} else if ( r_drawMode->i == DRAWMODE_GPU ) {
		nglBufferDataARB( target, size, data, usage );
	}
	if ( ( err = nglGetError() ) != GL_NO_ERROR ) {
		ri.Error( ERR_DROP, "Error allocating data for OpenGL hardware buffer: %s", GL_ErrorString( err ) );
	}
	nglBindBufferARB( target, 0 );

	return bufferId;
}

vertexBuffer_t *R_AllocateBuffer( const char *name, void *vertices, uint32_t verticesSize, void *indices, uint32_t indicesSize,
	bufferType_t type )
{
    vertexBuffer_t *buf;
	GLenum usage;
	uint32_t namelen;
	uint64_t i;
	GLenum err;

	switch ( type ) {
	case BUFFER_STATIC:
		usage = GL_STATIC_DRAW;
		break;
	case BUFFER_FRAME:
	case BUFFER_DYNAMIC:
		usage = GL_DYNAMIC_DRAW;
		break;
	case BUFFER_STREAM:
		usage = GL_STREAM_DRAW;
		break;
	default:
		ri.Error( ERR_FATAL, "Bad glUsage %i", type );
	};

	for ( i = 0; i < rg.numBuffers; i++ ) {
		if ( !N_stricmp( rg.buffers[i]->name, name ) ) {
			// resize buffers if necessary
			VBO_Bind( rg.buffers[i] );
			if ( rg.buffers[i]->vertex.size != verticesSize ) {
				glState.memstats.estBufferMemUsed -= rg.buffers[i]->vertex.size;
				R_InitBufferStorage( GL_ARRAY_BUFFER_ARB, verticesSize, vertices, usage, qfalse, rg.buffers[i]->vertex.id );
				rg.buffers[i]->vertex.size = verticesSize;
				glState.memstats.estBufferMemUsed += verticesSize;
			}
			if ( rg.buffers[i]->index.size != indicesSize ) {
				glState.memstats.estBufferMemUsed -= rg.buffers[i]->index.size;
				R_InitBufferStorage( GL_ELEMENT_ARRAY_BUFFER_ARB, indicesSize, indices, usage, qfalse, rg.buffers[i]->index.id );
				rg.buffers[i]->index.size = indicesSize;
				glState.memstats.estBufferMemUsed += indicesSize;
			}
			return rg.buffers[i];
		}
	}

	if ( rg.numBuffers == MAX_RENDER_BUFFERS ) {
		ri.Error( ERR_DROP, "R_AllocateBuffer: MAX_RENDER_BUFFERS hit" );
	}

    buf = rg.buffers[rg.numBuffers] = ri.Hunk_Alloc( sizeof( *buf ), h_low );
	memset( buf, 0, sizeof( *buf ) );
    rg.numBuffers++;

    buf->type = type;
	N_strncpyz( buf->name, name, sizeof( buf->name ) );

	nglGenVertexArrays( 1, &buf->vaoId );
	if ( ( err = nglGetError() ) != GL_NO_ERROR ) {
		ri.Error( ERR_DROP, "%s: Error generating OpenGL Vertex Array (0x%04x)", GL_ErrorString( err ), err );
	}

	nglBindVertexArray( buf->vaoId );

	buf->vertex.usage = BUF_GL_BUFFER;
	buf->index.usage = BUF_GL_BUFFER;

	buf->vertex.glUsage = usage;
	buf->index.glUsage = usage;

	buf->vertex.size = verticesSize;
	buf->index.size = indicesSize;

	buf->vertex.id = R_InitBufferStorage( GL_ARRAY_BUFFER_ARB, verticesSize, vertices, usage, qtrue, 0 );
	buf->index.id = R_InitBufferStorage( GL_ELEMENT_ARRAY_BUFFER_ARB, indicesSize, indices, usage, qtrue, 0 );

	GL_SetObjectDebugName( GL_VERTEX_ARRAY, buf->vaoId, name, "_vao" );
	GL_SetObjectDebugName( GL_BUFFER, buf->vertex.id, name, "_vbo" );
	GL_SetObjectDebugName( GL_BUFFER, buf->index.id, name, "_ibo" );

	glState.memstats.estBufferMemUsed += ( indicesSize + verticesSize );
	glState.memstats.estVertexMemUsed += verticesSize;
	glState.memstats.estIndexMemUsed += indicesSize;
	glState.memstats.numIndexBufers++;
	glState.memstats.numVertexBuffers++;
	glState.memstats.numVertexArrays++;
	glState.memstats.numBuffers++;

    return buf;
}

/*
============
VBO_Bind
============
*/
void VBO_Bind( vertexBuffer_t *vbo )
{
	if ( !vbo ) {
		ri.Error( ERR_DROP, "VBO_Bind: NULL buffer" );
		return;
	}

	if ( glState.currentVao != vbo ) {
		glState.currentVao = vbo;
		glState.vaoId = vbo->vaoId;
		glState.vboId = vbo->vertex.id;
		glState.iboId = vbo->index.id;
		backend.pc.c_bufferBinds++;

		if ( r_drawMode->i == DRAWMODE_CLIENT ) {
			nglEnableClientState( GL_COLOR_ARRAY );
			nglEnableClientState( GL_VERTEX_ARRAY );
			nglEnableClientState( GL_TEXTURE_COORD_ARRAY );
			nglEnableClientState( GL_NORMAL_ARRAY );
		} else if ( r_drawMode->i >= DRAWMODE_GPU ) {
			nglBindVertexArray( vbo->vaoId );
			nglBindBuffer( GL_ARRAY_BUFFER, vbo->vertex.id );
			nglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbo->index.id );

			// Intel Graphics doesn't save GL_ELEMENT_ARRAY_BUFFER binding with VAO binding.
			// [TheNomad] 6/10/24 you've gotta bind it, nothing saves the binding
//			if ( glContext.intelGraphics ) {
//				nglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbo->index.id );
//			}
		}
	}
}

/*
============
VBO_BindNull
============
*/
void VBO_BindNull( void )
{
	ri.GLimp_LogComment("--- VBO_BindNull ---\n");

	if ( glState.currentVao ) {
		glState.currentVao = NULL;
		glState.vaoId = glState.vboId = glState.iboId = 0;

		if ( r_drawMode->i == DRAWMODE_CLIENT ) {
			nglDisableClientState( GL_COLOR_ARRAY );
			nglDisableClientState( GL_VERTEX_ARRAY );
			nglDisableClientState( GL_TEXTURE_COORD_ARRAY );
			nglDisableClientState( GL_NORMAL_ARRAY );
		} else if ( r_drawMode->i >= DRAWMODE_GPU ) {
	        nglBindVertexArray( 0 );
			nglBindBuffer( GL_ARRAY_BUFFER, 0 );
			nglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	        // why you no save GL_ELEMENT_ARRAY_BUFFER binding, Intel?
//	        if ( glContext.intelGraphics ) {
//				nglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
//			}
		}
	}

	GL_CheckErrors();
}

void R_ShutdownBuffer( vertexBuffer_t *vbo )
{
	VBO_Bind( vbo );
	if ( vbo->vertex.id ) {
		if ( vbo->vertex.usage == BUF_GL_MAPPED ) {
			nglBindBuffer( GL_ARRAY_BUFFER, vbo->vertex.id );
			nglUnmapBuffer( GL_ARRAY_BUFFER );
			nglBindBuffer( GL_ARRAY_BUFFER, 0 );
		}
		nglDeleteBuffers( 1, &vbo->vertex.id );
	}
	
	if ( vbo->index.id ) {
		if ( vbo->index.usage == BUF_GL_MAPPED ) {
			nglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, vbo->index.id );
			nglUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );
			nglBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
		}
		nglDeleteBuffers( 1, &vbo->index.id );
	}

	if ( vbo->vaoId ) {
		nglDeleteVertexArrays( 1, &vbo->vaoId );
	}

	glState.memstats.numBuffers--;
	glState.memstats.estBufferMemUsed -= ( vbo->vertex.size + vbo->index.size );
	memset( vbo, 0, sizeof( *vbo ) );

	VBO_BindNull();
}

void RB_SetBatchBuffer( vertexBuffer_t *buffer, void *vertexBuffer, uintptr_t vtxSize, void *indexBuffer, uintptr_t idxSize )
{
    // is it already bound?
    if ( backend.drawBatch.buffer != buffer ) {
		VBO_BindNull();
	} else if ( backend.drawBatch.buffer == buffer ) {
		VBO_Bind( buffer );
		return;
	}

    // clear anything currently queued
    if ( backend.drawBatch.buffer && ( backend.drawBatch.vtxOffset || backend.drawBatch.idxOffset ) ) {
        RB_FlushBatchBuffer();
    }

    backend.drawBatch.buffer = buffer;

    backend.drawBatch.vtxDataSize = vtxSize;
    backend.drawBatch.idxDataSize = idxSize;

    backend.drawBatch.maxVertices = buffer->vertex.size;
    backend.drawBatch.maxIndices = buffer->index.size;

    backend.drawBatch.vertices = vertexBuffer;
    backend.drawBatch.indices = indexBuffer;

    // bind the new cache
	VBO_Bind( buffer );
	R_ClearVertexPointers();

	// set the new vertex attrib array state
	R_SetVertexPointers( buffer->attribs );
}

void RB_FlushBatchBuffer( void )
{
	vertexBuffer_t *buf;
	void *data;

    if ( !backend.drawBatch.buffer ) {
        return;
    }

    // do we actually have something there?
    if ( backend.drawBatch.vtxOffset == 0 || backend.drawBatch.idxOffset == 0 ) {
        return;
    }

    backend.pc.c_dynamicBufferDraws++;

    //
    // upload the data to the gpu
    //

	buf = backend.drawBatch.buffer;

	// orphan the old index buffer so that we don't stall on it
	if ( r_drawMode->i == DRAWMODE_MAPPED ) {
		nglInvalidateBufferData( backend.drawBuffer->index.id );
		data = nglMapBufferRange( GL_ELEMENT_ARRAY_BUFFER_ARB, 0, backend.drawBatch.maxIndices, GL_MAP_WRITE_BIT
			| GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );
		if ( data ) {
			memcpy( data, backend.drawBatch.indices, backend.drawBatch.idxOffset * backend.drawBatch.idxDataSize );
		}
		nglUnmapBuffer( GL_ELEMENT_ARRAY_BUFFER );
	} else if ( r_drawMode->i == DRAWMODE_GPU ) {
		nglBufferData( GL_ELEMENT_ARRAY_BUFFER, backend.drawBatch.maxIndices, NULL, backend.drawBatch.buffer->index.glUsage );
		nglBufferSubData( GL_ELEMENT_ARRAY_BUFFER, 0, backend.drawBatch.idxOffset * backend.drawBatch.idxDataSize, backend.drawBatch.indices );
	}

	// orphan the old vertex buffer so that we don't stall on it
	if ( r_drawMode->i == DRAWMODE_MAPPED ) {
		nglInvalidateBufferData( backend.drawBuffer->vertex.id );
		data = nglMapBufferRange( GL_ARRAY_BUFFER_ARB, 0, backend.drawBatch.maxVertices, GL_MAP_WRITE_BIT
			| GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT );
		if ( data ) {
			memcpy( data, backend.drawBatch.vertices, backend.drawBatch.vtxOffset * backend.drawBatch.vtxDataSize );
		}
		nglUnmapBuffer( GL_ARRAY_BUFFER );
	} else if ( r_drawMode->i == DRAWMODE_GPU ) {
		nglBufferData( GL_ARRAY_BUFFER, backend.drawBatch.maxVertices, NULL, backend.drawBatch.buffer->vertex.glUsage );
		nglBufferSubData( GL_ARRAY_BUFFER, 0, backend.drawBatch.vtxOffset * backend.drawBatch.vtxDataSize, backend.drawBatch.vertices );
	}

	RB_IterateShaderStages( backend.drawBatch.shader );

	backend.pc.c_bufferIndices += backend.drawBatch.idxOffset;
	backend.pc.c_bufferVertices += backend.drawBatch.vtxOffset;

	backend.drawBatch.vtxOffset = 0;
	backend.drawBatch.idxOffset = 0;
}

void RB_CommitDrawData( const void *verts, uint32_t numVerts, const void *indices, uint32_t numIndices )
{
    if ( numVerts > backend.drawBatch.maxVertices / backend.drawBatch.vtxDataSize ) {
        ri.Error( ERR_DROP, "RB_CommitDrawData: numVerts > backend.drawBatch.maxVertices (%u > %li)", numVerts,
			backend.drawBatch.maxVertices / backend.drawBatch.vtxDataSize );
    }
    if ( numIndices > backend.drawBatch.maxIndices / backend.drawBatch.idxDataSize ) {
        ri.Error( ERR_DROP, "RB_CommitDrawData: numIndices > backend.drawBatch.maxIndices (%u > %li)", numIndices,
			backend.drawBatch.maxIndices / backend.drawBatch.idxDataSize );
    }

    // do we need to flush?
    if ( backend.drawBatch.vtxOffset + numVerts > backend.drawBatch.maxVertices
    	|| backend.drawBatch.idxOffset + numIndices > backend.drawBatch.maxIndices
		&& ( backend.drawBatch.vtxOffset > 0 && backend.drawBatch.idxOffset > 0 ) )
	{
        RB_FlushBatchBuffer();
    }

    //
    // copy the data into the client side buffer
    //

    // we could be submitting either indices or vertices
    if ( verts ) {
        memcpy( (byte *)backend.drawBatch.vertices + ( backend.drawBatch.vtxOffset * backend.drawBatch.vtxDataSize ), verts,
			numVerts * backend.drawBatch.vtxDataSize );
    }
    if ( indices ) {
        memcpy( (byte *)backend.drawBatch.indices + ( backend.drawBatch.idxOffset * backend.drawBatch.idxDataSize ), indices,
			numIndices * backend.drawBatch.idxDataSize );
    }

    backend.drawBatch.vtxOffset += numVerts;
    backend.drawBatch.idxOffset += numIndices;
}


