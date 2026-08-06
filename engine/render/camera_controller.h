#ifndef RI_CMD_UI_CAMERA_CONTROLLER_H
#define RI_CMD_UI_CAMERA_CONTROLLER_H

#include "camera.h"
#include "engine/ui/ui.h" /* Para ri_ui_ctx_t e chaves */

/**
 * @brief Atualiza a posição da câmera baseado no input do usuário
 * @param cam Ponteiro para a câmera a ser movida
 * @param ctx Contexto da UI (para ler teclado)
 * @param dt Delta time em segundos
 */
void ri_camera_controller_update(ri_camera_t *cam, ri_ui_ctx_t ctx,
				  double dt);

#endif /* RI_CMD_UI_CAMERA_CONTROLLER_H */
