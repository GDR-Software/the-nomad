#include "rgl_local.h"

// this is here to that functions in n_shared.c can link

void GDR_DECL N_Error(errorCode_t code, const char *err, ...)
{
    va_list argptr;
    char msg[MAXPRINTMSG];

    va_start(argptr, err);
    N_vsnprintf(msg, sizeof(msg), err, argptr);
    va_end(argptr);

    ri.Error(code, "%s", msg);
}

void GDR_DECL Con_Printf(const char *fmt, ...)
{
    va_list argptr;
    char msg[MAXPRINTMSG];

    va_start(argptr, fmt);
    N_vsnprintf(msg, sizeof(msg), fmt, argptr);
    va_end(argptr);

    ri.Printf(PRINT_INFO, "%s", msg);
}

void RB_MakeViewMatrix( void )
{
    float aspect;

    aspect = glConfig.vidWidth / glConfig.vidHeight;

    VectorSet( glState.viewData.camera.origin, 0.0f, 0.0f, 0.0f );

    glState.viewData.zFar = -1.0f;
    glState.viewData.zNear = 1.0f;

    ri.GLM_MakeVPM( aspect, &glState.viewData.camera.zoom, glState.viewData.camera.origin, glState.viewData.camera.viewProjectionMatrix,
        glState.viewData.camera.projectionMatrix, glState.viewData.camera.viewMatrix );

#if 0
    // setup ortho projection matrix
    glm_ortho( -aspect * zoom, aspect * zoom, -aspect, aspect, glState.viewData.zNear, glState.viewData.zFar, glState.viewData.camera.projectionMatrix );

    // create the view matrix
    glm_mat4_identity( transpose );
    glm_translate( transpose, glState.viewData.camera.origin );
    glm_mat4_inv( transpose, transpose );
    glm_mat4_copy( glState.viewData.camera.viewMatrix, transpose );

    // make the final thinga-majig
    glm_mat4_mul( glState.viewData.camera.projectionMatrix, glState.viewData.camera.viewMatrix, glState.viewData.camera.viewProjectionMatrix );
#endif
}

/*
===============
R_Radix
===============
*/
static GDR_INLINE void R_RadixPoly( int32_t byte, uint32_t size, const srfPoly_t *source, srfPoly_t *dest )
{
    uint32_t       count[ 256 ] = { 0 };
    uint32_t       index[ 256 ];
    uint32_t       i;
    uint8_t        *sortKey;
    uint8_t        *end;

    sortKey = ( (uint8_t *)&source[ 0 ].hShader ) + byte;
    end = sortKey + ( size * sizeof( srfPoly_t ) );
    for ( ; sortKey < end; sortKey += sizeof( srfPoly_t ) ) {
        ++count[ *sortKey ];
    }

    index[ 0 ] = 0;

    for ( i = 1; i < 256; ++i ) {
        index[ i ] = index[ i - 1 ] + count[ i - 1 ];
    }

    sortKey = ( (uint8_t *)&source[ 0 ].hShader ) + byte;
    for ( i = 0; i < size; ++i, sortKey += sizeof( srfPoly_t ) ) {
        dest[ index[ *sortKey ]++ ] = source[ i ];
    }
}

/*
===============
R_Radix
===============
*/
static GDR_INLINE void R_RadixQuad( int32_t byte, uint32_t size, const srfQuad_t *source, srfQuad_t *dest )
{
    uint32_t       count[ 256 ] = { 0 };
    uint32_t       index[ 256 ];
    uint32_t       i;
    uint8_t        *sortKey;
    uint8_t        *end;

    sortKey = ( (uint8_t *)&source[ 0 ].hSpriteSheet ) + byte;
    end = sortKey + ( size * sizeof( srfQuad_t ) );
    for ( ; sortKey < end; sortKey += sizeof( srfQuad_t ) ) {
        ++count[ *sortKey ];
    }

    index[ 0 ] = 0;

    for ( i = 1; i < 256; ++i ) {
        index[ i ] = index[ i - 1 ] + count[ i - 1 ];
    }

    sortKey = ( (uint8_t *)&source[ 0 ].hSpriteSheet ) + byte;
    for ( i = 0; i < size; ++i, sortKey += sizeof( srfQuad_t ) ) {
        dest[ index[ *sortKey ]++ ] = source[ i ];
    }
}

