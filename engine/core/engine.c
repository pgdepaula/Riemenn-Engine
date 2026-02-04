/**
 * @file engine.c
 * @brief Implementação do Core da Engine
 */

#include "engine/engine.h"
#include "engine/components/components.h"
#include "engine/ecs/ecs.h"
#include "engine/scene/scene.h"
#include "engine/systems/physics_system.h"

#include <stddef.h>

/* Estado Global da Engine */
static struct {
	bhs_world_handle world;
	bool is_initialized;
} g_engine = { 0 };

/* Acesso friend interno */
bhs_world_handle bhs_engine_get_world_internal(void)
{
	return g_engine.world;
}

void bhs_engine_init(void)
{
	if (g_engine.is_initialized)
		return;

	/* 1. Init Memória/Arenas (Futuro) */

	/* 2. Init ECS */
	g_engine.world = bhs_ecs_create_world();

	/* 3. Registrar Componentes (Se dinâmico, mas usamos IDs estáticos) */

	g_engine.is_initialized = true;
}

void bhs_engine_shutdown(void)
{
	if (!g_engine.is_initialized)
		return;

	bhs_ecs_destroy_world(g_engine.world);
	g_engine.world = NULL;
	g_engine.is_initialized = false;
}

void bhs_engine_update(double dt)
{
	if (!g_engine.is_initialized)
		return;
	(void)dt;

	/* 1. Integração de Física - DESATIVADO (Gerenciado pela Camada de Simulação) */
	// bhs_physics_system_update(g_engine.world, dt);

	/* 2. Atualizações de Espaço-Tempo (Métrica) */

	/* 3. Lógica de Jogo / Scripting */
}

void bhs_scene_load(const char *path)
{
	// Placeholder: Carregamento hardcoded por enquanto
	// Em implementação real, isso faria parse de JSON/Binário
	(void)path;

	// Example: Create Earth
	bhs_entity_id earth = bhs_ecs_create_entity(g_engine.world);

	bhs_transform_t t = { .position = { 0, 0, 0 },
			      .scale = { 1, 1, 1 },
			      .rotation = { 0, 0, 0, 1 } };
	bhs_ecs_add_component(g_engine.world, earth, BHS_COMP_TRANSFORM,
			      sizeof(t), &t);

	bhs_physics_t p = { .mass = 5.97e24,
			    .velocity = { 0, 0, 0 },
			    .is_static = false };
	bhs_ecs_add_component(g_engine.world, earth, BHS_COMP_PHYSICS,
			      sizeof(p), &p);
}

/* ============================================================================
 * CONFIGURAÇÃO DE FÁBRICA (Estabilização)
 * ============================================================================
 */
bhs_entity_id bhs_entity_create_massive_body(struct bhs_vec3 pos,
					     struct bhs_vec3 vel, double mass,
					     double radius,
					     struct bhs_vec3 color, int type)
{
	(void)color;
	if (!g_engine.is_initialized)
		return 0;

	bhs_world_handle world = g_engine.world;
	bhs_entity_id e = bhs_ecs_create_entity(world);

	/* 1. Transformada */
	bhs_transform_t t = { .position = pos,
			      .scale = { radius, radius, radius },
			      .rotation = { 0, 0, 0, 1 } };
	bhs_ecs_add_component(world, e, BHS_COMP_TRANSFORM, sizeof(t), &t);

	/* 2. Física */
	bhs_physics_t p = {
		.mass = mass,
		.velocity = vel,
		.is_static = (type == 3) // 3 = BHS_BODY_BLACKHOLE
	};
	bhs_ecs_add_component(world, e, BHS_COMP_PHYSICS, sizeof(p), &p);

	return e;
}
