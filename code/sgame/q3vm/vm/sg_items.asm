export SG_SpawnItem
code
proc SG_SpawnItem 8 12
file "../sg_items.c"
line 4
;1:#include "sg_local.h"
;2:
;3:item_t *SG_SpawnItem( itemtype_t type )
;4:{
line 7
;5:    item_t *item;
;6:
;7:    if ( type >= NUMITEMS ) {
ADDRFP4 0
INDIRI4
CNSTI4 2
LTI4 $90
line 8
;8:        trap_Error( "SG_SpawnItem: incompatible mod with sgame, type >= NUMITEMS" );
ADDRGP4 $92
ARGP4
ADDRGP4 trap_Error
CALLV
pop
line 9
;9:    }
LABELV $90
line 10
;10:    if ( sg.numItems == MAXITEMS ) {
ADDRGP4 sg+43280
INDIRI4
CNSTI4 1024
NEI4 $93
line 11
;11:        trap_Error( "SG_SpawnItem: MAXITEMS hit" );
ADDRGP4 $96
ARGP4
ADDRGP4 trap_Error
CALLV
pop
line 12
;12:    }
LABELV $93
line 14
;13:
;14:    item = &sg.items[sg.numItems];
ADDRLP4 0
ADDRGP4 sg+43280
INDIRI4
CNSTI4 4
LSHI4
ADDRGP4 sg+26896
ADDP4
ASGNP4
line 15
;15:    memset( item, 0, sizeof(*item) );
ADDRLP4 0
INDIRP4
ARGP4
CNSTI4 0
ARGI4
CNSTU4 16
ARGU4
ADDRGP4 memset
CALLI4
pop
line 16
;16:    item->type = type;
ADDRLP4 0
INDIRP4
CNSTI4 12
ADDP4
ADDRFP4 0
INDIRI4
ASGNI4
line 18
;17:
;18:    sg.numItems++;
ADDRLP4 4
ADDRGP4 sg+43280
ASGNP4
ADDRLP4 4
INDIRP4
ADDRLP4 4
INDIRP4
INDIRI4
CNSTI4 1
ADDI4
ASGNI4
line 20
;19:
;20:    return item;
ADDRLP4 0
INDIRP4
RETP4
LABELV $89
endproc SG_SpawnItem 8 12
export SG_SpawnWeapon
proc SG_SpawnWeapon 16 12
line 24
;21:}
;22:
;23:weapon_t *SG_SpawnWeapon( weapontype_t type )
;24:{
line 28
;25:    item_t *item;
;26:    weapon_t *w;
;27:
;28:    if ( type >= NUMWEAPONTYPES ) {
ADDRFP4 0
INDIRI4
CNSTI4 22
LTI4 $101
line 29
;29:        trap_Error( "SG_SpawnWeapon: incompatible mod with sgame, type >= NUMWEAPONS" );
ADDRGP4 $103
ARGP4
ADDRGP4 trap_Error
CALLV
pop
line 30
;30:    }
LABELV $101
line 31
;31:    if ( sg.numWeapons == MAXWEAPONS ) {
ADDRGP4 sg+63764
INDIRI4
CNSTI4 1024
NEI4 $104
line 32
;32:        trap_Error( "SG_SpawnWeapon: MAXWEAPONS hit" );
ADDRGP4 $107
ARGP4
ADDRGP4 trap_Error
CALLV
pop
line 33
;33:    }
LABELV $104
line 35
;34:
;35:    w = &sg.weapons[sg.numWeapons];
ADDRLP4 0
ADDRGP4 sg+63764
INDIRI4
CNSTI4 20
MULI4
ADDRGP4 sg+43284
ADDP4
ASGNP4
line 36
;36:    memset( w, 0, sizeof(*w) );
ADDRLP4 0
INDIRP4
ARGP4
CNSTI4 0
ARGI4
CNSTU4 20
ARGU4
ADDRGP4 memset
CALLI4
pop
line 37
;37:    sg.numWeapons++;
ADDRLP4 8
ADDRGP4 sg+63764
ASGNP4
ADDRLP4 8
INDIRP4
ADDRLP4 8
INDIRP4
INDIRI4
CNSTI4 1
ADDI4
ASGNI4
line 39
;38:
;39:    item = SG_SpawnItem( I_WEAPON );
CNSTI4 1
ARGI4
ADDRLP4 12
ADDRGP4 SG_SpawnItem
CALLP4
ASGNP4
ADDRLP4 4
ADDRLP4 12
INDIRP4
ASGNP4
line 40
;40:    w->base = item;
ADDRLP4 0
INDIRP4
ADDRLP4 4
INDIRP4
ASGNP4
line 42
;41:
;42:    return w;
ADDRLP4 0
INDIRP4
RETP4
LABELV $100
endproc SG_SpawnWeapon 16 12
import memset
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
import RE_SetColor
import RE_DrawImage
import RE_AddSpriteToScene
import RE_AddPolyToScene
import RE_RenderScene
import RE_ClearScene
import RE_LoadWorldMap
import RE_RegisterSprite
import RE_RegisterSpriteSheet
import RE_RegisterShader
import Snd_ClearLoopingTrack
import Snd_SetLoopingTrack
import Snd_StopSfx
import Snd_PlaySfx
import Snd_RegisterTrack
import Snd_RegisterSfx
import trap_Key_ClearStates
import trap_Key_GetKey
import trap_Key_GetCatcher
import trap_Key_SetCatcher
import Sys_Milliseconds
import G_CheckWallHit
import G_SoundRecursive
import G_CastRay
import G_SetActiveMap
import G_LoadMap
import trap_GetHashString
import G_SetCameraData
import Sys_MemoryRemaining
import trap_RemoveCommand
import trap_AddCommand
import trap_SendConsoleCommand
import G_LoadVector4
import G_LoadVector3
import G_LoadVector2
import G_LoadString
import LoadFloat
import G_LoadInt
import G_LoadUInt
import G_GetSaveSection
import G_SaveVector4
import G_SaveVector3
import G_SaveVector2
import G_SaveFloat
import G_SaveString
import G_SaveUInt
import G_SaveInt
import G_SaveChar
import G_EndSaveSection
import G_BeginSaveSection
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
import SG_SpawnMob
import SG_Spawn
import Ent_SetState
import SG_InitEntities
import Ent_BuildBounds
import SG_BuildBounds
import SG_FreeEntity
import SG_AllocEntity
import Ent_RunTic
import Ent_CheckEntityCollision
import Ent_CheckWallCollision
import SG_PickupWeapon
import SG_LoadLevels
import SG_LoadLevelData
import SG_SaveLevelData
import SG_EndLevel
import SG_StartLevel
import SG_UpdateCvars
import G_Printf
import G_Error
import SG_Printf
import SG_Error
import SG_ShutdownCommands
import SG_InitCommands
import SGameCommand
import SG_DrawFrame
import sgc_godmode
import sg_cheatsOn
import sgc_deafMobs
import sgc_blindMobs
import sgc_infiniteAmmo
import sgc_infiniteRage
import sgc_infiniteHealth
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
import sg_gameDifficulty
import sg_savename
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
import sg_activeEnts
import sg_freeEnts
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
import I_GetParm
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
import ClearBounds
import RadiusFromBounds
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
lit
align 1
LABELV $107
byte 1 83
byte 1 71
byte 1 95
byte 1 83
byte 1 112
byte 1 97
byte 1 119
byte 1 110
byte 1 87
byte 1 101
byte 1 97
byte 1 112
byte 1 111
byte 1 110
byte 1 58
byte 1 32
byte 1 77
byte 1 65
byte 1 88
byte 1 87
byte 1 69
byte 1 65
byte 1 80
byte 1 79
byte 1 78
byte 1 83
byte 1 32
byte 1 104
byte 1 105
byte 1 116
byte 1 0
align 1
LABELV $103
byte 1 83
byte 1 71
byte 1 95
byte 1 83
byte 1 112
byte 1 97
byte 1 119
byte 1 110
byte 1 87
byte 1 101
byte 1 97
byte 1 112
byte 1 111
byte 1 110
byte 1 58
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
byte 1 109
byte 1 111
byte 1 100
byte 1 32
byte 1 119
byte 1 105
byte 1 116
byte 1 104
byte 1 32
byte 1 115
byte 1 103
byte 1 97
byte 1 109
byte 1 101
byte 1 44
byte 1 32
byte 1 116
byte 1 121
byte 1 112
byte 1 101
byte 1 32
byte 1 62
byte 1 61
byte 1 32
byte 1 78
byte 1 85
byte 1 77
byte 1 87
byte 1 69
byte 1 65
byte 1 80
byte 1 79
byte 1 78
byte 1 83
byte 1 0
align 1
LABELV $96
byte 1 83
byte 1 71
byte 1 95
byte 1 83
byte 1 112
byte 1 97
byte 1 119
byte 1 110
byte 1 73
byte 1 116
byte 1 101
byte 1 109
byte 1 58
byte 1 32
byte 1 77
byte 1 65
byte 1 88
byte 1 73
byte 1 84
byte 1 69
byte 1 77
byte 1 83
byte 1 32
byte 1 104
byte 1 105
byte 1 116
byte 1 0
align 1
LABELV $92
byte 1 83
byte 1 71
byte 1 95
byte 1 83
byte 1 112
byte 1 97
byte 1 119
byte 1 110
byte 1 73
byte 1 116
byte 1 101
byte 1 109
byte 1 58
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
byte 1 109
byte 1 111
byte 1 100
byte 1 32
byte 1 119
byte 1 105
byte 1 116
byte 1 104
byte 1 32
byte 1 115
byte 1 103
byte 1 97
byte 1 109
byte 1 101
byte 1 44
byte 1 32
byte 1 116
byte 1 121
byte 1 112
byte 1 101
byte 1 32
byte 1 62
byte 1 61
byte 1 32
byte 1 78
byte 1 85
byte 1 77
byte 1 73
byte 1 84
byte 1 69
byte 1 77
byte 1 83
byte 1 0
