#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>

#define MAX_CLIENTS   8
#define MAX_WORKSPACE 4

#define CLEANMASK(m)       ((m) & ~(XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2))
#define NELEMS(x)          (sizeof(x)/sizeof(x[0]))
#define CWS                (workspaces[ws_current])
#define CLAMP(v, min, max) ((v) < (min) ? (min) : ((v) > (max) ? (max) : (v)))

typedef union { 
    const char **v; 
    int i; 
} Arg;

typedef struct { 
    uint32_t mod; 
    xcb_keysym_t keysym; 
    void (*func)(const Arg*); 
    const Arg arg; 
    xcb_keycode_t keycode; 
} Key;

typedef struct { 
    xcb_window_t clients[MAX_CLIENTS]; 
    xcb_window_t last_focus; 
    uint8_t map; 
} Workspace;

static void cmd_quit(const Arg*);
static void cmd_spawn(const Arg*);
static void cmd_kill_client(const Arg*);
static void cmd_switch_focus(const Arg*);
static void cmd_snap(const Arg*);
static void cmd_view_ws(const Arg*);
static void cmd_send_ws(const Arg*);

#include "config.h"

static xcb_connection_t  *conn;
static xcb_key_symbols_t *syms;
static xcb_screen_t      *scr;
static xcb_window_t       focused = XCB_NONE;
static xcb_atom_t         WM_PROTOCOLS, WM_DELETE_WINDOW;
static Workspace          workspaces[MAX_WORKSPACE];
static uint8_t            ws_current;

static inline void 
u_focus(xcb_window_t w) {
    focused = w;
    xcb_set_input_focus(conn, XCB_INPUT_FOCUS_PARENT, w ? w : scr->root, XCB_CURRENT_TIME);
    if (w) xcb_configure_window(conn, w, XCB_CONFIG_WINDOW_STACK_MODE, (uint32_t[]){ XCB_STACK_MODE_ABOVE });
}

static inline int 
u_find_client(xcb_window_t w, int *ws_idx) {
    for (int ws = 0; ws < MAX_WORKSPACE; ws++)
        for (uint8_t m = workspaces[ws].map; m; m &= m - 1)
            if (workspaces[ws].clients[__builtin_ctz(m)] == w) { 
                if (ws_idx) *ws_idx = ws; 
                return __builtin_ctz(m); 
            }
    return -1;
}

static inline void 
u_move_resize(xcb_window_t w, int32_t x, int32_t y, uint32_t ww, uint32_t wh) {
    xcb_configure_window(conn, w, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | 
        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT | XCB_CONFIG_WINDOW_STACK_MODE, 
        (uint32_t[]){ (uint32_t)x, (uint32_t)y, ww, wh, XCB_STACK_MODE_ABOVE });
}

static inline void 
u_remove_and_refocus(xcb_window_t w) {
    int ws, idx;
    if ((idx = u_find_client(w, &ws)) == -1 || ws != ws_current) return;
    workspaces[ws].map &= ~(1 << idx); 
    if (focused == w) u_focus(workspaces[ws].map ? workspaces[ws].clients[__builtin_ctz(workspaces[ws].map)] : XCB_NONE);
}

static void 
cmd_view_ws(const Arg *arg) {
    if (arg->i >= MAX_WORKSPACE || arg->i == ws_current) return;

    CWS.last_focus = focused;
    for (uint8_t m = CWS.map; m; m &= m - 1) xcb_unmap_window(conn, CWS.clients[__builtin_ctz(m)]);

    ws_current = arg->i;
    for (uint8_t m = CWS.map; m; m &= m - 1) xcb_map_window(conn, CWS.clients[__builtin_ctz(m)]);
    
    int ws_found;
    xcb_window_t target = XCB_NONE;
    if (CWS.map) {
        target = (u_find_client(CWS.last_focus, &ws_found) != -1 && ws_found == ws_current) 
                 ? CWS.last_focus 
                 : CWS.clients[__builtin_ctz(CWS.map)];
    }

    u_focus(target);
    xcb_flush(conn);
}

static void 
cmd_send_ws(const Arg *arg) {
    int ws_idx, idx;
    if (arg->i >= MAX_WORKSPACE || arg->i == ws_current || focused == XCB_NONE || (idx = u_find_client(focused, &ws_idx)) == -1) return;

    xcb_unmap_window(conn, focused);
    CWS.map &= ~(1 << idx);       

    int n_idx = __builtin_ctz(~workspaces[arg->i].map);  
    workspaces[arg->i].clients[n_idx] = focused;
    workspaces[arg->i].map |= (1 << n_idx); 

    u_focus(CWS.map ? CWS.clients[__builtin_ctz(CWS.map)] : XCB_NONE);
    xcb_flush(conn);
}

static void 
cmd_switch_focus(const Arg *arg) { (void)arg; 
    int idx = u_find_client(focused, NULL);
    if (idx == -1 || CWS.map == 0) return;

    uint8_t higher = CWS.map & ~((1 << (idx + 1)) - 1); 

    u_focus(CWS.clients[__builtin_ctz(higher ? higher : CWS.map)]);
    xcb_flush(conn);
}

static void 
cmd_kill_client(const Arg *arg) { (void)arg; 
    if (focused == XCB_NONE) return;

    xcb_send_event(conn, 0, focused, XCB_EVENT_MASK_NO_EVENT, (char *)&(xcb_client_message_event_t){
        .response_type = XCB_CLIENT_MESSAGE, 
        .format = 32, 
        .window = focused, 
        .type = WM_PROTOCOLS, 
        .data.data32 = {WM_DELETE_WINDOW, XCB_CURRENT_TIME}
    });
    xcb_flush(conn);
}

