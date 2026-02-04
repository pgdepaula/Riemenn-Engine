#ifndef BHS_UI_RENDER_PLANET_RENDERER_H
#define BHS_UI_RENDER_PLANET_RENDERER_H

#include "engine/scene/scene.h"
#include "gui/ui/lib.h"
#include "src/ui/camera/camera.h"
#include "src/ui/screens/view_spacetime.h" /* For texture cache */

typedef struct bhs_planet_pass *bhs_planet_pass_t;

/**
 * @brief Initialize the Planet Render Pass
 * Loads shaders, creates pipeline, generates sphere mesh.
 */
int bhs_planet_pass_create(bhs_ui_ctx_t ctx, bhs_planet_pass_t *out_pass);

/**
 * @brief Destroy the pass and resources
 */
void bhs_planet_pass_destroy(bhs_planet_pass_t pass);

/**
 * @brief Draw all planets in the scene
 * @param pass The pass instance
 * @param cmd The command buffer to record into
 * @param scene Scene data
 * @param cam Camera info (for MVP)
 * @param assets View assets (for texture lookup)
 * @param output_width Render target width (for aspect ratio)
 * @param output_height Render target height
 */
void bhs_planet_pass_draw(bhs_planet_pass_t pass, bhs_gpu_cmd_buffer_t cmd,
			  bhs_scene_t scene, const bhs_camera_t *cam,
			  const bhs_view_assets_t *assets,
			  bhs_visual_mode_t mode, float output_width,
			  float output_height);

/**
 * @brief Submit a 3D line to be drawn with depth testing
 */
void bhs_planet_pass_submit_line(bhs_planet_pass_t pass, float x1, float y1,
				 float z1, float x2, float y2, float z2,
				 float r, float g, float b, float a);

#endif
