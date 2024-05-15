// rgl_scene.c -- only one scene per frame. All rendering data submissions must come through here or rgl_cmd.c

#include "rgl_local.h"

uint64_t r_numEntities;
uint64_t r_firstSceneEntity;

uint64_t r_numDLights;
uint64_t r_firstSceneDLight;

uint64_t r_numPolys;
uint64_t r_firstScenePoly;

uint64_t r_numPolyVerts;
uint64_t r_firstSceneVert;

uint64_t r_numQuads;

void R_InitNextFrame( void )
{
    backendData->commandList.usedBytes = 0;
    backendData->numIndices = 0;

    r_firstSceneDLight = 0;
    r_numDLights = 0;

    r_firstSceneEntity = 0;
    r_numEntities = 0;

    r_firstScenePoly = 0;
    r_numPolys = 0;

    r_firstSceneVert = 0;
    r_numPolyVerts = 0;
}

void RE_AddSpriteToScene( const vec3_t origin, nhandle_t hSpriteSheet, nhandle_t hSprite )
{
    srfPoly_t *poly;
    polyVert_t *vtx;
    srfVert_t *vt;
    vec3_t pos;
    uint32_t i;

    if ( !rg.registered ) {
        return;
    }
    if ( !rg.world ) {
        ri.Error( ERR_DROP, "RE_AddSpriteToScene: no world loaded\n" );
    }

    if ( !rg.sheets[ hSpriteSheet ] ) {
        ri.Printf( PRINT_DEVELOPER, "RE_AddSpriteToScene: invalid sprite sheet %i\n", hSpriteSheet );
        return;
    }
    if ( r_numPolyVerts + 4 >= r_maxPolys->i * 4 ) {
        ri.Printf( PRINT_DEVELOPER, "RE_AddSpriteToScene: r_maxPolyVerts hit, dropping %i vertices\n", 4 );
        return;
    }
//    ri.Printf( PRINT_INFO, "Drawing sprite at [ %f, %f, %f ] with sprite sheet %i\n", origin[0], origin[1], origin[2], hSpriteSheet );

    poly = &backendData->polys[r_numPolys];
    vtx = &backendData->polyVerts[r_numPolyVerts];

    pos[0] = origin[0];
    pos[1] = rg.world->height - origin[1];
    pos[2] = origin[2];

    R_WorldToGL2( vtx, pos, 4 );

    poly->verts = vtx;
    poly->numVerts = 4;
    poly->hShader = rg.sheets[hSpriteSheet]->hShader;

    ri.Printf( PRINT_INFO, "Drawing Sprite:\n" );
    for ( i = 0; i < 4; i++ ) {
        VectorCopy2( vtx[i].uv, rg.sheets[ hSpriteSheet ]->sprites[ hSprite ].texCoords[i] );
        VectorCopy( vtx[i].worldPos, pos );
        ri.Printf( PRINT_INFO, " xyz[ %i ]: %f, %f, %f\n", i, vtx[i].xyz[0], vtx[i].xyz[1], vtx[i].xyz[2] );
    }

    r_numPolyVerts += 4;
    r_numPolys++;
}

void RE_AddPolyToScene( nhandle_t hShader, const polyVert_t *verts, uint32_t numVerts )
{
    uint32_t i, offset;
    polyVert_t *vt;
    srfPoly_t *poly;

    if ( !rg.registered ) {
        return;
    }

    if ( r_numPolyVerts + numVerts >= r_maxPolys->i * 4 ) {
        ri.Printf( PRINT_DEVELOPER, "RE_AddPolyToScene: r_maxPolyVerts hit, dropping %i vertices\n", numVerts );
        return;
    }

    if ( hShader >= rg.numShaders || hShader < 0 ) {
        ri.Printf( PRINT_WARNING, "RE_AddPolyToScene: out of range hShader '%i'\n", hShader );
        return;
    }

    poly = &backendData->polys[numVerts];
    vt = &backendData->polyVerts[r_numPolyVerts];

    poly->verts = vt;
    poly->hShader = hShader;
    poly->numVerts = numVerts;

    memcpy( vt, verts, sizeof( *vt ) * numVerts );

    r_numPolyVerts += numVerts;
    r_numPolys++;
}

