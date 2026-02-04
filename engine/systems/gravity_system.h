/**
 * @file gravity_system.h
 * @brief Sistema de Gravidade (N-Body / Campo Central)
 *
 * "A gravidade não é uma força. É uma curvatura."
 * - Einstein (trollando Newton desde 1915)
 *
 * Dois modos de operação:
 * 1. Campo Central: Uma massa dominante no centro (buraco negro, sol)
 * 2. N-Body: Todas as massas interagem entre si (O(n²), cuidado)
 */

#ifndef BHS_ENGINE_SYSTEMS_GRAVITY_H
#define BHS_ENGINE_SYSTEMS_GRAVITY_H

#include "engine/ecs/ecs.h"
#include "math/vec4.h"

#include "engine/physics/physics_defs.h"

/**
 * bhs_gravity_system_central - Aplica gravidade de campo central
 * @world: Mundo ECS
 * @center: Posição da massa central
 * @central_mass: Massa do corpo central
 *
 * Todas as entidades com Transform + Physics são atraídas pelo centro.
 * Usado quando há um corpo dominante (buraco negro, sol).
 *
 * Acumula forças no force_accumulator. NÃO integra posição.
 */
void bhs_gravity_system_central(bhs_world_handle world, struct bhs_vec3 center,
				double central_mass);

/**
 * bhs_gravity_system_nbody - Aplica gravidade N-Body
 * @world: Mundo ECS
 *
 * Todas as entidades com Transform + Physics interagem entre si.
 * Complexidade O(n²) - use com cuidado em sistemas com muitos corpos.
 *
 * Acumula forças no force_accumulator. NÃO integra posição.
 */
void bhs_gravity_system_nbody(bhs_world_handle world);

#endif /* BHS_ENGINE_SYSTEMS_GRAVITY_H */