/*
===============
R_RadixSort

Radix sort with 4 byte size buckets
===============
*/
static void R_RadixSortPoly( srfPoly_t *source, uint32_t size )
{
    srfPoly_t *scratch = (srfPoly_t *)ri.Hunk_AllocateTempMemory( sizeof(*scratch) * r_maxPolys->i );
    if ( !scratch ) {
        N_Error( ERR_FATAL, "R_RadixSortPoly: failed to allocate sufficient memory for scratch sort buffer" );
    }
#ifdef GDR_LITTLE_ENDIAN
    R_RadixPoly( 0, size, source, scratch );
    R_RadixPoly( 1, size, scratch, source );
    R_RadixPoly( 2, size, source, scratch );
    R_RadixPoly( 3, size, scratch, source );
#else
    R_RadixPoly( 3, size, source, scratch );
    R_RadixPoly( 2, size, scratch, source );
    R_RadixPoly( 1, size, source, scratch );
    R_RadixPoly( 0, size, scratch, source );
#endif //Q3_LITTLE_ENDIAN
    ri.Hunk_FreeTempMemory( scratch );
}

/*
===============
R_RadixSort

Radix sort with 4 byte size buckets
===============
*/
static void R_RadixSortQuad( srfQuad_t *source, uint32_t size )
{
    srfQuad_t *scratch = (srfQuad_t *)ri.Hunk_AllocateTempMemory( sizeof(*scratch) * r_maxQuads->i );
    if ( !scratch ) {
        N_Error( ERR_FATAL, "R_RadixSortQuad: failed to allocate sufficient memory for scratch sort buffer" );
    }
#ifdef GDR_LITTLE_ENDIAN
    R_RadixQuad( 0, size, source, scratch );
    R_RadixQuad( 1, size, scratch, source );
    R_RadixQuad( 2, size, source, scratch );
    R_RadixQuad( 3, size, scratch, source );
#else
    R_RadixQuad( 3, size, source, scratch );
    R_RadixQuad( 2, size, scratch, source );
    R_RadixQuad( 1, size, source, scratch );
    R_RadixQuad( 0, size, scratch, source );
#endif //Q3_LITTLE_ENDIAN
    ri.Hunk_FreeTempMemory( scratch );
}

extern uint64_t r_numPolys, r_numPolyVerts, r_numQuads;

static int SortPoly( const void *a, const void *b ) {
    return (int)( ( (srfPoly_t *)a )->hShader - ( (srfPoly_t *)b )->hShader );
}

static int SortQuad( const void *a, const void *b ) {
    return (int)( ( (srfQuad_t *)a )->hSpriteSheet - ( (srfQuad_t *)b )->hSpriteSheet );
}

void R_WorldToGL( drawVert_t *verts, vec3_t pos )
{
    vec3_t xyz[4];

    ri.GLM_TransformToGL( pos, xyz, glState.viewData.camera.viewProjectionMatrix );

    for (uint32_t i = 0; i < 4; ++i)
        VectorCopy( verts[i].xyz, xyz[i] );
}

void R_WorldToGL2( polyVert_t *verts, vec3_t pos )
{
    vec3_t xyz[4];

    ri.GLM_TransformToGL( pos, xyz, glState.viewData.camera.viewProjectionMatrix );

    for (uint32_t i = 0; i < 4; ++i)
        VectorCopy( verts[i].xyz, xyz[i] );
}

/*
* R_DrawPolys: draws all sprites and polygons submitted into the current scene
*/
void R_DrawPolys( void )
{
    uint64_t i;
    const srfPoly_t *poly;
    const glIndex_t *idx;
    nhandle_t oldShader;
    polyVert_t *vtx;
    vec3_t pos;
    uint32_t numVerts;
    uint32_t numIndices;

    // no polygon submissions this frame
    if ( !r_numPolys && !r_numPolyVerts ) {
        return;
    }

    RB_SetBatchBuffer( backend.drawBuffer, backendData->polyVerts, sizeof(polyVert_t), backendData->indices, sizeof(glIndex_t) );

    // sort the polys to be more efficient with our shaders
#if 1
    R_RadixSortPoly( backendData->polys, r_numPolys );
#else
    qsort( backendData->polys, r_numPolys, sizeof(srfPoly_t), SortPoly );
#endif

    // submit all the indices
    RB_CommitDrawData( NULL, 0, backendData->indices, backendData->numIndices );

    vtx = backendData->polyVerts;
    poly = backendData->polys;
    
    oldShader = poly->hShader;
    backend.drawBatch.shader = R_GetShaderByHandle( oldShader );

    RB_CommitDrawData( NULL, 0, backendData->indices, backendData->numIndices );

    GLSL_UseProgram( &rg.basicShader );
    
    for ( i = 0; i < r_numPolys; i++ ) {
        if ( oldShader != poly->hShader ) {
            // if we have a new shader, flush the current batch
            RB_FlushBatchBuffer();
            oldShader = poly->hShader;
            backend.drawBatch.shader = R_GetShaderByHandle( poly->hShader );
        }

        // submit to draw buffer
        RB_CommitDrawData( poly->verts, poly->numVerts, NULL, 0 );
        
        poly++;
    }

    // flush out anything remaining
    RB_FlushBatchBuffer();
}

