/*
 * xdg-shell-protocol.c
 *
 * Definições das interfaces do protocolo xdg-shell.
 * Normalmente gerado por wayland-scanner, mas incluído diretamente aqui.
 */

#include <stdint.h>
#include <stdlib.h>
#include <wayland-util.h>

/* Forward declarations (em wayland-client) */
extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface wl_output_interface;
extern const struct wl_interface wl_seat_interface;

/* Nossa própria forward declaration */
extern const struct wl_interface xdg_wm_base_interface;
extern const struct wl_interface xdg_positioner_interface;
extern const struct wl_interface xdg_surface_interface;
extern const struct wl_interface xdg_toplevel_interface;
extern const struct wl_interface xdg_popup_interface;

/* ============================================================================
 * XDG_WM_BASE
 * ============================================================================
 */

static const struct wl_interface *xdg_wm_base_requests_types[] = {
	NULL,			   /* destroy */
	&xdg_positioner_interface, /* create_positioner */
	&xdg_surface_interface,	   /* get_xdg_surface */
	&wl_surface_interface,
	NULL, /* pong */
};

static const struct wl_message xdg_wm_base_requests[] = {
	{ "destroy", "", xdg_wm_base_requests_types + 0 },
	{ "create_positioner", "n", xdg_wm_base_requests_types + 1 },
	{ "get_xdg_surface", "no", xdg_wm_base_requests_types + 2 },
	{ "pong", "u", xdg_wm_base_requests_types + 4 },
};

static const struct wl_message xdg_wm_base_events[] = {
	{ "ping", "u", xdg_wm_base_requests_types + 0 },
};

const struct wl_interface xdg_wm_base_interface = {
	"xdg_wm_base", 5, 4, xdg_wm_base_requests, 1, xdg_wm_base_events,
};

/* ============================================================================
 * XDG_POSITIONER (stub - não usamos, mas precisa existir)
 * ============================================================================
 */

static const struct wl_message xdg_positioner_requests[] = {
	{ "destroy", "", NULL },
	{ "set_size", "ii", NULL },
	{ "set_anchor_rect", "iiii", NULL },
	{ "set_anchor", "u", NULL },
	{ "set_gravity", "u", NULL },
	{ "set_constraint_adjustment", "u", NULL },
	{ "set_offset", "ii", NULL },
	{ "set_reactive", "", NULL },
	{ "set_parent_size", "ii", NULL },
	{ "set_parent_configure", "u", NULL },
};

const struct wl_interface xdg_positioner_interface = {
	"xdg_positioner", 5, 10, xdg_positioner_requests, 0, NULL,
};

/* ============================================================================
 * XDG_SURFACE
 * ============================================================================
 */

static const struct wl_interface *xdg_surface_requests_types[] = {
	NULL,			 /* destroy */
	&xdg_toplevel_interface, /* get_toplevel */
	&xdg_popup_interface,	 /* get_popup */
	&xdg_surface_interface,
	&xdg_positioner_interface,
	NULL,
	NULL,
	NULL,
	NULL, /* set_window_geometry */
	NULL, /* ack_configure */
};

static const struct wl_message xdg_surface_requests[] = {
	{ "destroy", "", xdg_surface_requests_types + 0 },
	{ "get_toplevel", "n", xdg_surface_requests_types + 1 },
	{ "get_popup", "n?oo", xdg_surface_requests_types + 2 },
	{ "set_window_geometry", "iiii", xdg_surface_requests_types + 5 },
	{ "ack_configure", "u", xdg_surface_requests_types + 9 },
};

static const struct wl_message xdg_surface_events[] = {
	{ "configure", "u", xdg_surface_requests_types + 0 },
};

const struct wl_interface xdg_surface_interface = {
	"xdg_surface", 5, 5, xdg_surface_requests, 1, xdg_surface_events,
};

/* ============================================================================
 * XDG_TOPLEVEL
 * ============================================================================
 */

static const struct wl_interface *xdg_toplevel_requests_types[] = {
	NULL,			 /* destroy */
	&xdg_toplevel_interface, /* set_parent */
	NULL,			 /* set_title */
	NULL,			 /* set_app_id */
	&wl_seat_interface,	 /* show_window_menu */
	NULL,
	NULL,
	&wl_seat_interface, /* move */
	NULL,
	&wl_seat_interface, /* resize */
	NULL,
	NULL,
	NULL,
	NULL, /* set_max_size */
	NULL,
	NULL,		      /* set_min_size */
	NULL,		      /* set_maximized */
	NULL,		      /* unset_maximized */
	&wl_output_interface, /* set_fullscreen */
	NULL,		      /* unset_fullscreen */
	NULL,		      /* set_minimized */
};

static const struct wl_message xdg_toplevel_requests[] = {
	{ "destroy", "", xdg_toplevel_requests_types + 0 },
	{ "set_parent", "?o", xdg_toplevel_requests_types + 1 },
	{ "set_title", "s", xdg_toplevel_requests_types + 2 },
	{ "set_app_id", "s", xdg_toplevel_requests_types + 3 },
	{ "show_window_menu", "ouii", xdg_toplevel_requests_types + 4 },
	{ "move", "ou", xdg_toplevel_requests_types + 7 },
	{ "resize", "ouu", xdg_toplevel_requests_types + 9 },
	{ "set_max_size", "ii", xdg_toplevel_requests_types + 12 },
	{ "set_min_size", "ii", xdg_toplevel_requests_types + 14 },
	{ "set_maximized", "", xdg_toplevel_requests_types + 16 },
	{ "unset_maximized", "", xdg_toplevel_requests_types + 17 },
	{ "set_fullscreen", "?o", xdg_toplevel_requests_types + 18 },
	{ "unset_fullscreen", "", xdg_toplevel_requests_types + 19 },
	{ "set_minimized", "", xdg_toplevel_requests_types + 20 },
};

static const struct wl_message xdg_toplevel_events[] = {
	{ "configure", "iia", NULL },
	{ "close", "", NULL },
	{ "configure_bounds", "ii", NULL },
	{ "wm_capabilities", "a", NULL },
};

const struct wl_interface xdg_toplevel_interface = {
	"xdg_toplevel", 5, 14, xdg_toplevel_requests, 4, xdg_toplevel_events,
};

/* ============================================================================
 * XDG_POPUP (stub - não usamos por enquanto)
 * ============================================================================
 */

static const struct wl_message xdg_popup_requests[] = {
	{ "destroy", "", NULL },
	{ "grab", "ou", NULL },
	{ "reposition", "ou", NULL },
};

static const struct wl_message xdg_popup_events[] = {
	{ "configure", "iiii", NULL },
	{ "popup_done", "", NULL },
	{ "repositioned", "u", NULL },
};

const struct wl_interface xdg_popup_interface = {
	"xdg_popup", 5, 3, xdg_popup_requests, 3, xdg_popup_events,
};
