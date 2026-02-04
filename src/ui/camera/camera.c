/**
 * @file camera.c
 * @brief Dados e Inicialização da Câmera
 */

#include "camera.h"
#include <stddef.h>

void bhs_camera_init(bhs_camera_t *cam)
{
	if (!cam)
		return;
	cam->x = 0.0;
	cam->y = 10.0;	   /* Altura da camera (Y UP) */
	cam->z = -30.0;	   /* Distancia */
	cam->pitch = 0.3f; /* Levemente inclinado pra baixo */
	cam->yaw = 0.0f;
	cam->fov = 500.0f;
	cam->is_top_down_mode = false;
}
