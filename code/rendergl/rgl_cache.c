#include "rgl_local.h"

void R_VaoPackTangent(int16_t *out, vec4_t v)
{
	out[0] = v[0] * 32767.0f + (v[0] > 0.0f ? 0.5f : -0.5f);
	out[1] = v[1] * 32767.0f + (v[1] > 0.0f ? 0.5f : -0.5f);
	out[2] = v[2] * 32767.0f + (v[2] > 0.0f ? 0.5f : -0.5f);
	out[3] = v[3] * 32767.0f + (v[3] > 0.0f ? 0.5f : -0.5f);
}

void R_VaoPackNormal(int16_t *out, vec3_t v)
{
	out[0] = v[0] * 32767.0f + (v[0] > 0.0f ? 0.5f : -0.5f);
	out[1] = v[1] * 32767.0f + (v[1] > 0.0f ? 0.5f : -0.5f);
	out[2] = v[2] * 32767.0f + (v[2] > 0.0f ? 0.5f : -0.5f);
	out[3] = 0;
}

void R_VaoPackColor(uint16_t *out, const vec4_t c)
{
	out[0] = c[0] * 65535.0f + 0.5f;
	out[1] = c[1] * 65535.0f + 0.5f;
	out[2] = c[2] * 65535.0f + 0.5f;
	out[3] = c[3] * 65535.0f + 0.5f;
}

void R_VaoUnpackTangent(vec4_t v, int16_t *pack)
{
	v[0] = pack[0] / 32767.0f;
	v[1] = pack[1] / 32767.0f;
	v[2] = pack[2] / 32767.0f;
	v[3] = pack[3] / 32767.0f;
}

void R_VaoUnpackNormal(vec3_t v, int16_t *pack)
{
	v[0] = pack[0] / 32767.0f;
	v[1] = pack[1] / 32767.0f;
	v[2] = pack[2] / 32767.0f;
}


static vertexBuffer_t *hashTable[MAX_RENDER_BUFFERS];

static qboolean R_BufferExists(const char *name)
{
    return hashTable[Com_GenerateHashValue(name, MAX_RENDER_BUFFERS)] != NULL;
}

static void R_SetVertexPointers(const vertexAttrib_t attribs[ATTRIB_INDEX_COUNT], uintptr_t vertexStride)
{
    for (uint64_t i = 0; i < ATTRIB_INDEX_COUNT; i++) {
        if (attribs[i].enabled) {
            nglEnableVertexAttribArray(attribs[i].index);
            nglVertexAttribPointer(attribs[i].index, attribs[i].count, attribs[i].type, attribs[i].normalized, vertexStride, (const void *)attribs[i].offset);
        }
        else {
            nglDisableVertexAttribArray(attribs[i].index);
        }
    }
}

/*
R_ClearVertexPointers: clears all vertex pointers in the current GL state
*/
static void R_ClearVertexPointers(void)
{
    for (uint64_t i = 0; i < ATTRIB_INDEX_COUNT; i++) {
        nglDisableVertexAttribArray(i);
    }
}

