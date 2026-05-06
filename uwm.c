#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define WORKSPACES 4

static xcb_connection_t *conn;
static xcb_screen_t *scr;
static xcb_window_t ws[WORKSPACES] = {0};
static int cur = 0;
static xcb_key_symbols_t *syms;

void spawn(char **cmd) {
    if (fork() == 0) {
        setsid();
        execvp(cmd[0], cmd);
        _exit(1);
    }
}

void focus_window(xcb_window_t win) {
    xcb_set_input_focus(conn, XCB_INPUT_FOCUS_POINTER_ROOT, win, XCB_CURRENT_TIME);
}

void tile_fullscreen(xcb_window_t win) {
    uint32_t vals[] = {
        0, 0,
        scr->width_in_pixels,
        scr->height_in_pixels,
        XCB_STACK_MODE_ABOVE
    };

    xcb_configure_window(conn, win,
        XCB_CONFIG_WINDOW_X |
        XCB_CONFIG_WINDOW_Y |
        XCB_CONFIG_WINDOW_WIDTH |
        XCB_CONFIG_WINDOW_HEIGHT |
        XCB_CONFIG_WINDOW_STACK_MODE,
        vals);
}

void view(int n) {
    if (n == cur || n < 0 || n >= WORKSPACES) return;

    if (ws[cur])
        xcb_unmap_window(conn, ws[cur]);

    cur = n;

    if (ws[cur]) {
        xcb_map_window(conn, ws[cur]);
        tile_fullscreen(ws[cur]);
        focus_window(ws[cur]);
    } else {
        focus_window(scr->root);
    }

    xcb_flush(conn);
}

void grab_key(xcb_keysym_t sym, uint32_t mod) {
    xcb_keycode_t *kc = xcb_key_symbols_get_keycode(syms, sym);
    if (!kc) return;

    for (int i = 0; kc[i] != XCB_NO_SYMBOL; i++) {
        xcb_grab_key(conn, 1, scr->root, mod, kc[i], XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
    }

    free(kc);
}

int main(void) {
    conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) return 1;

    scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;
    syms = xcb_key_symbols_alloc(conn);

    signal(SIGCHLD, SIG_IGN);

    uint32_t mask = XCB_CW_EVENT_MASK;
    uint32_t values = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT | XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY;

    xcb_change_window_attributes(conn, scr->root, mask, &values);

    grab_key(XK_1, XCB_MOD_MASK_4);
    grab_key(XK_2, XCB_MOD_MASK_4);
    grab_key(XK_3, XCB_MOD_MASK_4);
    grab_key(XK_4, XCB_MOD_MASK_4);

    grab_key(XK_Return, XCB_MOD_MASK_4);
    grab_key(XK_d,      XCB_MOD_MASK_4);
    grab_key(XK_q,      XCB_MOD_MASK_4);
    grab_key(XK_Escape, XCB_MOD_MASK_4);

    xcb_flush(conn);

    xcb_generic_event_t *ev;
    while ((ev = xcb_wait_for_event(conn))) {

        switch (ev->response_type & ~0x80) {

        case XCB_MAP_REQUEST: {
            xcb_map_request_event_t *e = (xcb_map_request_event_t *)ev;

            if (!ws[cur]) {
                ws[cur] = e->window;

                tile_fullscreen(e->window);
                xcb_map_window(conn, e->window);
                focus_window(e->window);

            } else {
                xcb_kill_client(conn, e->window);
            }

            break;
        }

        case XCB_KEY_PRESS: {
            xcb_key_press_event_t *e = (xcb_key_press_event_t *)ev;
            xcb_keysym_t ks = xcb_key_symbols_get_keysym(syms, e->detail, 0);

            if (ks >= XK_1 && ks <= XK_4) {
                view(ks - XK_1);
            }
            else if (ks == XK_Return) {
                char *c[] = {"st", NULL};
                spawn(c);
            }
            else if (ks == XK_d) {
                char *c[] = {"rofi", "-show", "drun", NULL};
                spawn(c);
            }
            else if (ks == XK_q && ws[cur]) {
                xcb_kill_client(conn, ws[cur]);
                ws[cur] = 0;
            }
            else if (ks == XK_Escape) {
                break;
            }

            break;
        }

        case XCB_UNMAP_NOTIFY:
            break;

        case XCB_DESTROY_NOTIFY: {
            xcb_destroy_notify_event_t *e = (xcb_destroy_notify_event_t *)ev;

            for (int i = 0; i < WORKSPACES; i++) {
                if (ws[i] == e->window) {
                    ws[i] = 0;
                }
            }

            break;
        }

        default:
            break;
        }

        free(ev);
        xcb_flush(conn);
    }

    xcb_key_symbols_free(syms);
    xcb_disconnect(conn);
    return 0;
}
