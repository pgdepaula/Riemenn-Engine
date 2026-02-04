#include <stdio.h>
#include <stdlib.h>
#include "gui/epa/navigator/nav_internal.h"

int bhs_platform_init(bhs_platform_t *platform)
{
	if (!platform)
		return BHS_PLATFORM_ERR_INVALID;

	struct bhs_platform_impl *p = calloc(1, sizeof(*p));
	if (!p)
		return BHS_PLATFORM_ERR_NOMEM;

	// No browser, verificamos se o contexto WebGL/WebGPU não está perdido
	// Stub check
	if (!emscripten_is_webgl_context_lost) {
		// Just linking check
	}

	p->initialized = true;
	*platform = p;

	printf("[NAVIGATOR] Plataforma inicializada. Olá, WASM.\n");
	return BHS_PLATFORM_OK;
}

void bhs_platform_shutdown(bhs_platform_t platform)
{
	if (!platform)
		return;
	free(platform);
}

void bhs_platform_poll_events(bhs_platform_t platform)
{
	// Browser event loop handles dispatch
	(void)platform;
}

void bhs_platform_wait_events(bhs_platform_t platform)
{
	// Browser event loop handles wait
	(void)platform;
}

void *bhs_platform_get_native_display(bhs_platform_t platform)
{
	return NULL; // Browser is the display
}
