/**
 * @file scenario_mgr.c
 * @brief Implementação do Gerenciador de Cenários
 *
 * "Cada cenário é um universo. E cada universo precisa de um Deus.
 *  Nesse caso, o Deus é você. Parabéns pela responsabilidade."
 */

#include "scenario_mgr.h"
#include "src/app_state.h"
#include "src/simulation/presets/presets.h"

#include "engine/scene/scene.h"
#include "gui/log.h"
#include "math/vec4.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * HELPERS
 * ============================================================================
 */

/**
 * Reposiciona câmera para visualização inicial do cenário
 */
static void set_camera_for_scenario(struct app_state *app,
				    enum scenario_type type)
{
	if (!app)
		return;

	switch (type) {
	case SCENARIO_SOLAR_SYSTEM:
		/* Câmera acima do plano orbital, olhando pro centro (Top Down) */
		app->camera.x = 0.0f;
		app->camera.y = 2.0e11f; /* Alta altitude */
		app->camera.z = 0.0f;
		app->camera.pitch =
			-1.570796f; /* -90 graus (Olhando pra baixo) */
		app->camera.yaw = 0.0f;
		app->camera.fov = 1000.0f;
		app->camera.is_top_down_mode = true; /* [NEW] Start Top Down */
		break;

	case SCENARIO_EARTH_SUN:
		/* Câmera focada em observar a distância */
		app->camera.x = 0.0f;
		app->camera.y = 2.0e11f; /* Bem acima (1.3 AU) */
		app->camera.z = 0.0f;
		app->camera.pitch = -1.57f; /* Olhando para baixo (90 graus) */
		app->camera.yaw = 0.0f;
		app->camera.fov = 1000.0f;
		break;

	case SCENARIO_JUPITER_PLUTO_PULL:
		/* Câmera focada em Júpiter (5.2 AU) */
		/* Júpiter r = 778.5e9 m. */
		/* Queremos estar um pouco "atrás" e "acima" */
		app->camera.x = 7.785e11f; /* Perto de Júpiter em X */
		app->camera.y = 5.0e9f;	   /* Um pouco acima */
		app->camera.z = -2.0e10f;  /* Atrás (zoom in) */
		/* Na verdade o preset coloca Júpiter em X orbital... 
		   Vamos usar uma visão geral primeiro. */
		app->camera.x = 7.0e11f;
		app->camera.y = 1.0e11f;
		app->camera.z = -1.0e11f;
		app->camera.pitch = -0.6f;
		app->camera.yaw = 0.0f;
		app->camera.fov = 2000.0f;
		break;

	case SCENARIO_KERR_BLACKHOLE:
		/* Perto do evento horizon */
		app->camera.x = 15.0f;
		app->camera.y = 5.0f;
		app->camera.z = -20.0f;
		app->camera.pitch = -0.1f;
		app->camera.yaw = 0.0f;
		app->camera.fov = 500.0f;
		break;

	case SCENARIO_BINARY_STAR:
		/* Vista lateral do sistema binário */
		app->camera.x = 0.0f;
		app->camera.y = 30.0f;
		app->camera.z = -60.0f;
		app->camera.pitch = -0.3f;
		app->camera.yaw = 0.0f;
		app->camera.fov = 500.0f;
		break;

	case SCENARIO_DEBUG:
	default:
		/* Posição debug padrão */
		app->camera.x = 0.0f;
		app->camera.y = 20.0f;
		app->camera.z = -40.0f;
		app->camera.pitch = -0.3f;
		app->camera.yaw = 0.0f;
		app->camera.fov = 500.0f;
		app->camera.is_top_down_mode = false;
		break;
	}
}

/* ============================================================================
 * LOADERS DE CENÁRIO
 * ============================================================================
 */

static bool load_solar_system(struct app_state *app)
{
	BHS_LOG_INFO("Carregando Sistema Solar...");
	bhs_preset_solar_system(app->scene);
	return true;
}

