/**
 * @file scene.c
 * @brief Implementação do Orquestrador (ECS Adapter)
 */

#include "engine/scene/scene.h"
#include "engine/components/components.h"
#include "engine/ecs/ecs.h"
// #include "engine/physics/spacetime/spacetime.h" /* REMOVED */
#include "src/simulation/components/sim_components.h" // Game components
/* 
 * NÃO INCLUA "src/simulation/presets/presets.h" AQUI!
 * Bibliotecas de engine não devem depender da aplicação. 
 * Se você fizer isso de novo, Linus vai pessoalmente apagar seu repositório.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void bhs_engine_init(void);

// Temporary storage for legacy get_bodies
#define MAX_BODIES 128
static struct bhs_body g_legacy_bodies[MAX_BODIES];
// static bhs_spacetime_t g_spacetime_cache = NULL; /* REMOVED */

extern bhs_world_handle
bhs_engine_get_world_internal(void); // Need a way to get world if opaque

// Hack: we need access to the world handle hidden in engine.c
// Ideally engine.h would expose a getter for internal systems, or scene is part of engine.
// Let's assume for this Refactor that we can access it via a helper or scene creates it if engine isn't init?
// Actually engine_init creates it.
// We will add `bhs_engine_get_world()` to engine.h or just rely on global?
// Let's declare it external here for "Friend" access within engine module.
// In real engine, this would be cleaner.
bhs_world_handle bhs_engine_get_world_unsafe(void)
{
	/* 
	 * Retorna o world do engine. Se você usar isso pra fazer merda, 
	 * a culpa é do seu professor de C que não te ensinou encapsulamento. 
	 */
	extern bhs_world_handle bhs_engine_get_world_internal(void);
	return bhs_engine_get_world_internal();
}

struct bhs_scene_impl {
	bhs_world_handle world;
	// bhs_spacetime_t spacetime; /* REMOVED */
};

bhs_scene_t bhs_scene_create(void)
{
	// Ensure engine is up
	bhs_engine_init();

	bhs_scene_t scene = calloc(1, sizeof(struct bhs_scene_impl));
	if (!scene)
		return NULL;

	// We don't get the world handle easily?
	// Let's modify engine.c to expose `bhs_engine_get_world()`?
	// Or scene IS the owner of logic?
	// Plan: Engine Core owns world. Scene is just a high level helper.
	// For now, let's assume we can get the world or we rely on engine global state commands.

	/* Spacetime Grid removed - relocated to app_state visualization logic */
	// scene->spacetime = bhs_spacetime_create(500.0, 80);
	// g_spacetime_cache = scene->spacetime;

	// Connect to Engine World
	extern bhs_world_handle bhs_engine_get_world_internal(void);
	scene->world = bhs_engine_get_world_internal();

	return scene;
}

void bhs_scene_destroy(bhs_scene_t scene)
{
	if (!scene)
		return;
	if (!scene)
		return;
	// if (scene->spacetime) bhs_spacetime_destroy(scene->spacetime);
	free(scene);
}

void bhs_scene_init_default(bhs_scene_t scene)
{
	/* 
	 * A engine agora entrega uma cena limpa. 
	 * Se você quer planetas, popule a cena no seu código de aplicação.
	 * O teste de integração agora faz isso manualmente.
	 */
	(void)scene;
}

void bhs_scene_update(bhs_scene_t scene, double dt)
{
	(void)scene;
	// Run Engine Update
	bhs_engine_update(dt);

	// Sync spacetime (visual)
	// We need to pass bodies to spacetime.
	// This is where we reconstruct the legacy body array for visualization tools
	// that haven't been ported to ECS.
	// Sync spacetime (visual) -> Moved to App State
	int count = 0;
	// const struct bhs_body *bodies = bhs_scene_get_bodies(scene, &count); /* UNUSED NOW */
	(void)count; /* Suppress unused warning for count if needed */
	//     bhs_spacetime_update(scene->spacetime, bodies, count);
	// }
}

/* bhs_scene_get_spacetime removed */

bhs_world_handle bhs_scene_get_world(bhs_scene_t scene)
{
	return scene ? scene->world : NULL;
}

