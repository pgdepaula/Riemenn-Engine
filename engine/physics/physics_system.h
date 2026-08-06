/**
 * @file physics_system.h
 * @brief Sistema de Física (CPU Reference Implementation)
 */

#ifndef RI_ENGINE_SYSTEMS_PHYSICS_H
#define RI_ENGINE_SYSTEMS_PHYSICS_H

#include "engine/ecs/ecs.h"

/**
 * Atualiza a física de todas as entidades com Transform + Physics
 * @param world Mundo ECS
 * @param dt Delta time em segundos
 */
void ri_physics_system_update(ri_world_handle world, double dt);

#endif /* RI_ENGINE_SYSTEMS_PHYSICS_H */
