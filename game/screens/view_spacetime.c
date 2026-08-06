/**
 * @file view_spacetime.c
 * @brief Orquestrador da View (Cola: Scene + Camera + Renderer)
 */

#include "view_spacetime.h"
#include "engine/assets/image_loader.h"
#include "game/simulation/data/planet.h"
#include "engine/render/camera_controller.h"
#include "game/render/planet_renderer.h"
#include "game/render/spacetime_renderer.h"

/* === Interface === */

void ri_camera_init_view(ri_camera_t *cam)
{
	ri_camera_init(cam);
}

void ri_camera_update_view(ri_camera_t *cam, ri_ui_ctx_t ctx, double dt)
{
	ri_camera_controller_update(cam, ctx, dt);
}

void ri_view_spacetime_draw(ri_ui_ctx_t ctx, ri_scene_t scene,
			     const ri_camera_t *cam, int width, int height,
			     const ri_view_assets_t *assets,
			     ri_visual_mode_t mode,
			     struct ri_planet_pass *planet_pass)
{
	if (!ctx)
		return;

	/* Draw 2.5D Elements (Skybox, BH Quad) */
	ri_spacetime_renderer_draw(ctx, scene, cam, width, height, assets,
				    mode, planet_pass);

	/* Draw 3D Elements */
	if (planet_pass) {
		ri_gpu_cmd_buffer_t cmd =
			(ri_gpu_cmd_buffer_t)ri_ui_get_current_cmd(ctx);
		if (cmd) {
			ri_ui_flush(ctx);

			ri_planet_pass_draw(planet_pass, cmd, scene, cam,
					     assets, mode, (float)width,
					     (float)height);

			ri_ui_reset_render_state(ctx);
		}
	}
}