vertexBuffer_t *R_AllocateBuffer(const char *name, uint32_t numVertices, uint32_t numIndices, uint32_t attribBits, bufferType_t type)
{
    vertexBuffer_t *buf;
    uint64_t hash;

    if (r_drawMode->i != DRAW_GPU_BUFFERED) {
        return NULL;
    }

    if (R_BufferExists(name)) {
        ri.Error(ERR_DROP, "R_AllocateBuffer: buffer '%s' already exists", name);
    }

    hash = Com_GenerateHashValue(name, MAX_RENDER_BUFFERS);

    // these buffers are only allocated on a per vm
    // and map basis, so use the hunk
    buf = rg.buffers[rg.numBuffers] = ri.Hunk_Alloc(sizeof(*buf), h_low);
    hashTable[hash] = buf;
    rg.numBuffers++;

    buf->type = type;

    memset(buf, 0, sizeof(*buf));
    buf->vertices.size = sizeof(drawVert_t) * numVertices;
    buf->vertices.dataSize = sizeof(drawVert_t);

    buf->indices.size = sizeof(glIndex_t) * numIndices;
    buf->indices.dataSize = sizeof(glIndex_t);

    buf->attribs[ATTRIB_INDEX_POSITION].enabled     = attribBits & ATTRIB_POSITION;
    buf->attribs[ATTRIB_INDEX_COLOR].enabled        = attribBits & ATTRIB_COLOR;
    buf->attribs[ATTRIB_INDEX_NORMAL].enabled       = attribBits & ATTRIB_NORMAL;
    buf->attribs[ATTRIB_INDEX_TEXCOORD].enabled     = attribBits & ATTRIB_TEXCOORD;
    buf->attribs[ATTRIB_INDEX_ALPHA].enabled        = attribBits & ATTRIB_ALPHA;

    buf->attribs[ATTRIB_INDEX_POSITION].offset      = offsetof(drawVert_t, xyz);
    buf->attribs[ATTRIB_INDEX_COLOR].offset         = offsetof(drawVert_t, color);
    buf->attribs[ATTRIB_INDEX_NORMAL].offset        = offsetof(drawVert_t, normal);
    buf->attribs[ATTRIB_INDEX_TEXCOORD].offset      = offsetof(drawVert_t, uv);

    buf->attribs[ATTRIB_INDEX_POSITION].count       = 3;
    buf->attribs[ATTRIB_INDEX_COLOR].count          = 4;
    buf->attribs[ATTRIB_INDEX_NORMAL].count         = 4;
    buf->attribs[ATTRIB_INDEX_TEXCOORD].count       = 2;

    buf->attribs[ATTRIB_INDEX_POSITION].type        = GL_FLOAT;
    buf->attribs[ATTRIB_INDEX_COLOR].type           = GL_UNSIGNED_SHORT;
    buf->attribs[ATTRIB_INDEX_NORMAL].type          = GL_SHORT;
    buf->attribs[ATTRIB_INDEX_TEXCOORD].type        = GL_FLOAT;

    buf->attribs[ATTRIB_INDEX_POSITION].index       = 0;
    buf->attribs[ATTRIB_INDEX_COLOR].index          = 1;
    buf->attribs[ATTRIB_INDEX_NORMAL].index         = 2;
    buf->attribs[ATTRIB_INDEX_TEXCOORD].index       = 3;
    buf->attribs[ATTRIB_INDEX_ALPHA].index          = 4;

    buf->attribs[ATTRIB_INDEX_POSITION].normalized  = GL_FALSE;
    buf->attribs[ATTRIB_INDEX_COLOR].normalized     = GL_FALSE;
    buf->attribs[ATTRIB_INDEX_NORMAL].normalized    = GL_FALSE;
    buf->attribs[ATTRIB_INDEX_TEXCOORD].normalized  = GL_FALSE;
    buf->attribs[ATTRIB_INDEX_ALPHA].normalized     = GL_FALSE;

    if (r_drawMode->i == DRAW_GPU_BUFFERED) {
        // allocate gpu buffers
        nglGenVertexArrays(1, (GLuint *)&buf->vaoId);
        nglGenBuffers(1, (GLuint *)&buf->vboId);
        nglGenBuffers(1, (GLuint *)&buf->iboId);

        // make sure nothing is bound
        VBO_BindNull();

        nglBindVertexArray(buf->vaoId);
        nglBindBuffer(GL_ARRAY_BUFFER, buf->vboId);
        nglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf->iboId);

        R_SetVertexPointers(buf->attribs, sizeof(drawVert_t));

        nglBufferData(GL_ARRAY_BUFFER, sizeof(drawVert_t) * numVertices, NULL, GL_DYNAMIC_DRAW);
        nglBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glIndex_t) * numIndices, NULL, GL_DYNAMIC_DRAW);

        memset(buf->vertices.data, 0, sizeof(drawVert_t) * numVertices);
        memset(buf->indices.data, 0, sizeof(glIndex_t) * numIndices);
        
        nglBindVertexArray(0);
        nglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    return buf;
}

void R_ShutdownBuffers(void)
{
    vertexBuffer_t *vbo;

    if (r_drawMode->i != DRAW_GPU_BUFFERED)
        return;

    for (uint64_t i = 0; i < rg.numBuffers; i++) {
        vbo = rg.buffers[i];

        if (vbo->vaoId)
            nglDeleteVertexArrays(1, (const GLuint *)&vbo->vaoId);
        if (vbo->vboId)
            nglDeleteBuffers(1, (const GLuint *)&vbo->vboId);
        if (vbo->iboId)
            nglDeleteBuffers(1, (const GLuint *)&vbo->iboId);
    }
}

