/**
 * @file engine.c
 * @brief Implementação do Core da Engine
 */

#include "engine/engine.h"
#include "engine/components/components.h"
#include "engine/ecs/ecs.h"
#include "engine/scene/scene.h"
#include "engine/physics/physics_system.h"

#include <stddef.h>

/* Estado Global da Engine */
static struct {
	ri_world_handle world;
	bool is_initialized;
} g_engine = { 0 };

/* Acesso friend interno */
ri_world_handle ri_engine_get_world_internal(void)
{
	return g_engine.world;
}

void ri_engine_init(void)
{
	if (g_engine.is_initialized)
		return;

	/* 1. Init Memória/Arenas (Futuro) */

	/* 2. Init ECS */
	g_engine.world = ri_ecs_create_world();

	/* 3. Registrar Componentes (Se dinâmico, mas usamos IDs estáticos) */

	g_engine.is_initialized = true;
}

void ri_engine_shutdown(void)
{
	if (!g_engine.is_initialized)
		return;

	ri_ecs_destroy_world(g_engine.world);
	g_engine.world = NULL;
	g_engine.is_initialized = false;
}

void ri_engine_update(double dt)
{
	if (!g_engine.is_initialized)
		return;
	(void)dt;

	/* 1. Integração de Física - DESATIVADO (Gerenciado pela Camada de Simulação) */
	// ri_physics_system_update(g_engine.world, dt);

	/* 2. Atualizações de Espaço-Tempo (Métrica) */

	/* 3. Lógica de Jogo / Scripting */
}

void ri_scene_load(const char *path)
{
	// Placeholder: Carregamento hardcoded por enquanto
	// Em implementação real, isso faria parse de JSON/Binário
	(void)path;

	// Example: Create Earth
	ri_entity_id earth = ri_ecs_create_entity(g_engine.world);

	ri_transform_t t = { .position = { 0, 0, 0 },
			      .scale = { 1, 1, 1 },
			      .rotation = { 0, 0, 0, 1 } };
	ri_ecs_add_component(g_engine.world, earth, RI_COMP_TRANSFORM,
			      sizeof(t), &t);

	ri_physics_t p = { .mass = 5.97e24,
			    .velocity = { 0, 0, 0 },
			    .is_static = false };
	ri_ecs_add_component(g_engine.world, earth, RI_COMP_PHYSICS,
			      sizeof(p), &p);
}

/* ============================================================================
 * CONFIGURAÇÃO DE FÁBRICA (Estabilização)
 * ============================================================================
 */
ri_entity_id ri_entity_create_massive_body(struct ri_vec3 pos,
					     struct ri_vec3 vel, double mass,
					     double radius,
					     struct ri_vec3 color, int type)
{
	(void)color;
	if (!g_engine.is_initialized)
		return 0;

	ri_world_handle world = g_engine.world;
	ri_entity_id e = ri_ecs_create_entity(world);

	/* 1. Transformada */
	ri_transform_t t = { .position = pos,
			      .scale = { radius, radius, radius },
			      .rotation = { 0, 0, 0, 1 } };
	ri_ecs_add_component(world, e, RI_COMP_TRANSFORM, sizeof(t), &t);

	/* 2. Física */
	ri_physics_t p = {
		.mass = mass,
		.velocity = vel,
		.is_static = (type == 3) // 3 = RI_BODY_BLACKHOLE
	};
	ri_ecs_add_component(world, e, RI_COMP_PHYSICS, sizeof(p), &p);

	return e;
}
