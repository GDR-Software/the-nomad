export Ent_SetState
code
proc Ent_SetState 16 4
file "../sg_entity.c"
line 12
;1:#include "sg_local.h"
;2:
;3:sgentity_t sg_entities[MAXENTITIES];
;4:
;5:// Use a heuristic approach to detect infinite state cycles: Count the number
;6:// of times the loop in Ent_SetState() executes and exit with an error once
;7:// an arbitrary very large limit is reached.
;8:
;9:#define ENTITY_CYCLE_LIMIT 100000
;10:
;11:qboolean Ent_SetState( sgentity_t *self, statenum_t state )
;12:{
line 16
;13:	state_t *st;
;14:	int counter;
;15:
;16:	counter = 0;
ADDRLP4 4
CNSTI4 0
ASGNI4
LABELV $90
line 17
;17:	do {
line 18
;18:		if ( state == S_NULL ) {
ADDRFP4 4
INDIRI4
CNSTI4 0
NEI4 $93
line 19
;19:			self->state = &stateinfo[S_NULL];
ADDRFP4 0
INDIRP4
CNSTI4 92
ADDP4
ADDRGP4 stateinfo
ASGNP4
line 20
;20:			SG_FreeEntity(self);
ADDRFP4 0
INDIRP4
ARGP4
ADDRGP4 SG_FreeEntity
CALLV
pop
line 21
;21:			return qfalse;
CNSTI4 0
RETI4
ADDRGP4 $89
JUMPV
LABELV $93
line 24
;22:		}
;23:
;24:		st = &stateinfo[state];
ADDRLP4 0
ADDRFP4 4
INDIRI4
CNSTI4 20
MULI4
ADDRGP4 stateinfo
ADDP4
ASGNP4
line 25
;25:		self->state = st;
ADDRFP4 0
INDIRP4
CNSTI4 92
ADDP4
ADDRLP4 0
INDIRP4
ASGNP4
line 26
;26:		self->ticker = st->tics;
ADDRFP4 0
INDIRP4
CNSTI4 116
ADDP4
ADDRLP4 0
INDIRP4
CNSTI4 8
ADDP4
INDIRU4
CVUI4 4
ASGNI4
line 27
;27:		self->frame = st->frames;
ADDRFP4 0
INDIRP4
CNSTI4 120
ADDP4
ADDRLP4 0
INDIRP4
CNSTI4 4
ADDP4
INDIRU4
CVUI4 4
ASGNI4
line 28
;28:		self->sprite = st->sprite + self->facing;
ADDRLP4 8
ADDRFP4 0
INDIRP4
ASGNP4
ADDRLP4 8
INDIRP4
CNSTI4 152
ADDP4
ADDRLP4 0
INDIRP4
INDIRI4
ADDRLP4 8
INDIRP4
CNSTI4 124
ADDP4
INDIRI4
ADDI4
ASGNI4
line 30
;29:
;30:		state = st->nextstate;
ADDRFP4 4
ADDRLP4 0
INDIRP4
CNSTI4 16
ADDP4
INDIRI4
ASGNI4
line 31
;31:		if ( counter++ > ENTITY_CYCLE_LIMIT ) {
ADDRLP4 12
ADDRLP4 4
INDIRI4
ASGNI4
ADDRLP4 4
ADDRLP4 12
INDIRI4
CNSTI4 1
ADDI4
ASGNI4
ADDRLP4 12
INDIRI4
CNSTI4 100000
LEI4 $95
line 32
;32:			G_Error("Ent_SetState: infinite state cycle detected!");
ADDRGP4 $97
ARGP4
ADDRGP4 G_Error
CALLV
pop
line 33
;33:		}
LABELV $95
line 34
;34:	} while ( !self->ticker );
LABELV $91
ADDRFP4 0
INDIRP4
CNSTI4 116
ADDP4
INDIRI4
CNSTI4 0
EQI4 $90
line 36
;35:
;36:	return qtrue;
CNSTI4 1
RETI4
LABELV $89
endproc Ent_SetState 16 4
export Ent_CheckEntityCollision
proc Ent_CheckEntityCollision 12 8
line 40
;37:}
;38:
;39:sgentity_t *Ent_CheckEntityCollision( const sgentity_t *ent )
;40:{
line 44
;41:	sgentity_t *it;
;42:	int i;
;43:
;44:	it = &sg_entities[0];
ADDRLP4 0
ADDRGP4 sg_entities
ASGNP4
line 45
;45:	for ( i = 0; i < sg.numEntities; i++, it++ ) {
ADDRLP4 4
CNSTI4 0
ASGNI4
ADDRGP4 $102
JUMPV
LABELV $99
line 46
;46:		if ( BoundsIntersect( &it->bounds, &ent->bounds ) && it != ent ) {
ADDRLP4 0
INDIRP4
CNSTI4 40
ADDP4
ARGP4
ADDRFP4 0
INDIRP4
CNSTI4 40
ADDP4
ARGP4
ADDRLP4 8
ADDRGP4 BoundsIntersect
CALLI4
ASGNI4
ADDRLP4 8
INDIRI4
CNSTI4 0
EQI4 $104
ADDRLP4 0
INDIRP4
CVPU4 4
ADDRFP4 0
INDIRP4
CVPU4 4
EQU4 $104
line 47
;47:			return it;
ADDRLP4 0
INDIRP4
RETP4
ADDRGP4 $98
JUMPV
LABELV $104
line 49
;48:		}
;49:	}
LABELV $100
line 45
ADDRLP4 4
ADDRLP4 4
INDIRI4
CNSTI4 1
ADDI4
ASGNI4
ADDRLP4 0
ADDRLP4 0
INDIRP4
CNSTI4 160
ADDP4
ASGNP4
LABELV $102
ADDRLP4 4
INDIRI4
ADDRGP4 sg+76
INDIRI4
LTI4 $99
line 51
;50:
;51:	return NULL;
CNSTP4 0
RETP4
LABELV $98
endproc Ent_CheckEntityCollision 12 8
export Ent_CheckWallCollision
proc Ent_CheckWallCollision 28 8
line 58
;52:}
;53:
;54:/*
;55: * Ent_CheckWallCollision: returns true if there's a wall in the way
;56: */
;57:qboolean Ent_CheckWallCollision( const sgentity_t *e )
;58:{
line 61
;59:	dirtype_t d;
;60:
;61:	switch ( e->dir ) {
ADDRLP4 4
ADDRFP4 0
INDIRP4
CNSTI4 144
ADDP4
INDIRI4
ASGNI4
ADDRLP4 4
INDIRI4
CNSTI4 0
LTI4 $107
ADDRLP4 4
INDIRI4
CNSTI4 7
GTI4 $107
ADDRLP4 4
INDIRI4
CNSTI4 2
LSHI4
ADDRGP4 $112
ADDP4
INDIRP4
JUMPV
data
align 4
LABELV $112
address $110
address $111
address $110
address $111
address $110
address $111
address $110
address $111
code
LABELV $110
line 66
;62:	case DIR_NORTH:
;63:	case DIR_SOUTH:
;64:	case DIR_EAST:
;65:	case DIR_WEST:
;66:		d = inversedirs[e->dir];
ADDRLP4 0
ADDRFP4 0
INDIRP4
CNSTI4 144
ADDP4
INDIRI4
CNSTI4 2
LSHI4
ADDRGP4 inversedirs
ADDP4
INDIRI4
ASGNI4
line 67
;67:		break;
ADDRGP4 $108
JUMPV
LABELV $111
line 72
;68:	case DIR_NORTH_WEST:
;69:	case DIR_NORTH_EAST:
;70:	case DIR_SOUTH_WEST:
;71:	case DIR_SOUTH_EAST:
;72:		return qtrue;
CNSTI4 1
RETI4
ADDRGP4 $106
JUMPV
LABELV $107
LABELV $108
line 73
;73:	};
line 77
;74:
;75:	// check for a wall collision
;76:	// if we're touching a wall with the side marked for collision, return true
;77:	if ( trap_CheckWallHit( e->origin, d ) ) {
ADDRLP4 12
ADDRFP4 0
INDIRP4
CNSTI4 64
ADDP4
INDIRB
ASGNB 12
ADDRLP4 12
ARGP4
ADDRLP4 0
INDIRI4
ARGI4
ADDRLP4 24
ADDRGP4 trap_CheckWallHit
CALLI4
ASGNI4
ADDRLP4 24
INDIRI4
CNSTI4 0
EQI4 $113
line 78
;78:		return qtrue;
CNSTI4 1
RETI4
ADDRGP4 $106
JUMPV
LABELV $113
line 81
;79:	}
;80:
;81:	return qfalse;
CNSTI4 0
RETI4
LABELV $106
endproc Ent_CheckWallCollision 28 8
export SG_BuildBounds
proc SG_BuildBounds 0 0
line 85
;82:}
;83:
;84:void SG_BuildBounds( bbox_t *bounds, float width, float height, const vec3_t origin )
;85:{
line 86
;86:	bounds->mins.x = origin.x - height;
ADDRFP4 0
INDIRP4
ADDRFP4 12
INDIRP4
INDIRF4
ADDRFP4 8
INDIRF4
SUBF4
ASGNF4
line 87
;87:	bounds->mins.y = origin.y - width;
ADDRFP4 0
INDIRP4
CNSTI4 4
ADDP4
ADDRFP4 12
INDIRP4
CNSTI4 4
ADDP4
INDIRF4
ADDRFP4 4
INDIRF4
SUBF4
ASGNF4
line 88
;88:	bounds->mins.z = origin.z;
ADDRFP4 0
INDIRP4
CNSTI4 8
ADDP4
ADDRFP4 12
INDIRP4
CNSTI4 8
ADDP4
INDIRF4
ASGNF4
line 90
;89:
;90:	bounds->maxs.x = origin.x + height;
ADDRFP4 0
INDIRP4
CNSTI4 12
ADDP4
ADDRFP4 12
INDIRP4
INDIRF4
ADDRFP4 8
INDIRF4
ADDF4
ASGNF4
line 91
;91:	bounds->maxs.y = origin.y + width;
ADDRFP4 0
INDIRP4
CNSTI4 16
ADDP4
ADDRFP4 12
INDIRP4
CNSTI4 4
ADDP4
INDIRF4
ADDRFP4 4
INDIRF4
ADDF4
ASGNF4
line 92
;92:	bounds->maxs.z = origin.z;
ADDRFP4 0
INDIRP4
CNSTI4 20
ADDP4
ADDRFP4 12
INDIRP4
CNSTI4 8
ADDP4
INDIRF4
ASGNF4
line 93
;93:}
LABELV $115
endproc SG_BuildBounds 0 0
export Ent_BuildBounds
proc Ent_BuildBounds 24 0
line 96
;94:
;95:void Ent_BuildBounds( sgentity_t *ent )
;96:{
line 97
;97:	ent->bounds.mins.x = ent->origin.x - ent->height;
ADDRLP4 0
ADDRFP4 0
INDIRP4
ASGNP4
ADDRLP4 0
INDIRP4
CNSTI4 40
ADDP4
ADDRLP4 0
INDIRP4
CNSTI4 64
ADDP4
INDIRF4
ADDRLP4 0
INDIRP4
CNSTI4 136
ADDP4
INDIRF4
SUBF4
ASGNF4
line 98
;98:	ent->bounds.mins.y = ent->origin.y - ent->width;
ADDRLP4 4
ADDRFP4 0
INDIRP4
ASGNP4
ADDRLP4 4
INDIRP4
CNSTI4 44
ADDP4
ADDRLP4 4
INDIRP4
CNSTI4 68
ADDP4
INDIRF4
ADDRLP4 4
INDIRP4
CNSTI4 132
ADDP4
INDIRF4
SUBF4
ASGNF4
line 99
;99:	ent->bounds.mins.z = ent->origin.z;
ADDRLP4 8
ADDRFP4 0
INDIRP4
ASGNP4
ADDRLP4 8
INDIRP4
CNSTI4 48
ADDP4
ADDRLP4 8
INDIRP4
CNSTI4 72
ADDP4
INDIRF4
ASGNF4
line 101
;100:
;101:	ent->bounds.maxs.x = ent->origin.x + ent->height;
ADDRLP4 12
ADDRFP4 0
INDIRP4
ASGNP4
ADDRLP4 12
INDIRP4
CNSTI4 52
ADDP4
ADDRLP4 12
INDIRP4
CNSTI4 64
ADDP4
INDIRF4
ADDRLP4 12
INDIRP4
CNSTI4 136
ADDP4
INDIRF4
ADDF4
ASGNF4
line 102
;102:	ent->bounds.maxs.y = ent->origin.y + ent->width;
ADDRLP4 16
ADDRFP4 0
INDIRP4
ASGNP4
ADDRLP4 16
INDIRP4
CNSTI4 56
ADDP4
ADDRLP4 16
INDIRP4
CNSTI4 68
ADDP4
INDIRF4
ADDRLP4 16
INDIRP4
CNSTI4 132
ADDP4
INDIRF4
ADDF4
ASGNF4
line 103
;103:	ent->bounds.maxs.z = ent->origin.z;
ADDRLP4 20
ADDRFP4 0
INDIRP4
ASGNP4
ADDRLP4 20
INDIRP4
CNSTI4 60
ADDP4
ADDRLP4 20
INDIRP4
CNSTI4 72
ADDP4
INDIRF4
ASGNF4
line 104
;104:}
LABELV $116
endproc Ent_BuildBounds 24 0
export SG_FreeEntity
proc SG_FreeEntity 4 4
line 107
;105:
;106:void SG_FreeEntity( sgentity_t *ent )
;107:{
line 108
;108:	if ( !ent->health ) {
ADDRFP4 0
INDIRP4
CNSTI4 112
ADDP4
INDIRI4
CNSTI4 0
NEI4 $118
line 109
;109:		trap_Print( COLOR_YELLOW "WARNING: SG_FreeEntity: freed a freed entity" );
ADDRGP4 $120
ARGP4
ADDRGP4 trap_Print
CALLV
pop
line 110
;110:		return;
ADDRGP4 $117
JUMPV
LABELV $118
line 113
;111:	}
;112:
;113:	sg.numEntities--;
ADDRLP4 0
ADDRGP4 sg+76
ASGNP4
ADDRLP4 0
INDIRP4
ADDRLP4 0
INDIRP4
INDIRI4
CNSTI4 1
SUBI4
ASGNI4
line 114
;114:}
LABELV $117
endproc SG_FreeEntity 4 4
export SG_AllocEntity
proc SG_AllocEntity 24 12
line 117
;115:
;116:sgentity_t *SG_AllocEntity( entitytype_t type )
;117:{
line 120
;118:	sgentity_t *ent;
;119:
;120:	if ( sg.numEntities == MAXENTITIES ) {
ADDRGP4 sg+76
INDIRI4
CNSTI4 2048
NEI4 $123
line 121
;121:		trap_Error( "SG_AllocEntity: MAXENTITIES hit" );
ADDRGP4 $126
ARGP4
ADDRGP4 trap_Error
CALLV
pop
line 122
;122:	}
LABELV $123
line 124
;123:
;124:	ent = &sg_entities[sg.numEntities];
ADDRLP4 0
ADDRGP4 sg+76
INDIRI4
CNSTI4 160
MULI4
ADDRGP4 sg_entities
ADDP4
ASGNP4
line 125
;125:	sg.numEntities++;
ADDRLP4 4
ADDRGP4 sg+76
ASGNP4
ADDRLP4 4
INDIRP4
ADDRLP4 4
INDIRP4
INDIRI4
CNSTI4 1
ADDI4
ASGNI4
line 127
;126:
;127:	memset( ent, 0, sizeof(*ent) );
ADDRLP4 0
INDIRP4
ARGP4
CNSTI4 0
ARGI4
CNSTU4 160
ARGU4
ADDRGP4 memset
CALLP4
pop
line 128
;128:	ent->type = type;
ADDRLP4 0
INDIRP4
CNSTI4 108
ADDP4
ADDRFP4 0
INDIRI4
ASGNI4
line 129
;129:	ent->ticker = -1;
ADDRLP4 0
INDIRP4
CNSTI4 116
ADDP4
CNSTI4 -1
ASGNI4
line 130
;130:	ent->width = ent->height = 0;
ADDRLP4 12
CNSTF4 0
ASGNF4
ADDRLP4 0
INDIRP4
CNSTI4 136
ADDP4
ADDRLP4 12
INDIRF4
ASGNF4
ADDRLP4 0
INDIRP4
CNSTI4 132
ADDP4
ADDRLP4 12
INDIRF4
ASGNF4
line 132
;131:
;132:	switch ( type ) {
ADDRLP4 16
ADDRFP4 0
INDIRI4
ASGNI4
ADDRLP4 16
INDIRI4
CNSTI4 0
LTI4 $129
ADDRLP4 16
INDIRI4
CNSTI4 5
GTI4 $129
ADDRLP4 16
INDIRI4
CNSTI4 2
LSHI4
ADDRGP4 $144
ADDP4
INDIRP4
JUMPV
data
align 4
LABELV $144
address $134
address $142
address $138
address $132
address $136
address $140
code
LABELV $132
line 134
;133:	case ET_MOB:
;134:		ent->classname = "mob";
ADDRLP4 0
INDIRP4
CNSTI4 96
ADDP4
ADDRGP4 $133
ASGNP4
line 135
;135:		break;
ADDRGP4 $130
JUMPV
LABELV $134
line 137
;136:	case ET_ITEM:
;137:		ent->classname = "item";
ADDRLP4 0
INDIRP4
CNSTI4 96
ADDP4
ADDRGP4 $135
ASGNP4
line 138
;138:		break;
ADDRGP4 $130
JUMPV
LABELV $136
line 140
;139:	case ET_BOT:
;140:		ent->classname = "bot";
ADDRLP4 0
INDIRP4
CNSTI4 96
ADDP4
ADDRGP4 $137
ASGNP4
line 141
;141:		break;
ADDRGP4 $130
JUMPV
LABELV $138
line 143
;142:	case ET_PLAYR:
;143:		ent->classname = "player";
ADDRLP4 0
INDIRP4
CNSTI4 96
ADDP4
ADDRGP4 $139
ASGNP4
line 144
;144:		break;
ADDRGP4 $130
JUMPV
LABELV $140
line 146
;145:	case ET_WALL:
;146:		ent->classname = "wall";
ADDRLP4 0
INDIRP4
CNSTI4 96
ADDP4
ADDRGP4 $141
ASGNP4
line 147
;147:		break;
ADDRGP4 $130
JUMPV
LABELV $142
line 149
;148:	case ET_WEAPON:
;149:		ent->classname = "weapon";
ADDRLP4 0
INDIRP4
CNSTI4 96
ADDP4
ADDRGP4 $143
ASGNP4
line 150
;150:		break;
LABELV $129
LABELV $130
line 151
;151:	};
line 153
;152:
;153:	Ent_BuildBounds( ent );
ADDRLP4 0
INDIRP4
ARGP4
ADDRGP4 Ent_BuildBounds
CALLV
pop
line 155
;154:
;155:	return ent;
ADDRLP4 0
INDIRP4
RETP4
LABELV $122
endproc SG_AllocEntity 24 12
import Cvar_VariableStringBuffer
import Cvar_Set
import Cvar_Update
import Cvar_Register
import trap_FS_FileTell
import trap_FS_FileSeek
import trap_FS_GetFileList
import trap_FS_Read
import trap_FS_Write
import trap_FS_FClose
import trap_FS_FOpenRead
import trap_FS_FOpenWrite
import trap_FS_FOpenFile
import Sys_GetGPUConfig
import RE_AddSpriteToScene
import RE_AddPolyToScene
import RE_RenderScene
import RE_ClearScene
import RE_LoadWorldMap
import RE_RegisterSprite
import RE_RegisterSpriteSheet
import RE_RegisterShader
import trap_Snd_ClearLoopingTrack
import trap_Snd_SetLoopingTrack
import trap_Snd_StopSfx
import trap_Snd_PlaySfx
import trap_Snd_QueueTrack
import trap_Snd_RegisterTrack
import trap_Snd_RegisterSfx
import trap_Key_ClearStates
import trap_Key_GetKey
import trap_Key_GetCatcher
import trap_Key_SetCatcher
import trap_Milliseconds
import trap_CheckWallHit
import G_SoundRecursive
import G_CastRay
import G_LoadMap
import G_SetCameraData
import trap_MemoryRemaining
import trap_RemoveCommand
import trap_AddCommand
import trap_SendConsoleCommand
import trap_Args
import trap_Argv
import trap_Argc
import trap_Error
import trap_Print
import P_GiveWeapon
import P_GiveItem
import P_ParryTicker
import P_MeleeTicker
import P_Ticker
import SG_SendUserCmd
import SG_MouseEvent
import SG_KeyEvent
import SG_InitPlayer
import SG_OutOfMemory
import SG_ClearToMemoryMark
import SG_MakeMemoryMark
import SG_MemInit
import SG_MemAlloc
import String_Alloc
import SG_SpawnMobOnMap
import SG_SpawnMob
import SG_AddArchiveHandle
import SG_LoadGame
import SG_SaveGame
import SG_LoadSection
import SG_WriteSection
import SG_InitEntities
import Ent_RunTic
import SG_DrawLevelStats
import SG_DrawAbortMission
import Lvl_AddKillEntity
import SG_EndLevel
import SG_InitLevel
import SG_UpdateCvars
import G_Printf
import G_Error
import SG_Printf
import SG_Error
import SG_BuildMoveCommand
import SGameCommand
import SG_DrawFrame
import pm_wallTime
import pm_wallrunAccelMove
import pm_wallrunAccelVertical
import pm_airAccel
import pm_baseSpeed
import pm_baseAccel
import pm_waterAccel
import pm_airFriction
import pm_waterFriction
import pm_groundFriction
import sg_memoryDebug
import sg_numSaves
import sg_savename
import sg_levelDataFile
import sg_levelIndex
import sg_levelInfoFile
import sg_gibs
import sg_decalDetail
import sg_printLevelStats
import sg_mouseAcceleration
import sg_mouseInvert
import sg_paused
import sg_debugPrint
import ammoCaps
import mobinfo
import iteminfo
import weaponinfo
import sg
bss
export sg_entities
align 4
LABELV sg_entities
skip 327680
import inversedirs
import dirvectors
import stateinfo
import ImGui_CloseCurrentPopup
import ImGui_OpenPopup
import ImGui_EndPopup
import ImGui_BeginPopupModal
import ImGui_ColoredText
import ImGui_Text
import ImGui_ColoredTextUnformatted
import ImGui_TextUnformatted
import ImGui_SameLine
import ImGui_ProgressBar
import ImGui_Separator
import ImGui_SeparatorText
import ImGui_NewLine
import ImGui_PopColor
import ImGui_PushColor
import ImGui_GetCursorScreenPos
import ImGui_SetCursorScreenPos
import ImGui_GetCursorPos
import ImGui_SetCursorPos
import ImGui_GetFontScale
import ImGui_Button
import ImGui_Checkbox
import ImGui_ArrowButton
import ImGui_ColorEdit4
import ImGui_ColorEdit3
import ImGui_SliderInt4
import ImGui_SliderInt3
import ImGui_SliderInt2
import ImGui_SliderInt
import ImGui_SliderFloat4
import ImGui_SliderFloat3
import ImGui_SliderFloat2
import ImGui_SliderFloat
import ImGui_InputInt4
import ImGui_InputInt3
import ImGui_InputInt2
import ImGui_InputInt
import ImGui_InputFloat4
import ImGui_InputFloat3
import ImGui_InputFloat2
import ImGui_InputFloat
import ImGui_InputTextWithHint
import ImGui_InputTextMultiline
import ImGui_InputText
import ImGui_EndTable
import ImGui_TableNextColumn
import ImGui_TableNextRow
import ImGui_BeginTable
import ImGui_SetItemTooltip
import ImGui_SetItemTooltipUnformatted
import ImGui_MenuItem
import ImGui_EndMenu
import ImGui_BeginMenu
import ImGui_SetWindowFontScale
import ImGui_SetWindowSize
import ImGui_SetWindowPos
import ImGui_SetWindowCollapsed
import ImGui_IsWindowCollapsed
import ImGui_EndWindow
import ImGui_BeginWindow
import I_GetParm
import Com_TouchMemory
import Hunk_TempIsClear
import Hunk_Check
import Hunk_Print
import Hunk_SetMark
import Hunk_ClearToMark
import Hunk_CheckMark
import Hunk_SmallLog
import Hunk_Log
import Hunk_MemoryRemaining
import Hunk_ClearTempMemory
import Hunk_FreeTempMemory
import Hunk_AllocateTempMemory
import Hunk_Clear
import Hunk_Alloc
import Hunk_InitMemory
import Z_InitMemory
import Z_InitSmallZoneMemory
import CopyString
import Z_AvailableMemory
import Z_FreeTags
import Z_Free
import S_Malloc
import Z_Malloc
import Z_Realloc
import CPU_flags
import FS_ReadLine
import FS_ListFiles
import FS_FreeFileList
import FS_FreeFile
import FS_SetBFFIndex
import FS_GetCurrentChunkList
import FS_Initialized
import FS_FileIsInBFF
import FS_StripExt
import FS_AllowedExtension
import FS_GetFileList
import FS_LoadLibrary
import FS_CopyString
import FS_BuildOSPath
import FS_FilenameCompare
import FS_FileTell
import FS_FileLength
import FS_FileSeek
import FS_FileExists
import FS_LastBFFIndex
import FS_LoadStack
import FS_Rename
import FS_FOpenFileRead
import FS_FOpenAppend
import FS_FOpenRW
import FS_FOpenWrite
import FS_FOpenRead
import FS_FOpenFileWithMode
import FS_FOpenWithMode
import FS_FileToFileno
import FS_Printf
import FS_GetGamePath
import FS_GetHomePath
import FS_GetBasePath
import FS_GetBaseGameDir
import FS_GetCurrentGameDir
import FS_Flush
import FS_ForceFlush
import FS_FClose
import FS_LoadFile
import FS_WriteFile
import FS_Write
import FS_Read
import FS_Remove
import FS_Restart
import FS_Shutdown
import FS_InitFilesystem
import FS_Startup
import FS_VM_CloseFiles
import FS_VM_FileLength
import FS_VM_Read
import FS_VM_Write
import FS_VM_WriteFile
import FS_VM_FClose
import FS_VM_FOpenFileRead
import FS_VM_FOpenFileWrite
import FS_VM_FOpenFile
import FS_VM_FileTell
import FS_VM_FileSeek
import FS_VM_FOpenRW
import FS_VM_FOpenAppend
import FS_VM_FOpenWrite
import FS_VM_FOpenRead
import com_errorMessage
import com_fullyInitialized
import com_errorEntered
import com_cacheLine
import com_frameTime
import com_fps
import com_frameNumber
import com_maxfps
import sys_cpuString
import com_devmode
import com_version
import com_logfile
import com_journal
import com_demo
import Con_HistoryGetNext
import Con_HistoryGetPrev
import Con_SaveField
import Con_ResetHistory
import Field_CompleteCommand
import Field_CompleteFilename
import Field_CompleteKeyBind
import Field_CompleteKeyname
import Field_AutoComplete
import Field_Clear
import Cbuf_Init
import Cbuf_Clear
import Cbuf_AddText
import Cbuf_Execute
import Cbuf_InsertText
import Cbuf_ExecuteText
import va
import Cmd_CompleteArgument
import Cmd_CommandCompletion
import Cmd_Clear
import Cmd_Argv
import Cmd_ArgsFrom
import Cmd_SetCommandCompletionFunc
import Cmd_TokenizeStringIgnoreQuotes
import Cmd_TokenizeString
import Cmd_ArgvBuffer
import Cmd_Argc
import Cmd_ExecuteString
import Cmd_ExecuteText
import Cmd_ArgsBuffer
import Cmd_ExecuteCommand
import Cmd_RemoveCommand
import Cmd_AddCommand
import Cmd_Init
import keys
import Key_WriteBindings
import Key_SetOverstrikeMode
import Key_GetOverstrikeMode
import Key_GetKey
import Key_GetCatcher
import Key_SetCatcher
import Key_ClearStates
import Key_GetBinding
import Key_IsDown
import Key_KeynumToString
import Key_StringToKeynum
import Key_KeynameCompletion
import Com_EventLoop
import Com_KeyEvent
import Com_SendKeyEvents
import Com_QueueEvent
import Com_InitKeyCommands
import Parse3DMatrix
import Parse2DMatrix
import Parse1DMatrix
import ParseHex
import SkipRestOfLine
import SkipBracedSection
import com_tokentype
import COM_ParseComplex
import Com_BlockChecksum
import COM_ParseWarning
import COM_ParseError
import COM_Compress
import COM_ParseExt
import COM_Parse
import COM_GetCurrentParseLine
import COM_BeginParseSession
import COM_StripExtension
import COM_GetExtension
import Com_TruncateLongString
import Com_SortFileList
import Com_Base64Decode
import Com_HasPatterns
import Com_FilterPath
import Com_Filter
import Com_FilterExt
import Com_HexStrToInt
import COM_DefaultExtension
import Com_WriteConfig
import Con_RenderConsole
import Com_GenerateHashValue
import Com_Shutdown
import Com_Init
import Com_StartupVariable
import crc32_buffer
import Com_EarlyParseCmdLine
import Com_Milliseconds
import Com_Frame
import Sys_SnapVector
import Con_DPrintf
import Con_Printf
import Con_Shutdown
import Con_Init
import Con_DrawConsole
import Con_AddText
import ColorIndexFromChar
import g_color_table
import Info_RemoveKey
import Info_NextPair
import Info_ValidateKeyValue
import Info_Validate
import Info_SetValueForKey_s
import Info_ValueForKeyToken
import Info_Tokenize
import Info_ValueForKey
import Com_Clamp
import bytedirs
import N_isnan
import N_crandom
import N_random
import N_rand
import N_fabs
import N_acos
import N_log2
import ColorBytes4
import ColorBytes3
import VectorNormalize
import AddPointToBounds
import ClearBounds
import RadiusFromBounds
import NormalizeColor
import _VectorMA
import _VectorScale
import _VectorCopy
import _VectorAdd
import _VectorSubtract
import _DotProduct
import ByteToDir
import DirToByte
import CrossProduct
import VectorInverse
import VectorNormalizeFast
import DistanceSquared
import Distance
import VectorLengthSquared
import VectorLength
import VectorCompare
import BoundsIntersectPoint
import BoundsIntersectSphere
import BoundsIntersect
import disBetweenOBJ
import vec3_set
import vec3_get
import ClampShort
import ClampCharMove
import ClampChar
import N_exp2f
import N_log2f
import Q_rsqrt
import N_Error
import locase
import colorDkGrey
import colorMdGrey
import colorLtGrey
import colorWhite
import colorCyan
import colorMagenta
import colorYellow
import colorBlue
import colorGreen
import colorRed
import colorBlack
import vec2_origin
import vec3_origin
import COM_SkipPath
import Com_Split
import N_replace
import N_memcmp
import N_memchr
import N_memcpy
import N_memset
import N_strncpyz
import N_strncpy
import N_strcpy
import N_stradd
import N_strneq
import N_streq
import N_strlen
import N_atof
import N_atoi
import N_fmaxf
import N_stristr
import N_strcat
import N_strupr
import N_strlwr
import N_stricmpn
import N_stricmp
import N_strncmp
import N_strcmp
import N_isanumber
import N_isintegral
import N_isalpha
import N_isupper
import N_islower
import N_isprint
import Com_SkipCharset
import Com_SkipTokens
import Com_snprintf
import acos
import fabs
import abs
import tan
import atan2
import cos
import sin
import sqrt
import floor
import ceil
import sscanf
import vsprintf
import rand
import srand
import qsort
import toupper
import tolower
import strncmp
import strcmp
import strstr
import strchr
import strlen
import strcat
import strcpy
import memmove
import memset
import memchr
import memcpy
lit
align 1
LABELV $143
byte 1 119
byte 1 101
byte 1 97
byte 1 112
byte 1 111
byte 1 110
byte 1 0
align 1
LABELV $141
byte 1 119
byte 1 97
byte 1 108
byte 1 108
byte 1 0
align 1
LABELV $139
byte 1 112
byte 1 108
byte 1 97
byte 1 121
byte 1 101
byte 1 114
byte 1 0
align 1
LABELV $137
byte 1 98
byte 1 111
byte 1 116
byte 1 0
align 1
LABELV $135
byte 1 105
byte 1 116
byte 1 101
byte 1 109
byte 1 0
align 1
LABELV $133
byte 1 109
byte 1 111
byte 1 98
byte 1 0
align 1
LABELV $126
byte 1 83
byte 1 71
byte 1 95
byte 1 65
byte 1 108
byte 1 108
byte 1 111
byte 1 99
byte 1 69
byte 1 110
byte 1 116
byte 1 105
byte 1 116
byte 1 121
byte 1 58
byte 1 32
byte 1 77
byte 1 65
byte 1 88
byte 1 69
byte 1 78
byte 1 84
byte 1 73
byte 1 84
byte 1 73
byte 1 69
byte 1 83
byte 1 32
byte 1 104
byte 1 105
byte 1 116
byte 1 0
align 1
LABELV $120
byte 1 94
byte 1 51
byte 1 87
byte 1 65
byte 1 82
byte 1 78
byte 1 73
byte 1 78
byte 1 71
byte 1 58
byte 1 32
byte 1 83
byte 1 71
byte 1 95
byte 1 70
byte 1 114
byte 1 101
byte 1 101
byte 1 69
byte 1 110
byte 1 116
byte 1 105
byte 1 116
byte 1 121
byte 1 58
byte 1 32
byte 1 102
byte 1 114
byte 1 101
byte 1 101
byte 1 100
byte 1 32
byte 1 97
byte 1 32
byte 1 102
byte 1 114
byte 1 101
byte 1 101
byte 1 100
byte 1 32
byte 1 101
byte 1 110
byte 1 116
byte 1 105
byte 1 116
byte 1 121
byte 1 0
align 1
LABELV $97
byte 1 69
byte 1 110
byte 1 116
byte 1 95
byte 1 83
byte 1 101
byte 1 116
byte 1 83
byte 1 116
byte 1 97
byte 1 116
byte 1 101
byte 1 58
byte 1 32
byte 1 105
byte 1 110
byte 1 102
byte 1 105
byte 1 110
byte 1 105
byte 1 116
byte 1 101
byte 1 32
byte 1 115
byte 1 116
byte 1 97
byte 1 116
byte 1 101
byte 1 32
byte 1 99
byte 1 121
byte 1 99
byte 1 108
byte 1 101
byte 1 32
byte 1 100
byte 1 101
byte 1 116
byte 1 101
byte 1 99
byte 1 116
byte 1 101
byte 1 100
byte 1 33
byte 1 0