/**
 * @file world.c
 * @brief Gerenciamento do Mundo ECS
 */

#include "engine/ecs/ecs.h"

static bhs_world_handle g_world = NULL;

void bhs_world_init(void)
{
	g_world = bhs_ecs_create_world();
}

bhs_world_handle bhs_world_get(void)
{
	return g_world;
}