static void R_DrawWorld( void )
{
    uint32_t y, x;
    uint32_t i;
    vec3_t pos;
    drawVert_t *vtx;

    if ((backend.refdef.flags & RSF_NOWORLDMODEL)) {
        // nothing to draw
        return;
    }

    if (!rg.world) {
        ri.Error( ERR_FATAL, "R_DrawWorld: no world model loaded" );
    }

    // prepare the batch
    RB_SetBatchBuffer( rg.world->buffer, rg.world->vertices, sizeof(drawVert_t), rg.world->indices, sizeof(glIndex_t) );

    // submit the indices, we won't be ever changing those
    RB_CommitDrawData( NULL, 0, rg.world->indices, rg.world->numIndices );

    backend.drawBatch.shader = rg.world->shader;

    vtx = rg.world->vertices;

    for (y = 0; y < rg.world->height; y++) {
        for (x = 0; x < rg.world->width; x++) {
            pos[0] = x - ( glState.viewData.camera.origin[0] * 0.5f );
            pos[1] = glState.viewData.camera.origin[1] - y;
//            pos[0] = x - (rg.world->width * 0.5f);
//            pos[1] = rg.world->height - y;
            pos[2] = 0.0f;

            // convert the local world coordinates to OpenGL screen coordinates
            R_WorldToGL( vtx, pos );

            for (i = 0; i < 4; i++) {
                VectorCopy2( vtx[i].uv, rg.world->tiles[y * rg.world->width + x].texcoords[i] );
                VectorSet( vtx[i].worldPos, x, y, 0.0f );
            }

            // submit the processed vertices
            RB_CommitDrawData( vtx, 4, NULL, 0 );

            vtx += 4;
        }
    }

    // flush it we have anything left in there
    RB_FlushBatchBuffer();
}

/*
=================
R_SetupFrustum

Set up the culling frustum planes for the current view using the results we got from computing the first two rows of
the projection matrix.
=================
*/
static void R_SetupFrustum( viewData_t *dest, float xmin, float xmax, float ymax, float zProj, float zFar, float stereoSep )
{
	vec3_t ofsorigin;
	float oppleg, adjleg, length;
    uint32_t i;

	if ( stereoSep == 0 && xmin == -xmax ) {
		// symmetric case can be simplified
		VectorCopy( dest->camera.origin, ofsorigin );

		length = sqrt( xmax * xmax + zProj * zProj );
		oppleg = xmax / length;
		adjleg = zProj / length;

		VectorScale( dest->camera.axis[0], oppleg, dest->frustum[0].normal );
		VectorMA( dest->frustum[0].normal, adjleg, dest->camera.axis[1], dest->frustum[0].normal );

		VectorScale( dest->camera.axis[0], oppleg, dest->frustum[1].normal );
		VectorMA( dest->frustum[1].normal, -adjleg, dest->camera.axis[1], dest->frustum[1].normal );
	}
	else {
		// In stereo rendering, due to the modification of the projection matrix, dest->camera.origin is not the
		// actual origin that we're rendering so offset the tip of the view pyramid.
		VectorMA( dest->camera.origin, stereoSep, dest->camera.axis[1], ofsorigin );
	
		oppleg = xmax + stereoSep;
		length = sqrt( oppleg * oppleg + zProj * zProj );
		VectorScale( dest->camera.axis[0], oppleg / length, dest->frustum[0].normal );
		VectorMA( dest->frustum[0].normal, zProj / length, dest->camera.axis[1], dest->frustum[0].normal );

		oppleg = xmin + stereoSep;
		length = sqrt( oppleg * oppleg + zProj * zProj);
		VectorScale( dest->camera.axis[0], -oppleg / length, dest->frustum[1].normal );
		VectorMA( dest->frustum[1].normal, -zProj / length, dest->camera.axis[1], dest->frustum[1].normal );
	}

	length = sqrt( ymax * ymax + zProj * zProj );
	oppleg = ymax / length;
	adjleg = zProj / length;

	VectorScale( dest->camera.axis[0], oppleg, dest->frustum[2].normal );
	VectorMA( dest->frustum[2].normal, adjleg, dest->camera.axis[2], dest->frustum[2].normal );

	VectorScale( dest->camera.axis[0], oppleg, dest->frustum[3].normal );
	VectorMA( dest->frustum[3].normal, -adjleg, dest->camera.axis[2], dest->frustum[3].normal );
	
	for ( i = 0; i < 4; i++ ) {
		dest->frustum[i].type = PLANE_NON_AXIAL;
		dest->frustum[i].dist = DotProduct( ofsorigin, dest->frustum[i].normal );
		SetPlaneSignbits( &dest->frustum[i] );
	}

	if ( zFar != 0.0f ) {
		vec3_t farpoint;

		VectorMA( ofsorigin, zFar, dest->camera.axis[0], farpoint );
		VectorScale( dest->camera.axis[0], -1.0f, dest->frustum[4].normal );

		dest->frustum[4].type = PLANE_NON_AXIAL;
		dest->frustum[4].dist = DotProduct( farpoint, dest->frustum[4].normal );
		SetPlaneSignbits( &dest->frustum[4] );
//		dest->flags |= VPF_FARPLANEFRUSTUM;
	}
}