static void 
cmd_snap(const Arg *arg) {
    if (focused == XCB_NONE) return;
    uint32_t sw = scr->width_in_pixels, sh = scr->height_in_pixels;
    static uint8_t state = 0;

    // 1: Atas (4), 2: Bawah (8), 3: Kanan (2), 4: Kiri (1)
    if (arg->i == 4) state = (state & 2) ? (state & ~2) : (state | 1); // Kiri: lepas kanan, baru pasang kiri
    if (arg->i == 3) state = (state & 1) ? (state & ~1) : (state | 2); // Kanan: lepas kiri, baru pasang kanan
    if (arg->i == 1) state = (state & 8) ? (state & ~8) : (state | 4); // Atas: lepas bawah, baru pasang atas
    if (arg->i == 2) state = (state & 4) ? (state & ~4) : (state | 8); // Bawah: lepas atas, baru pasang bawah

    // Hitung geometri kilat pakai bitwise shift murni (4 Byte / 1 Byte state)
    u_move_resize(focused, 
        (state & 2)  ? (sw >> 1) : 0,  (state & 8)  ? (sh >> 1) : 0,
        (state & 3)  ? (sw >> 1) : sw, (state & 12) ? (sh >> 1) : sh
    );
    xcb_flush(conn);
}

static void 
cmd_spawn(const Arg *arg) { 
    if (fork() == 0) { 
        if (conn) 
            close(xcb_get_file_descriptor(conn)); 

        setsid(); 
        execvp(((char **)arg->v)[0], (char **)arg->v); 
        exit(1); 
    } 
}

static void 
cmd_quit(const Arg *arg) { (void)arg; 
    exit(0); 
}

static void 
handle_map_request(xcb_generic_event_t *ev) {
    if (CWS.map == 0xFF) return;

    xcb_map_request_event_t *e = (xcb_map_request_event_t *)ev;
    xcb_change_window_attributes(conn, e->window, XCB_CW_EVENT_MASK, (uint32_t[]){ 
        XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_STRUCTURE_NOTIFY 
    });

    u_move_resize(e->window, 0, 0, scr->width_in_pixels, scr->height_in_pixels);

    int i = __builtin_ctz(~CWS.map); CWS.clients[i] = e->window; CWS.map |= (1 << i);
    xcb_map_window(conn, e->window); u_focus(e->window); xcb_flush(conn);
}

static void 
handle_unmap_notify(xcb_generic_event_t *ev) { 
    u_remove_and_refocus(((xcb_unmap_notify_event_t *)ev)->window); 
    xcb_flush(conn); 
}

static void 
handle_destroy_notify(xcb_generic_event_t *ev) { 
    u_remove_and_refocus(((xcb_destroy_notify_event_t *)ev)->window); 
    xcb_flush(conn); 
}

static void 
handle_key_press(xcb_generic_event_t *ev) {
    xcb_key_press_event_t *e = (xcb_key_press_event_t *)ev;
    for (size_t i = 0; i < NELEMS(keys); i++)
        if (e->detail == keys[i].keycode && CLEANMASK(e->state) == CLEANMASK(keys[i].mod)) { keys[i].func(&keys[i].arg); break; }
}

static void 
setup_main(void) {
    if (!(scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data)) exit(1);
    signal(SIGCHLD, SIG_IGN);

    if (xcb_request_check(conn, xcb_change_window_attributes_checked(conn, scr->root, 
                    XCB_CW_EVENT_MASK, (uint32_t[]){ 
                    XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | 
                    XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY }))
        ) exit(1);

    xcb_intern_atom_cookie_t c1 = xcb_intern_atom(conn, 0, 12, "WM_PROTOCOLS"), c2 = xcb_intern_atom(conn, 0, 16, "WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t *r1 = xcb_intern_atom_reply(conn, c1, NULL), *r2 = xcb_intern_atom_reply(conn, c2, NULL);

    if (r1) { WM_PROTOCOLS     = r1->atom; free(r1); } 
    if (r2) { WM_DELETE_WINDOW = r2->atom; free(r2); }

    syms = xcb_key_symbols_alloc(conn);
    for (size_t i = 0; i < NELEMS(keys); i++) {
        xcb_keycode_t *kc = xcb_key_symbols_get_keycode(syms, keys[i].keysym);
        if (kc) { 
            keys[i].keycode = kc[0]; 
            xcb_grab_key(conn, 1, scr->root, keys[i].mod, kc[0], 
                    XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC); 
            free(kc); 
        }
    }

    xcb_flush(conn);
}

int main(void) {
    if (xcb_connection_has_error(conn = xcb_connect(NULL, NULL))) return 1;
    setup_main(); xcb_generic_event_t *ev;
    while ((ev = xcb_wait_for_event(conn))) {
        switch (ev->response_type & ~0x80) {
            case XCB_MAP_REQUEST:    handle_map_request(ev);    break;
            case XCB_UNMAP_NOTIFY:   handle_unmap_notify(ev);   break;
            case XCB_DESTROY_NOTIFY: handle_destroy_notify(ev); break;
            case XCB_KEY_PRESS:      handle_key_press(ev);      break;
        }
        free(ev);
    }
    xcb_key_symbols_free(syms); xcb_disconnect(conn); return 0;
}
