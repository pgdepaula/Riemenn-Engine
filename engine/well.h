/**
 * @file well.h
 * @brief Lógica de Deformação de Espaço-Tempo (Modular)
 */

#ifndef BHS_SIM_WELL_H
#define BHS_SIM_WELL_H

#include "math/core.h"

/**
 * @struct bhs_gravity_well
 * @brief Define um ponto de massa que distorce a malha
 */
struct bhs_gravity_well {
	struct bhs_vec4 pos;
	float mass;
	float radius;
};

/**
 * @brief Calcula a deformação (altura) em um ponto (x, y)
 * Formula: z = -Sum(M_i / (dist_i + soft))
 */
static inline float bhs_calculate_height(float x, float y,
					 const struct bhs_gravity_well *wells,
					 int count)
{
	float h = 0.0f;
	for (int i = 0; i < count; i++) {
		float dx = x - wells[i].pos.x;
		float dy = y - wells[i].pos.y;
		float dist = (float)sqrt(dx * dx + dy * dy);
		h -= wells[i].mass / (dist + 0.5f);
	}
	return h;
}

#endif /* BHS_SIM_WELL_H */
