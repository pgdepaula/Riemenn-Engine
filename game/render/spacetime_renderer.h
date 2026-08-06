#ifndef RI_CMD_UI_RENDER_SPACETIME_RENDERER_H
#define RI_CMD_UI_RENDER_SPACETIME_RENDERER_H

#include "engine/scene/scene.h"
#include "engine/ui/ui.h"
#include "engine/render/camera.h"
#include "game/screens/view_spacetime.h" /* ri_visual_mode_t */

struct ri_planet_pass;

/**
 * @brief Renderiza a malha do espaço-tempo
 * @param ctx Contexto de desenho
 * @param scene Cena contendo a simulação
 * @param cam Câmera para projeção
 * @param width Largura da tela
 * @param height Altura da tela
 * @param mode Modo de visualização (para escalar labels e marcadores)
 */
void ri_spacetime_renderer_draw(ri_ui_ctx_t ctx, ri_scene_t scene,
				 const ri_camera_t *cam, int width, int height,
				 const void *assets, ri_visual_mode_t mode,
				 struct ri_planet_pass *planet_pass);

/* [NEW] Exporting project point for hit testing in other modules */
void ri_project_point(const ri_camera_t *cam, float x, float y, float z,
		       float sw, float sh, float *ox, float *oy);

#endif /* RI_CMD_UI_RENDER_SPACETIME_RENDERER_H */
