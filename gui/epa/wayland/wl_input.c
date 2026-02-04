/**
 * @file wl_input.c
 * @brief Tratamento de Input (Teclado/Mouse)
 */

#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "gui/epa/wayland/wl_internal.h"

/* ============================================================================
 * POINTER LISTENERS
 * ============================================================================
 */

static void pointer_enter(void *data, struct wl_pointer *pointer,
			  uint32_t serial, struct wl_surface *surface,
			  wl_fixed_t sx, wl_fixed_t sy)
{
	(void)pointer;
	(void)serial;
	(void)surface;

	struct bhs_platform_impl *p = data;
	p->last_pointer_serial = serial;

	/* Invariante: Suporta apenas uma janela focada por vez (Single Window App) */
	if (p->focused_window) {
		p->focused_window->mouse_x = wl_fixed_to_int(sx);
		p->focused_window->mouse_y = wl_fixed_to_int(sy);
	}
}

static void pointer_leave(void *data, struct wl_pointer *pointer,
			  uint32_t serial, struct wl_surface *surface)
{
	(void)data;
	(void)pointer;
	(void)serial;
	(void)surface;
}

static void pointer_motion(void *data, struct wl_pointer *pointer,
			   uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
	(void)pointer;
	(void)time;

	struct bhs_platform_impl *p = data;
	if (!p->focused_window)
		return;

	struct bhs_window_impl *win = p->focused_window;
	int32_t new_x = wl_fixed_to_int(sx);
	int32_t new_y = wl_fixed_to_int(sy);

	struct bhs_event ev = { 0 };
	ev.type = BHS_EVENT_MOUSE_MOVE;
	ev.timestamp_ns = bhs_wl_timestamp_ns();
	ev.mouse_move.x = new_x;
	ev.mouse_move.y = new_y;
	ev.mouse_move.dx = new_x - win->mouse_x;
	ev.mouse_move.dy = new_y - win->mouse_y;

	win->mouse_x = new_x;
	win->mouse_y = new_y;
	bhs_wl_push_event(win, &ev);
}

static void pointer_button(void *data, struct wl_pointer *pointer,
			   uint32_t serial, uint32_t time, uint32_t button,
			   uint32_t state)
{
	(void)pointer;
	(void)serial;
	(void)time;
	struct bhs_platform_impl *p = data;
	if (!p->focused_window)
		return;

	struct bhs_window_impl *win = p->focused_window;
	struct bhs_event ev = { 0 };
	ev.type = (state == WL_POINTER_BUTTON_STATE_PRESSED)
			  ? BHS_EVENT_MOUSE_DOWN
			  : BHS_EVENT_MOUSE_UP;
	ev.timestamp_ns = bhs_wl_timestamp_ns();
	ev.mouse_button.x = win->mouse_x;
	ev.mouse_button.y = win->mouse_y;
	ev.mouse_button.click_count = 1;

	switch (button) {
	case 0x110:
		ev.mouse_button.button = BHS_MOUSE_LEFT;
		break;
	case 0x111:
		ev.mouse_button.button = BHS_MOUSE_RIGHT;
		break;
	case 0x112:
		ev.mouse_button.button = BHS_MOUSE_MIDDLE;
		break;
	default:
		ev.mouse_button.button = BHS_MOUSE_LEFT;
		break;
	}
	bhs_wl_push_event(win, &ev);
}

static void pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time,
			 uint32_t axis, wl_fixed_t value)
{
	(void)pointer;
	(void)time;
	struct bhs_platform_impl *p = data;
	if (!p->focused_window)
		return;

	struct bhs_window_impl *win = p->focused_window;
	struct bhs_event ev = { 0 };
	ev.type = BHS_EVENT_MOUSE_SCROLL;
	ev.timestamp_ns = bhs_wl_timestamp_ns();
	ev.scroll.x = win->mouse_x;
	ev.scroll.y = win->mouse_y;

	float val = wl_fixed_to_double(value) / 10.0f;
	if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
		ev.scroll.dy = -val;
	else
		ev.scroll.dx = val;

	bhs_wl_push_event(win, &ev);
}

