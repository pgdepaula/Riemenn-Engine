/**
 * @file celestial_system.c
 * @brief Implementação do Sistema Celestial
 *
 * "Quando duas estrelas colidem, a física grita.
 * Eu escuto e transformo o grito em supernova."
 */

#include "game/simulation/systems/celestial_system.h"
#include <math.h>
#include <stdio.h>
#include "engine/components/components.h"
#include "engine/ecs/events.h"
#include "engine/scene/scene.h"
#include "game/simulation/components/sim_components.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ============================================================================
 * LOGICA DE ATUALIZAÇÃO
 * ============================================================================
 */

void ri_celestial_system_update(ri_scene_t scene, double dt)
{
	ri_world_handle world = ri_scene_get_world(scene);
	if (!world)
		return;

	ri_ecs_query q;
	/* Query all entities with CELESTIAL component */
	ri_ecs_query_init(&q, world, (1 << RI_COMP_CELESTIAL));

	ri_entity_id id;
	while (ri_ecs_query_next(&q, &id)) {
		ri_celestial_component *c =
			ri_ecs_get_component(world, id, RI_COMP_CELESTIAL);
		if (!c)
			continue;

		/* Update Planet Rotation */
		if (c->type == RI_CELESTIAL_PLANET) {
			/* Handle Tidal Locking */
			ri_orbital_component *orb = ri_ecs_get_component(
				world, id, RI_COMP_ORBITAL);
			if (orb && (orb->flags & RI_ORBITAL_FLAG_TIDAL_LOCK)) {
				/* Synchronous Rotation: spin period = orbital period */
				if (orb->period > 0.1) {
					c->data.planet.rotation_speed =
						(2.0 * M_PI) / orb->period;
				}
			}

			c->data.planet.current_rotation_angle +=
				c->data.planet.rotation_speed * dt;

			/* Normalize 0..2PI */
			if (c->data.planet.current_rotation_angle >
			    2.0 * M_PI) {
				c->data.planet.current_rotation_angle = fmod(
					c->data.planet.current_rotation_angle,
					2.0 * M_PI);
			} else if (c->data.planet.current_rotation_angle <
				   0.0) {
				/* Handle retrograde negative overflow */
				c->data.planet.current_rotation_angle =
					fmod(c->data.planet
						     .current_rotation_angle,
					     2.0 * M_PI) +
					2.0 * M_PI;
			}
		}
		/* Simplesmente ignore Stars for now, or rotate them too */
	}
}

/* ============================================================================
 * CALLBACKS DE EVENTOS
 * ============================================================================
 */

/**
 * Handler para eventos de colisão.
 * Verifica o tipo de corpos envolvidos e reage apropriadamente.
 */
static void on_collision(ri_world_handle world, enum ri_event_type type,
			 const void *data, void *user_data)
{
	(void)type; /* Sabemos que é COLLISION */
	(void)user_data;

	const struct ri_collision_event *ev = data;
	if (!ev)
		return;

	/* Tenta obter componente Celestial de ambas entidades */
	ri_celestial_component *cel_a =
		ri_ecs_get_component(world, ev->entity_a, RI_COMP_CELESTIAL);
	ri_celestial_component *cel_b =
		ri_ecs_get_component(world, ev->entity_b, RI_COMP_CELESTIAL);

	/* Se nenhum tem componente celestial, não nos interessa */
	if (!cel_a && !cel_b)
		return;

	/* ========== COLISÃO COM BURACO NEGRO ========== */
	bool a_is_bh = cel_a && cel_a->type == RI_CELESTIAL_BLACKHOLE;
	bool b_is_bh = cel_b && cel_b->type == RI_CELESTIAL_BLACKHOLE;

	if (a_is_bh || b_is_bh) {
		ri_entity_id blackhole = a_is_bh ? ev->entity_a : ev->entity_b;
		ri_entity_id victim = a_is_bh ? ev->entity_b : ev->entity_a;

		printf("[CELESTIAL] Entidade %u foi devorada pelo buraco negro "
		       "%u. "
		       "F pra pagar respeito.\n",
		       victim, blackhole);

		/* Transfere massa pro buraco negro (se vítima tem física) */
		ri_physics_t *ph_victim =
			ri_ecs_get_component(world, victim, RI_COMP_PHYSICS);
		ri_physics_t *ph_bh = ri_ecs_get_component(world, blackhole,
							     RI_COMP_PHYSICS);

		if (ph_victim && ph_bh) {
			ph_bh->mass += ph_victim->mass;
			ph_bh->inverse_mass = 1.0 / ph_bh->mass;
			printf("[CELESTIAL] Buraco negro absorveu %.2f kg. "
			       "Nova massa: %.2f kg\n",
			       ph_victim->mass, ph_bh->mass);
		}

		ri_ecs_destroy_entity(world, victim);
		return;
	}

	/* ========== COLISÃO ESTRELA + ESTRELA ========== */
	bool a_is_star = cel_a && cel_a->type == RI_CELESTIAL_STAR;
	bool b_is_star = cel_b && cel_b->type == RI_CELESTIAL_STAR;

	if (a_is_star && b_is_star) {
		printf("[CELESTIAL] Fusao estelar detectada entre %u e %u! "
		       "SUPERNOVA INCOMING!\n",
		       ev->entity_a, ev->entity_b);

		/*
		 * TODO: Spawn supernova na posição de contato
		 * Por enquanto, só destruímos ambas.
		 * Podemos criar um remanescente (buraco negro ou anã branca)
		 * baseado na massa total.
		 */
		ri_physics_t *ph_a = ri_ecs_get_component(world, ev->entity_a,
							    RI_COMP_PHYSICS);
		ri_physics_t *ph_b = ri_ecs_get_component(world, ev->entity_b,
							    RI_COMP_PHYSICS);

		real_t total_mass = 0;
		if (ph_a)
			total_mass += ph_a->mass;
		if (ph_b)
			total_mass += ph_b->mass;

		/* Massa alta -> buraco negro, baixa -> anã branca */
		if (total_mass > 25.0) {
			printf("[CELESTIAL] Massa combinada %.2f > 25 Msol. "
			       "Criando buraco negro...\n",
			       total_mass);
			/* TODO: Spawn buraco negro no contact_point */
		} else {
			printf("[CELESTIAL] Massa combinada %.2f < 25 Msol. "
			       "Criando ana branca...\n",
			       total_mass);
			/* TODO: Spawn anã branca */
		}

		ri_ecs_destroy_entity(world, ev->entity_a);
		ri_ecs_destroy_entity(world, ev->entity_b);
		return;
	}

	/* ========== OUTRAS COLISÕES ========== */
	/* Planeta + Planeta, Asteroide + qualquer coisa, etc */
	printf("[CELESTIAL] Colisao generica entre %u e %u. "
	       "Implementar logica especifica aqui.\n",
	       ev->entity_a, ev->entity_b);
}

/* ============================================================================
 * API PÚBLICA
 * ============================================================================
 */

void ri_celestial_system_init(ri_world_handle world)
{
	int ret = ri_ecs_subscribe(world, RI_EVENT_COLLISION, on_collision,
				    NULL);
	if (ret < 0) {
		fprintf(stderr,
			"[CELESTIAL] Falha ao registrar listener de colisão\n");
		return;
	}

	printf("[CELESTIAL] Sistema inicializado. Escutando eventos de "
	       "colisão.\n");
}

void ri_celestial_system_shutdown(ri_world_handle world)
{
	ri_ecs_unsubscribe(world, RI_EVENT_COLLISION, on_collision);
	printf("[CELESTIAL] Sistema finalizado.\n");
}
