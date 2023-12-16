#include "sg_local.h"

static mobj_t sg_mobs[MAXMOBS];

static qboolean SG_CheckSight( mobj_t *m )
{
	float dis = disBetweenOBJ( m->ent->origin, m->target->origin );

	if ( dis <= m->sight_range ) {
		return qfalse;
	}
	return qtrue;
}

//======================================================

mobj_t *SG_SpawnMob( mobtype_t type )
{
	mobj_t *m;
	sgentity_t *e;
	
	if ( type >= NUMMOBS ) {
		trap_Error("SG_SpawnMob: bad mob index");
	}

    e = SG_AllocEntity( ET_MOB );

    m = &sg_mobs[sg.numMobs];
    memset( m, 0, sizeof(*m) );

    memcpy( m, &mobinfo[type], sizeof(*m) );
    m->ent = e;

	switch ( type ) {
	case MT_GRUNT:
		Ent_SetState( e, S_GRUNT_IDLE );
		break;
	};

    sg.numMobs++;
	
	return m;
}

void SG_KillMob( mobj_t *m )
{
	SG_FreeEntity( m->ent );
}

void SG_SpawnMobOnMap( mobtype_t id, float x, float y, float elevation )
{
	mobj_t *m;
	
	m = SG_SpawnMob( id );
	
	m->ent->origin[0] = x;
	m->ent->origin[1] = y;
	m->ent->origin[2] = elevation;
}
