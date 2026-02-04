/**
 * @file view_spacetime.c
 * @brief Orquestrador da View (Cola: Scene + Camera + Renderer)
 */

#include "view_spacetime.h"
#include "engine/assets/image_loader.h"
#include "src/simulation/data/planet.h"
#include "src/ui/camera/camera_controller.h"
#include "src/ui/render/planet_renderer.h"
#include "src/ui/render/spacetime_renderer.h"

/* === Interface === */

void bhs_camera_init_view(bhs_camera_t *cam)
{
	bhs_camera_init(cam);
}

void bhs_camera_update_view(bhs_camera_t *cam, bhs_ui_ctx_t ctx, double dt)
{
	bhs_camera_controller_update(cam, ctx, dt);
}

void bhs_view_spacetime_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
			     const bhs_camera_t *cam, int width, int height,
			     const bhs_view_assets_t *assets,
			     bhs_visual_mode_t mode,
			     struct bhs_planet_pass *planet_pass)
{
	if (!ctx)
		return;

	/* Draw 2.5D Elements (Skybox, BH Quad) */
	bhs_spacetime_renderer_draw(ctx, scene, cam, width, height, assets,
				    mode, planet_pass);

	/* Draw 3D Elements */
	if (planet_pass) {
		bhs_gpu_cmd_buffer_t cmd =
			(bhs_gpu_cmd_buffer_t)bhs_ui_get_current_cmd(ctx);
		if (cmd) {
			bhs_ui_flush(ctx);

			bhs_planet_pass_draw(planet_pass, cmd, scene, cam,
					     assets, mode, (float)width,
					     (float)height);

			bhs_ui_reset_render_state(ctx);
		}
	}
}
