/**
 * @file physics_system.h
 * @brief Sistema de Física (CPU Reference Implementation)
 */

#ifndef BHS_ENGINE_SYSTEMS_PHYSICS_H
#define BHS_ENGINE_SYSTEMS_PHYSICS_H

#include "engine/ecs/ecs.h"

/**
 * Atualiza a física de todas as entidades com Transform + Physics
 * @param world Mundo ECS
 * @param dt Delta time em segundos
 */
void bhs_physics_system_update(bhs_world_handle world, double dt);

#endif /* BHS_ENGINE_SYSTEMS_PHYSICS_H */
