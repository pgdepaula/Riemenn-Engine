/**
 * @file simulation_init.c
 * @brief Inicialização do Conteúdo da Simulação (Game Logic)
 */

#include "src/simulation/simulation_init.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine/scene/scene.h"
#include "math/vec4.h" // for bhs_vec3
#include "src/simulation/presets/presets.h"

void bhs_simulation_init(bhs_scene_t scene)
{
	if (!scene)
		return;

	/*
     * Modos de inicialização via Env Var
     */
	const char *debug_env = getenv("BHS_DEBUG_SCENE");

	// Spacetime config is now inside Scene for visual grid,
	// but Engine Core handles physics.
	// If we want to change grid size, we might need a scene API.
	// Assuming defaults for now or API expansion.

	if (debug_env && debug_env[0] == '2') {
		/* Sistema Solar */
		printf("[SIMULATION] Modo SOLAR ativado.\n");
		bhs_preset_solar_system(scene);
	} else if (debug_env && debug_env[0] == '1') {
		/* Sistema Debug Simples */
		printf("[SIMULATION] Modo Debug Simples.\n");

		// Manual creation using legacy shim
		struct bhs_vec3 center = { 0, 0, 0 };
		struct bhs_vec3 zero = { 0, 0, 0 };
		struct bhs_vec3 black = { 0, 0, 0 };

		bhs_scene_add_body(scene, BHS_BODY_BLACKHOLE, center, zero,
				   10.0, 2.0, black);

		double r = 15.0;
		double v = sqrt(10.0 / r); // v = sqrt(M/r) for G=1
		bhs_scene_add_body(scene, BHS_BODY_PLANET,
				   (struct bhs_vec3){ r, 0, 0 },
				   (struct bhs_vec3){ 0, 0, v }, 0.1, 0.5,
				   (struct bhs_vec3){ 0.3, 0.5, 1.0 });

		r = 25.0;
		v = sqrt(10.0 / r);
		bhs_scene_add_body(scene, BHS_BODY_PLANET,
				   (struct bhs_vec3){ 0, 0, r },
				   (struct bhs_vec3){ v, 0, 0 }, 0.15, 0.6,
				   (struct bhs_vec3){ 1.0, 0.5, 0.3 });
	}
}