static void pointer_frame(void *data, struct wl_pointer *pointer)
{
	(void)data;
	(void)pointer;
}
static void pointer_axis_source(void *d, struct wl_pointer *p, uint32_t a)
{
	(void)d;
	(void)p;
	(void)a;
}
static void pointer_axis_stop(void *d, struct wl_pointer *p, uint32_t t,
			      uint32_t a)
{
	(void)d;
	(void)p;
	(void)t;
	(void)a;
}
static void pointer_axis_discrete(void *d, struct wl_pointer *p, uint32_t a,
				  int32_t v)
{
	(void)d;
	(void)p;
	(void)a;
	(void)v;
}

static const struct wl_pointer_listener pointer_listener = {
	.enter = pointer_enter,
	.leave = pointer_leave,
	.motion = pointer_motion,
	.button = pointer_button,
	.axis = pointer_axis,
	.frame = pointer_frame,
	.axis_source = pointer_axis_source,
	.axis_stop = pointer_axis_stop,
	.axis_discrete = pointer_axis_discrete,
};

/* ============================================================================
 * KEYBOARD LISTENERS
 * ============================================================================
 */

static void keyboard_keymap(void *data, struct wl_keyboard *keyboard,
			    uint32_t format, int32_t fd, uint32_t size)
{
	(void)keyboard;
	struct bhs_platform_impl *p = data;

	if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
		close(fd);
		return;
	}

	char *map_str = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (map_str == MAP_FAILED) {
		close(fd);
		return;
	}

	if (p->xkb_keymap)
		xkb_keymap_unref(p->xkb_keymap);
	if (p->xkb_state)
		xkb_state_unref(p->xkb_state);

	p->xkb_keymap = xkb_keymap_new_from_string(p->xkb_ctx, map_str,
						   XKB_KEYMAP_FORMAT_TEXT_V1,
						   XKB_KEYMAP_COMPILE_NO_FLAGS);
	munmap(map_str, size);
	close(fd);

	if (p->xkb_keymap)
		p->xkb_state = xkb_state_new(p->xkb_keymap);
}

static void keyboard_enter(void *data, struct wl_keyboard *keyboard,
			   uint32_t serial, struct wl_surface *surface,
			   struct wl_array *keys)
{
	(void)keyboard;
	(void)serial;
	(void)surface;
	(void)keys;
	struct bhs_platform_impl *p = data;
	if (p->focused_window) {
		struct bhs_event ev = { 0 };
		ev.type = BHS_EVENT_WINDOW_FOCUS;
		ev.timestamp_ns = bhs_wl_timestamp_ns();
		bhs_wl_push_event(p->focused_window, &ev);
	}
}

static void keyboard_leave(void *data, struct wl_keyboard *keyboard,
			   uint32_t serial, struct wl_surface *surface)
{
	(void)keyboard;
	(void)serial;
	(void)surface;
	struct bhs_platform_impl *p = data;
	if (p->focused_window) {
		struct bhs_event ev = { 0 };
		ev.type = BHS_EVENT_WINDOW_BLUR;
		ev.timestamp_ns = bhs_wl_timestamp_ns();
		bhs_wl_push_event(p->focused_window, &ev);
	}
}

static void keyboard_key(void *data, struct wl_keyboard *keyboard,
			 uint32_t serial, uint32_t time, uint32_t key,
			 uint32_t state)
{
	(void)keyboard;
	(void)serial;
	(void)time;
	struct bhs_platform_impl *p = data;
	if (!p->focused_window || !p->xkb_state)
		return;

	struct bhs_window_impl *win = p->focused_window;
	struct bhs_event ev = { 0 };
	ev.type = (state == WL_KEYBOARD_KEY_STATE_PRESSED) ? BHS_EVENT_KEY_DOWN
							   : BHS_EVENT_KEY_UP;
	ev.timestamp_ns = bhs_wl_timestamp_ns();
	ev.key.scancode = key;
	ev.key.keycode = key + 8;

	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		xkb_keysym_t sym =
			xkb_state_key_get_one_sym(p->xkb_state, key + 8);
		xkb_keysym_to_utf8(sym, ev.key.text, sizeof(ev.key.text));
	}

	bhs_wl_push_event(win, &ev);
}