void VBO_SwapData(vertexBuffer_t *buf)
{
    ri.GLimp_LogComment("---- VBO_SwapData ----\n");
    switch (r_drawMode->i) {
    case DRAW_IMMEDIATE:
        break;
    case DRAW_CLIENT_BUFFERED:
        ri.Error(ERR_FATAL, "Client buffered draw mode isn't supported yet, sry!");
        break;
    case DRAW_GPU_BUFFERED:
        // swap out gpu data with cpu buffers
        nglBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, buf->indices.size, buf->indices.data);
        nglBufferSubData(GL_ARRAY_BUFFER, 0, buf->vertices.size, buf->vertices.data);
        break;
    };
    buf->vertices.offset = 0;
    buf->indices.offset = 0;
}

/*
============
VBO_Bind
============
*/
void VBO_Bind(vertexBuffer_t *vbo)
{
	if (!vbo) {
		ri.Error(ERR_DROP, "VBO_Bind: NULL buffer");
		return;
	}

	if (glState.vaoId != vbo->vaoId) {
		glState.vaoId = 0;
		backend->pc.c_bufferBinds++;

		nglBindVertexArray(vbo->vaoId);

		// Intel Graphics doesn't save GL_ELEMENT_ARRAY_BUFFER binding with VAO binding.
		if (glContext->intelGraphics)
			nglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo->iboId);
	}
}

/*
============
VBO_BindNull
============
*/
void VBO_BindNull(void)
{
	ri.GLimp_LogComment("--- VBO_BindNull ---\n");

	if (glState.vaoId) {
        nglBindVertexArray(0);

        // why you no save GL_ELEMENT_ARRAY_BUFFER binding, Intel?
        if (glContext->intelGraphics) nglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	GL_CheckErrors();
}

void VBO_Flush(vertexBuffer_t *buf)
{
    VBO_Bind(buf);

    // swap the buffer
    VBO_SwapData(buf);
    // draw it
    R_DrawElements(buf->indices.size / sizeof(glIndex_t), 0);

    VBO_BindNull();
}

void VBO_RecycleIndexBuffer(vertexBuffer_t *buf)
{
    nglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buf->iboId);
    nglBufferData(GL_ELEMENT_ARRAY_BUFFER, buf->indices.size, NULL, GL_DYNAMIC_DRAW);
    buf->indices.offset = 0;
}

void VBO_RecycleVertexBuffer(vertexBuffer_t *buf)
{
    nglBindBuffer(GL_ARRAY_BUFFER, buf->vboId);
    nglBufferData(GL_ARRAY_BUFFER, buf->vertices.size, NULL, GL_DYNAMIC_DRAW);
    buf->vertices.offset = 0;
}

void VBO_FlushAll(void)
{
    vertexBuffer_t *buf;
    
    for (uint64_t i = 0; i < rg.numBuffers; i++) {
        buf = rg.buffers[i];

        VBO_Bind(buf);

        // swap the buffer
        VBO_SwapData(buf);
        // draw it
        R_DrawElements(buf->indices.size / sizeof(glIndex_t), 0);

        VBO_BindNull();
    }
}


// FIXME: This sets a limit of 65536 verts/262144 indexes per static surface
// This is higher than the old vq3 limits but is worth noting
#define VAOCACHE_QUEUE_MAX_SURFACES (1 << 10)
#define VAOCACHE_QUEUE_MAX_VERTEXES (1 << 16)
#define VAOCACHE_QUEUE_MAX_INDEXES (VAOCACHE_QUEUE_MAX_VERTEXES * 4)

typedef struct queuedSurface_s
{
	drawVert_t *vertexes;
	int numVerts;
	glIndex_t *indexes;
	int numIndexes;
} queuedSurface_t;

static struct
{
	queuedSurface_t surfaces[VAOCACHE_QUEUE_MAX_SURFACES];
	int numSurfaces;

	drawVert_t vertexes[VAOCACHE_QUEUE_MAX_VERTEXES];
	int vertexCommitSize;

	glIndex_t indexes[VAOCACHE_QUEUE_MAX_INDEXES];
	int indexCommitSize;
} vcq;

#define VAOCACHE_MAX_SURFACES (1 << 16)
#define VAOCACHE_MAX_BATCHES (1 << 10)

// drawVert_t is 60 bytes
// assuming each vert is referenced 4 times, need 16 bytes (4 glIndex_t) per vert
// -> need about 4/15ths the space for indexes as vertexes
#if GL_INDEX_TYPE == GL_UNSIGNED_SHORT
#define VAOCACHE_VERTEX_BUFFER_SIZE (sizeof(drawVert_t) * USHRT_MAX)
#define VAOCACHE_INDEX_BUFFER_SIZE (sizeof(glIndex_t) * USHRT_MAX * 4)
#else // GL_UNSIGNED_INT
#define VAOCACHE_VERTEX_BUFFER_SIZE (16 * 1024 * 1024)
#define VAOCACHE_INDEX_BUFFER_SIZE (5 * 1024 * 1024)
#endif