static bool load_earth_sun(struct app_state *app)
{
	BHS_LOG_INFO("Carregando Sol, Terra e Lua...");
	bhs_preset_earth_moon_sun(app->scene);
	return true;
}

static bool load_kerr_blackhole(struct app_state *app)
{
	BHS_LOG_INFO("Carregando Black Hole Kerr...");

	struct bhs_vec3 center = { 0, 0, 0 };
	struct bhs_vec3 zero = { 0, 0, 0 };
	struct bhs_vec3 black = { 0, 0, 0 };

	/* Black hole central */
	bhs_scene_add_body(app->scene, BHS_BODY_BLACKHOLE, center, zero, 10.0,
			   2.0, black);

	/* Partículas em órbita */
	for (int i = 0; i < 8; i++) {
		double angle = (double)i * 3.14159 / 4.0;
		double r = 15.0 + (double)i * 3.0;
		double v = sqrt(10.0 / r);

		struct bhs_vec3 pos = { .x = r * cos(angle),
					.y = 0.0,
					.z = r * sin(angle) };
		struct bhs_vec3 vel = { .x = -v * sin(angle),
					.y = 0.0,
					.z = v * cos(angle) };
		struct bhs_vec3 col = { .x = 0.3f + 0.1f * i,
					.y = 0.5f,
					.z = 1.0f - 0.1f * i };

		bhs_scene_add_body(app->scene, BHS_BODY_PLANET, pos, vel, 0.1,
				   0.5, col);
	}

	return true;
}

static bool load_binary_star(struct app_state *app)
{
	BHS_LOG_INFO("Carregando Sistema Binário...");

	/* Duas estrelas orbitando centro de massa */
	double separation = 20.0;
	double m1 = 5.0, m2 = 3.0;
	double total_m = m1 + m2;
	double r1 = separation * m2 / total_m;
	double r2 = separation * m1 / total_m;
	double v_orb = sqrt(total_m / separation);

	struct bhs_vec3 yellow = { 1.0, 0.9, 0.3 };
	struct bhs_vec3 orange = { 1.0, 0.5, 0.2 };

	/* Estrela 1 */
	bhs_scene_add_body(app->scene, BHS_BODY_STAR,
			   (struct bhs_vec3){ -r1, 0, 0 },
			   (struct bhs_vec3){ 0, 0, v_orb * r1 / separation },
			   m1, 2.0, yellow);

	/* Estrela 2 */
	bhs_scene_add_body(app->scene, BHS_BODY_STAR,
			   (struct bhs_vec3){ r2, 0, 0 },
			   (struct bhs_vec3){ 0, 0, -v_orb * r2 / separation },
			   m2, 1.5, orange);

	/* Planeta orbitando o sistema */
	double r_planet = 50.0;
	double v_planet = sqrt(total_m / r_planet);
	bhs_scene_add_body(app->scene, BHS_BODY_PLANET,
			   (struct bhs_vec3){ r_planet, 0, 0 },
			   (struct bhs_vec3){ 0, 0, v_planet }, 0.1, 0.4,
			   (struct bhs_vec3){ 0.3, 0.5, 1.0 });

	return true;
}