// LEGACY ADAPTER: Reconstruct bhs_body structs from ECS components
const struct bhs_body *bhs_scene_get_bodies(bhs_scene_t scene, int *count)
{
	if (!scene || !scene->world) {
		*count = 0;
		return NULL;
	}
	bhs_world_handle world = scene->world;

	bhs_ecs_query q;
	// Query movable things
	bhs_ecs_query_init(
		&q, world,
		(1 << BHS_COMP_TRANSFORM)); // Get all with transform?

	int idx = 0;
	bhs_entity_id id;
	while (bhs_ecs_query_next(&q, &id) && idx < MAX_BODIES) {
		bhs_transform_t *t =
			bhs_ecs_get_component(world, id, BHS_COMP_TRANSFORM);
		bhs_physics_t *p =
			bhs_ecs_get_component(world, id, BHS_COMP_PHYSICS);
		bhs_celestial_component *c = bhs_ecs_get_component(
			world, id, BHS_COMP_CELESTIAL); // Defined in src?
		// Wait, Engine doesn't know BHS_COMP_CELESTIAL unless I include it.
		// using ID cast.

		if (!t)
			continue;

		struct bhs_body *b = &g_legacy_bodies[idx];

		/* [FIX] Manage Trail Memory Ownership (Dynamic 2M points) */
		if (b->entity_id != 0 && b->entity_id != id) {
			if (b->trail_positions) {
				free(b->trail_positions);
				b->trail_positions = NULL;
			}
			b->trail_head = 0;
			b->trail_count = 0;
		}
		b->entity_id = id; /* [NOVO] Store ID */

		// Map Transform
		b->state.pos = t->position;
		// Map Physics
		if (p) {
			b->state.vel = p->velocity;
			b->state.mass = p->mass;
			b->is_fixed = p->is_static;
			b->is_alive = true; // ECS only returns alive
		} else {
			b->state.vel = (struct bhs_vec3){ 0 };
			b->state.mass = 0;
		}

		// Map Celestial
		if (c) {
			snprintf(b->name, sizeof(b->name), "%.31s", c->name);
			b->visual_flags =
				c->visual_flags; /* [NOVO] Copy flags */

			if (c->type == BHS_CELESTIAL_PLANET) {
				b->type = BHS_BODY_PLANET;
				b->state.radius = c->data.planet.radius;
				b->color = c->data.planet.color;
				b->prop.planet.density = c->data.planet.density;
				b->prop.planet.has_atmosphere =
					c->data.planet.has_atmosphere;

				/* Map Rotation State */
				b->state.rot_speed =
					c->data.planet.rotation_speed;
				b->state.rot_axis =
					c->data.planet.rotation_axis;
				b->state.current_rotation_angle =
					c->data.planet.current_rotation_angle;

				/* [FIX] Reconstruct Properties for UI (Inspector) */
				/* Tilt = angle between Up(0,1,0) and Axis? 
                   Factory: axis = (sin(tilt), cos(tilt), 0)
                   So tilt = atan2(x, y) */
				b->prop.planet.axis_tilt =
					atan2(b->state.rot_axis.x,
					      b->state.rot_axis.y);

				/* Period = 2pi / speed */
				if (fabs(b->state.rot_speed) > 1e-9) {
					b->prop.planet.rotation_period =
						(2.0 * 3.14159265359) /
						b->state.rot_speed;
				} else {
					b->prop.planet.rotation_period = 0.0;
				}

			} else if (c->type == BHS_CELESTIAL_STAR) {
				b->type = BHS_BODY_STAR;
				b->prop.star.luminosity =
					c->data.star.luminosity;
				b->prop.star.temp_effective =
					c->data.star.temp_effective;
				b->color = c->data.star.color;
				// Star radius is visual only in transform, but we can store if needed.
				// For now, rely on Transform scale as master for radius.
				b->state.radius = t->scale.x;
			} else if (c->type == BHS_CELESTIAL_BLACKHOLE) {
				b->type = BHS_BODY_BLACKHOLE;
			}
		} else {
			// Fallback defaults
			b->type = BHS_BODY_ASTEROID;
			b->state.radius = 1.0;
			b->color = (struct bhs_vec3){ 1, 1, 1 };
		}

		idx++;
	}

	*count = idx;
	return g_legacy_bodies;
}

