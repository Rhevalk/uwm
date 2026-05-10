/* uwm - micro window manager
 * XCB-based, single-file, "Organic Memory" philosophy.
 * Edit config.h to configure. Recompile after changes.
 */

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_icccm.h>
#include <X11/keysym.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

#define LEN(x)          (sizeof(x) / sizeof((x)[0]))
#define CLEANMASK(mask) ((mask) & ~(XCB_MOD_MASK_LOCK | XCB_MOD_MASK_2))

typedef union {
    int i;
    const char **v;
} Arg;

typedef struct {
    uint32_t      mod;
    xcb_keysym_t  keysym;
    void        (*func)(const Arg *);
    const Arg     arg;
    xcb_keycode_t code;
} Key;

typedef struct {
    uint32_t mod;
    uint8_t  button; /* XCB_BUTTON_INDEX_1, _2, _3 */
    void   (*func)(void);
} Button;

/* --- forward declarations --- */
static void view(const Arg *arg);
static void spawn(const Arg *arg);
static void logout(const Arg *arg);
static void move_ws(const Arg *arg);
static void focus_stack(const Arg *arg);
static void kill_client(const Arg *arg);
static void mouse_drag(void);
static void mouse_resize(void);
static void subscribe_window(xcb_window_t win);

#include "config.h"

/* --- global state --- */
static xcb_connection_t *conn;
static xcb_screen_t     *scr;

static xcb_atom_t WM_PROTOCOLS;
static xcb_atom_t WM_DELETE_WINDOW;

static xcb_window_t ws[WORKSPACES][2];
static xcb_window_t floating[WORKSPACES][MAX_FLOATING];
static int          float_count[WORKSPACES];

static xcb_window_t focused = XCB_NONE;
static int          cur     = 0;

/* --- drag & resize state --- */
typedef enum { MOUSE_NONE, MOUSE_DRAG, MOUSE_RESIZE } MouseMode;

static struct {
    MouseMode    mode;
    xcb_window_t win;
    int          start_x, start_y; /* posisi pointer saat button ditekan */
    int          win_x,   win_y;   /* posisi window saat button ditekan  */
    int          win_w,   win_h;   /* ukuran window saat button ditekan  */
} mouse = { MOUSE_NONE, XCB_NONE, 0, 0, 0, 0, 0, 0 };

/* ================================================================
 * HELPERS
 * ================================================================ */

static void
mapw(xcb_window_t w)
{
    if (w) xcb_map_window(conn, w);
}

static void
unmapw(xcb_window_t w)
{
    if (w) xcb_unmap_window(conn, w);
}

static void
foreach_window(int wsid, void (*fn)(xcb_window_t))
{
    if (ws[wsid][0]) fn(ws[wsid][0]);
    if (ws[wsid][1]) fn(ws[wsid][1]);
    for (int i = 0; i < float_count[wsid]; i++)
        if (floating[wsid][i]) fn(floating[wsid][i]);
}

/* Returns 1 jika window ada di salah satu slot workspace/floating.
 * Juga mengisi *out_ws dan *out_is_tiling jika pointer tidak NULL. */
static int
find_window(xcb_window_t win, int *out_ws, int *out_is_tiling)
{
    for (int i = 0; i < WORKSPACES; i++) {
        if (ws[i][0] == win || ws[i][1] == win) {
            if (out_ws)       *out_ws       = i;
            if (out_is_tiling) *out_is_tiling = 1;
            return 1;
        }
        for (int j = 0; j < float_count[i]; j++) {
            if (floating[i][j] == win) {
                if (out_ws)       *out_ws       = i;
                if (out_is_tiling) *out_is_tiling = 0;
                return 1;
            }
        }
    }
    return 0;
}

static int
is_managed(xcb_window_t win)
{
    return find_window(win, NULL, NULL);
}

static int
is_transient(xcb_window_t win)
{
    xcb_window_t transient;
    return xcb_icccm_get_wm_transient_for_reply(
        conn,
        xcb_icccm_get_wm_transient_for(conn, win),
        &transient,
        NULL
    );
}