static void keyboard_modifiers(void *data, struct wl_keyboard *k, uint32_t s,
			       uint32_t dep, uint32_t lat, uint32_t loc,
			       uint32_t grp)
{
	(void)k;
	(void)s;
	struct bhs_platform_impl *p = data;
	if (p->xkb_state)
		xkb_state_update_mask(p->xkb_state, dep, lat, loc, 0, 0, grp);
}

static void keyboard_repeat_info(void *d, struct wl_keyboard *k, int32_t r,
				 int32_t dl)
{
	(void)d;
	(void)k;
	(void)r;
	(void)dl;
}

static const struct wl_keyboard_listener keyboard_listener = {
	.keymap = keyboard_keymap,
	.enter = keyboard_enter,
	.leave = keyboard_leave,
	.key = keyboard_key,
	.modifiers = keyboard_modifiers,
	.repeat_info = keyboard_repeat_info,
};

/* ============================================================================
 * SEAT INIT
 * ============================================================================
 */

static void seat_capabilities(void *data, struct wl_seat *seat,
			      uint32_t capabilities)
{
	struct bhs_platform_impl *p = data;

	if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !p->pointer) {
		p->pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(p->pointer, &pointer_listener, p);
	} else if (!(capabilities & WL_SEAT_CAPABILITY_POINTER) && p->pointer) {
		wl_pointer_destroy(p->pointer);
		p->pointer = NULL;
	}

	if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !p->keyboard) {
		p->keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(p->keyboard, &keyboard_listener, p);
	} else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) &&
		   p->keyboard) {
		wl_keyboard_destroy(p->keyboard);
		p->keyboard = NULL;
	}
}

static void seat_name(void *data, struct wl_seat *seat, const char *name)
{
	(void)data;
	(void)seat;
	(void)name;
}

static const struct wl_seat_listener seat_listener = {
	.capabilities = seat_capabilities,
	.name = seat_name,
};

void bhs_wl_init_seat_listeners(struct bhs_platform_impl *p)
{
	if (p->seat) {
		wl_seat_add_listener(p->seat, &seat_listener, p);
	}
}

/* ============================================================================
 * APIs PÚBLICAS DE INPUT
 * ============================================================================
 */

static const char *cursor_name_from_enum(enum bhs_cursor_shape shape)
{
	switch (shape) {
	case BHS_CURSOR_DEFAULT:
		return "left_ptr";
	case BHS_CURSOR_TEXT:
		return "xterm"; // ou "text"
	case BHS_CURSOR_POINTER:
		return "hand1"; // ou "pointer"
	case BHS_CURSOR_CROSSHAIR:
		return "crosshair";
	case BHS_CURSOR_RESIZE_H:
		return "sb_h_double_arrow";
	case BHS_CURSOR_RESIZE_V:
		return "sb_v_double_arrow";
	case BHS_CURSOR_RESIZE_NWSE:
		return "fd_double_arrow";
	case BHS_CURSOR_RESIZE_NESW:
		return "bd_double_arrow";
	case BHS_CURSOR_GRAB:
		return "hand1";
	case BHS_CURSOR_GRABBING:
		return "grabbing";
	default:
		return "left_ptr";
	}
}