static bool load_debug(struct app_state *app)
{
	BHS_LOG_INFO("Carregando cenário de debug (SINGLE PLANET)...");

	/* Single Planet at (30, 0, 0) */
	/* Radius 1.0 (will be scaled x30 visually = 30.0) */
	/* Should be HUGE on screen if camera is at (0, 20, -40) looking at (0,0,0) */
	/* Wait, (30, 0, 0) is to the RIGHT. */

	/* Let's put it DEAD CENTER 0,0,0 to be sure */
	bhs_scene_add_body(app->scene, BHS_BODY_PLANET,
			   (struct bhs_vec3){ 0, 0, 0 }, /* Pos 0,0,0 */
			   (struct bhs_vec3){ 0, 0, 0 }, /* Static */
			   1.0, 1.0,			 /* Mass, Radius=1.0 */
			   (struct bhs_vec3){ 1.0, 0.0, 1.0 }
			   /* Magenta Color */
	);

	/* REFERENCE PLANET (Blue) at (20, 0, 0) */
	bhs_scene_add_body(app->scene, BHS_BODY_PLANET,
			   (struct bhs_vec3){ 30.0, 0, 0 }, /* Pos 30,0,0 */
			   (struct bhs_vec3){ 0, 0, 0 },    /* Static */
			   1.0, 1.0, /* Mass, Radius=1.0 */
			   (struct bhs_vec3){ 0.0, 1.0, 1.0 } /* Cyan Color */
	);

	return true;
}

static bool load_earth_moon_only(struct app_state *app)
{
	BHS_LOG_INFO("Carregando cenário Terra-Lua (Isolado)...");
	bhs_preset_earth_moon_only(app->scene);
	return true;
}

static bool load_jupiter_pluto_pull(struct app_state *app)
{
	BHS_LOG_INFO("Carregando cenário Júpiter & Plutão Pull...");
	bhs_preset_jupiter_pluto_pull(app->scene);
	return true;
}

/* ============================================================================
 * API PÚBLICA
 * ============================================================================
 */

bool scenario_load(struct app_state *app, enum scenario_type type)
{
	if (!app || !app->scene) {
		BHS_LOG_ERROR("scenario_load: app ou scene nulos");
		return false;
	}

	/* Limpa cenário anterior */
	scenario_unload(app);

	/* [FIX] Force clear workspace path so we don't think we are in a file */
	if (type != SCENARIO_EMPTY) {
		app->current_workspace[0] = '\0';
	}

	/* Carrega o novo */
	bool ok = false;

	BHS_LOG_INFO("scenario_load: Loading Checkpoint A (Type=%d)", type);

	switch (type) {
	case SCENARIO_EMPTY:
		ok = true; /* Já está vazio */
		break;
	case SCENARIO_SOLAR_SYSTEM:
		ok = load_solar_system(app);
		break;
	case SCENARIO_EARTH_SUN:
		ok = load_earth_sun(app);
		break;
	case SCENARIO_EARTH_MOON_ONLY:
		ok = load_earth_moon_only(app);
		break;
	case SCENARIO_JUPITER_PLUTO_PULL:
		ok = load_jupiter_pluto_pull(app);
		break;
	case SCENARIO_KERR_BLACKHOLE:
		ok = load_kerr_blackhole(app);
		break;
	case SCENARIO_BINARY_STAR:
		ok = load_binary_star(app);
		break;
	case SCENARIO_DEBUG:
		ok = load_debug(app);
		break;
	default:
		BHS_LOG_WARN("Cenário desconhecido: %d", type);
		return false;
	}

	BHS_LOG_INFO("scenario_load: Checkpoint B (OK=%d)", ok);

	if (ok) {
		/* Mapear para enum do app_state */
		switch (type) {
		case SCENARIO_SOLAR_SYSTEM:
			app->scenario = APP_SCENARIO_SOLAR_SYSTEM;
			break;
		case SCENARIO_EARTH_SUN:
			/* Reusando SOLAR_SYSTEM para app state por enquanto, ou NONE */
			app->scenario = APP_SCENARIO_SOLAR_SYSTEM;
			break;
		case SCENARIO_EARTH_MOON_ONLY:
			app->scenario =
				APP_SCENARIO_SOLAR_SYSTEM; /* Reusing solar system state for now */
			break;
		case SCENARIO_JUPITER_PLUTO_PULL:
			app->scenario =
				APP_SCENARIO_SOLAR_SYSTEM; /* Uses solar system rendering constants */
			break;
		case SCENARIO_KERR_BLACKHOLE:
			app->scenario = APP_SCENARIO_KERR_BLACKHOLE;
			break;
		case SCENARIO_BINARY_STAR:
			app->scenario = APP_SCENARIO_BINARY_STAR;
			break;
		case SCENARIO_DEBUG:
			app->scenario = APP_SCENARIO_DEBUG;
			break;
		default:
			app->scenario = APP_SCENARIO_NONE;
			break;
		}
		set_camera_for_scenario(app, type);
		app->accumulated_time = 0.0;
		BHS_LOG_INFO("scenario_load: Time reset to 0.0");

		BHS_LOG_INFO("Cenário '%s' carregado com sucesso",
			     scenario_get_name(type));
	} else {
		BHS_LOG_ERROR("scenario_load: Failed to load scenario type %d",
			      type);
	}

	return ok;
}