/*
===============
R_SetupProjection
===============
*/
void R_SetupProjection( viewData_t *dest, float zProj, float zFar, qboolean computeFrustum )
{
	float	xmin, xmax, ymin, ymax;
	float	width, height, stereoSep = r_stereoSeparation->f;
    float   *projectionMatrix;

	/*
	 * offset the view origin of the viewer for stereo rendering 
	 * by setting the projection matrix appropriately.
	 */

	if ( stereoSep != 0 ) {
		if ( dest->stereoFrame == STEREO_LEFT ) {
			stereoSep = zProj / stereoSep;
        } else if ( dest->stereoFrame == STEREO_RIGHT ) {
			stereoSep = zProj / -stereoSep;
    	} else {
			stereoSep = 0;
        }
	}

	ymax = zProj * tan( dest->fovY * M_PI / 360.0f );
	ymin = -ymax;

	xmax = zProj * tan( dest->fovX * M_PI / 360.0f );
	xmin = -xmax;

	width = xmax - xmin;
	height = ymax - ymin;

    projectionMatrix = &dest->camera.projectionMatrix[0][0];
	
	projectionMatrix[0] = 2 * zProj / width;
	projectionMatrix[4] = 0;
	projectionMatrix[8] = ( xmax + xmin + 2 * stereoSep ) / width;
	projectionMatrix[12] = 2 * zProj * stereoSep / width;

	projectionMatrix[1] = 0;
	projectionMatrix[5] = 2 * zProj / height;
	projectionMatrix[9] = ( ymax + ymin ) / height;	// normally 0
	projectionMatrix[13] = 0;

	projectionMatrix[3] = 0;
	projectionMatrix[7] = 0;
	projectionMatrix[11] = -1;
	projectionMatrix[15] = 0;
	
	// Now that we have all the data for the projection matrix we can also setup the view frustum.
	if ( computeFrustum ) {
		R_SetupFrustum( dest, xmin, xmax, ymax, zProj, zFar, stereoSep );
    }

    ri.GLM_MakeVPMQuake3( dest->camera.viewProjectionMatrix, dest->camera.projectionMatrix, dest->camera.viewMatrix );
}

void R_RenderView( const viewData_t *parms )
{
    rg.viewCount++;

    if ( parms->flags & RSF_NOWORLDMODEL ) {
        // do things Quake3 style

        memcpy( &glState.viewData, parms, sizeof(*parms) );

        R_SetupProjection( &glState.viewData, r_zproj->f, parms->zFar, qtrue );

        // issue all currently pending rendering commands
        R_IssuePendingRenderCommands();

        // draw all submitted images
        R_DrawPolys();

        return;
    }

    memcpy( &glState.viewData, parms, sizeof(*parms) );

    // setup the correct matrices
    RB_MakeViewMatrix();

    // issue all currently queued rendering commands
    R_IssuePendingRenderCommands();

    // draw the tilemap
    R_DrawWorld();

    // render all submitted sgame polygons
    R_DrawPolys();

    GLSL_UseProgram( NULL );
}


