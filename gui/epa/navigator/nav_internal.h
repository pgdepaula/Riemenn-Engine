#ifndef NAV_INTERNAL_H
#define NAV_INTERNAL_H

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <stdbool.h>
#include <stdint.h>
#include "gui/epa/epa.h"

struct bhs_platform_impl {
	bool initialized;
};

struct bhs_window_impl {
	char canvas_id[64]; // #canvas
	int32_t width;
	int32_t height;
	bool should_close;
	bool mouse_locked;

	bhs_event_callback_fn event_cb;
	void *event_user_data;
};

// Utils (input)
uint64_t nav_get_time_ns(void);

#endif
