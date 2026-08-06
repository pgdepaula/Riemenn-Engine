/**
 * @file well.h
 * @brief Lógica de Deformação de Espaço-Tempo (Modular)
 */

#ifndef RI_SIM_WELL_H
#define RI_SIM_WELL_H

#include "engine/math/core.h"

/**
 * @struct ri_gravity_well
 * @brief Define um ponto de massa que distorce a malha
 */
struct ri_gravity_well {
	struct ri_vec4 pos;
	float mass;
	float radius;
};

/**
 * @brief Calcula a deformação (altura) em um ponto (x, y)
 * Formula: z = -Sum(M_i / (dist_i + soft))
 */
static inline float ri_calculate_height(float x, float y,
					 const struct ri_gravity_well *wells,
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

#endif /* RI_SIM_WELL_H */
