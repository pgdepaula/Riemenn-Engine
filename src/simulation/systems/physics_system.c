/**
 * @file physics_system.c
 * @brief Sistema de Física Unificado (Adapter para Integrador High-Fidelity)
 */

#include <stdio.h>
#include <string.h>
#include "engine/components/components.h"
#include "engine/physics/integrator.h"
#include "src/simulation/components/sim_components.h"
#include "src/simulation/systems/systems.h"

void physics_system_update(bhs_world_handle world, double dt)
{
	/* 1. Extract ECS data to Integrator State */
	struct bhs_system_state state;
	state.n_bodies = 0;
	state.time =
		0.0; // Cumulative time handled by app_state, this is local step

	bhs_ecs_query q;
	/* We need Transform (Pos) and Physics (Vel, Mass) */
	bhs_ecs_query_init(&q, world,
			   (1 << BHS_COMP_PHYSICS) | (1 << BHS_COMP_TRANSFORM));

	/* Map IDs to array indices to write back later */
	bhs_entity_id entity_map[BHS_MAX_BODIES];

	bhs_entity_id id;
	while (bhs_ecs_query_next(&q, &id) && state.n_bodies < BHS_MAX_BODIES) {
		bhs_transform_t *t =
			bhs_ecs_get_component(world, id, BHS_COMP_TRANSFORM);
		bhs_physics_t *p =
			bhs_ecs_get_component(world, id, BHS_COMP_PHYSICS);

		if (!t || !p)
			continue;

		int i = state.n_bodies;
		state.bodies[i].pos = t->position;
		state.bodies[i].vel = p->velocity;
		state.bodies[i].mass = p->mass;
		state.bodies[i].gm = p->mass * 6.67430e-11; /* G_SI */
		state.bodies[i].is_fixed = p->is_static;
		state.bodies[i].is_alive = true;

		/* [NEW] Extract J2 and Radius from Celestial Component if available */
		state.bodies[i].radius = 0.0;
		state.bodies[i].j2 = 0.0;
		state.bodies[i].inertia = 0.0;
		state.bodies[i].rot_vel =
			(struct bhs_vec3){ 0, 0, 0 }; /* Default */

		bhs_celestial_component *c =
			bhs_ecs_get_component(world, id, BHS_COMP_CELESTIAL);
		if (c) {
			if (c->type == BHS_CELESTIAL_PLANET) {
				state.bodies[i].radius = c->data.planet.radius;
				state.bodies[i].j2 = c->data.planet.j2;

				/* [NEW] 6-DOF Setup */
				/* Calculate Inertia for Sphere: 2/5 * M * R^2 */
				state.bodies[i].inertia =
					0.4 * state.bodies[i].mass *
					state.bodies[i].radius *
					state.bodies[i].radius;

				/* Rotation Velocity Vector */
				/* Converting scalar speed + axis to vector w */
				state.bodies[i].rot_vel.x =
					c->data.planet.rotation_axis.x *
					c->data.planet.rotation_speed;
				state.bodies[i].rot_vel.y =
					c->data.planet.rotation_axis.y *
					c->data.planet.rotation_speed;
				state.bodies[i].rot_vel.z =
					c->data.planet.rotation_axis.z *
					c->data.planet.rotation_speed;

			} else if (c->type == BHS_CELESTIAL_STAR) {
				state.bodies[i].radius = 696340000.0;
				state.bodies[i].inertia =
					0.07 * state.bodies[i].mass *
					state.bodies[i].radius *
					state.bodies[i]
						.radius; /* Condensed star */
				// Star rotation could be added here similar to planet if star component has it
			}
		}

		entity_map[i] = id;
		state.n_bodies++;
	}

	/* 2. Run High-Fidelity Integrator (Yoshida - 4th Order Symplectic) */
	/* This handles Gravity (N^2), 1PN Relativity, and Symplectic Integration */
	/* NOTE: Yoshida function in integrator.c is NOT updated for Torque yet (only RK4/Leapfrog).
       Ideally switch to LEAPFROG for rotational dynamics stability or update Yoshida.
       Let's stick to LEAPFROG which I updated. */
	bhs_integrator_leapfrog(
		&state,
		dt); /* [CHANGED] Yoshida -> Leapfrog for Rotational Physics support */

	/* 3. Write back to ECS */
	for (int i = 0; i < state.n_bodies; i++) {
		bhs_entity_id eid = entity_map[i];
		// Only update if not static (though integrator handles fixed flag,
		// we double check to avoid dirtying cache lines if needed)
		if (state.bodies[i].is_fixed)
			continue;

		bhs_transform_t *t =
			bhs_ecs_get_component(world, eid, BHS_COMP_TRANSFORM);
		bhs_physics_t *p =
			bhs_ecs_get_component(world, eid, BHS_COMP_PHYSICS);
		bhs_celestial_component *c =
			bhs_ecs_get_component(world, eid, BHS_COMP_CELESTIAL);

		if (t)
			t->position = state.bodies[i].pos;
		if (p)
			p->velocity = state.bodies[i].vel;

		/* [NEW] Write back Rotation (Angular Velocity & Angle) */
		if (c && c->type == BHS_CELESTIAL_PLANET) {
			/* Update Speed (Magnitude of w) */
			double w2 = state.bodies[i].rot_vel.x *
					    state.bodies[i].rot_vel.x +
				    state.bodies[i].rot_vel.y *
					    state.bodies[i].rot_vel.y +
				    state.bodies[i].rot_vel.z *
					    state.bodies[i].rot_vel.z;
			double w = sqrt(w2);
			c->data.planet.rotation_speed = w;

			/* Update Axis (Normalized w) */
			if (w > 1e-15) {
				c->data.planet.rotation_axis.x =
					state.bodies[i].rot_vel.x / w;
				c->data.planet.rotation_axis.y =
					state.bodies[i].rot_vel.y / w;
				c->data.planet.rotation_axis.z =
					state.bodies[i].rot_vel.z / w;
			}

			/* Integrate Angle Scalar (theta += w * dt) */
			c->data.planet.current_rotation_angle += w * dt;
			/* Wrap */
			if (c->data.planet.current_rotation_angle > 6.2831853)
				c->data.planet.current_rotation_angle -=
					6.2831853;
		}
	}
}