/* Returns 1 jika layar hanya ada 1 window tiling tanpa floating */
static int
is_single_tiled(void)
{
    return ws[cur][0] && !ws[cur][1] && float_count[cur] == 0;
}

/* ================================================================
 * ATOM HELPER - menghindari crash jika X server tidak merespons
 * ================================================================ */

static xcb_atom_t
intern_atom(const char *name)
{
    xcb_intern_atom_cookie_t ck =
        xcb_intern_atom(conn, 0, (uint16_t)strlen(name), name);
    xcb_intern_atom_reply_t *r =
        xcb_intern_atom_reply(conn, ck, NULL);
    if (!r) return XCB_ATOM_NONE;
    xcb_atom_t atom = r->atom;
    free(r);
    return atom;
}

/* ================================================================
 * WINDOW MANAGEMENT
 * ================================================================ */

static void
set_border(xcb_window_t win, uint32_t color)
{
    if (!win) return;
    uint32_t bw[] = { BORDER_WIDTH };
    xcb_configure_window(conn, win,
        XCB_CONFIG_WINDOW_BORDER_WIDTH, bw);
    xcb_change_window_attributes(conn, win,
        XCB_CW_BORDER_PIXEL, &color);
}

static void
focus(xcb_window_t win)
{
    if (!win || focused == win) return;

    if (focused && is_managed(focused))
        set_border(focused, COLOR_NORMAL);

    focused = win;

    if (!is_single_tiled())
        set_border(win, COLOR_FOCUS);

    uint32_t above[] = { XCB_STACK_MODE_ABOVE };
    xcb_configure_window(conn, win,
        XCB_CONFIG_WINDOW_STACK_MODE, above);

    xcb_set_input_focus(conn,
        XCB_INPUT_FOCUS_POINTER_ROOT,
        win, XCB_CURRENT_TIME);

    xcb_flush(conn);
}

/* Reset fokus ke window pertama yang tersedia di cur workspace */
static void
fix_focus(void)
{
    /* Coba tiling slot dulu, lalu floating */
    if (ws[cur][0]) { focus(ws[cur][0]); return; }
    if (ws[cur][1]) { focus(ws[cur][1]); return; }
    for (int i = 0; i < float_count[cur]; i++)
        if (floating[cur][i]) { focus(floating[cur][i]); return; }

    /* Workspace kosong - hapus border dari focused lama */
    if (focused && is_managed(focused))
        set_border(focused, COLOR_NORMAL);
    focused = XCB_NONE;
}

static void
remove_floating(int wsid, xcb_window_t win)
{
    for (int i = 0; i < float_count[wsid]; i++) {
        if (floating[wsid][i] != win) continue;
        /* Shift array ke kiri, hapus slot kosong di akhir */
        float_count[wsid]--;
        for (int j = i; j < float_count[wsid]; j++)
            floating[wsid][j] = floating[wsid][j + 1];
        floating[wsid][float_count[wsid]] = XCB_NONE;
        return;
    }
}

/* Promosikan floating tertua jadi tiling jika ada slot kosong */
static void
promote_floating(int wsid)
{
    if (float_count[wsid] <= 0) return;
    if (ws[wsid][0] && ws[wsid][1]) return; /* tiling sudah penuh */

    xcb_window_t promoted = floating[wsid][0];
    remove_floating(wsid, promoted);

    if (!ws[wsid][0])      ws[wsid][0] = promoted;
    else if (!ws[wsid][1]) ws[wsid][1] = promoted;
}

static void
remove_window(xcb_window_t win)
{
    for (int i = 0; i < WORKSPACES; i++) {
        if (ws[i][0] == win) {
            /* Geser slot 1 ke 0, lalu coba promosi floating */
            ws[i][0] = ws[i][1];
            ws[i][1] = XCB_NONE;
            promote_floating(i);
        } else if (ws[i][1] == win) {
            ws[i][1] = XCB_NONE;
            promote_floating(i);
        }
        remove_floating(i, win);
    }

    if (focused == win) focused = XCB_NONE;
}

/* ================================================================
 * LAYOUT
 * ================================================================ */

