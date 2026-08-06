/**
 * @file physics_system.c
 * @brief Integrador de Física (CPU)
 */

#include "engine/physics/physics_system.h"
#include "engine/components/components.h"
#include "engine/physics/physics_defs.h"

// Define helper mask if not in header
#define RI_MASK_MOVABLE ((1 << RI_COMP_TRANSFORM) | (1 << RI_COMP_PHYSICS))

void ri_physics_system_update(ri_world_handle world, double dt)
{
	if (!world)
		return;

	ri_ecs_query q;
	ri_ecs_query_init(&q, world, RI_MASK_MOVABLE);

	ri_entity_id id;
	while (ri_ecs_query_next(&q, &id)) {
		ri_transform_t *tr =
			ri_ecs_get_component(world, id, RI_COMP_TRANSFORM);
		ri_physics_t *ph =
			ri_ecs_get_component(world, id, RI_COMP_PHYSICS);

		if (!tr || !ph || ph->is_static)
			continue;

		// Symplectic Euler
		// v = v + a * dt
		// x = x + v * dt

		// F = ma -> a = F/m (or F * inv_mass)
		struct ri_vec3 acc = { 0 };
		if (ph->mass > 0) {
			double inv_mass = 1.0 / ph->mass; // Could cache this
			acc = ri_vec3_scale(ph->force_accumulator, inv_mass);
		}

		struct ri_vec3 dv = ri_vec3_scale(acc, dt);
		ph->velocity = ri_vec3_add(ph->velocity, dv);

		struct ri_vec3 dx = ri_vec3_scale(ph->velocity, dt);
		tr->position = ri_vec3_add(tr->position, dx);

		// Reset forces
		ph->force_accumulator = (struct ri_vec3){ 0, 0, 0 };
	}
}