typedef struct buffered_s
{
	void *data;
    uint64_t size;
	uintptr_t bufferOffset;
} buffered_t;

static struct
{
	vertexBuffer_t *vao;
	buffered_t surfaceIndexSets[VAOCACHE_MAX_SURFACES];
	uint64_t numSurfaces;

	uint64_t batchLengths[VAOCACHE_MAX_BATCHES];
	uint64_t numBatches;

	uintptr_t vertexOffset;
	uintptr_t indexOffset;
} vc;

void VaoCache_Commit(void)
{
	buffered_t *indexSet;
	uint64_t *batchLength;
	queuedSurface_t *surf, *end = vcq.surfaces + vcq.numSurfaces;

    VBO_Bind(vc.vao);

	// Search for a matching batch
	// FIXME: Use faster search
	indexSet = vc.surfaceIndexSets;
	batchLength = vc.batchLengths;
	for (; batchLength < vc.batchLengths + vc.numBatches; batchLength++) {
		if (*batchLength == vcq.numSurfaces) {
			buffered_t *indexSet2 = indexSet;
			for (surf = vcq.surfaces; surf < end; surf++, indexSet2++) {
				if (surf->indexes != indexSet2->data || (surf->numIndexes * sizeof(glIndex_t)) != indexSet2->size)
					break;
			}

			if (surf == end)
				break;
		}

		indexSet += *batchLength;
	}

	// If found, use it
	if (indexSet < vc.surfaceIndexSets + vc.numSurfaces) {
		backend->dbuf.firstIndex = indexSet->bufferOffset / sizeof(glIndex_t);
		//ri.Printf(PRINT_ALL, "firstIndex %d numIndexes %d as %d\n", tess.firstIndex, tess.numIndexes, (int)(batchLength - vc.batchLengths));
		//ri.Printf(PRINT_ALL, "vc.numSurfaces %d vc.numBatches %d\n", vc.numSurfaces, vc.numBatches);
	}
	// If not, rebuffer the batch
	// FIXME: keep track of the vertexes so we don't have to reupload them every time
	else {
		drawVert_t *dstVertex = vcq.vertexes;
		glIndex_t *dstIndex = vcq.indexes;

		batchLength = vc.batchLengths + vc.numBatches;
		*batchLength = vcq.numSurfaces;
		vc.numBatches++;

		backend->dbuf.firstIndex = vc.indexOffset / sizeof(glIndex_t);
		vcq.vertexCommitSize = 0;
		vcq.indexCommitSize = 0;

		for (surf = vcq.surfaces; surf < end; surf++) {
			glIndex_t *srcIndex = surf->indexes;
			uint32_t vertexesSize = surf->numVerts * sizeof(drawVert_t);
			uint32_t indexesSize = surf->numIndexes * sizeof(glIndex_t);
			uint32_t i, indexOffset = (vc.vertexOffset + vcq.vertexCommitSize) / sizeof(drawVert_t);

			memcpy(dstVertex, surf->vertexes, vertexesSize);
			dstVertex += surf->numVerts;

			vcq.vertexCommitSize += vertexesSize;

			indexSet = vc.surfaceIndexSets + vc.numSurfaces;
			indexSet->data = surf->indexes;
			indexSet->size = indexesSize;
			indexSet->bufferOffset = vc.indexOffset + vcq.indexCommitSize;
			vc.numSurfaces++;

			for (i = 0; i < surf->numIndexes; i++)
				*dstIndex++ = *srcIndex++ + indexOffset;

			vcq.indexCommitSize += indexesSize;
		}

		//ri.Printf(PRINT_ALL, "committing %d to %d, %d to %d as %d\n", vcq.vertexCommitSize, vc.vertexOffset, vcq.indexCommitSize, vc.indexOffset, (int)(batchLength - vc.batchLengths));

		if (vcq.vertexCommitSize) {
			nglBindBuffer(GL_ARRAY_BUFFER, vc.vao->vboId);
			nglBufferSubData(GL_ARRAY_BUFFER, vc.vertexOffset, vcq.vertexCommitSize, vcq.vertexes);
			vc.vertexOffset += vcq.vertexCommitSize;
		}

		if (vcq.indexCommitSize) {
			nglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vc.vao->iboId);
			nglBufferSubData(GL_ELEMENT_ARRAY_BUFFER, vc.indexOffset, vcq.indexCommitSize, vcq.indexes);
			vc.indexOffset += vcq.indexCommitSize;
		}
	}
}