void scenario_unload(struct app_state *app)
{
	if (!app || !app->scene)
		return;

	/*
	 * TODO: Implementar bhs_scene_clear() na engine
	 * Por enquanto, removemos corpo por corpo (ineficiente mas funciona)
	 */
	int count = 0;
	bhs_scene_get_bodies(app->scene, &count);
	BHS_LOG_INFO("scenario_unload: Cleaning up %d bodies...", count);

	int safety = 0;
	while (count > 0 && safety++ < 1000) {
		bhs_scene_remove_body(app->scene, 0);	  // Always remove head
		bhs_scene_get_bodies(app->scene, &count); // Update count
	}

	if (count > 0) {
		BHS_LOG_ERROR("scenario_unload: Failed to remove all bodies. "
			      "Remaining: %d",
			      count);
	}

	/* Reseta contadores de nomes para próximo cenário começar do 1 */
	bhs_scene_reset_counters();

	/* Reseta marcadores de órbita para evitar "trails" fantasmas da simulação anterior */
	bhs_orbit_markers_init(&app->orbit_markers);
	
	/* Limpa cache legado (trails azuis) */
	bhs_scene_clear_legacy_cache();

	app->scenario = APP_SCENARIO_NONE;
}

const char *scenario_get_name(enum scenario_type type)
{
	switch (type) {
	case SCENARIO_EMPTY:
		return "Espaço Vazio";
	case SCENARIO_SOLAR_SYSTEM:
		return "Sistema Solar";
	case SCENARIO_EARTH_SUN:
		return "Terra e Sol";
	case SCENARIO_KERR_BLACKHOLE:
		return "Black Hole Kerr";
	case SCENARIO_BINARY_STAR:
		return "Sistema Binário";
	case SCENARIO_DEBUG:
		return "Debug";
	case SCENARIO_EARTH_MOON_ONLY:
		return "Terra e Lua (Isolado)";
	case SCENARIO_JUPITER_PLUTO_PULL:
		return "Júpiter & Plutão Pull";
	default:
		return "Desconhecido";
	}
}

/* ============================================================================
 * PERSISTENCE IMPLEMENTATION
 * ============================================================================
 */

#include <time.h>
#include "src/simulation/components/sim_components.h"

/* Helper to generate filename: data/snapshot_YYYY-MM-DD_HHMMSS.bin */
static void generate_snapshot_filename(char *buf, size_t size)
{
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	snprintf(buf, size, "data/snapshot_%04d-%02d-%02d_%02d%02d%02d.bin",
		 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
		 tm.tm_min, tm.tm_sec);
}