// Full-Fidelity Create
bhs_entity_id bhs_scene_add_body_struct(bhs_scene_t scene, struct bhs_body b)
{
	extern bhs_world_handle bhs_engine_get_world_internal(void);
	bhs_world_handle world = bhs_engine_get_world_internal();
	if (!world)
		return BHS_ENTITY_INVALID;
	(void)scene;

	bhs_entity_id e = bhs_ecs_create_entity(world);

	// Transform
	bhs_transform_t t = {
		.position = b.state.pos,
		.scale = { b.state.radius, b.state.radius, b.state.radius },
		.rotation = { 0, 0, 0, 1 } // Quat identity
	};
	// Rotation from axis/angle if needed, but simplified for now
	bhs_ecs_add_component(world, e, BHS_COMP_TRANSFORM, sizeof(t), &t);

	// Physics
	bhs_physics_t p = { .mass = b.state.mass,
			    .velocity = b.state.vel,
			    .is_static = b.is_fixed };
	bhs_ecs_add_component(world, e, BHS_COMP_PHYSICS, sizeof(p), &p);

	// Celestial Logic
	bhs_celestial_component c = { 0 };
	snprintf(c.name, sizeof(c.name), "%s", b.name);
	
	/* Default Flags: Enable All by default for consistency, allow UI to toggle off via Global */
	/* User wants global off by default, but local customizable. 
       Let's enable all LOCAL flags so that they are "Ready" to be shown.
       The renderer will do `global || local`? No, user said "Activate all... can cause clutter".
       And "Default deactivated".
       If I set local=true, and global=false, `global || local` is true. Visibile.
       So default local must be FALSE if we want them hidden.
       But user wants to enable specifics.
       Wait, if I set local=false, and global=false -> Hidden.
       User clicks "Global View" -> global=true -> All visible.
       User clicks "Planet X" -> local=true -> Only X visible.
       This works. Default needs to be FALSE.
    */
    c.visual_flags = 0; /* Starts clean */

	if (b.type == BHS_BODY_PLANET) {
		c.type = BHS_CELESTIAL_PLANET;
		c.data.planet.radius = b.state.radius;
		c.data.planet.color = b.color;
		c.data.planet.density = b.prop.planet.density;
		c.data.planet.has_atmosphere = b.prop.planet.has_atmosphere;

		/* Rotation Logic - Use calculated state from factory */
		c.data.planet.rotation_speed = b.state.rot_speed;
		c.data.planet.rotation_axis = b.state.rot_axis;

		/* [DEBUG] Force initialization of current angle */
		c.data.planet.current_rotation_angle = 0.0;

	} else if (b.type == BHS_BODY_STAR) {
		c.type = BHS_CELESTIAL_STAR;
		c.data.star.luminosity = b.prop.star.luminosity;
		c.data.star.temp_effective = b.prop.star.temp_effective;
		c.data.star.color = b.color;
	} else if (b.type == BHS_BODY_BLACKHOLE) {
		c.type = BHS_CELESTIAL_BLACKHOLE;
		// bh mechanics...
	} else {
		c.type = BHS_CELESTIAL_ASTEROID;
	}

	bhs_ecs_add_component(world, e, BHS_COMP_CELESTIAL, sizeof(c), &c);

	return e;
}

/* Contador global para nomes únicos (lazy, mas funciona) */
static int g_planet_counter = 0;
static int g_star_counter = 0;
static int g_asteroid_counter = 0;

void bhs_scene_reset_counters(void)
{
	g_planet_counter = 0;
	g_star_counter = 0;
	g_asteroid_counter = 0;
}

bhs_entity_id bhs_scene_add_body(bhs_scene_t scene, enum bhs_body_type type,
				 struct bhs_vec3 pos, struct bhs_vec3 vel,
				 double mass, double radius,
				 struct bhs_vec3 color)
{
	/* Gera nome único baseado no tipo */
	char name[64];
	switch (type) {
	case BHS_BODY_PLANET:
		snprintf(name, sizeof(name), "Planet %d", ++g_planet_counter);
		break;
	case BHS_BODY_STAR:
		snprintf(name, sizeof(name), "Star %d", ++g_star_counter);
		break;
	case BHS_BODY_BLACKHOLE:
		snprintf(name, sizeof(name), "Black Hole");
		break;
	case BHS_BODY_ASTEROID:
		snprintf(name, sizeof(name), "Asteroid %d",
			 ++g_asteroid_counter);
		break;
	default:
		snprintf(name, sizeof(name), "Body");
		break;
	}
	return bhs_scene_add_body_named(scene, type, pos, vel, mass, radius,
					color, name);
}

