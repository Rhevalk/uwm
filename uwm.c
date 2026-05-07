#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define CLEANMASK(mask) (mask & ~(XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2))

typedef union {
    int i;
    const char **v;
} Arg;

typedef struct {
    uint32_t mod;
    xcb_keysym_t keysym;
    void (*func)(const Arg *);
    const Arg arg;
    xcb_keycode_t code;
} Key;

static void spawn(const Arg *);
static void view(const Arg *);
static void swap_ws(const Arg *);
static void killclient(const Arg *);
static void swap_lr(const Arg *);
static void setup_keys();   
static void logout(const Arg *arg);

#include "config.h"

/* Global */
static xcb_connection_t *conn;
static xcb_screen_t *scr;

/* 2 window per workspace */
static xcb_window_t ws[WORKSPACES][2] = {{0}};
static int cur = 0;
static int focus_idx = 0;

static void setup_keys() {
    xcb_key_symbols_t *syms = xcb_key_symbols_alloc(conn);

    for (unsigned int i = 0; i < sizeof(keys)/sizeof(Key); i++) {
        xcb_keycode_t *kc = xcb_key_symbols_get_keycode(syms, keys[i].keysym);

        if (!kc) continue;

        for (int j = 0; kc[j] != XCB_NO_SYMBOL; j++) {
            xcb_grab_key(conn, 1, scr->root,
                         keys[i].mod, kc[j],
                         XCB_GRAB_MODE_ASYNC,
                         XCB_GRAB_MODE_ASYNC);
        }

        keys[i].code = kc[0];
        free(kc);
    }

    xcb_key_symbols_free(syms);
}

/* ---------- Core ---------- */
static void spawn(const Arg *arg) {
    if (fork() == 0) {
        if (conn) close(xcb_get_file_descriptor(conn));
        setsid();
        execvp(((char **)arg->v)[0], (char **)arg->v);
        _exit(1);
    }
}

static void focus_window(xcb_window_t win) {
    xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, win, XCB_CURRENT_TIME);
}

/* ---------- FIXED LAYOUT ---------- */
static void tile_two(xcb_window_t w0, xcb_window_t w1) {

    /* 1 WINDOW → FULLSCREEN */
    if (w0 && !w1) {
        uint32_t v[] = {
            0, 0,
            scr->width_in_pixels,
            scr->height_in_pixels
        };

        xcb_configure_window(conn, w0,
            XCB_CONFIG_WINDOW_X |
            XCB_CONFIG_WINDOW_Y |
            XCB_CONFIG_WINDOW_WIDTH |
            XCB_CONFIG_WINDOW_HEIGHT,
            v);
        return;
    }

    /* fallback safety */
    if (!w0) return;

    /* 2 WINDOW → SPLIT */
    uint32_t half = scr->width_in_pixels / 2;

    uint32_t v0[] = {
        0, 0,
        half,
        scr->height_in_pixels
    };

    xcb_configure_window(conn, w0,
        XCB_CONFIG_WINDOW_X |
        XCB_CONFIG_WINDOW_Y |
        XCB_CONFIG_WINDOW_WIDTH |
        XCB_CONFIG_WINDOW_HEIGHT,
        v0);

    if (w1) {
        uint32_t v1[] = {
            half, 0,
            scr->width_in_pixels - half,
            scr->height_in_pixels
        };

        xcb_configure_window(conn, w1,
            XCB_CONFIG_WINDOW_X |
            XCB_CONFIG_WINDOW_Y |
            XCB_CONFIG_WINDOW_WIDTH |
            XCB_CONFIG_WINDOW_HEIGHT,
            v1);
    }
}

/* ---------- VIEW ---------- */
static void view(const Arg *arg) {
    int n = arg->i;
    if (n == cur || n < 0 || n >= WORKSPACES) return;

    for (int i = 0; i < 2; i++)
        if (ws[cur][i])
            xcb_unmap_window(conn, ws[cur][i]);

    cur = n;

    if (ws[cur][0]) {
        for (int i = 0; i < 2; i++)
            if (ws[cur][i])
                xcb_map_window(conn, ws[cur][i]);

        tile_two(ws[cur][0], ws[cur][1]);

        focus_window(ws[cur][focus_idx ? 1 : 0]);
    } else {
        focus_window(scr->root);
    }
}