void VaoCache_Init(void)
{
	vc.vao = R_AllocateBuffer("VaoCache",  VAOCACHE_VERTEX_BUFFER_SIZE, VAOCACHE_INDEX_BUFFER_SIZE,
        ATTRIB_POSITION | ATTRIB_TEXCOORD | ATTRIB_NORMAL | ATTRIB_COLOR | ATTRIB_ALPHA, BUFFER_FRAME);

	vc.vao->attribs[ATTRIB_INDEX_POSITION].enabled       = qtrue;
	vc.vao->attribs[ATTRIB_INDEX_TEXCOORD].enabled       = qtrue;
//	vc.vao->attribs[ATTRIB_INDEX_LIGHTCOORD].enabled     = qtrue;
	vc.vao->attribs[ATTRIB_INDEX_NORMAL].enabled         = qtrue;
//	vc.vao->attribs[ATTRIB_INDEX_TANGENT].enabled        = qtrue;
//	vc.vao->attribs[ATTRIB_INDEX_LIGHTDIRECTION].enabled = qtrue;
	vc.vao->attribs[ATTRIB_INDEX_COLOR].enabled          = qtrue;

	vc.vao->attribs[ATTRIB_INDEX_POSITION].count       = 3;
	vc.vao->attribs[ATTRIB_INDEX_TEXCOORD].count       = 2;
//	vc.vao->attribs[ATTRIB_INDEX_LIGHTCOORD].count     = 2;
	vc.vao->attribs[ATTRIB_INDEX_NORMAL].count         = 4;
//	vc.vao->attribs[ATTRIB_INDEX_TANGENT].count        = 4;
//	vc.vao->attribs[ATTRIB_INDEX_LIGHTDIRECTION].count = 4;
	vc.vao->attribs[ATTRIB_INDEX_COLOR].count          = 4;

	vc.vao->attribs[ATTRIB_INDEX_POSITION].type             = GL_FLOAT;
	vc.vao->attribs[ATTRIB_INDEX_TEXCOORD].type             = GL_FLOAT;
//	vc.vao->attribs[ATTRIB_INDEX_LIGHTCOORD].type           = GL_SHORT;
	vc.vao->attribs[ATTRIB_INDEX_NORMAL].type               = GL_SHORT;
//	vc.vao->attribs[ATTRIB_INDEX_TANGENT].type              = GL_SHORT;
//	vc.vao->attribs[ATTRIB_INDEX_LIGHTDIRECTION].type       = GL_SHORT;
	vc.vao->attribs[ATTRIB_INDEX_COLOR].type                = GL_UNSIGNED_SHORT;

	vc.vao->attribs[ATTRIB_INDEX_POSITION].normalized       = GL_FALSE;
	vc.vao->attribs[ATTRIB_INDEX_TEXCOORD].normalized       = GL_FALSE;
//	vc.vao->attribs[ATTRIB_INDEX_LIGHTCOORD].normalized     = GL_FALSE;
	vc.vao->attribs[ATTRIB_INDEX_NORMAL].normalized         = GL_TRUE;
//	vc.vao->attribs[ATTRIB_INDEX_TANGENT].normalized        = GL_TRUE;
//	vc.vao->attribs[ATTRIB_INDEX_LIGHTDIRECTION].normalized = GL_TRUE;
	vc.vao->attribs[ATTRIB_INDEX_COLOR].normalized          = GL_TRUE;

	vc.vao->attribs[ATTRIB_INDEX_POSITION].offset       = offsetof(drawVert_t, xyz);
	vc.vao->attribs[ATTRIB_INDEX_TEXCOORD].offset       = offsetof(drawVert_t, uv);
//	vc.vao->attribs[ATTRIB_INDEX_LIGHTCOORD].offset     = offsetof(drawVert_t, lightmap);
	vc.vao->attribs[ATTRIB_INDEX_NORMAL].offset         = offsetof(drawVert_t, normal);
//	vc.vao->attribs[ATTRIB_INDEX_TANGENT].offset        = offsetof(drawVert_t, tangent);
//	vc.vao->attribs[ATTRIB_INDEX_LIGHTDIRECTION].offset = offsetof(drawVert_t, lightdir);
	vc.vao->attribs[ATTRIB_INDEX_COLOR].offset          = offsetof(drawVert_t, color);

	R_SetVertexPointers(vc.vao->attribs, sizeof(drawVert_t));

	vc.numSurfaces = 0;
	vc.numBatches = 0;
	vc.vertexOffset = 0;
	vc.indexOffset = 0;
	vcq.vertexCommitSize = 0;
	vcq.indexCommitSize = 0;
	vcq.numSurfaces = 0;
}

