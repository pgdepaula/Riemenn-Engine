/**
 * @file wl_internal.h
 * @brief Definições internas do backend Wayland
 *
 * "Se não tá aqui, não é interno." - Linus (provavelmente)
 */

#ifndef BHS_UX_PLATFORM_WAYLAND_INTERNAL_H
#define BHS_UX_PLATFORM_WAYLAND_INTERNAL_H

#define _POSIX_C_SOURCE 200809L

#include "gui/epa/epa.h"

#include <stdio.h>
#include <time.h>
#include <wayland-client.h>
#include <wayland-cursor.h>
#include <xkbcommon/xkbcommon.h>
#include "pointer-constraints-client-protocol.h"
#include "relative-pointer-client-protocol.h"
#include "xdg-shell-client-protocol.h"

/* ============================================================================
 * MACROS & CONSTANTES
 * ============================================================================
 */

#define BHS_EVENT_QUEUE_MAX 1024

#define BHS_WL_LOG(fmt, ...)                                                   \
	fprintf(stderr, "[wayland] " fmt "\n", ##__VA_ARGS__)

/* ============================================================================
 * ESTRUTURAS
 * ============================================================================
 */

struct bhs_event_queue {
	struct bhs_event events[BHS_EVENT_QUEUE_MAX];
	uint32_t head;
	uint32_t tail;
	uint32_t count;
};

struct bhs_platform_impl {
	/* Conexão */
	struct wl_display *display;
	struct wl_registry *registry;

	/* Globals */
	struct wl_compositor *compositor;
	struct xdg_wm_base *xdg_wm_base;
	struct wl_seat *seat;
	struct wl_shm *shm;

	/* Pointer Constraints & Relative Pointer */
	struct zwp_pointer_constraints_v1 *pointer_constraints;
	struct zwp_relative_pointer_manager_v1 *relative_pointer_manager;

	/* Input */
	struct wl_pointer *pointer;
	struct wl_keyboard *keyboard;

	/* XKB */
	struct xkb_context *xkb_ctx;
	struct xkb_keymap *xkb_keymap;
	struct xkb_state *xkb_state;

	/* Cursors */
	struct wl_cursor_theme *cursor_theme;
	struct wl_surface *cursor_surface;
	struct wl_cursor
		*cursors[BHS_CURSOR_HIDDEN + 1]; /* Cache de cursores */

	/* Estado */
	bool initialized;
	bool should_quit;
	uint32_t last_pointer_serial;
	struct bhs_window_impl *focused_window;
};

struct bhs_window_impl {
	struct bhs_platform_impl *platform;

	struct wl_surface *surface;
	struct xdg_surface *xdg_surface;
	struct xdg_toplevel *xdg_toplevel;

	bool should_close;
	bool configured;
	int32_t width;
	int32_t height;
	int32_t pending_width;
	int32_t pending_height;

	struct bhs_event_queue events;
	bhs_event_callback_fn event_callback;
	void *callback_userdata;

	/* Mouse tracking para deltas */
	int32_t mouse_x;
	int32_t mouse_y;

	/* Mouse Lock (Pointer Constraints) */
	struct zwp_locked_pointer_v1 *locked_pointer;
	struct zwp_relative_pointer_v1 *relative_pointer;
	bool mouse_locked;
};

/* ============================================================================
 * PROTÓTIPOS INTERNOS COMPARTILHADOS
 * ============================================================================
 */

/* wl_window.c */
void bhs_wl_push_event(struct bhs_window_impl *win, const struct bhs_event *ev);
uint64_t bhs_wl_timestamp_ns(void);

#endif /* BHS_UX_PLATFORM_WAYLAND_INTERNAL_H */