/* Set geometri awal untuk floating window agar tidak mewarisi
 * ukuran fullscreen dari workspace sebelumnya.
 * Posisi: centered, ukuran FLOAT_W x FLOAT_H (dari config.h). */
static void
place_floating(xcb_window_t win)
{
    uint32_t sw = scr->width_in_pixels;
    uint32_t sh = scr->height_in_pixels;

    uint32_t fw = FLOAT_W < sw ? FLOAT_W : sw / 2;
    uint32_t fh = FLOAT_H < sh ? FLOAT_H : sh / 2;

    uint32_t v[] = {
        (sw - fw) / 2,  /* x: center horizontal */
        (sh - fh) / 2,  /* y: center vertical   */
        fw,
        fh
    };

    set_border(win, COLOR_NORMAL);
    xcb_configure_window(conn, win,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, v);
}

static void
tile_two(xcb_window_t w0, xcb_window_t w1)
{
    uint32_t sw = scr->width_in_pixels;
    uint32_t sh = scr->height_in_pixels;

    if (!w0) return;

    if (!w1) {
        /* Single window: fullscreen tanpa border */
        uint32_t bw[] = { 0 };
        xcb_configure_window(conn, w0,
            XCB_CONFIG_WINDOW_BORDER_WIDTH, bw);
        uint32_t v[] = { 0, 0, sw, sh };
        xcb_configure_window(conn, w0,
            XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
            XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, v);
        return;
    }

    /* Vertical split 50/50 */
    uint32_t half = sw / 2;
    uint32_t bw2  = BORDER_WIDTH * 2;

    set_border(w0, focused == w0 ? COLOR_FOCUS : COLOR_NORMAL);
    set_border(w1, focused == w1 ? COLOR_FOCUS : COLOR_NORMAL);

    uint32_t v0[] = { 0,    0, half - bw2,      sh - bw2 };
    uint32_t v1[] = { half, 0, sw - half - bw2, sh - bw2 };

    xcb_configure_window(conn, w0,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, v0);
    xcb_configure_window(conn, w1,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y |
        XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, v1);
}

/* arrange() hanya map/unmap workspace yang berubah.
 * Memanggil tile_two untuk layout workspace aktif. */
static void
arrange(void)
{
    for (int i = 0; i < WORKSPACES; i++)
        foreach_window(i, i == cur ? mapw : unmapw);

    tile_two(ws[cur][0], ws[cur][1]);

    if (!focused || !is_managed(focused))
        fix_focus();

    xcb_flush(conn);
}

/* ================================================================
 * ACTIONS (dipanggil dari key bindings)
 * ================================================================ */

static void
spawn(const Arg *arg)
{
    if (fork() != 0) return;
    close(xcb_get_file_descriptor(conn));
    setsid();
    execvp(((char **)arg->v)[0], (char **)arg->v);
    _exit(1);
}

static void
view(const Arg *arg)
{
    if (arg->i == cur) return;
    cur    = arg->i;
    focused = XCB_NONE;
    arrange();
}

static void
move_ws(const Arg *arg)
{
    if (!focused) return;

    int target = arg->i;
    if (target == cur) return;

    xcb_window_t win = focused;
    remove_window(win);

    /* Tentukan slot tujuan: tiling 0→1, lalu floating (jika ada slot).
     * Jika masuk sebagai floating, reset geometrinya agar tidak mewarisi
     * ukuran dari workspace sebelumnya (misal: fullscreen). */
    if (!ws[target][0]) {
        ws[target][0] = win;
    } else if (!ws[target][1]) {
        ws[target][1] = win;
    } else if (float_count[target] < MAX_FLOATING) {
        floating[target][float_count[target]++] = win;
        place_floating(win);
    } else {
        /* Tidak ada slot tersedia - kembalikan ke cur sebagai floating */
        if (float_count[cur] < MAX_FLOATING) {
            floating[cur][float_count[cur]++] = win;
            place_floating(win);
        } else {
            xcb_unmap_window(conn, win); /* last resort */
        }
    }

    focused = XCB_NONE;
    arrange();
}

