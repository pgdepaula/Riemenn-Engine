/**
 * @file physics_system.c
 * @brief Integrador de Física (CPU)
 */

#include "engine/systems/physics_system.h"
#include "engine/components/components.h"
#include "engine/physics/physics_defs.h"

// Define helper mask if not in header
#define BHS_MASK_MOVABLE ((1 << BHS_COMP_TRANSFORM) | (1 << BHS_COMP_PHYSICS))

void bhs_physics_system_update(bhs_world_handle world, double dt)
{
	if (!world)
		return;

	bhs_ecs_query q;
	bhs_ecs_query_init(&q, world, BHS_MASK_MOVABLE);

	bhs_entity_id id;
	while (bhs_ecs_query_next(&q, &id)) {
		bhs_transform_t *tr =
			bhs_ecs_get_component(world, id, BHS_COMP_TRANSFORM);
		bhs_physics_t *ph =
			bhs_ecs_get_component(world, id, BHS_COMP_PHYSICS);

		if (!tr || !ph || ph->is_static)
			continue;

		// Symplectic Euler
		// v = v + a * dt
		// x = x + v * dt

		// F = ma -> a = F/m (or F * inv_mass)
		struct bhs_vec3 acc = { 0 };
		if (ph->mass > 0) {
			double inv_mass = 1.0 / ph->mass; // Could cache this
			acc = bhs_vec3_scale(ph->force_accumulator, inv_mass);
		}

		struct bhs_vec3 dv = bhs_vec3_scale(acc, dt);
		ph->velocity = bhs_vec3_add(ph->velocity, dv);

		struct bhs_vec3 dx = bhs_vec3_scale(ph->velocity, dt);
		tr->position = bhs_vec3_add(tr->position, dx);

		// Reset forces
		ph->force_accumulator = (struct bhs_vec3){ 0, 0, 0 };
	}
}
