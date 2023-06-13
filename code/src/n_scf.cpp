#include "n_shared.h"
#include "g_game.h"

/*
char s[64];
float f;
int32_t i;
qboolean b;
*/

cvar_t g_vert_fov          = {"g_vert_fov",          "",                        0.0f, 24,   qfalse,   TYPE_INT,  qtrue};
cvar_t g_horz_fov          = {"g_horz_fov",          "",                        0.0f, 64,   qfalse,   TYPE_INT,  qtrue};
cvar_t g_pspeed            = {"g_pspeed",            "",                        1.0f, 0,    qfalse, TYPE_FLOAT,  qtrue};
cvar_t g_gravity           = {"g_gravity",           "",                        1.5f, 0,    qfalse, TYPE_FLOAT,  qtrue};

cvar_t snd_musicvol        = {"snd_musicvol",        "",                        0.7f, 0,    qfalse, TYPE_FLOAT,  qtrue};
cvar_t snd_sfxvol          = {"snd_sfxvol",          "",                        1.0f, 0,    qfalse, TYPE_FLOAT,  qtrue};
cvar_t snd_musicon         = {"snd_musicon",         "",                        0.0f, 0,    qtrue,  TYPE_BOOL,   qtrue};
cvar_t snd_sfxon           = {"snd_sfxon",           "",                        0.0f, 0,    qtrue,  TYPE_BOOL,   qtrue};

cvar_t r_ticrate           = {"r_ticrate",           "",                        0.0f, 35,   qfalse, TYPE_INT,    qtrue};
cvar_t r_texture_magfilter = {"r_texture_magfilter", "GL_NEAREST",              0.0f, 0,    qfalse, TYPE_STRING, qtrue};
cvar_t r_texture_minfilter = {"r_texture_minfilter", "GL_LINEAR_MIPMAP_LINEAR", 0.0f, 0,    qfalse, TYPE_STRING, qtrue};
cvar_t r_screenheight      = {"r_screenheight",      "",                        0.0f, 720,  qfalse, TYPE_INT,    qtrue};
cvar_t r_screenwidth       = {"r_screenwidth",       "",                        0.0f, 1024, qfalse, TYPE_INT,    qtrue};
cvar_t r_vsync             = {"r_vsync",             "",                        0.0f, 0,    qtrue,  TYPE_BOOL,   qtrue};
cvar_t r_fullscreen        = {"r_fullscreen",        "",                        0.0f, 0,    qtrue,  TYPE_BOOL,   qtrue};
cvar_t r_native_fullscreen = {"r_native_fullscreen", "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};
cvar_t r_hidden            = {"r_hidden",            "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};
cvar_t r_drawFPS           = {"r_drawFPS",           "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};
cvar_t r_renderapi         = {"r_renderapi",         "R_OPENGL",                0.0f, 0,    qfalse, TYPE_STRING, qtrue};
cvar_t r_msaa_amount       = {"r_msaa_amount",       "",                        0.0f, 2,    qfalse, TYPE_INT,    qtrue};
cvar_t r_dither            = {"r_dither",            "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};

eastl::vector<const byte*> api_extensions;

cvar_t c_fastmobs1         = {"c_fastmobs1",         "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};
cvar_t c_fastmobs2         = {"c_fastmobs2",         "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};
cvar_t c_fastmobs3         = {"c_fastmobs3",         "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};
cvar_t c_deafmobs          = {"c_deafmobs",          "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};
cvar_t c_blindmobs         = {"c_blindmobs",         "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};
cvar_t c_nosmell           = {"c_nosmell",           "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};
cvar_t c_nomobs            = {"c_nomobs",            "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};
cvar_t c_godmode           = {"c_godmode",           "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};
cvar_t c_infinite_ammo     = {"c_infinite_ammo",     "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};
cvar_t c_bottomless_clip   = {"c_bottomless_clip",   "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};
cvar_t c_devmode           = {"c_devmode",           "",                        0.0f, 0,    qfalse, TYPE_BOOL,   qtrue};

static cvar_t *cvars[] = {
    &g_vert_fov,
    &g_horz_fov,
    &snd_musicvol,
    &snd_sfxvol,
    &snd_musicon,
    &snd_sfxon,
    &r_ticrate,
    &r_texture_magfilter,
    &r_texture_minfilter,
    &r_screenheight,
    &r_screenwidth,
    &r_vsync,
    &r_fullscreen,
    &r_native_fullscreen,
    &r_hidden,
    &r_drawFPS,
    &r_renderapi,
    &r_msaa_amount,
    &r_dither,
    &c_fastmobs1,
    &c_fastmobs2,
    &c_fastmobs3,
    &c_deafmobs,
    &c_blindmobs,
    &c_nosmell,
    &c_godmode,
    &c_infinite_ammo,
    &c_bottomless_clip,
    &c_devmode,
};

#define PRINTARG(x) fmt::arg(#x, x)

uint32_t G_NumCvars(void)
{
    return arraylen(cvars);
}

cvar_t** G_GetCvars(void)
{
    return cvars;
}

static void SCF_ParseFile(void)
{
    int parm = I_GetParm("-config");
    if (parm != -1) {

    }
}

static void Cvar_Load(const json& data, const eastl::string& name, cvar_t* cvar)
{
    if (!cvar->save) {
        return;
    }
    if (cvar->name[0] == 'c' && !data.contains(name)) { // cheats are not required in the config file
        return;
    }
    if (!data.contains(name)) {
        N_Error("Cvar_Load: cvar %s required in configuration file to run the game", name.c_str());
    }
    const eastl::string value = data[name];
    if (value.size() >= 64)
        N_Error("Cvar_Load: cvar value is too long (max of 64 characters), was %lu characters long", value.size());

    switch (cvar->type) {
    case TYPE_BOOL:
        if (value == "true")
            cvar->b = qtrue;
        else
            cvar->b = qfalse;
        break;
    case TYPE_INT:
        cvar->i = N_atoi(value.c_str());
        break;
    case TYPE_FLOAT:
        cvar->f = N_atof(value.c_str());
        break;
    case TYPE_STRING:
        memset(cvar->s, 0, sizeof(cvar->s));
        strncpy(cvar->s, value.c_str(), value.size());
        break;
    };

    Con_Printf("Initialized cvar %s with value %s", cvar->name, value.c_str());
}


void G_LoadSCF()
{
    std::ifstream stream("default.scf", std::ios::in);
    if (stream.fail()) {
        N_Error("G_LoadSCF: failed to open default.scf");
    }
    json data;
    try {
        data = json::parse(stream);
    } catch (const json::exception& e) {
        N_Error("G_LoadSCF: json error occurred, error id: %i, error message: %s", e.id, e.what());
    }
    stream.close();

    for (uint32_t i = 0; i < arraylen(cvars); i++) {
        Cvar_Load(data, cvars[i]->name, cvars[i]);
    }
}