/* ---------- SWAP WORKSPACE ---------- */
static void swap_ws(const Arg *arg) {
    int n = arg->i;
    if (n < 0 || n >= WORKSPACES || n == cur) return;

    for (int i = 0; i < 2; i++) {
        xcb_window_t tmp = ws[n][i];
        ws[n][i] = ws[cur][i];
        ws[cur][i] = tmp;
    }

    for (int i = 0; i < WORKSPACES; i++)
        for (int j = 0; j < 2; j++)
            if (ws[i][j])
                xcb_unmap_window(conn, ws[i][j]);

    if (ws[cur][0]) {
        for (int i = 0; i < 2; i++)
            if (ws[cur][i])
                xcb_map_window(conn, ws[cur][i]);

        tile_two(ws[cur][0], ws[cur][1]);
        focus_window(ws[cur][focus_idx ? 1 : 0]);
    } else {
        focus_window(scr->root);
    }
}

/* ---------- FIX: NO SWAP POSITION, ONLY FOCUS ---------- */
static void swap_lr(const Arg *arg) {
    if (!ws[cur][0] && !ws[cur][1]) return;

    /* cuma pindah fokus */
    if (ws[cur][1]) {
        focus_idx ^= 1;
        focus_window(ws[cur][focus_idx]);
    }
}

/* ---------- KILL ---------- */
static void killclient(const Arg *arg) {
    xcb_window_t win = ws[cur][focus_idx];

    if (win) {
        xcb_kill_client(conn, win);

        if (focus_idx == 0) {
            ws[cur][0] = ws[cur][1];
            ws[cur][1] = 0;
        } else {
            ws[cur][1] = 0;
        }

        tile_two(ws[cur][0], ws[cur][1]);
    }
}

/* ---------- MAP REQUEST ---------- */
static void handle_map(xcb_window_t win) {
    if (!ws[cur][0]) {
        ws[cur][0] = win;
    } else if (!ws[cur][1]) {
        ws[cur][1] = win;
    } else {
        xcb_kill_client(conn, win);
        return;
    }

    xcb_map_window(conn, win);
    tile_two(ws[cur][0], ws[cur][1]);
    focus_window(win);
}

static void logout(const Arg *arg) {
    if (fork() == 0) {
        if (conn) close(xcb_get_file_descriptor(conn));
        setsid();

        /* cara paling simple logout */
        execlp("loginctl", "loginctl", "terminate-session", NULL);

        _exit(1);
    }
}

/* ---------- MAIN LOOP (unchanged except map fix) ---------- */
int main(void) {
    conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) return 1;

    scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

    signal(SIGCHLD, SIG_IGN);

    uint32_t values[] = {
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
    };

    xcb_change_window_attributes(conn, scr->root,
        XCB_CW_EVENT_MASK, values);

    setup_keys();
    xcb_flush(conn);

    xcb_generic_event_t *ev;

    while ((ev = xcb_wait_for_event(conn))) {
        switch (ev->response_type & ~0x80) {

        case XCB_MAP_REQUEST: {
            xcb_map_request_event_t *e = (xcb_map_request_event_t *)ev;
            handle_map(e->window);
            break;
        }

        case XCB_KEY_PRESS: {
            xcb_key_press_event_t *e = (xcb_key_press_event_t *)ev;

            for (unsigned int i = 0; i < sizeof(keys)/sizeof(Key); i++) {
                if (e->detail == keys[i].code &&
                    CLEANMASK(e->state) == CLEANMASK(keys[i].mod)) {
                    keys[i].func(&(keys[i].arg));
                    break;
                }
            }
            break;
        }

        case XCB_DESTROY_NOTIFY: {
            xcb_destroy_notify_event_t *e =
                (xcb_destroy_notify_event_t *)ev;

            for (int i = 0; i < WORKSPACES; i++) {
                for (int j = 0; j < 2; j++) {
                    if (ws[i][j] == e->window) {
                        ws[i][j] = 0;
                        tile_two(ws[i][0], ws[i][1]);
                    }
                }
            }
            break;
        }

        case XCB_UNMAP_NOTIFY:
            break;
        }

        free(ev);
        xcb_flush(conn);
    }

    xcb_disconnect(conn);
    return 0;
}