static void R_CalcSpriteTextureCoords( uint32_t x, uint32_t y, uint32_t spriteWidth, uint32_t spriteHeight,
    uint32_t sheetWidth, uint32_t sheetHeight, spriteCoord_t texCoords )
{
    const vec2_t min = { (((float)x + 1) * spriteWidth) / sheetWidth, (((float)y + 1) * spriteHeight) / sheetHeight };
    const vec2_t max = { ((float)x * spriteWidth) / sheetWidth, ((float)y * spriteHeight) / sheetHeight };

    texCoords[0][0] = min[0];
    texCoords[0][1] = max[1];

    texCoords[1][0] = min[0];
    texCoords[1][1] = min[1];

    texCoords[2][0] = max[0];
    texCoords[2][1] = min[1];
    
    texCoords[3][0] = max[0];
    texCoords[3][1] = max[1];
}

void R_ScreenToGL(vec3_t *xyz)
{
    xyz[0][0] = 2.0f * xyz[0][0] / glConfig.vidWidth - 1.0f;
	xyz[0][1] = 1.0f - 2.0f * xyz[0][1] / glConfig.vidHeight;

    xyz[1][0] = 2.0f * xyz[1][0] / glConfig.vidWidth - 1.0f;
	xyz[1][1] = 1.0f - 2.0f * xyz[1][1] / glConfig.vidHeight;

    xyz[2][0] = 2.0f * xyz[2][0] / glConfig.vidWidth - 1.0f;
	xyz[2][1] = 1.0f - 2.0f * xyz[2][1] / glConfig.vidHeight;

    xyz[3][0] = 2.0f * xyz[3][0] / glConfig.vidWidth - 1.0f;
	xyz[3][1] = 1.0f - 2.0f * xyz[3][1] / glConfig.vidHeight;
}

nhandle_t RE_RegisterSpriteSheet( const char *npath, uint32_t sheetWidth, uint32_t sheetHeight, uint32_t spriteWidth, uint32_t spriteHeight )
{
    spriteSheet_t *sheet;
    uint64_t len;
    uint64_t size;
    nhandle_t handle;
    uint32_t numSprites, spriteCountY, spriteCountX;
    sprite_t *sprite;

    len = strlen( npath );

    if ( ( (float)sheetWidth / (float)spriteWidth ) != (int)( (float)sheetWidth / (float)spriteWidth )
        ||  ( (float)sheetHeight / (float)spriteHeight ) != (int)( (float)sheetHeight / (float)spriteHeight ) )
    {
        ri.Error( ERR_DROP, "RE_RegisterSpriteSheet: please ensure your sprite dimensions and sheet dimensions are powers of two" );
    }
    if ( len >= MAX_GDR_PATH ) {
        ri.Error( ERR_DROP, "RE_RegisterSpriteSheet: name '%s' too long", npath );
    }

    //
    // check if we already have it
    //
    for ( uint64_t i = 0; i < rg.numSpriteSheets; i++ ) {
        if ( !N_stricmp( npath, rg.sheets[i]->name ) ) {
            return (nhandle_t)i;
        }
    }

    len = PAD( len, sizeof(uintptr_t) );

    numSprites = ( sheetWidth / spriteWidth ) * ( sheetHeight / spriteHeight );

    size = 0;
    size += PAD( sizeof(*sheet), sizeof(uintptr_t) );
    size += PAD( sizeof(*sheet->sprites) * numSprites, sizeof(uintptr_t) );
    size += PAD( len, sizeof(uintptr_t) );

    handle = rg.numSpriteSheets;
    sheet = rg.sheets[rg.numSpriteSheets] = (spriteSheet_t *)ri.Hunk_Alloc( size, h_low );
    sheet->name = (char *)( sheet + 1 );
    sheet->sprites = (sprite_t *)( sheet->name + len );

    strcpy( sheet->name, npath );

    sheet->numSprites = numSprites;
    sheet->sheetWidth = sheetWidth;
    sheet->sheetHeight = sheetHeight;
    sheet->spriteWidth = spriteWidth;
    sheet->spriteHeight = spriteHeight;

    spriteCountX = sheetWidth / spriteWidth;
    spriteCountY = sheetHeight / spriteHeight;

    sheet->hShader = RE_RegisterShader( npath );
    if ( sheet->hShader == FS_INVALID_HANDLE ) {
        ri.Printf( PRINT_WARNING, "RE_RegisterSpriteSheet: failed to find shader '%s'.\n", npath );
        return FS_INVALID_HANDLE;
    }

    sprite = sheet->sprites;
    for ( uint32_t y = 0; y < spriteCountY; y++ ) {
        for ( uint32_t x = 0; x < spriteCountX; x++ ) {
            R_CalcSpriteTextureCoords( x, y, spriteWidth, spriteHeight, sheetWidth, sheetHeight, sheet->sprites[y * spriteCountX + x].texCoords );
            sprite->hSpriteSheet = handle;
        }
    }

    rg.numSpriteSheets++;

    return handle;
}

