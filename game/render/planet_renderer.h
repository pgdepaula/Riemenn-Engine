#ifndef RI_UI_RENDER_PLANET_RENDERER_H
#define RI_UI_RENDER_PLANET_RENDERER_H

#include "engine/scene/scene.h"
#include "engine/ui/ui.h"
#include "engine/render/camera.h"
#include "game/screens/view_spacetime.h" /* For texture cache */

typedef struct ri_planet_pass *ri_planet_pass_t;

/**
 * @brief Initialize the Planet Render Pass
 * Loads shaders, creates pipeline, generates sphere mesh.
 */
int ri_planet_pass_create(ri_ui_ctx_t ctx, ri_planet_pass_t *out_pass);

/**
 * @brief Destroy the pass and resources
 */
void ri_planet_pass_destroy(ri_planet_pass_t pass);

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
void ri_planet_pass_draw(ri_planet_pass_t pass, ri_gpu_cmd_buffer_t cmd,
			  ri_scene_t scene, const ri_camera_t *cam,
			  const ri_view_assets_t *assets,
			  ri_visual_mode_t mode, float output_width,
			  float output_height);

/**
 * @brief Submit a 3D line to be drawn with depth testing
 */
void ri_planet_pass_submit_line(ri_planet_pass_t pass, float x1, float y1,
				 float z1, float x2, float y2, float z2,
				 float r, float g, float b, float a);

#endif
