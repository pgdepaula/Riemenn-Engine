#ifndef BHS_CMD_UI_CAMERA_CONTROLLER_H
#define BHS_CMD_UI_CAMERA_CONTROLLER_H

#include "camera.h"
#include "gui/ui/lib.h" /* Para bhs_ui_ctx_t e chaves */

/**
 * @brief Atualiza a posição da câmera baseado no input do usuário
 * @param cam Ponteiro para a câmera a ser movida
 * @param ctx Contexto da UI (para ler teclado)
 * @param dt Delta time em segundos
 */
void bhs_camera_controller_update(bhs_camera_t *cam, bhs_ui_ctx_t ctx,
				  double dt);

#endif /* BHS_CMD_UI_CAMERA_CONTROLLER_H */