static void
focus_stack(const Arg *arg)
{
    xcb_window_t list[2 + MAX_FLOATING];
    int n = 0;

    if (ws[cur][0]) list[n++] = ws[cur][0];
    if (ws[cur][1]) list[n++] = ws[cur][1];
    for (int i = 0; i < float_count[cur]; i++)
        if (floating[cur][i]) list[n++] = floating[cur][i];

    if (n <= 1) return;

    int idx = 0;
    for (int i = 0; i < n; i++)
        if (list[i] == focused) { idx = i; break; }

    focus(list[(idx + arg->i + n) % n]);
    xcb_flush(conn);
}

static void
kill_client(const Arg *arg)
{
    if (!focused) return;

    xcb_client_message_event_t ev = { 0 };
    ev.response_type  = XCB_CLIENT_MESSAGE;
    ev.window         = focused;
    ev.type           = WM_PROTOCOLS;
    ev.format         = 32;
    ev.data.data32[0] = WM_DELETE_WINDOW;
    ev.data.data32[1] = XCB_CURRENT_TIME;

    xcb_send_event(conn, 0, focused,
        XCB_EVENT_MASK_NO_EVENT, (char *)&ev);
    xcb_flush(conn);
}

static void
logout(const Arg *arg)
{
    xcb_disconnect(conn);
    exit(0);
}

/* ================================================================
 * KEY BINDING SETUP
 * ================================================================ */

