#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui/epa/navigator/nav_internal.h"

// Forward declarations from nav_input.c
EM_BOOL nav_on_resize(int eventType, const EmscriptenUiEvent *e,
		      void *userData);
EM_BOOL nav_on_mouse_move(int eventType, const EmscriptenMouseEvent *e,
			  void *userData);
EM_BOOL nav_on_mouse_button(int eventType, const EmscriptenMouseEvent *e,
			    void *userData);
EM_BOOL nav_on_wheel(int eventType, const EmscriptenWheelEvent *e,
		     void *userData);
EM_BOOL nav_on_key(int eventType, const EmscriptenKeyboardEvent *e,
		   void *userData);

int bhs_window_create(bhs_platform_t platform,
		      const struct bhs_window_config *config,
		      bhs_window_t *window)
{
	if (!platform)
		return BHS_PLATFORM_ERR_INVALID;

	struct bhs_window_impl *win = calloc(1, sizeof(*win));
	if (!win)
		return BHS_PLATFORM_ERR_NOMEM;

	// WGPUSurface needs selector ("#canvas")
	strncpy(win->canvas_id, "#canvas", 63);

	win->width = config->width;
	win->height = config->height;
	win->should_close = false;

	emscripten_set_canvas_element_size(win->canvas_id, win->width,
					   win->height);

	// Emscripten callbacks need ID ("canvas")
	const char *target_id = win->canvas_id + 1;

	emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, win,
				       EM_FALSE, nav_on_resize);
	emscripten_set_mousemove_callback(target_id, win, EM_FALSE,
					  nav_on_mouse_move);
	emscripten_set_mousedown_callback(target_id, win, EM_FALSE,
					  nav_on_mouse_button);
	emscripten_set_mouseup_callback(target_id, win, EM_FALSE,
					nav_on_mouse_button);
	emscripten_set_wheel_callback(target_id, win, EM_FALSE, nav_on_wheel);

	emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, win,
					EM_FALSE, nav_on_key);
	emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, win,
				      EM_FALSE, nav_on_key);

	if (config->title) {
		emscripten_set_window_title(config->title);
	}

	printf("[NAVIGATOR] Janela criada: %dx%d\n", win->width, win->height);

	*window = win;
	return BHS_PLATFORM_OK;
}

void bhs_window_destroy(bhs_window_t window)
{
	if (!window)
		return;
	free(window);
}

void bhs_window_show(bhs_window_t window)
{
	(void)window;
}
void bhs_window_hide(bhs_window_t window)
{
	(void)window;
}

void bhs_window_set_title(bhs_window_t window, const char *title)
{
	if (title)
		emscripten_set_window_title(title);
}

void bhs_window_get_size(bhs_window_t window, int32_t *width, int32_t *height)
{
	if (!window)
		return;
	struct bhs_window_impl *win = window;
	if (width)
		*width = win->width;
	if (height)
		*height = win->height;
}

void bhs_window_get_framebuffer_size(bhs_window_t window, int32_t *width,
				     int32_t *height)
{
	if (!window)
		return;
	struct bhs_window_impl *win = window;

	int w, h;
	emscripten_get_canvas_element_size(win->canvas_id, &w, &h);

	if (width)
		*width = w;
	if (height)
		*height = h;
}

bool bhs_window_should_close(bhs_window_t window)
{
	if (!window)
		return true;
	return ((struct bhs_window_impl *)window)->should_close;
}

void bhs_window_set_should_close(bhs_window_t window, bool should_close)
{
	if (!window)
		return;
	((struct bhs_window_impl *)window)->should_close = should_close;
}

void bhs_window_set_cursor(bhs_window_t window, enum bhs_cursor_shape shape)
{
	// TODO: Mapear para cursor CSS se eu descobrir como faz isso sem querer morrer.
	(void)window;
	(void)shape;
}

void bhs_window_set_mouse_lock(bhs_window_t window, bool locked)
{
	if (!window)
		return;
	struct bhs_window_impl *win = window;
	win->mouse_locked = locked;

	if (locked) {
		emscripten_request_pointerlock(win->canvas_id, EM_TRUE);
	} else {
		emscripten_exit_pointerlock();
	}
}

void bhs_window_set_event_callback(bhs_window_t window,
				   bhs_event_callback_fn callback,
				   void *userdata)
{
	if (!window)
		return;
	struct bhs_window_impl *win = window;
	win->event_cb = callback;
	win->event_user_data = userdata;
}

bool bhs_window_next_event(bhs_window_t window, struct bhs_event *event)
{
	return false; // Callback driven
}

void *bhs_window_get_native_handle(bhs_window_t window)
{
	if (!window)
		return NULL;
	// Returns selector string for WebGPU
	return ((struct bhs_window_impl *)window)->canvas_id;
}

void *bhs_window_get_native_layer(bhs_window_t window)
{
	return NULL;
}
