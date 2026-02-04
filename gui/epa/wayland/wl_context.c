/**
 * @file wl_context.c
 * @brief Inicialização e Contexto Global Wayland
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "gui/epa/wayland/wl_internal.h"

/* ============================================================================
 * LISTENERS PARA REGISTRY & SEAT (Stub para input)
 * ============================================================================
 */

/* Forward declaration das funções de input que estarão em wl_input.c */
void bhs_wl_init_seat_listeners(struct bhs_platform_impl *p);

static void registry_global(void *data, struct wl_registry *registry,
			    uint32_t name, const char *interface,
			    uint32_t version)
{
	struct bhs_platform_impl *p = data;

	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		p->compositor = wl_registry_bind(registry, name,
						 &wl_compositor_interface, 4);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		p->xdg_wm_base = wl_registry_bind(registry, name,
						  &xdg_wm_base_interface, 1);
		/* Listener do xdg_wm_base.ping deve ser configurado pelo wl_window ou aqui?
       Geralmente é global. Vamos definir aqui. */
		extern const struct xdg_wm_base_listener xdg_base_listener;
		xdg_wm_base_add_listener(p->xdg_wm_base, &xdg_base_listener, p);
	} else if (strcmp(interface, wl_seat_interface.name) == 0) {
		p->seat = wl_registry_bind(registry, name, &wl_seat_interface,
					   version < 7 ? version : 7);
		/* Inicia listeners de input (definidos em wl_input.c) */
		bhs_wl_init_seat_listeners(p);
	} else if (strcmp(interface, wl_shm_interface.name) == 0) {
		p->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	} else if (strcmp(interface,
			  zwp_pointer_constraints_v1_interface.name) == 0) {
		p->pointer_constraints = wl_registry_bind(
			registry, name, &zwp_pointer_constraints_v1_interface,
			1);
	} else if (strcmp(interface,
			  zwp_relative_pointer_manager_v1_interface.name) ==
		   0) {
		p->relative_pointer_manager = wl_registry_bind(
			registry, name,
			&zwp_relative_pointer_manager_v1_interface, 1);
	}
}

static void registry_global_remove(void *data, struct wl_registry *registry,
				   uint32_t name)
{
	(void)data;
	(void)registry;
	(void)name;
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_global,
	.global_remove = registry_global_remove,
};

/* ============================================================================
 * XDG WM BASE LISTENER (Ping/Pong global)
 * ============================================================================
 */
static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base,
			     uint32_t serial)
{
	(void)data;
	xdg_wm_base_pong(xdg_wm_base, serial);
}

const struct xdg_wm_base_listener xdg_base_listener = {
	.ping = xdg_wm_base_ping,
};

/* ============================================================================
 * IMPLEMENTAÇÃO
 * ============================================================================
 */

int bhs_platform_init(bhs_platform_t *platform)
{
	if (!platform)
		return BHS_PLATFORM_ERR_INVALID;

	struct bhs_platform_impl *p = calloc(1, sizeof(*p));
	if (!p)
		return BHS_PLATFORM_ERR_NOMEM;

	/* XKB Context */
	p->xkb_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!p->xkb_ctx) {
		BHS_WL_LOG("erro: xkb_context_new falhou");
		free(p);
		return BHS_PLATFORM_ERR_INIT;
	}

	/* Connect Display */
	p->display = wl_display_connect(NULL);
	if (!p->display) {
		BHS_WL_LOG("erro: wl_display_connect falhou");
		xkb_context_unref(p->xkb_ctx);
		free(p);
		return BHS_PLATFORM_ERR_INIT;
	}

	/* Registry */
	p->registry = wl_display_get_registry(p->display);
	wl_registry_add_listener(p->registry, &registry_listener, p);
	wl_display_roundtrip(p->display);

	if (!p->compositor || !p->xdg_wm_base) {
		BHS_WL_LOG("erro: compositor faltando interfaces "
			   "(compositor/xdg_wm_base)");
		bhs_platform_shutdown(p);
		return BHS_PLATFORM_ERR_INIT;
	}

	/* Cursor Theme */
	const char *cursor_theme_name = getenv("XCURSOR_THEME");
	int cursor_size = 24;
	const char *cursor_size_str = getenv("XCURSOR_SIZE");
	if (cursor_size_str)
		cursor_size = atoi(cursor_size_str);

	p->cursor_theme =
		wl_cursor_theme_load(cursor_theme_name, cursor_size, p->shm);
	if (p->cursor_theme) {
		p->cursor_surface = wl_compositor_create_surface(p->compositor);
	}

	p->initialized = true;
	*platform = p;
	return BHS_PLATFORM_OK;
}

void bhs_platform_shutdown(bhs_platform_t platform)
{
	if (!platform)
		return;

	struct bhs_platform_impl *p = platform;

	if (p->pointer)
		wl_pointer_destroy(p->pointer);
	if (p->keyboard)
		wl_keyboard_destroy(p->keyboard);
	if (p->seat)
		wl_seat_destroy(p->seat);

	if (p->cursor_surface)
		wl_surface_destroy(p->cursor_surface);
	if (p->cursor_theme)
		wl_cursor_theme_destroy(p->cursor_theme);

	if (p->xdg_wm_base)
		xdg_wm_base_destroy(p->xdg_wm_base);
	if (p->compositor)
		wl_compositor_destroy(p->compositor);
	if (p->shm)
		wl_shm_destroy(p->shm);
	if (p->registry)
		wl_registry_destroy(p->registry);

	if (p->display) {
		wl_display_flush(p->display);
		wl_display_disconnect(p->display);
	}

	if (p->xkb_state)
		xkb_state_unref(p->xkb_state);
	if (p->xkb_keymap)
		xkb_keymap_unref(p->xkb_keymap);
	if (p->xkb_ctx)
		xkb_context_unref(p->xkb_ctx);

	free(p);
}

void bhs_platform_poll_events(bhs_platform_t platform)
{
	if (!platform)
		return;

	struct bhs_platform_impl *p = platform;

	if (wl_display_dispatch(p->display) == -1) {
		BHS_WL_LOG("erro: wl_display_dispatch falhou");
	}
}

void *bhs_platform_get_native_display(bhs_platform_t platform)
{
	if (!platform)
		return NULL;
	return platform->display;
}

// fiquei com preguiça de escrever comentários, vamos fingir que você entende
// esse código perfeitamente.