static void
setup_keys(void)
{
    xcb_key_symbols_t *syms = xcb_key_symbols_alloc(conn);

    for (size_t i = 0; i < LEN(keys); i++) {
        xcb_keycode_t *kc =
            xcb_key_symbols_get_keycode(syms, keys[i].keysym);
        if (!kc) continue;

        for (int j = 0; kc[j] != XCB_NO_SYMBOL; j++) {
            xcb_grab_key(conn, 1, scr->root,
                keys[i].mod, kc[j],
                XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }

        keys[i].code = kc[0];
        free(kc);
    }

    xcb_key_symbols_free(syms);
}

/* ================================================================
 * EVENT HANDLERS
 * ================================================================ */

static int
should_manage(xcb_window_t win)
{
    xcb_get_window_attributes_cookie_t ck =
        xcb_get_window_attributes(conn, win);
    xcb_get_window_attributes_reply_t *a =
        xcb_get_window_attributes_reply(conn, ck, NULL);
    if (!a) return 0;
    int r = !a->override_redirect;
    free(a);
    return r;
}

static void
handle_map(xcb_window_t win)
{
    if (!should_manage(win) || is_managed(win)) return;

    int placed = 0;

    if (is_transient(win)) {
        /* Dialog/popup → floating, centered */
        if (float_count[cur] < MAX_FLOATING) {
            floating[cur][float_count[cur]++] = win;
            place_floating(win);
            placed = 1;
        }
    } else if (!ws[cur][0]) {
        ws[cur][0] = win;
        placed = 1;
    } else if (!ws[cur][1]) {
        ws[cur][1] = win;
        placed = 1;
    } else if (float_count[cur] < MAX_FLOATING) {
        /* Tiling penuh → floating, centered */
        floating[cur][float_count[cur]++] = win;
        place_floating(win);
        placed = 1;
    }

    if (!placed) {
        /* Semua slot penuh, tolak window */
        xcb_unmap_window(conn, win);
        return;
    }

    subscribe_window(win);
    focus(win);
    arrange();
}

static void
handle_configure_request(xcb_configure_request_event_t *e)
{
    /* Window tiling dikontrol oleh arrange(), bukan oleh permintaan klien */
    int tiled = (ws[cur][0] == e->window || ws[cur][1] == e->window);

    if (tiled) {
        arrange();
        xcb_flush(conn);
        return;
    }

    /* Floating / unmanaged: hormati permintaan klien */
    uint32_t values[7];
    int i = 0;
    if (e->value_mask & XCB_CONFIG_WINDOW_X)            values[i++] = e->x;
    if (e->value_mask & XCB_CONFIG_WINDOW_Y)            values[i++] = e->y;
    if (e->value_mask & XCB_CONFIG_WINDOW_WIDTH)        values[i++] = e->width;
    if (e->value_mask & XCB_CONFIG_WINDOW_HEIGHT)       values[i++] = e->height;
    if (e->value_mask & XCB_CONFIG_WINDOW_BORDER_WIDTH) values[i++] = e->border_width;
    if (e->value_mask & XCB_CONFIG_WINDOW_SIBLING)      values[i++] = e->sibling;
    if (e->value_mask & XCB_CONFIG_WINDOW_STACK_MODE)   values[i++] = e->stack_mode;

    xcb_configure_window(conn, e->window, e->value_mask, values);
    xcb_flush(conn);
}

static void
handle_key_press(xcb_key_press_event_t *e)
{
    for (size_t i = 0; i < LEN(keys); i++) {
        if (e->detail == keys[i].code &&
            CLEANMASK(e->state) == CLEANMASK(keys[i].mod)) {
            keys[i].func(&keys[i].arg);
            return;
        }
    }
}

/* Subscribe ENTER_NOTIFY (hover focus) ke setiap window yang di-manage.
 * Dipanggil sekali saat window pertama kali di-map. */
static void
subscribe_window(xcb_window_t win)
{
    uint32_t mask[] = { XCB_EVENT_MASK_ENTER_WINDOW };
    xcb_change_window_attributes(conn, win, XCB_CW_EVENT_MASK, mask);
}

/* Cek apakah window adalah floating di workspace manapun */
static int
is_floating(xcb_window_t win)
{
    for (int i = 0; i < WORKSPACES; i++)
        for (int j = 0; j < float_count[i]; j++)
            if (floating[i][j] == win) return 1;
    return 0;
}

/* Action yang bisa dipasang ke buttons[] di config.h */
static void
mouse_drag(void)
{
    mouse.mode = MOUSE_DRAG;
}

static void
mouse_resize(void)
{
    mouse.mode = MOUSE_RESIZE;
}

/* Super + klik → dispatch dari buttons[] di config.h.
 * Hanya berlaku untuk floating window. */
static void
handle_button_press(xcb_button_press_event_t *e)
{
    xcb_window_t win = e->child;

    /* Klik tanpa modifier atau bukan floating: hanya fokus */
    if (!win || !is_floating(win)) {
        if (win && is_managed(win)) focus(win);
        return;
    }

    focus(win);

    /* Cari binding yang cocok di buttons[] */
    int matched = 0;
    for (size_t i = 0; i < LEN(buttons); i++) {
        if (e->detail == buttons[i].button &&
            CLEANMASK(e->state) == CLEANMASK(buttons[i].mod)) {

            /* Snapshot geometri window sebelum action dijalankan */
            xcb_get_geometry_reply_t *g =
                xcb_get_geometry_reply(conn, xcb_get_geometry(conn, win), NULL);
            if (!g) return;

            mouse.win     = win;
            mouse.start_x = e->root_x;
            mouse.start_y = e->root_y;
            mouse.win_x   = g->x;
            mouse.win_y   = g->y;
            mouse.win_w   = g->width;
            mouse.win_h   = g->height;
            free(g);

            buttons[i].func();
            matched = 1;
            break;
        }
    }

    if (!matched) return;

    /* Grab pointer agar MOTION_NOTIFY mengalir selama button ditekan */
    xcb_grab_pointer(conn, 0, scr->root,
        XCB_EVENT_MASK_BUTTON_RELEASE |
        XCB_EVENT_MASK_POINTER_MOTION,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
        XCB_NONE, XCB_NONE, XCB_CURRENT_TIME);

    xcb_flush(conn);
}

static void
handle_motion_notify(xcb_motion_notify_event_t *e)
{
    if (mouse.mode == MOUSE_NONE || !mouse.win) return;

    int dx = e->root_x - mouse.start_x;
    int dy = e->root_y - mouse.start_y;

    if (mouse.mode == MOUSE_DRAG) {
        uint32_t v[] = {
            (uint32_t)(mouse.win_x + dx),
            (uint32_t)(mouse.win_y + dy)
        };
        xcb_configure_window(conn, mouse.win,
            XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, v);

    } else if (mouse.mode == MOUSE_RESIZE) {
        int nw = mouse.win_w + dx;
        int nh = mouse.win_h + dy;
        if (nw < 64) nw = 64; /* ukuran minimum */
        if (nh < 64) nh = 64;
        uint32_t v[] = { (uint32_t)nw, (uint32_t)nh };
        xcb_configure_window(conn, mouse.win,
            XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, v);
    }

    xcb_flush(conn);
}

static void
handle_button_release(xcb_button_release_event_t *e)
{
    (void)e;
    mouse.mode = MOUSE_NONE;
    mouse.win  = XCB_NONE;
    xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
    xcb_flush(conn);
}

/* Focus window saat pointer masuk (hover focus).
 * Hanya aktif jika bukan sedang drag/resize. */
static void
handle_enter_notify(xcb_enter_notify_event_t *e)
{
    if (mouse.mode != MOUSE_NONE) return;
    if (e->mode != XCB_NOTIFY_MODE_NORMAL) return;
    if (!is_managed(e->event)) return;
    focus(e->event);
}

/* ================================================================
 * MAIN
 * ================================================================ */

int
main(void)
{
    conn = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(conn)) return 1;

    scr = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

    signal(SIGCHLD, SIG_IGN);

    /* Daftarkan diri sebagai WM — akan gagal jika ada WM lain */
    uint32_t mask[] = {
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY   |
        XCB_EVENT_MASK_BUTTON_PRESS          |
        XCB_EVENT_MASK_POINTER_MOTION
    };
    xcb_void_cookie_t ck = xcb_change_window_attributes_checked(
        conn, scr->root, XCB_CW_EVENT_MASK, mask);

    xcb_generic_error_t *err = xcb_request_check(conn, ck);
    if (err) {
        fprintf(stderr, "uwm: another WM is already running\n");
        free(err);
        xcb_disconnect(conn);
        return 1;
    }

    /* Intern atoms dengan null-check */
    WM_PROTOCOLS    = intern_atom("WM_PROTOCOLS");
    WM_DELETE_WINDOW = intern_atom("WM_DELETE_WINDOW");

    if (WM_PROTOCOLS == XCB_ATOM_NONE || WM_DELETE_WINDOW == XCB_ATOM_NONE) {
        fprintf(stderr, "uwm: failed to intern required atoms\n");
        xcb_disconnect(conn);
        return 1;
    }

    setup_keys();

    /* Grab semua button binding dari buttons[] di config.h */
    for (size_t i = 0; i < LEN(buttons); i++) {
        xcb_grab_button(conn, 0, scr->root,
            XCB_EVENT_MASK_BUTTON_PRESS,
            XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC,
            XCB_NONE, XCB_NONE,
            buttons[i].button, buttons[i].mod);
    }
    xcb_flush(conn);

    /* Event loop utama */
    xcb_generic_event_t *ev;
    while ((ev = xcb_wait_for_event(conn))) {
        switch (ev->response_type & ~0x80) {
        case XCB_MAP_REQUEST:
            handle_map(((xcb_map_request_event_t *)ev)->window);
            break;
        case XCB_DESTROY_NOTIFY:
            remove_window(((xcb_destroy_notify_event_t *)ev)->window);
            arrange();
            break;
        case XCB_CONFIGURE_REQUEST:
            handle_configure_request((xcb_configure_request_event_t *)ev);
            break;
        case XCB_KEY_PRESS:
            handle_key_press((xcb_key_press_event_t *)ev);
            break;
        case XCB_BUTTON_PRESS:
            handle_button_press((xcb_button_press_event_t *)ev);
            break;
        case XCB_MOTION_NOTIFY:
            handle_motion_notify((xcb_motion_notify_event_t *)ev);
            break;
        case XCB_BUTTON_RELEASE:
            handle_button_release((xcb_button_release_event_t *)ev);
            break;
        case XCB_ENTER_NOTIFY:
            handle_enter_notify((xcb_enter_notify_event_t *)ev);
            break;
        }

        free(ev);
        xcb_flush(conn);
    }

    xcb_disconnect(conn);
    return 0;
}