nhandle_t RE_RegisterSprite( nhandle_t hSpriteSheet, uint32_t index )
{
    spriteSheet_t *sheet;

    if ( hSpriteSheet == FS_INVALID_HANDLE ) {
        ri.Printf( PRINT_WARNING, "RE_RegisterSprite: bad sprite sheet given.\n" );
        return FS_INVALID_HANDLE;
    }

    sheet = rg.sheets[hSpriteSheet];

    if ( index >= sheet->numSprites ) {
        ri.Printf( PRINT_WARNING, "RE_RegisterSprite: invalid index given.\n" );
    }

    return (nhandle_t)index; // nothing too fancy...
}

/*
=============
R_CalcTexDirs

Lengyel, Eric. �Computing Tangent Space Basis Vectors for an Arbitrary Mesh�. Terathon Software 3D Graphics Library, 2001. http://www.terathon.com/code/tangent.html
=============
*/
void R_CalcTexDirs(vec3_t sdir, vec3_t tdir, const vec3_t v1, const vec3_t v2,
				   const vec3_t v3, const vec2_t w1, const vec2_t w2, const vec2_t w3)
{
	float			x1, x2, y1, y2, z1, z2;
	float			s1, s2, t1, t2, r;

	x1 = v2[0] - v1[0];
	x2 = v3[0] - v1[0];
	y1 = v2[1] - v1[1];
	y2 = v3[1] - v1[1];
	z1 = v2[2] - v1[2];
	z2 = v3[2] - v1[2];

	s1 = w2[0] - w1[0];
	s2 = w3[0] - w1[0];
	t1 = w2[1] - w1[1];
	t2 = w3[1] - w1[1];

	r = s1 * t2 - s2 * t1;
	if (r) r = 1.0f / r;

	VectorSet(sdir, (t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
	VectorSet(tdir, (s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);
}

/*
=============
R_CalcTangentSpace

Lengyel, Eric. �Computing Tangent Space Basis Vectors for an Arbitrary Mesh�. Terathon Software 3D Graphics Library, 2001. http://www.terathon.com/code/tangent.html
=============
*/
vec_t R_CalcTangentSpace( vec3_t tangent, vec3_t bitangent, const vec3_t normal, const vec3_t sdir, const vec3_t tdir )
{
	vec3_t n_cross_t;
	vec_t n_dot_t, handedness;

	// Gram-Schmidt orthogonalize
	n_dot_t = DotProduct(normal, sdir);
	VectorMA(sdir, -n_dot_t, normal, tangent);
	VectorNormalize(tangent);

	// Calculate handedness
	CrossProduct(normal, sdir, n_cross_t);
	handedness = (DotProduct(n_cross_t, tdir) < 0.0f) ? -1.0f : 1.0f;

	// Calculate orthogonal bitangent, if necessary
	if (bitangent)
		CrossProduct(normal, tangent, bitangent);

	return handedness;
}

qboolean R_CalcTangentVectors(drawVert_t * dv[3])
{
	int             i;
	float           bb, s, t;
	vec3_t          bary;


	/* calculate barycentric basis for the triangle */
	bb = (dv[1]->uv[0] - dv[0]->uv[0]) * (dv[2]->uv[1] - dv[0]->uv[1]) - (dv[2]->uv[0] - dv[0]->uv[0]) * (dv[1]->uv[1] - dv[0]->uv[1]);
	if(fabs(bb) < 0.00000001f)
		return qfalse;

	/* do each vertex */
	for(i = 0; i < 3; i++)
	{
		vec4_t tangent;
		vec3_t normal, bitangent, nxt;

		// calculate s tangent vector
		s = dv[i]->uv[0] + 10.0f;
		t = dv[i]->uv[1];
		bary[0] = ((dv[1]->uv[0] - s) * (dv[2]->uv[1] - t) - (dv[2]->uv[0] - s) * (dv[1]->uv[1] - t)) / bb;
		bary[1] = ((dv[2]->uv[0] - s) * (dv[0]->uv[1] - t) - (dv[0]->uv[0] - s) * (dv[2]->uv[1] - t)) / bb;
		bary[2] = ((dv[0]->uv[0] - s) * (dv[1]->uv[1] - t) - (dv[1]->uv[0] - s) * (dv[0]->uv[1] - t)) / bb;

		tangent[0] = bary[0] * dv[0]->xyz[0] + bary[1] * dv[1]->xyz[0] + bary[2] * dv[2]->xyz[0];
		tangent[1] = bary[0] * dv[0]->xyz[1] + bary[1] * dv[1]->xyz[1] + bary[2] * dv[2]->xyz[1];
		tangent[2] = bary[0] * dv[0]->xyz[2] + bary[1] * dv[1]->xyz[2] + bary[2] * dv[2]->xyz[2];

		VectorSubtract(tangent, dv[i]->xyz, tangent);
		VectorNormalize(tangent);

		// calculate t tangent vector
		s = dv[i]->uv[0];
		t = dv[i]->uv[1] + 10.0f;
		bary[0] = ((dv[1]->uv[0] - s) * (dv[2]->uv[1] - t) - (dv[2]->uv[0] - s) * (dv[1]->uv[1] - t)) / bb;
		bary[1] = ((dv[2]->uv[0] - s) * (dv[0]->uv[1] - t) - (dv[0]->uv[0] - s) * (dv[2]->uv[1] - t)) / bb;
		bary[2] = ((dv[0]->uv[0] - s) * (dv[1]->uv[1] - t) - (dv[1]->uv[0] - s) * (dv[0]->uv[1] - t)) / bb;

		bitangent[0] = bary[0] * dv[0]->xyz[0] + bary[1] * dv[1]->xyz[0] + bary[2] * dv[2]->xyz[0];
		bitangent[1] = bary[0] * dv[0]->xyz[1] + bary[1] * dv[1]->xyz[1] + bary[2] * dv[2]->xyz[1];
		bitangent[2] = bary[0] * dv[0]->xyz[2] + bary[1] * dv[1]->xyz[2] + bary[2] * dv[2]->xyz[2];

		VectorSubtract(bitangent, dv[i]->xyz, bitangent);
		VectorNormalize(bitangent);

		// store bitangent handedness
		R_VaoUnpackNormal(normal, dv[i]->normal);
		CrossProduct(normal, tangent, nxt);
		tangent[3] = (DotProduct(nxt, bitangent) < 0.0f) ? -1.0f : 1.0f;

		R_VaoPackTangent(dv[i]->tangent, tangent);

		// debug code
		//% Sys_FPrintf( SYS_VRB, "%d S: (%f %f %f) T: (%f %f %f)\n", i,
		//%     stv[ i ][ 0 ], stv[ i ][ 1 ], stv[ i ][ 2 ], ttv[ i ][ 0 ], ttv[ i ][ 1 ], ttv[ i ][ 2 ] );
	}

	return qtrue;
}

/*
==========================
myGlMultMatrix

==========================
*/
static void myGlMultMatrix( const float *a, const float *b, float *out ) {
	int		i, j;

	for ( i = 0 ; i < 4 ; i++ ) {
		for ( j = 0 ; j < 4 ; j++ ) {
			out[ i * 4 + j ] =
				a [ i * 4 + 0 ] * b [ 0 * 4 + j ]
				+ a [ i * 4 + 1 ] * b [ 1 * 4 + j ]
				+ a [ i * 4 + 2 ] * b [ 2 * 4 + j ]
				+ a [ i * 4 + 3 ] * b [ 3 * 4 + j ];
		}
	}
}

static const float s_flipMatrix[16] = {
	// convert from Q3's coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	 0, 0, -1, 0,
	-1, 0,  0, 0,
	 0, 1,  0, 0,
	 0, 0,  0, 1
};

