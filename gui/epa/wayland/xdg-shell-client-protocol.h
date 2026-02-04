/*
 * xdg-shell-client-protocol.h
 *
 * Gerado a partir do protocolo xdg-shell do wayland-protocols.
 * Normalmente você geraria isso com wayland-scanner, mas vou
 * incluir diretamente pra não depender de ferramentas externas.
 *
 * Este é um subset mínimo necessário para criar janelas toplevel.
 * O protocolo completo tem mais coisas (popups, positioners, etc).
 */

#ifndef XDG_SHELL_CLIENT_PROTOCOL_H
#define XDG_SHELL_CLIENT_PROTOCOL_H

#include <stdint.h>
#include <wayland-client.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * XDG_WM_BASE
 * ============================================================================
 */

extern const struct wl_interface xdg_wm_base_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_toplevel_interface;
extern const struct wl_interface xdg_positioner_interface;

struct xdg_wm_base;
struct xdg_surface;
struct xdg_toplevel;
struct xdg_positioner;

struct xdg_wm_base_listener {
	void (*ping)(void *data, struct xdg_wm_base *xdg_wm_base,
		     uint32_t serial);
};

static inline int
xdg_wm_base_add_listener(struct xdg_wm_base *xdg_wm_base,
			 const struct xdg_wm_base_listener *listener,
			 void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *)xdg_wm_base,
				     (void (**)(void))listener, data);
}

#define XDG_WM_BASE_DESTROY 0
#define XDG_WM_BASE_CREATE_POSITIONER 1
#define XDG_WM_BASE_GET_XDG_SURFACE 2
#define XDG_WM_BASE_PONG 3

static inline void xdg_wm_base_destroy(struct xdg_wm_base *xdg_wm_base)
{
	wl_proxy_marshal((struct wl_proxy *)xdg_wm_base, XDG_WM_BASE_DESTROY);
	wl_proxy_destroy((struct wl_proxy *)xdg_wm_base);
}

static inline struct xdg_surface *
xdg_wm_base_get_xdg_surface(struct xdg_wm_base *xdg_wm_base,
			    struct wl_surface *surface)
{
	struct wl_proxy *id;
	id = wl_proxy_marshal_constructor(
		(struct wl_proxy *)xdg_wm_base, XDG_WM_BASE_GET_XDG_SURFACE,
		&xdg_surface_interface, NULL, surface);
	return (struct xdg_surface *)id;
}

static inline void xdg_wm_base_pong(struct xdg_wm_base *xdg_wm_base,
				    uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *)xdg_wm_base, XDG_WM_BASE_PONG,
			 serial);
}

/* ============================================================================
 * XDG_SURFACE
 * ============================================================================
 */

struct xdg_surface_listener {
	void (*configure)(void *data, struct xdg_surface *xdg_surface,
			  uint32_t serial);
};

static inline int
xdg_surface_add_listener(struct xdg_surface *xdg_surface,
			 const struct xdg_surface_listener *listener,
			 void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *)xdg_surface,
				     (void (**)(void))listener, data);
}

#define XDG_SURFACE_DESTROY 0
#define XDG_SURFACE_GET_TOPLEVEL 1
#define XDG_SURFACE_GET_POPUP 2
#define XDG_SURFACE_SET_WINDOW_GEOMETRY 3
#define XDG_SURFACE_ACK_CONFIGURE 4

static inline void xdg_surface_destroy(struct xdg_surface *xdg_surface)
{
	wl_proxy_marshal((struct wl_proxy *)xdg_surface, XDG_SURFACE_DESTROY);
	wl_proxy_destroy((struct wl_proxy *)xdg_surface);
}

static inline struct xdg_toplevel *
xdg_surface_get_toplevel(struct xdg_surface *xdg_surface)
{
	struct wl_proxy *id;
	id = wl_proxy_marshal_constructor((struct wl_proxy *)xdg_surface,
					  XDG_SURFACE_GET_TOPLEVEL,
					  &xdg_toplevel_interface, NULL);
	return (struct xdg_toplevel *)id;
}

static inline void xdg_surface_ack_configure(struct xdg_surface *xdg_surface,
					     uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *)xdg_surface,
			 XDG_SURFACE_ACK_CONFIGURE, serial);
}

/* ============================================================================
 * XDG_TOPLEVEL
 * ============================================================================
 */

struct xdg_toplevel_listener {
	void (*configure)(void *data, struct xdg_toplevel *xdg_toplevel,
			  int32_t width, int32_t height,
			  struct wl_array *states);
	void (*close)(void *data, struct xdg_toplevel *xdg_toplevel);
	void (*configure_bounds)(void *data, struct xdg_toplevel *xdg_toplevel,
				 int32_t width, int32_t height);
	void (*wm_capabilities)(void *data, struct xdg_toplevel *xdg_toplevel,
				struct wl_array *capabilities);
};

static inline int
xdg_toplevel_add_listener(struct xdg_toplevel *xdg_toplevel,
			  const struct xdg_toplevel_listener *listener,
			  void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *)xdg_toplevel,
				     (void (**)(void))listener, data);
}

#define XDG_TOPLEVEL_DESTROY 0
#define XDG_TOPLEVEL_SET_PARENT 1
#define XDG_TOPLEVEL_SET_TITLE 2
#define XDG_TOPLEVEL_SET_APP_ID 3
#define XDG_TOPLEVEL_SHOW_WINDOW_MENU 4
#define XDG_TOPLEVEL_MOVE 5
#define XDG_TOPLEVEL_RESIZE 6
#define XDG_TOPLEVEL_SET_MAX_SIZE 7
#define XDG_TOPLEVEL_SET_MIN_SIZE 8
#define XDG_TOPLEVEL_SET_MAXIMIZED 9
#define XDG_TOPLEVEL_UNSET_MAXIMIZED 10
#define XDG_TOPLEVEL_SET_FULLSCREEN 11
#define XDG_TOPLEVEL_UNSET_FULLSCREEN 12
#define XDG_TOPLEVEL_SET_MINIMIZED 13

static inline void xdg_toplevel_destroy(struct xdg_toplevel *xdg_toplevel)
{
	wl_proxy_marshal((struct wl_proxy *)xdg_toplevel, XDG_TOPLEVEL_DESTROY);
	wl_proxy_destroy((struct wl_proxy *)xdg_toplevel);
}

static inline void xdg_toplevel_set_title(struct xdg_toplevel *xdg_toplevel,
					  const char *title)
{
	wl_proxy_marshal((struct wl_proxy *)xdg_toplevel,
			 XDG_TOPLEVEL_SET_TITLE, title);
}

static inline void xdg_toplevel_set_app_id(struct xdg_toplevel *xdg_toplevel,
					   const char *app_id)
{
	wl_proxy_marshal((struct wl_proxy *)xdg_toplevel,
			 XDG_TOPLEVEL_SET_APP_ID, app_id);
}

#ifdef __cplusplus
}
#endif

#endif /* XDG_SHELL_CLIENT_PROTOCOL_H */