void VaoCache_BindVao(void)
{
    VBO_Bind(vc.vao);
}

void VaoCache_CheckAdd(qboolean *endSurface, qboolean *recycleVertexBuffer, qboolean *recycleIndexBuffer, uint32_t numVerts, uint32_t numIndexes)
{
	uint64_t vertexesSize = sizeof(drawVert_t) * numVerts;
	uint64_t indexesSize = sizeof(glIndex_t) * numIndexes;

	if (vc.vao->vertices.size < vc.vertexOffset + vcq.vertexCommitSize + vertexesSize) {
		//ri.Printf(PRINT_ALL, "out of space in vertex cache: %d < %d + %d + %d\n", vc.vao->vertexesSize, vc.vertexOffset, vcq.vertexCommitSize, vertexesSize);
		*recycleVertexBuffer = qtrue;
		*recycleIndexBuffer = qtrue;
		*endSurface = qtrue;
	}

	if (vc.vao->indices.size < vc.indexOffset + vcq.indexCommitSize + indexesSize) {
		//ri.Printf(PRINT_ALL, "out of space in index cache\n");
		*recycleIndexBuffer = qtrue;
		*endSurface = qtrue;
	}

	if (vc.numSurfaces + vcq.numSurfaces >= VAOCACHE_MAX_SURFACES) {
		//ri.Printf(PRINT_ALL, "out of surfaces in index cache\n");
		*recycleIndexBuffer = qtrue;
		*endSurface = qtrue;
	}

	if (vc.numBatches >= VAOCACHE_MAX_BATCHES) {
		//ri.Printf(PRINT_ALL, "out of batches in index cache\n");
		*recycleIndexBuffer = qtrue;
		*endSurface = qtrue;
	}

	if (vcq.numSurfaces >= VAOCACHE_QUEUE_MAX_SURFACES) {
		//ri.Printf(PRINT_ALL, "out of queued surfaces\n");
		*endSurface = qtrue;
	}

	if (VAOCACHE_QUEUE_MAX_VERTEXES * sizeof(drawVert_t) < vcq.vertexCommitSize + vertexesSize) {
		//ri.Printf(PRINT_ALL, "out of queued vertexes\n");
		*endSurface = qtrue;
	}

	if (VAOCACHE_QUEUE_MAX_INDEXES * sizeof(glIndex_t) < vcq.indexCommitSize + indexesSize) {
		//ri.Printf(PRINT_ALL, "out of queued indexes\n");
		*endSurface = qtrue;
	}
}

void VaoCache_RecycleVertexBuffer(void)
{
	nglBindBuffer(GL_ARRAY_BUFFER, vc.vao->vboId);
	nglBufferData(GL_ARRAY_BUFFER, vc.vao->vertices.size, NULL, GL_DYNAMIC_DRAW);
	vc.vertexOffset = 0;
}

void VaoCache_RecycleIndexBuffer(void)
{
	nglBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vc.vao->iboId);
	nglBufferData(GL_ELEMENT_ARRAY_BUFFER, vc.vao->indices.size, NULL, GL_DYNAMIC_DRAW);
	vc.indexOffset = 0;
	vc.numSurfaces = 0;
	vc.numBatches = 0;
}

void VaoCache_InitQueue(void)
{
	vcq.vertexCommitSize = 0;
	vcq.indexCommitSize = 0;
	vcq.numSurfaces = 0;
}

void VaoCache_AddSurface(drawVert_t *verts, uint32_t numVerts, glIndex_t *indexes, uint32_t numIndexes)
{
	queuedSurface_t *queueEntry = vcq.surfaces + vcq.numSurfaces;
	queueEntry->vertexes = verts;
	queueEntry->numVerts = numVerts;
	queueEntry->indexes = indexes;
	queueEntry->numIndexes = numIndexes;
	vcq.numSurfaces++;

	vcq.vertexCommitSize += sizeof(drawVert_t) * numVerts;
	vcq.indexCommitSize += sizeof(glIndex_t) * numIndexes;
}