bhs_entity_id bhs_scene_add_body_named(bhs_scene_t scene,
				       enum bhs_body_type type,
				       struct bhs_vec3 pos, struct bhs_vec3 vel,
				       double mass, double radius,
				       struct bhs_vec3 color, const char *name)
{
	extern bhs_world_handle bhs_engine_get_world_internal(void);
	bhs_world_handle world = bhs_engine_get_world_internal();
	if (!world)
		return BHS_ENTITY_INVALID;
	(void)scene; // Mark unused

	bhs_entity_id e = bhs_ecs_create_entity(world);

	// Transform
	bhs_transform_t t = { .position = pos,
			      .scale = { radius, radius,
					 radius }, // Approximate visual scale
			      .rotation = { 0, 0, 0, 1 } };
	bhs_ecs_add_component(world, e, BHS_COMP_TRANSFORM, sizeof(t), &t);

	// Physics
	bhs_physics_t p = { .mass = mass,
			    .velocity = vel,
			    .is_static = (type == BHS_BODY_BLACKHOLE) };
	bhs_ecs_add_component(world, e, BHS_COMP_PHYSICS, sizeof(p), &p);

	// Celestial
	bhs_celestial_component c = { 0 };
	c.visual_flags = 0; /* Default Hidden */
	if (type == BHS_BODY_PLANET) {
		c.type = BHS_CELESTIAL_PLANET;
		c.data.planet.radius = radius;
		c.data.planet.color = color;
		snprintf(c.name, sizeof(c.name), "%s", name ? name : "Planet");
	} else if (type == BHS_BODY_BLACKHOLE) {
		c.type = BHS_CELESTIAL_BLACKHOLE;
		snprintf(c.name, sizeof(c.name), "%s",
			 name ? name : "Black Hole");
	} else if (type == BHS_BODY_STAR) {
		c.type = BHS_CELESTIAL_STAR;
		snprintf(c.name, sizeof(c.name), "%s", name ? name : "Star");
	} else {
		c.type = BHS_CELESTIAL_ASTEROID;
		snprintf(c.name, sizeof(c.name), "%s",
			 name ? name : "Asteroid");
	}
	// Note: BHS_COMP_CELESTIAL ID must match what is used in src/
	// Since we included sim_components.h which defines BHS_COMP_CELESTIAL, use that.
	bhs_ecs_add_component(world, e, BHS_COMP_CELESTIAL, sizeof(c), &c);

	return e;
}

void bhs_scene_remove_body(bhs_scene_t scene, int index)
{
	if (!scene || !scene->world)
		return;
	bhs_world_handle world = scene->world;

	bhs_ecs_query q;
	// Query movable things - Same mask as get_bodies
	bhs_ecs_query_init(&q, world, (1 << BHS_COMP_TRANSFORM));

	int idx = 0;
	bhs_entity_id id;
	bool found = false;

	while (bhs_ecs_query_next(&q, &id)) {
		bhs_transform_t *t =
			bhs_ecs_get_component(world, id, BHS_COMP_TRANSFORM);
		if (!t)
			continue;

		if (idx == index) {
			found = true;
			break; /* Found our victim */
		}
		idx++;
	}

	if (found) {
		printf("[SCENE] Destroying entity ID %d (Index %d)\n", id,
		       index);
		bhs_ecs_destroy_entity(world, id);
	} else {
		printf("[SCENE] Failed to delete: Index %d not found.\n",
		       index);
	}
}

void bhs_scene_clear_legacy_cache(void)
{
	/* Limpa todo o cache legado, incluindo históricos de trail */
	for (int i = 0; i < MAX_BODIES; i++) {
		if (g_legacy_bodies[i].trail_positions) {
			free(g_legacy_bodies[i].trail_positions);
			g_legacy_bodies[i].trail_positions = NULL;
		}
	}
	memset(g_legacy_bodies, 0, sizeof(g_legacy_bodies));
}
