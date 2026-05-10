#pragma once

#define WORKSPACES 8
#define MAX_FLOATING 8

#define BORDER_WIDTH 1

#define COLOR_FOCUS  0x383838
#define COLOR_NORMAL 0x222222

#define MODKEY XCB_MOD_MASK_4

#include <X11/XF86keysym.h>

#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

static const char *termcmd[] = { "st", NULL };
static const char *menu[]    = { "rofi", "-show", "drun", NULL };

static const char *brightup[]   = { "brightnessctl", "set", "+10%", NULL };
static const char *brightdown[] = { "brightnessctl", "set", "10%-", NULL };

static Key keys[] = {
    /* Modifier                     Key                      Function           Argument */
    { MODKEY,                       XK_Return,               spawn,             {.v = termcmd} },
    { MODKEY,                       XK_d,                    spawn,             {.v = menu}    },
    { MODKEY,                       XK_Tab,                  focus_stack,       {.i = +1}      },
    { MODKEY|XCB_MOD_MASK_SHIFT,    XK_Tab,                  focus_stack,       {.i = -1}      },
    { MODKEY|XCB_MOD_MASK_SHIFT,    XK_q,                    kill_client,       {0}            },
    { MODKEY|XCB_MOD_MASK_SHIFT,    XK_Escape,               logout,            {0}            },

    /* Workspace Switching */
    { MODKEY,                       XK_1,                    view,              {.i = 0} },
    { MODKEY,                       XK_2,                    view,              {.i = 1} },
    { MODKEY,                       XK_3,                    view,              {.i = 2} },
    { MODKEY,                       XK_4,                    view,              {.i = 3} },
    { MODKEY,                       XK_5,                    view,              {.i = 4} },
    { MODKEY,                       XK_6,                    view,              {.i = 5} },
    { MODKEY,                       XK_7,                    view,              {.i = 6} },
    { MODKEY,                       XK_8,                    view,              {.i = 7} },

    /* Move Window to Workspace */
    { MODKEY|XCB_MOD_MASK_SHIFT,    XK_1,                    move_ws,{.i = 0} },
    { MODKEY|XCB_MOD_MASK_SHIFT,    XK_2,                    move_ws,{.i = 1} },
    { MODKEY|XCB_MOD_MASK_SHIFT,    XK_3,                    move_ws,{.i = 2} },
    { MODKEY|XCB_MOD_MASK_SHIFT,    XK_4,                    move_ws,{.i = 3} },
    { MODKEY|XCB_MOD_MASK_SHIFT,    XK_5,                    move_ws,{.i = 4} },
    { MODKEY|XCB_MOD_MASK_SHIFT,    XK_6,                    move_ws,{.i = 5} },
    { MODKEY|XCB_MOD_MASK_SHIFT,    XK_7,                    move_ws,{.i = 6} },
    { MODKEY|XCB_MOD_MASK_SHIFT,    XK_8,                    move_ws,{.i = 7} },

    /* Hardware Keys (Brightness) */
    { 0,                            XF86XK_MonBrightnessUp,   spawn,             {.v = brightup} },
    { 0,                            XF86XK_MonBrightnessDown, spawn,             {.v = brightdown} },

    /* Screenshots & System Info */
    { 0,                            XK_Print,                spawn,             SHCMD("maim ~/Pictures/Screenshots/%Y-%m-%d_%H-%M-%S.png") },
    { MODKEY,                       XK_Print,                spawn,             SHCMD("maim -s ~/Pictures/Screenshots/%Y-%m-%d_%H-%M-%S.png") },
    { MODKEY|XCB_MOD_MASK_SHIFT,    XK_d,                    spawn,             SHCMD("~/.local/bin/sysinfo | rofi -dmenu -p Info") },
};