void bhs_window_set_cursor(bhs_window_t window, enum bhs_cursor_shape shape)
{
	if (!window)
		return;
	struct bhs_platform_impl *p = window->platform;

	if (shape == BHS_CURSOR_HIDDEN) {
		if (p->pointer) {
			wl_pointer_set_cursor(
				p->pointer, p->last_pointer_serial, NULL, 0, 0);
		}
		return;
	}

	if (!p->cursor_theme || !p->pointer)
		return;

	const char *name = cursor_name_from_enum(shape);
	struct wl_cursor *cursor =
		wl_cursor_theme_get_cursor(p->cursor_theme, name);
	if (!cursor)
		return;

	struct wl_cursor_image *image = cursor->images[0];
	struct wl_buffer *buffer = wl_cursor_image_get_buffer(image);

	if (!buffer)
		return;

	wl_pointer_set_cursor(p->pointer, p->last_pointer_serial,
			      p->cursor_surface, image->hotspot_x,
			      image->hotspot_y);

	wl_surface_attach(p->cursor_surface, buffer, 0, 0);
	wl_surface_damage(p->cursor_surface, 0, 0, image->width, image->height);
	wl_surface_commit(p->cursor_surface);
}

/* ============================================================================
 * RELATIVE POINTER LISTENER
 * ============================================================================
 */

static void relative_pointer_motion(void *data,
				    struct zwp_relative_pointer_v1 *pointer,
				    uint32_t time_hi, uint32_t time_lo,
				    wl_fixed_t dx, wl_fixed_t dy,
				    wl_fixed_t dx_unaccel,
				    wl_fixed_t dy_unaccel)
{
	(void)pointer;
	(void)time_hi;
	(void)time_lo;
	(void)dx_unaccel;
	(void)dy_unaccel;
	(void)dx;
	(void)dy;

	struct bhs_window_impl *win = data;
	if (!win)
		return;

	struct bhs_event ev = { 0 };
	ev.type = BHS_EVENT_MOUSE_MOVE;
	ev.timestamp_ns = bhs_wl_timestamp_ns();

	/* Relative motion deltas - use unaccel para consistência em jogos */
	ev.mouse_move.dx = wl_fixed_to_double(dx_unaccel);
	ev.mouse_move.dy = wl_fixed_to_double(dy_unaccel);
	ev.mouse_move.x = win->mouse_x; /* Posição pode não mudar em lock */
	ev.mouse_move.y = win->mouse_y;

	bhs_wl_push_event(win, &ev);
}

static const struct zwp_relative_pointer_v1_listener
	relative_pointer_listener = {
		.relative_motion = relative_pointer_motion,
	};

/* ============================================================================
 * MOUSE LOCK API
 * ============================================================================
 */

void bhs_window_set_mouse_lock(bhs_window_t window, bool locked)
{
	if (!window)
		return;

	struct bhs_window_impl *win = window;
	struct bhs_platform_impl *p = win->platform;

	if (locked && !win->mouse_locked) {
		/* Ativa lock */
		if (!p->pointer_constraints || !p->relative_pointer_manager) {
			BHS_WL_LOG("aviso: pointer constraints não disponível "
				   "no compositor");
			return;
		}

		/* Cria locked pointer */
		win->locked_pointer = zwp_pointer_constraints_v1_lock_pointer(
			p->pointer_constraints, win->surface, p->pointer, NULL,
			ZWP_POINTER_CONSTRAINTS_V1_LIFETIME_PERSISTENT);

		/* Cria relative pointer para receber motion events */
		win->relative_pointer =
			zwp_relative_pointer_manager_v1_get_relative_pointer(
				p->relative_pointer_manager, p->pointer);
		zwp_relative_pointer_v1_add_listener(
			win->relative_pointer, &relative_pointer_listener, win);

		/* Esconde cursor */
		wl_pointer_set_cursor(p->pointer, p->last_pointer_serial, NULL,
				      0, 0);

		win->mouse_locked = true;
	} else if (!locked && win->mouse_locked) {
		/* Desativa lock */
		if (win->relative_pointer) {
			zwp_relative_pointer_v1_destroy(win->relative_pointer);
			win->relative_pointer = NULL;
		}
		if (win->locked_pointer) {
			zwp_locked_pointer_v1_destroy(win->locked_pointer);
			win->locked_pointer = NULL;
		}

		/* Restaura cursor */
		bhs_window_set_cursor(window, BHS_CURSOR_DEFAULT);

		win->mouse_locked = false;
	}
}