void RE_AddPolyListToScene(const poly_t *polys, uint32_t numPolys)
{
    uint32_t i;

    if (!rg.registered) {
        return;
    }

    if (r_numPolys + numPolys >= r_maxPolys->i) {
        ri.Printf(PRINT_DEVELOPER, "RE_AddPolyListToScene: r_maxPolys hit, dropping %i polygons\n", numPolys);
        return;
    }

    for (i = 0; i < numPolys; i++) {
        RE_AddPolyToScene( polys[i].hShader, polys[i].verts, polys[i].numVerts );
    }
}

void RE_AddEntityToScene( const renderEntityRef_t *ent )
{
    if (!rg.registered) {
        return;
    }

    if (r_numEntities >= MAX_RENDER_ENTITIES) {
        ri.Printf(PRINT_DEVELOPER, "RE_AddEntityToScene: MAX_RENDER_ENTITIES hit, dropping entity\n");
        return;
    }
    if ( N_isnan(ent->origin[0]) || N_isnan(ent->origin[1]) || N_isnan(ent->origin[2]) ) {
		static qboolean firstTime = qtrue;
		if (firstTime) {
			firstTime = qfalse;
			ri.Printf( PRINT_INFO, COLOR_YELLOW "RE_AddEntityToScene passed a refEntity which has an origin with a NaN component\n");
		}
		return;
	}

    backendData->entities[r_numEntities].e = *ent;

    r_numEntities++;
}

void RE_BeginScene( const renderSceneRef_t *fd )
{
    backend.refdef.x = fd->x;
    backend.refdef.y = fd->y;
    backend.refdef.width = fd->width;
    backend.refdef.height = fd->height;
    backend.refdef.flags = fd->flags;

    backend.refdef.time = fd->time;
    backend.refdef.floatTime = (double)backend.refdef.time * 0.001f; // -EC-: cast to double

    backend.refdef.numDLights = r_numDLights - r_firstSceneDLight;
    backend.refdef.dlights = backendData->dlights;
    
    backend.refdef.numEntities = r_numEntities - r_firstSceneEntity;
    backend.refdef.entities = &backendData->entities[r_firstSceneEntity];

    backend.refdef.numPolys = r_numPolys - r_firstScenePoly;
    backend.refdef.polys = &backendData->polys[r_firstScenePoly];

    backend.refdef.drawn = qfalse;

    rg.frameSceneNum++;
    rg.frameCount++;
}

void RE_ClearScene( void )
{
    r_firstSceneDLight = r_numDLights;
    r_firstSceneEntity = r_numEntities;
    r_firstScenePoly = r_numPolys;
    r_firstSceneVert = r_numPolyVerts;
}

void RE_EndScene( void )
{
    r_firstSceneDLight = r_numDLights;
    r_firstSceneEntity = r_numEntities;
    r_firstScenePoly = r_numPolys;
    r_firstSceneVert = r_numPolyVerts;
}

void RE_RenderScene( const renderSceneRef_t *fd )
{
    viewData_t parms;
    int64_t startTime;

    if ( !rg.registered ) {
        return;
    }

    startTime = ri.Milliseconds();

    if ( !rg.world && !( fd->flags & RSF_NOWORLDMODEL ) ) {
        ri.Error(ERR_FATAL, "RE_RenderScene: no world loaded");
    }

    RE_BeginScene( fd );

    memset( &parms, 0, sizeof(parms) );
    parms.viewportX = backend.refdef.x;
    parms.viewportY = backend.refdef.y;

    parms.flags = fd->flags;

    parms.viewportWidth = backend.refdef.width;
    parms.viewportHeight = backend.refdef.height;
    parms.camera = glState.viewData.camera;
    parms.stereoFrame = backend.refdef.stereoFrame;

    R_RenderView( &parms );

    if ( !( parms.flags & RSF_NOWORLDMODEL ) ) {
        R_AddPostProcessCmd();
    }

    RE_EndScene();

    rg.frontEndMsec += ri.Milliseconds() - startTime;
}


