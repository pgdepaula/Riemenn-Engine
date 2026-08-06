#ifndef RI_CMD_UI_CAMERA_CAMERA_H
#define RI_CMD_UI_CAMERA_CAMERA_H

#include <stdbool.h>

/**
 * @brief Estrutura da Câmera
 */
typedef struct ri_camera {
	double x, y, z;	       /* Posição World (Y UP) - Double para RTC */
	float pitch;	       /* Rotação X (radianos) */
	float yaw;	       /* Rotação Y (radianos) */
	float fov;	       /* Field of View / Scale Factor */
	bool is_top_down_mode; /* [NEW] Modo Top Down */
} ri_camera_t;

/**
 * @brief Inicializa a câmera com valores padrão
 */
void ri_camera_init(ri_camera_t *cam);

#endif /* RI_CMD_UI_CAMERA_CAMERA_H */
