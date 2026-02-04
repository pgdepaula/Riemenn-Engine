#ifndef BHS_CMD_UI_CAMERA_CAMERA_H
#define BHS_CMD_UI_CAMERA_CAMERA_H

#include <stdbool.h>

/**
 * @brief Estrutura da Câmera
 */
typedef struct bhs_camera {
	double x, y, z;	       /* Posição World (Y UP) - Double para RTC */
	float pitch;	       /* Rotação X (radianos) */
	float yaw;	       /* Rotação Y (radianos) */
	float fov;	       /* Field of View / Scale Factor */
	bool is_top_down_mode; /* [NEW] Modo Top Down */
} bhs_camera_t;

/**
 * @brief Inicializa a câmera com valores padrão
 */
void bhs_camera_init(bhs_camera_t *cam);

#endif /* BHS_CMD_UI_CAMERA_CAMERA_H */
