/**
 * @file gravity_system.c
 * @brief Implementação do Sistema de Gravidade
 */

#include "engine/systems/gravity_system.h"
#include <math.h>
#include "engine/components/components.h"
#include "engine/physics/physics_defs.h"

/* Distância mínima pra evitar singularidade (divisão por zero) */
#define MIN_DISTANCE 0.1

void bhs_gravity_system_central(bhs_world_handle world, struct bhs_vec3 center,
				double central_mass)
{
	if (!world)
		return;

	bhs_ecs_query q;
	// NOTE: Use a mask for Physics + Transform
	const bhs_component_mask mask =
		(1 << BHS_COMP_TRANSFORM) | (1 << BHS_COMP_PHYSICS);
	bhs_ecs_query_init(&q, world, mask);

	bhs_entity_id id;
	while (bhs_ecs_query_next(&q, &id)) {
		bhs_transform_t *tr =
			bhs_ecs_get_component(world, id, BHS_COMP_TRANSFORM);
		bhs_physics_t *ph =
			bhs_ecs_get_component(world, id, BHS_COMP_PHYSICS);

		if (!tr || !ph || ph->is_static)
			continue;

		/* Vetor do corpo pro centro */
		struct bhs_vec3 diff = bhs_vec3_sub(center, tr->position);
		double r_sq = bhs_vec3_norm2(diff);
		double r = sqrt(r_sq);

		/* Evita singularidade */
		if (r < MIN_DISTANCE)
			continue;

		/* F = G * M * m / r^2 */
		double force_mag = (BHS_G * central_mass * ph->mass) / r_sq;
		struct bhs_vec3 dir =
			bhs_vec3_scale(diff, 1.0 / r); // Normalize

		struct bhs_vec3 force = bhs_vec3_scale(dir, force_mag);
		ph->force_accumulator =
			bhs_vec3_add(ph->force_accumulator, force);
	}
}

void bhs_gravity_system_nbody(bhs_world_handle world)
{
	if (!world)
		return;

	const bhs_component_mask mask =
		(1 << BHS_COMP_TRANSFORM) | (1 << BHS_COMP_PHYSICS);
	bhs_ecs_query q;
	bhs_ecs_query_init_cached(&q, world, mask);

	for (uint32_t i = 0; i < q.count; i++) {
		bhs_entity_id id_a = q.cache[i];
		bhs_transform_t *tr_a =
			bhs_ecs_get_component(world, id_a, BHS_COMP_TRANSFORM);
		bhs_physics_t *ph_a =
			bhs_ecs_get_component(world, id_a, BHS_COMP_PHYSICS);

		if (!tr_a || !ph_a)
			continue;

		for (uint32_t j = i + 1; j < q.count; j++) {
			bhs_entity_id id_b = q.cache[j];
			bhs_transform_t *tr_b = bhs_ecs_get_component(
				world, id_b, BHS_COMP_TRANSFORM);
			bhs_physics_t *ph_b = bhs_ecs_get_component(
				world, id_b, BHS_COMP_PHYSICS);

			if (!tr_b || !ph_b)
				continue;

			struct bhs_vec3 diff =
				bhs_vec3_sub(tr_b->position, tr_a->position);
			double r_sq = bhs_vec3_norm2(diff);
			double r = sqrt(r_sq);

			if (r < MIN_DISTANCE)
				continue;

			double force_mag =
				(BHS_G * ph_a->mass * ph_b->mass) / r_sq;
			struct bhs_vec3 dir = bhs_vec3_scale(diff, 1.0 / r);

			// Force on A (towards B)
			struct bhs_vec3 f_a = bhs_vec3_scale(dir, force_mag);
			if (!ph_a->is_static)
				ph_a->force_accumulator = bhs_vec3_add(
					ph_a->force_accumulator, f_a);

			// Force on B (opposite)
			struct bhs_vec3 f_b = bhs_vec3_scale(dir, -force_mag);
			if (!ph_b->is_static)
				ph_b->force_accumulator = bhs_vec3_add(
					ph_b->force_accumulator, f_b);
		}
	}

	bhs_ecs_query_destroy(&q);
}
