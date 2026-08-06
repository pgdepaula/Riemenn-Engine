/**
 * @file wl_window.c
 * @brief Gerenciamento de Janelas (XDG Shell)
 */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "engine/platform/wayland/wl_internal.h"

/* ============================================================================
 * HELPERS DE TEMPO E EVENTOS
 * ============================================================================
 */

uint64_t ri_wl_timestamp_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void ri_wl_push_event(struct ri_window_impl *win, const struct ri_event *ev)
{
	if (!win || !ev)
		return;

	/* Callback direto se existir */
	if (win->event_callback) {
		win->event_callback((ri_window_t)win, ev,
				    win->callback_userdata);
	}

	/* Fila circular */
	struct ri_event_queue *q = &win->events;
	if (q->count < RI_EVENT_QUEUE_MAX) {
		q->events[q->tail] = *ev;
		q->tail = (q->tail + 1) % RI_EVENT_QUEUE_MAX;
		q->count++;
	}
}

static bool ri_wl_pop_event(struct ri_window_impl *win, struct ri_event *ev)
{
	if (!win || !ev)
		return false;

	struct ri_event_queue *q = &win->events;
	if (q->count == 0)
		return false;

	*ev = q->events[q->head];
	q->head = (q->head + 1) % RI_EVENT_QUEUE_MAX;
	q->count--;
	return true;
}

/* ============================================================================
 * XDG LISTENERS
 * ============================================================================
 */

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface,
				  uint32_t serial)
{
	struct ri_window_impl *win = data;

	if (win->pending_width > 0 && win->pending_height > 0) {
		win->width = win->pending_width;
		win->height = win->pending_height;
	}

	win->configured = true;
	xdg_surface_ack_configure(xdg_surface, serial);

	struct ri_event ev = { 0 };
	ev.type = RI_EVENT_WINDOW_RESIZE;
	ev.timestamp_ns = ri_wl_timestamp_ns();
	ev.resize.width = win->width;
	ev.resize.height = win->height;
	ri_wl_push_event(win, &ev);
}

static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_configure,
};

static void xdg_toplevel_configure(void *data,
				   struct xdg_toplevel *xdg_toplevel,
				   int32_t width, int32_t height,
				   struct wl_array *states)
{
	(void)xdg_toplevel;
	(void)states;
	struct ri_window_impl *win = data;

	if (width > 0 && height > 0) {
		win->pending_width = width;
		win->pending_height = height;
	}
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *xdg_toplevel)
{
	(void)xdg_toplevel;
	struct ri_window_impl *win = data;
	win->should_close = true;

	struct ri_event ev = { 0 };
	ev.type = RI_EVENT_WINDOW_CLOSE;
	ev.timestamp_ns = ri_wl_timestamp_ns();
	ri_wl_push_event(win, &ev);
}

static void xdg_toplevel_configure_bounds(void *data,
					  struct xdg_toplevel *xdg_toplevel,
					  int32_t width, int32_t height)
{
	(void)data;
	(void)xdg_toplevel;
	(void)width;
	(void)height;
}

static void xdg_toplevel_wm_capabilities(void *data,
					 struct xdg_toplevel *xdg_toplevel,
					 struct wl_array *capabilities)
{
	(void)data;
	(void)xdg_toplevel;
	(void)capabilities;
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
	.configure = xdg_toplevel_configure,
	.close = xdg_toplevel_close,
	.configure_bounds = xdg_toplevel_configure_bounds,
	.wm_capabilities = xdg_toplevel_wm_capabilities,
};

/* ============================================================================
 * PUBLIC API
 * ============================================================================
 */

int ri_window_create(ri_platform_t platform,
		      const struct ri_window_config *config,
		      ri_window_t *window)
{
	if (!platform || !config || !window)
		return RI_PLATFORM_ERR_INVALID;

	struct ri_platform_impl *p = platform;
	struct ri_window_impl *win = calloc(1, sizeof(*win));
	if (!win)
		return RI_PLATFORM_ERR_NOMEM;

	win->platform = p;
	win->width = config->width;
	win->height = config->height;

	/* Cria Surface */
	win->surface = wl_compositor_create_surface(p->compositor);
	if (!win->surface) {
		free(win);
		return RI_PLATFORM_ERR_WINDOW;
	}

	/* Cria XDG Surface */
	win->xdg_surface =
		xdg_wm_base_get_xdg_surface(p->xdg_wm_base, win->surface);
	xdg_surface_add_listener(win->xdg_surface, &xdg_surface_listener, win);

	win->xdg_toplevel = xdg_surface_get_toplevel(win->xdg_surface);
	xdg_toplevel_add_listener(win->xdg_toplevel, &xdg_toplevel_listener,
				  win);

	xdg_toplevel_set_title(win->xdg_toplevel, config->title);
	xdg_toplevel_set_app_id(win->xdg_toplevel,
				"ri_sim"); /* Deveria ser config? */

	wl_surface_commit(win->surface);

	/* Espera configure inicial */
	wl_display_roundtrip(p->display);

	/* Define como janela focada por padrão se for a única */
	if (p->focused_window == NULL) {
		p->focused_window = win;
	}

	*window = win;
	return RI_PLATFORM_OK;
}

void ri_window_destroy(ri_window_t window)
{
	if (!window)
		return;

	struct ri_window_impl *win = window;

	if (win->platform->focused_window == win)
		win->platform->focused_window = NULL;

	if (win->xdg_toplevel)
		xdg_toplevel_destroy(win->xdg_toplevel);
	if (win->xdg_surface)
		xdg_surface_destroy(win->xdg_surface);
	if (win->surface)
		wl_surface_destroy(win->surface);

	free(win);
}

void *ri_window_get_native_handle(ri_window_t window)
{
	if (!window)
		return NULL;
	return window->surface;
}

void *ri_window_get_native_layer(ri_window_t window)
{
	(void)window;
	return NULL; /* Wayland: Renderização direta via EGL/Vulkan Surface. Layer not
                  required. */
}

bool ri_window_should_close(ri_window_t window)
{
	if (!window)
		return true;
	return window->should_close;
}

void ri_window_get_size(ri_window_t window, int32_t *width, int32_t *height)
{
	if (!window)
		return;
	if (width)
		*width = window->width;
	if (height)
		*height = window->height;
}

bool ri_window_next_event(ri_window_t window, struct ri_event *event)
{
	return ri_wl_pop_event(window, event);
}

void ri_window_set_event_callback(ri_window_t window,
				   ri_event_callback_fn callback,
				   void *userdata)
{
	if (!window)
		return;
	struct ri_window_impl *win = window;
	win->event_callback = callback;
	win->callback_userdata = userdata;
}
