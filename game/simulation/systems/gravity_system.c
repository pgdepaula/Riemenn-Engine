/**
 * @file gravity_system.c
 * @brief Implementação do Sistema de Gravidade
 */

#include "game/simulation/systems/gravity_system.h"
#include <math.h>
#include "engine/components/components.h"
#include "engine/physics/physics_defs.h"

/* Distância mínima pra evitar singularidade (divisão por zero) */
#define MIN_DISTANCE 0.1

void ri_gravity_system_central(ri_world_handle world, struct ri_vec3 center,
				double central_mass)
{
	if (!world)
		return;

	ri_ecs_query q;
	// NOTE: Use a mask for Physics + Transform
	const ri_component_mask mask =
		(1 << RI_COMP_TRANSFORM) | (1 << RI_COMP_PHYSICS);
	ri_ecs_query_init(&q, world, mask);

	ri_entity_id id;
	while (ri_ecs_query_next(&q, &id)) {
		ri_transform_t *tr =
			ri_ecs_get_component(world, id, RI_COMP_TRANSFORM);
		ri_physics_t *ph =
			ri_ecs_get_component(world, id, RI_COMP_PHYSICS);

		if (!tr || !ph || ph->is_static)
			continue;

		/* Vetor do corpo pro centro */
		struct ri_vec3 diff = ri_vec3_sub(center, tr->position);
		double r_sq = ri_vec3_norm2(diff);
		double r = sqrt(r_sq);

		/* Evita singularidade */
		if (r < MIN_DISTANCE)
			continue;

		/* F = G * M * m / r^2 */
		double force_mag = (RI_G * central_mass * ph->mass) / r_sq;
		struct ri_vec3 dir =
			ri_vec3_scale(diff, 1.0 / r); // Normalize

		struct ri_vec3 force = ri_vec3_scale(dir, force_mag);
		ph->force_accumulator =
			ri_vec3_add(ph->force_accumulator, force);
	}
}

void ri_gravity_system_nbody(ri_world_handle world)
{
	if (!world)
		return;

	const ri_component_mask mask =
		(1 << RI_COMP_TRANSFORM) | (1 << RI_COMP_PHYSICS);
	ri_ecs_query q;
	ri_ecs_query_init_cached(&q, world, mask);

	for (uint32_t i = 0; i < q.count; i++) {
		ri_entity_id id_a = q.cache[i];
		ri_transform_t *tr_a =
			ri_ecs_get_component(world, id_a, RI_COMP_TRANSFORM);
		ri_physics_t *ph_a =
			ri_ecs_get_component(world, id_a, RI_COMP_PHYSICS);

		if (!tr_a || !ph_a)
			continue;

		for (uint32_t j = i + 1; j < q.count; j++) {
			ri_entity_id id_b = q.cache[j];
			ri_transform_t *tr_b = ri_ecs_get_component(
				world, id_b, RI_COMP_TRANSFORM);
			ri_physics_t *ph_b = ri_ecs_get_component(
				world, id_b, RI_COMP_PHYSICS);

			if (!tr_b || !ph_b)
				continue;

			struct ri_vec3 diff =
				ri_vec3_sub(tr_b->position, tr_a->position);
			double r_sq = ri_vec3_norm2(diff);
			double r = sqrt(r_sq);

			if (r < MIN_DISTANCE)
				continue;

			double force_mag =
				(RI_G * ph_a->mass * ph_b->mass) / r_sq;
			struct ri_vec3 dir = ri_vec3_scale(diff, 1.0 / r);

			// Force on A (towards B)
			struct ri_vec3 f_a = ri_vec3_scale(dir, force_mag);
			if (!ph_a->is_static)
				ph_a->force_accumulator = ri_vec3_add(
					ph_a->force_accumulator, f_a);

			// Force on B (opposite)
			struct ri_vec3 f_b = ri_vec3_scale(dir, -force_mag);
			if (!ph_b->is_static)
				ph_b->force_accumulator = ri_vec3_add(
					ph_b->force_accumulator, f_b);
		}
	}

	ri_ecs_query_destroy(&q);
}
