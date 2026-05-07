#pragma once

#define WORKSPACES 8
#define MODKEY XCB_MOD_MASK_4

#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

static const char *termcmd[] = { "st", NULL };
static const char *menu[]    = { "rofi", "-show", "drun", NULL };

#include <X11/XF86keysym.h>
/* volume */
static const char *volup[]    = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "+5%", NULL };
static const char *voldown[]  = { "pactl", "set-sink-volume", "@DEFAULT_SINK@", "-5%", NULL };
static const char *volmute[]  = { "pactl", "set-sink-mute",   "@DEFAULT_SINK@", "toggle", NULL };

/* brightness */
static const char *brightup[]   = { "brightnessctl", "set", "+10%", NULL };
static const char *brightdown[] = { "brightnessctl", "set", "10%-", NULL };

static Key keys[] = {
    { MODKEY, XK_Return, spawn,      {.v = termcmd} },
    { MODKEY, XK_d,      spawn,      {.v = menu} },
    { MODKEY, XK_q,      killclient, {0} },
    { MODKEY, XK_Escape, logout,     {0} },

    { MODKEY, XK_Left,   swap_lr,    {0} },
    { MODKEY, XK_Right,  swap_lr,    {0} },

    { MODKEY, XK_1, view, {.i = 0} },
    { MODKEY, XK_2, view, {.i = 1} },
    { MODKEY, XK_3, view, {.i = 2} },
    { MODKEY, XK_4, view, {.i = 3} },
    { MODKEY, XK_5, view, {.i = 4} },
    { MODKEY, XK_6, view, {.i = 5} },
    { MODKEY, XK_7, view, {.i = 6} },
    { MODKEY, XK_8, view, {.i = 7} },

    { MODKEY|XCB_MOD_MASK_SHIFT, XK_1, swap_ws, {.i = 0} },
    { MODKEY|XCB_MOD_MASK_SHIFT, XK_2, swap_ws, {.i = 1} },
    { MODKEY|XCB_MOD_MASK_SHIFT, XK_3, swap_ws, {.i = 2} },
    { MODKEY|XCB_MOD_MASK_SHIFT, XK_4, swap_ws, {.i = 3} },
    { MODKEY|XCB_MOD_MASK_SHIFT, XK_5, swap_ws, {.i = 4} },
    { MODKEY|XCB_MOD_MASK_SHIFT, XK_6, swap_ws, {.i = 5} },
    { MODKEY|XCB_MOD_MASK_SHIFT, XK_7, swap_ws, {.i = 6} },
    { MODKEY|XCB_MOD_MASK_SHIFT, XK_8, swap_ws, {.i = 7} },

	  { MODKEY|XCB_MOD_MASK_SHIFT,     XK_d,      spawn,          SHCMD("~/.local/bin/sysinfo | rofi -dmenu -p Info") },

    {0, XF86XK_AudioRaiseVolume, spawn, {.v = volup}},
    {0, XF86XK_AudioLowerVolume, spawn, {.v = voldown}},
    {0, XF86XK_AudioMute,       spawn, {.v = volmute}},

    {0, XF86XK_MonBrightnessUp,   spawn, {.v = brightup}},
    {0, XF86XK_MonBrightnessDown, spawn, {.v = brightdown}},

    { 0,          XK_Print, spawn, SHCMD("scrot ~/Pictures/Screenshots/%Y-%m-%d_%H-%M-%S.png") },
    { MODKEY,     XK_Print, spawn, SHCMD("scrot -s ~/Pictures/Screenshots/%Y-%m-%d_%H-%M-%S.png") },

};
