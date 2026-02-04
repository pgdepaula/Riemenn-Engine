/**
 * @file camera_controller.c
 * @brief Lógica de controle da câmera (WASD, Setas, Zoom)
 */

#include "camera_controller.h"
#include <math.h>
#include "gui/ui/lib.h"

void bhs_camera_controller_update(bhs_camera_t *cam, bhs_ui_ctx_t ctx,
				  double dt)
{
	if (!cam)
		return;

	/* Dynamic Speed Scaling based on Altitude (Zoom Level) */
	/* Base movement on visible range. at Y=1e11, we see ~1e11. Speed should be fraction of that. */
	float altitude = fabsf((float)cam->y);
	if (altitude < 1000.0f)
		altitude = 1000.0f; /* Min clamp */

	/* Speed: Cross screen in ~2 seconds? */
	float move_speed;
	if (cam->is_top_down_mode) {
		move_speed = altitude * 1.5f * (float)dt;
	} else {
		move_speed =
			2.0e9f * (float)dt; /* Original value for Normal mode */
	}

	/* Shift Boost */
	if (bhs_ui_key_down(ctx, BHS_KEY_LEFTSHIFT)) {
		move_speed *= 5.0f;
	}

	/* Calculate Forward and Right vectors */
	float sin_y = sinf(cam->yaw);
	float cos_y = cosf(cam->yaw);

	/* Mouse Zoom */
	float scroll = bhs_ui_mouse_scroll(ctx);
	if (scroll != 0.0f) {
		float zoom_speed;
		if (cam->is_top_down_mode) {
			/* Logarithmic-ish zoom or proportional */
			zoom_speed = altitude * 0.15f;
		} else {
			zoom_speed = 5.0e8f;
		}

		float sin_p = sinf(cam->pitch);
		float cos_p = cosf(cam->pitch);

		/* Move along View Vector */
		cam->x += sin_y * cos_p * scroll * zoom_speed;
		cam->z += cos_y * cos_p * scroll * zoom_speed;
		cam->y += sin_p * scroll * zoom_speed;
	}

	/* Mouse Pan (Left Button + Drag) */
	{
		static int32_t last_mx = 0;
		static int32_t last_my = 0;
		static bool is_dragging = false;

		if (bhs_ui_mouse_down(ctx, 0)) { /* Left Button */
			int32_t mx, my;
			bhs_ui_mouse_pos(ctx, &mx, &my);

			if (!is_dragging) {
				is_dragging = true;
				last_mx = mx;
				last_my = my;
			} else {
				float dx = (float)(mx - last_mx);
				float dy = (float)(my - last_my);

				if (!cam->is_top_down_mode) {
					/* Normal Mode: Rotation */
					cam->yaw += dx * 0.005f;
					cam->pitch -= dy * 0.005f;
				} else {
					/* Top-Down Mode: Pan */
					/* Scale speed by altitude */
					float pan_scale =
						altitude *
						0.0015f; /* Pixel to World ratio */

					/* Rotate pan vector by YAW */
					/* Drag Right (dx>0) -> Camera Left (Move -RightVec) */
					/* Drag Down (dy>0) -> User wants map to move Down (South) -> Camera moves Down (South, Z+) */
					/* Previously we did cam->z -= ... (North). Now we want Z+ */

					/* X Axis (Sides) - User said it was normal. */
					/* dx>0 -> cam->x -= ... (Left) OK */
					cam->x -= (cos_y * dx + sin_y * dy) *
						  pan_scale;

					/* Z Axis (Vertical) - User said inverted. */
					/* Previously: cam->z -= (-sin*dx + cos*dy). 
					   If Yaw=0, z -= dy. dy>0 -> z decreases (North).
					   We want z increases (South). So z += dy. */
					cam->z += (-sin_y * dx + cos_y * dy) *
						  pan_scale;
				}

				last_mx = mx;
				last_my = my;
			}
		} else {
			is_dragging = false;
		}
	}

	/* W/S - Forward/Back (Standard FPS: Forward moves INTO scene - Planar for walking) */
	if (bhs_ui_key_down(ctx, BHS_KEY_W)) {
		cam->x += sin_y * move_speed;
		cam->z += cos_y * move_speed;
	}
	if (bhs_ui_key_down(ctx, BHS_KEY_S)) {
		cam->x -= sin_y * move_speed;
		cam->z -= cos_y * move_speed;
	}

	/* A/D - Strafe Left/Right (User said WASD inverted? Maybe A/D too? Let's assume yes) */
	if (bhs_ui_key_down(ctx, BHS_KEY_A)) {
		/* Left -> Right */
		cam->x -= cos_y * move_speed;
		cam->z += sin_y * move_speed;
	}
	if (bhs_ui_key_down(ctx, BHS_KEY_D)) {
		/* Right -> Left */
		cam->x += cos_y * move_speed;
		cam->z -= sin_y * move_speed;
	}

	/* Q/E - Up/Down - Keeping standard unless requested */
	if (bhs_ui_key_down(ctx, BHS_KEY_Q))
		cam->y += move_speed;
	if (bhs_ui_key_down(ctx, BHS_KEY_E))
		cam->y -= move_speed;
}
