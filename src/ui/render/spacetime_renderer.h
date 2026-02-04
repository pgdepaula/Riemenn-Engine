#ifndef BHS_CMD_UI_RENDER_SPACETIME_RENDERER_H
#define BHS_CMD_UI_RENDER_SPACETIME_RENDERER_H

#include "engine/scene/scene.h"
#include "gui/ui/lib.h"
#include "src/ui/camera/camera.h"
#include "src/ui/screens/view_spacetime.h" /* bhs_visual_mode_t */

struct bhs_planet_pass;

/**
 * @brief Renderiza a malha do espaço-tempo
 * @param ctx Contexto de desenho
 * @param scene Cena contendo a simulação
 * @param cam Câmera para projeção
 * @param width Largura da tela
 * @param height Altura da tela
 * @param mode Modo de visualização (para escalar labels e marcadores)
 */
void bhs_spacetime_renderer_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
				 const bhs_camera_t *cam, int width, int height,
				 const void *assets, bhs_visual_mode_t mode,
				 struct bhs_planet_pass *planet_pass);

/* [NEW] Exporting project point for hit testing in other modules */
void bhs_project_point(const bhs_camera_t *cam, float x, float y, float z,
		       float sw, float sh, float *ox, float *oy);

#endif /* BHS_CMD_UI_RENDER_SPACETIME_RENDERER_H */