bool scenario_save_snapshot(struct app_state *app)
{
	if (!app || !app->scene)
		return false;

	bhs_world_handle world = bhs_scene_get_world(app->scene);
	if (!world)
		return false;

	/* 1. Prepare Metadata */
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);

	char date_str[64];
	snprintf(date_str, 64, "%04d-%02d-%02d %02d:%02d", tm.tm_year + 1900,
		 tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min);

	char display_name[64];
	if (app->hud.save_input_buf[0] != '\0') {
		strncpy(display_name, app->hud.save_input_buf, 63);
	} else {
		/* Default: "Meu [DATA] [SCENARIO]" */
		/* Simplified: "Meu Save [DATA]" or use Scenario Name */
		const char *scen_name =
			scenario_get_name((enum scenario_type)app->scenario);
		snprintf(display_name, 64, "Meu %04d-%02d-%02d %s",
			 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			 scen_name);
	}

	bhs_entity_id meta_id = bhs_ecs_create_entity(world);

	bhs_metadata_component meta = { .accumulated_time =
						app->accumulated_time,
					.scenario_type = app->scenario,
					.time_scale_snapshot =
						app->time_scale };
	strncpy(meta.display_name, display_name, 63);
	strncpy(meta.date_string, date_str, 63);

	bhs_ecs_add_component(world, meta_id, BHS_COMP_METADATA, sizeof(meta),
			      &meta);

	/* 2. Generate Filename (Unique Timestamp) */
	char filename[256];
	generate_snapshot_filename(filename, sizeof(filename));

	/* 3. Save */
	BHS_LOG_INFO("Salvando Snapshot: %s ('%s')", filename, display_name);
	bool ok = bhs_ecs_save_world(world, filename);

	/* 4. Cleanup Metadata (don't want it persisting in runtime memory) */
	bhs_ecs_destroy_entity(world, meta_id);

	if (ok) {
		/* Update current workspace to this new save */
		snprintf(app->current_workspace, sizeof(app->current_workspace),
			 "%s", filename);
	}

	return ok;
}

bool scenario_load_from_file(struct app_state *app, const char *filename)
{
	if (!app || !app->scene || !filename)
		return false;

	BHS_LOG_INFO("Carregando Workspace: %s", filename);

	/* Unload current content first */
	scenario_unload(app);

	bhs_world_handle world = bhs_scene_get_world(app->scene);

	/* Load Binary */
	if (!bhs_ecs_load_world(world, filename)) {
		BHS_LOG_ERROR("Falha ao carregar arquivo binário.");
		return false;
	}

	/* Search for Metadata & Restore State */
	bhs_ecs_query q;
	bhs_ecs_query_init(&q, world, (1 << BHS_COMP_METADATA));

	bhs_entity_id meta_id;
	if (bhs_ecs_query_next(&q, &meta_id)) {
		bhs_metadata_component *meta = bhs_ecs_get_component(
			world, meta_id, BHS_COMP_METADATA);
		if (meta) {
			app->accumulated_time = meta->accumulated_time;
			app->scenario = meta->scenario_type;
			/* We NOT restore time_scale instantly, we start PAUSED as requested */
			BHS_LOG_INFO(
				"Metadados restaurados: Time=%.2f, Scen=%d",
				app->accumulated_time, app->scenario);
		}

		/* Destroy metadata entity after consuming */
		bhs_ecs_destroy_entity(world, meta_id);
	} else {
		BHS_LOG_WARN("Metadados não encontrados no save (Legacy?). "
			     "Resetando tempo.");
		app->accumulated_time = 0;
		app->scenario = APP_SCENARIO_NONE;
	}

	/* Enforce Rules: Paused & Physics Ready */
	app->sim_status = APP_SIM_PAUSED;

	/* Track file */
	snprintf(app->current_workspace, sizeof(app->current_workspace), "%s",
		 filename);

	/* Setup Camera based on Scenario Type? 
	   If we saved camera state, we'd restore it.
	   Since we don't save camera yet, let's use the scenario type heuristic */
	/* Don't reset camera if reload? Or yes? 
	   If user moved, maybe they want to keep position. 
	   But if loading new file... 
	   Let's leave camera as is for now, user can move. */

	return true;
}

bool scenario_reload_current(struct app_state *app)
{
	if (!app || app->current_workspace[0] == '\0') {
		BHS_LOG_WARN("Nenhum workspace ativo para recarregar.");
		return false;
	}
	return scenario_load_from_file(app, app->current_workspace);
}
