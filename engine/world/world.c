/**
 * @file world.c
 * @brief Gerenciamento do Mundo ECS
 */

#include "engine/ecs/ecs.h"

static ri_world_handle g_world = NULL;

void ri_world_init(void)
{
	g_world = ri_ecs_create_world();
}

ri_world_handle ri_world_get(void)
{
	return g_world;
}
