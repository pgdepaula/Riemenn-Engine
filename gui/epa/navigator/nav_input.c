#include <string.h>
#include "gui/epa/navigator/nav_internal.h"

static uint32_t get_mods(const EmscriptenUiEvent *e)
{
	uint32_t mods = 0;
	if (e->ctrlKey)
		mods |= BHS_MOD_CTRL;
	if (e->shiftKey)
		mods |= BHS_MOD_SHIFT;
	if (e->altKey)
		mods |= BHS_MOD_ALT;
	if (e->metaKey)
		mods |= BHS_MOD_SUPER;
	return mods;
}

static uint32_t get_mouse_mods(const EmscriptenMouseEvent *e)
{
	uint32_t mods = 0;
	if (e->ctrlKey)
		mods |= BHS_MOD_CTRL;
	if (e->shiftKey)
		mods |= BHS_MOD_SHIFT;
	if (e->altKey)
		mods |= BHS_MOD_ALT;
	if (e->metaKey)
		mods |= BHS_MOD_SUPER;
	return mods;
}

uint64_t nav_get_time_ns(void)
{
	return (uint64_t)(emscripten_get_now() * 1000000.0);
}

EM_BOOL nav_on_resize(int eventType, const EmscriptenUiEvent *e, void *userData)
{
	struct bhs_window_impl *win = userData;
	if (!win || !win->event_cb)
		return EM_FALSE;

	double w, h;
	emscripten_get_element_css_size(win->canvas_id, &w, &h);
	win->width = (int32_t)w;
	win->height = (int32_t)h;

	emscripten_set_canvas_element_size(win->canvas_id, win->width,
					   win->height);

	struct bhs_event ev = { .type = BHS_EVENT_WINDOW_RESIZE,
				.timestamp_ns = nav_get_time_ns(),
				.resize = { .width = win->width,
					    .height = win->height } };

	win->event_cb((bhs_window_t)win, &ev, win->event_user_data);
	return EM_TRUE;
}

EM_BOOL nav_on_mouse_move(int eventType, const EmscriptenMouseEvent *e,
			  void *userData)
{
	struct bhs_window_impl *win = userData;
	if (!win || !win->event_cb)
		return EM_FALSE;

	struct bhs_event ev = { .type = BHS_EVENT_MOUSE_MOVE,
				.mods = get_mouse_mods(e),
				.timestamp_ns = nav_get_time_ns(),
				.mouse_move = { .x = e->targetX,
						.y = e->targetY,
						.dx = e->movementX,
						.dy = e->movementY } };

	win->event_cb((bhs_window_t)win, &ev, win->event_user_data);
	return EM_TRUE;
}

EM_BOOL nav_on_mouse_button(int eventType, const EmscriptenMouseEvent *e,
			    void *userData)
{
	struct bhs_window_impl *win = userData;
	if (!win || !win->event_cb)
		return EM_FALSE;

	struct bhs_event ev = { .type = (eventType ==
					 EMSCRIPTEN_EVENT_MOUSEDOWN)
						? BHS_EVENT_MOUSE_DOWN
						: BHS_EVENT_MOUSE_UP,
				.mods = get_mouse_mods(e),
				.timestamp_ns = nav_get_time_ns(),
				.mouse_button = { .x = e->targetX,
						  .y = e->targetY,
						  .button = e->button,
						  .click_count = 1 } };

	if (e->button == 1)
		ev.mouse_button.button = BHS_MOUSE_MIDDLE;
	else if (e->button == 2)
		ev.mouse_button.button = BHS_MOUSE_RIGHT;
	else
		ev.mouse_button.button = (enum bhs_mouse_button)e->button;

	win->event_cb((bhs_window_t)win, &ev, win->event_user_data);
	return EM_TRUE;
}

EM_BOOL nav_on_wheel(int eventType, const EmscriptenWheelEvent *e,
		     void *userData)
{
	struct bhs_window_impl *win = userData;
	if (!win || !win->event_cb)
		return EM_FALSE;

	struct bhs_event ev = { .type = BHS_EVENT_MOUSE_SCROLL,
				.mods = get_mouse_mods(&e->mouse),
				.timestamp_ns = nav_get_time_ns(),
				.scroll = { .x = e->mouse.targetX,
					    .y = e->mouse.targetY,
					    .dx = (float)-e->deltaX,
					    .dy = (float)-e->deltaY,
					    .is_precise = (e->deltaMode ==
							   DOM_DELTA_PIXEL) } };

	win->event_cb((bhs_window_t)win, &ev, win->event_user_data);
	return EM_TRUE;
}

EM_BOOL nav_on_key(int eventType, const EmscriptenKeyboardEvent *e,
		   void *userData)
{
	struct bhs_window_impl *win = userData;
	if (!win || !win->event_cb)
		return EM_FALSE;

	struct bhs_event ev = { .timestamp_ns = nav_get_time_ns(),
				.key = { .keycode = e->keyCode,
					 .scancode = e->which } };

	strncpy(ev.key.text, e->key, sizeof(ev.key.text) - 1);

	if (eventType == EMSCRIPTEN_EVENT_KEYDOWN) {
		ev.type = e->repeat ? BHS_EVENT_KEY_REPEAT : BHS_EVENT_KEY_DOWN;
	} else {
		ev.type = BHS_EVENT_KEY_UP;
	}

	if (e->ctrlKey)
		ev.mods |= BHS_MOD_CTRL;
	if (e->shiftKey)
		ev.mods |= BHS_MOD_SHIFT;
	if (e->altKey)
		ev.mods |= BHS_MOD_ALT;
	if (e->metaKey)
		ev.mods |= BHS_MOD_SUPER;

	win->event_cb((bhs_window_t)win, &ev, win->event_user_data);
	return EM_TRUE;
}
