#pragma once

#include <X11/keysym.h>

#define MOD          XCB_MOD_MASK_4
#define SHIFT        XCB_MOD_MASK_SHIFT

#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* ================= CONFIGURATION ================= */
static const char *term[] = { "term", NULL }; 
static const char *menu[] = { "menu", NULL };

static Key keys[] = {
    /* modifier          key            function         argument */
    { MOD,               XK_d,          cmd_spawn,       {.v = menu} },
    { MOD,               XK_Return,     cmd_spawn,       {.v = term} },

    { MOD,               XK_Tab,        cmd_switch_focus, {0} },

    { MOD|SHIFT,         XK_q,          cmd_kill_client,  {0} },
    { MOD|SHIFT,         XK_Escape,     cmd_quit,         {0} },

    { MOD,               XK_Up,         cmd_snap,         {.i = 1} },
    { MOD,               XK_Down,       cmd_snap,         {.i = 2} },
    { MOD,               XK_Right,      cmd_snap,         {.i = 3} },
    { MOD,               XK_Left,       cmd_snap,         {.i = 4} },

    { MOD,               XK_1,          cmd_view_ws,      {.i = 0} },
    { MOD,               XK_2,          cmd_view_ws,      {.i = 1} },
    { MOD,               XK_3,          cmd_view_ws,      {.i = 2} },
    { MOD,               XK_4,          cmd_view_ws,      {.i = 3} },

    { MOD|SHIFT,         XK_1,          cmd_send_ws,      {.i = 0} },
    { MOD|SHIFT,         XK_2,          cmd_send_ws,      {.i = 1} },
    { MOD|SHIFT,         XK_3,          cmd_send_ws,      {.i = 2} },
    { MOD|SHIFT,         XK_4,          cmd_send_ws,      {.i = 3} },
};
