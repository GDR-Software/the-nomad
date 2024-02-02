code
proc SG_CheckSight 12 8
file "../sg_mthink.c"
line 6
;1:#include "sg_local.h"
;2:
;3:static mobj_t sg_mobs[MAXMOBS];
;4:
;5:static qboolean SG_CheckSight( mobj_t *m )
;6:{
line 7
;7:	float dis = disBetweenOBJ( &m->ent->origin, &m->target->origin );
ADDRLP4 4
ADDRFP4 0
INDIRP4
ASGNP4
ADDRLP4 4
INDIRP4
INDIRP4
CNSTI4 64
ADDP4
ARGP4
ADDRLP4 4
INDIRP4
CNSTI4 4
ADDP4
INDIRP4
CNSTI4 64
ADDP4
ARGP4
ADDRLP4 8
ADDRGP4 disBetweenOBJ
CALLF4
ASGNF4
ADDRLP4 0
ADDRLP4 8
INDIRF4
ASGNF4
line 9
;8:
;9:	if ( dis <= m->sight_range ) {
ADDRLP4 0
INDIRF4
ADDRFP4 0
INDIRP4
CNSTI4 12
ADDP4
INDIRF4
GTF4 $90
line 10
;10:		return qfalse;
CNSTI4 0
RETI4
ADDRGP4 $89
JUMPV
LABELV $90
line 12
;11:	}
;12:	return qtrue;
CNSTI4 1
RETI4
LABELV $89
endproc SG_CheckSight 12 8
export SG_SpawnMob
proc SG_SpawnMob 20 12
line 18
;13:}
;14:
;15://======================================================
;16:
;17:mobj_t *SG_SpawnMob( mobtype_t type )
;18:{
line 22
;19:	mobj_t *m;
;20:	sgentity_t *e;
;21:	
;22:	if ( type >= NUMMOBS ) {
ADDRFP4 0
INDIRI4
CNSTI4 4
LTI4 $93
line 23
;23:		trap_Error( "SG_SpawnMob: bad mob index" );
ADDRGP4 $95
ARGP4
ADDRGP4 trap_Error
CALLV
pop
line 24
;24:	}
LABELV $93
line 25
;25:	if ( sg.numMobs == MAXMOBS ) {
ADDRGP4 sg+24744
INDIRI4
CNSTI4 1024
NEI4 $96
line 26
;26:		trap_Error( "SG_SpawnMob: mod incompatible with current sgame" );
ADDRGP4 $99
ARGP4
ADDRGP4 trap_Error
CALLV
pop
line 27
;27:	}
LABELV $96
line 29
;28:
;29:    e = SG_AllocEntity( ET_MOB );
CNSTI4 2
ARGI4
ADDRLP4 8
ADDRGP4 SG_AllocEntity
CALLP4
ASGNP4
ADDRLP4 4
ADDRLP4 8
INDIRP4
ASGNP4
line 31
;30:
;31:    m = &sg_mobs[sg.numMobs];
ADDRLP4 0
ADDRGP4 sg+24744
INDIRI4
CNSTI4 24
MULI4
ADDRGP4 sg_mobs
ADDP4
ASGNP4
line 32
;32:    memset( m, 0, sizeof(*m) );
ADDRLP4 0
INDIRP4
ARGP4
CNSTI4 0
ARGI4
CNSTU4 24
ARGU4
ADDRGP4 memset
CALLP4
pop
line 34
;33:
;34:    memcpy( m, &mobinfo[type], sizeof(*m) );
ADDRLP4 0
INDIRP4
ARGP4
ADDRFP4 0
INDIRI4
CNSTI4 24
MULI4
ADDRGP4 mobinfo
ADDP4
ARGP4
CNSTU4 24
ARGU4
ADDRGP4 memcpy
CALLP4
pop
line 35
;35:    m->ent = e;
ADDRLP4 0
INDIRP4
ADDRLP4 4
INDIRP4
ASGNP4
line 37
;36:
;37:	switch ( type ) {
ADDRLP4 12
ADDRFP4 0
INDIRI4
ASGNI4
ADDRLP4 12
INDIRI4
CNSTI4 0
EQI4 $104
ADDRGP4 $101
JUMPV
LABELV $104
line 39
;38:	case MT_GRUNT:
;39:		Ent_SetState( e, S_GRUNT_IDLE );
ADDRLP4 4
INDIRP4
ARGP4
CNSTI4 11
ARGI4
ADDRGP4 Ent_SetState
CALLI4
pop
line 40
;40:		break;
LABELV $101
LABELV $102
line 41
;41:	};
line 43
;42:
;43:    sg.numMobs++;
ADDRLP4 16
ADDRGP4 sg+24744
ASGNP4
ADDRLP4 16
INDIRP4
ADDRLP4 16
INDIRP4
INDIRI4
CNSTI4 1
ADDI4
ASGNI4
line 45
;44:	
;45:	return m;
ADDRLP4 0
INDIRP4
RETP4
LABELV $92
endproc SG_SpawnMob 20 12
export SG_KillMob
proc SG_KillMob 0 4
line 49
;46:}
;47:
;48:void SG_KillMob( mobj_t *m )
;49:{
line 50
;50:	SG_FreeEntity( m->ent );
ADDRFP4 0
INDIRP4
INDIRP4
ARGP4
ADDRGP4 SG_FreeEntity
CALLV
pop
line 51
;51:}
LABELV $106
endproc SG_KillMob 0 4
export SG_SpawnMobOnMap
proc SG_SpawnMobOnMap 8 4
line 54
;52:
;53:void SG_SpawnMobOnMap( mobtype_t id, float x, float y, float elevation )
;54:{
line 57
;55:	mobj_t *m;
;56:	
;57:	m = SG_SpawnMob( id );
ADDRFP4 0
INDIRI4
ARGI4
ADDRLP4 4
ADDRGP4 SG_SpawnMob
CALLP4
ASGNP4
ADDRLP4 0
ADDRLP4 4
INDIRP4
ASGNP4
line 59
;58:	
;59:	m->ent->origin.x = x;
ADDRLP4 0
INDIRP4
INDIRP4
CNSTI4 64
ADDP4
ADDRFP4 4
INDIRF4
ASGNF4
line 60
;60:	m->ent->origin.y = y;
ADDRLP4 0
INDIRP4
INDIRP4
CNSTI4 68
ADDP4
ADDRFP4 8
INDIRF4
ASGNF4
line 61
;61:	m->ent->origin.z = elevation;
ADDRLP4 0
INDIRP4
INDIRP4
CNSTI4 72
ADDP4
ADDRFP4 12
INDIRF4
ASGNF4
line 62
;62:}
LABELV $107
endproc SG_SpawnMobOnMap 8 4
bss
align 4
LABELV sg_mobs
skip 24576
import Cvar_VariableStringBuffer
import Cvar_Set
import Cvar_Update
import Cvar_Register
import trap_FS_Printf
import trap_FS_FileTell
import trap_FS_FileLength
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
import G_SetActiveMap
import G_LoadMap
import G_SetCameraData
import trap_MemoryRemaining
import trap_RemoveCommand
import trap_AddCommand
import trap_SendConsoleCommand
import trap_LoadVec4
import trap_LoadVec3
import trap_LoadVec2
import trap_LoadString
import trap_LoadFloat
import trap_LoadInt
import trap_LoadUInt
import trap_GetSaveSection
import trap_WriteVec4
import trap_WriteVec3
import trap_WriteVec2
import trap_WriteFloat
import trap_WriteString
import trap_WriteUInt
import trap_WriteInt
import trap_WriteChar
import trap_EndSaveSection
import trap_BeginSaveSection
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
import Ent_SetState
import SG_InitEntities
import Ent_BuildBounds
import SG_BuildBounds
import SG_FreeEntity
import SG_AllocEntity
import Ent_RunTic
import Ent_CheckEntityCollision
import Ent_CheckWallCollision
import SG_DrawLevelStats
import SG_DrawAbortMission
import Lvl_AddKillEntity
import SG_EndLevel
import SG_StartLevel
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
import sg_entities
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
LABELV $99
byte 1 83
byte 1 71
byte 1 95
byte 1 83
byte 1 112
byte 1 97
byte 1 119
byte 1 110
byte 1 77
byte 1 111
byte 1 98
byte 1 58
byte 1 32
byte 1 109
byte 1 111
byte 1 100
byte 1 32
byte 1 105
byte 1 110
byte 1 99
byte 1 111
byte 1 109
byte 1 112
byte 1 97
byte 1 116
byte 1 105
byte 1 98
byte 1 108
byte 1 101
byte 1 32
byte 1 119
byte 1 105
byte 1 116
byte 1 104
byte 1 32
byte 1 99
byte 1 117
byte 1 114
byte 1 114
byte 1 101
byte 1 110
byte 1 116
byte 1 32
byte 1 115
byte 1 103
byte 1 97
byte 1 109
byte 1 101
byte 1 0
align 1
LABELV $95
byte 1 83
byte 1 71
byte 1 95
byte 1 83
byte 1 112
byte 1 97
byte 1 119
byte 1 110
byte 1 77
byte 1 111
byte 1 98
byte 1 58
byte 1 32
byte 1 98
byte 1 97
byte 1 100
byte 1 32
byte 1 109
byte 1 111
byte 1 98
byte 1 32
byte 1 105
byte 1 110
byte 1 100
byte 1 101
byte 1 120
byte 1 0
