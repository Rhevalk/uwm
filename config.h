#pragma once

#include <X11/keysym.h>
#include <X11/XF86keysym.h> 

#define MOD          XCB_MOD_MASK_4
#define SHIFT        XCB_MOD_MASK_SHIFT

#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }
/* ================= CONFIGURATION ================= */
static const char *info[]     = { "/home/arcshzen/.local/bin/menu", "i", NULL };
static const char *shell[]    = { "/home/arcshzen/.local/bin/menu", "s", NULL };

/* volume */
static const char *volup[]    = { "wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%+", NULL };
static const char *voldown[]  = { "wpctl", "set-volume", "@DEFAULT_AUDIO_SINK@", "5%-", NULL };
static const char *volmute[]  = { "wpctl", "set-mute",   "@DEFAULT_AUDIO_SINK@", "toggle", NULL };

/* brightness */
static const char *brightup[]   = { "brightnessctl", "set", "+10%", NULL };
static const char *brightdown[] = { "brightnessctl", "set", "10%-", NULL };

/* screenshot */
static const char *screenshot[]      = { "/bin/sh", "-c", "maim -s ~/Pictures/Screenshots/$(date +%Y-%m-%d_%H-%M-%S).png", NULL };
static const char *screenshot_full[] = { "/bin/sh", "-c", "maim ~/Pictures/Screenshots/$(date +%Y-%m-%d_%H-%M-%S).png", NULL };

static Key keys[] = {
    { MOD|SHIFT, XK_d,         cmd_spawn,        {.v = info}     },
    { MOD,       XK_Return,    cmd_spawn,        {.v = shell}    },

    { MOD,       XK_Tab,       cmd_switch_focus, {0} },

    { MOD|SHIFT, XK_q,         cmd_kill_client,  {0} },
    { MOD|SHIFT, XK_Escape,    cmd_quit,         {0} },


    { MOD,       XK_Up,        cmd_snap,         {.i = 1} },
    { MOD,       XK_Down,      cmd_snap,         {.i = 2} },
    { MOD,       XK_Right,     cmd_snap,         {.i = 3} },
    { MOD,       XK_Left,      cmd_snap,         {.i = 4} },

    { MOD,       XK_1,         cmd_view_ws,      {.i = 0} },
    { MOD,       XK_2,         cmd_view_ws,      {.i = 1} },
    { MOD,       XK_3,         cmd_view_ws,      {.i = 2} },
    { MOD,       XK_4,         cmd_view_ws,      {.i = 3} },

    { MOD|SHIFT, XK_1,         cmd_send_ws,      {.i = 0} },
    { MOD|SHIFT, XK_2,         cmd_send_ws,      {.i = 1} },
    { MOD|SHIFT, XK_3,         cmd_send_ws,      {.i = 2} },
    { MOD|SHIFT, XK_4,         cmd_send_ws,      {.i = 3} },

    { 0, XF86XK_AudioRaiseVolume,  cmd_spawn, {.v = volup}   },
    { 0, XF86XK_AudioLowerVolume,  cmd_spawn, {.v = voldown} }, 
    { 0, XF86XK_AudioMute,         cmd_spawn, {.v = volmute} },

    { 0, XF86XK_MonBrightnessUp,   cmd_spawn, {.v = brightup}   },
    { 0, XF86XK_MonBrightnessDown, cmd_spawn, {.v = brightdown} },

    { 0,   XK_Print, cmd_spawn, {.v = screenshot_full} }, 
    { MOD, XK_Print, cmd_spawn, {.v = screenshot}      }, 
};
