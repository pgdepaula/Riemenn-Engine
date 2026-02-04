/**
 * @file main.c
 * @brief Ponto de Entrada - O Nirvana do Minimalismo
 *
 * "30 linhas de código que movem o universo."
 *
 * Este arquivo é propositalmente minimalista. Toda a lógica real
 * está em app_state.c, scenario_mgr.c e input_layer.c.
 *
 * Se você está editando este arquivo, provavelmente está fazendo errado.
 * A não ser que esteja mudando o cenário inicial. Aí pode.
 */

#include "app_state.h"
#include "gui/log.h"
#include "simulation/scenario_mgr.h"

#include <stdlib.h>

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	/* 1. Inicializar Aplicação */
	struct app_state app = { 0 };
	if (!app_init(&app, "BlackHole TensorRay", 1920, 1080)) {
		BHS_LOG_FATAL("Falha ao inicializar aplicação");
		return EXIT_FAILURE;
	}

	/* 2. Carregar Cenário Inicial */
	const char *scene_env = getenv("BHS_DEBUG_SCENE");

	if (scene_env) {
		enum scenario_type scenario = SCENARIO_DEBUG;
		switch (scene_env[0]) {
		case '0':
			scenario = SCENARIO_EMPTY;
			break;
		case '1':
			scenario = SCENARIO_DEBUG;
			break;
		case '2':
			scenario = SCENARIO_SOLAR_SYSTEM;
			break;
		case '3':
			scenario = SCENARIO_EARTH_SUN;
			break;
		case '4':
			scenario = SCENARIO_KERR_BLACKHOLE;
			break;
		case '5':
			scenario = SCENARIO_BINARY_STAR;
			break;
		}
		scenario_load(&app, scenario);
	} else {
		/* [NEW] Por padrão, mostra a tela de início */
		app.sim_status = APP_SIM_START_SCREEN;
	}

	/* 3. Rodar Loop Principal */
	app_run(&app);

	/* 4. Encerrar (Shutdown) */
	app_shutdown(&app);

	return EXIT_SUCCESS;
}
