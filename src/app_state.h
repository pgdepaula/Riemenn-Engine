/**
 * @file app_state.h
 * @brief Estado Global da Aplicação - O Cérebro do Maestro
 *
 * "Uma struct pra governar todas, uma struct pra encontrá-las,
 *  uma struct pra a todas trazer e na escuridão aprisioná-las."
 *
 * Este arquivo centraliza TODO o estado da aplicação. Nada de variáveis
 * globais soltas por aí como se fosse código de estagiário.
 *
 * Invariantes:
 * - Após app_init(), todos os ponteiros são válidos ou NULL com erro tratado
 * - Camera sempre tem valores sãos (fov > 0, etc)
 * - sim_status só muda via funções de controle
 */

#ifndef BHS_SRC_APP_STATE_H
#define BHS_SRC_APP_STATE_H

#include <stdbool.h>
#include "engine/scene/scene.h"
#include "gui/rhi/rhi.h"
#include "gui/ui/lib.h"
#include "src/simulation/data/orbit_marker.h" /* [NOVO] Sistema de marcadores */
#include "src/ui/camera/camera.h"
#include "src/ui/render/planet_renderer.h"
#include "src/ui/screens/hud.h"

/* ============================================================================
 * ENUMS DE ESTADO
 * ============================================================================
 */

/**
 * Estado da simulação - pausado ou rodando
 */
enum app_sim_state {
	APP_SIM_RUNNING,
	APP_SIM_PAUSED,
	APP_SIM_START_SCREEN /* [NEW] Tela de boot estilizada */
};

/**
 * Tipo de cenário carregado
 */
enum app_scenario {
	APP_SCENARIO_NONE = 0,
	APP_SCENARIO_SOLAR_SYSTEM,
	APP_SCENARIO_KERR_BLACKHOLE,
	APP_SCENARIO_BINARY_STAR,
	APP_SCENARIO_DEBUG
};

/* ============================================================================
 * ESTRUTURA PRINCIPAL
 * ============================================================================
 */

/**
 * struct app_state - O estado completo da aplicação
 *
 * Invariantes:
 * - ui != NULL após init bem sucedido
 * - scene != NULL após init bem sucedido
 * - time_scale > 0.0 sempre
 *
 * Ciclo de vida:
 * 1. Zerada com {0}
 * 2. app_init() preenche tudo
 * 3. app_run() usa até should_quit
 * 4. app_shutdown() limpa
 */
struct app_state {
	/* ---- Subsistemas ---- */
	bhs_ui_ctx_t ui;   /* gui: Janela, GPU, Input */
	bhs_scene_t scene; /* Engine: ECS, Physics, Bodies */

	/* ---- Assets de Rendering ---- */
	bhs_gpu_texture_t bg_tex;     /* Textura do skybox */
	bhs_gpu_texture_t sphere_tex; /* Impostor de esfera (fallback) */

	/* Texture Cache (Name -> GPU Texture) */
	struct {
		char name[32];
		bhs_gpu_texture_t tex;
	} tex_cache[32];
	int tex_cache_count;

	/* ---- Compute Passes ---- */
	struct bhs_blackhole_pass *bh_pass;  /* [NOVO] Handle opaco */
	struct bhs_planet_pass *planet_pass; /* [NOVO] Renderizador 3D */

	/* ---- Estado da Câmera ---- */
	bhs_camera_t camera; /* Posição, rotação, FOV */

	/* ---- Controle de Simulação ---- */
	enum app_sim_state sim_status; /* Running/Paused */
	enum app_scenario scenario;    /* Cenário atual */
	double time_scale;	       /* Multiplicador de tempo (1.0 = real) */
	double accumulated_time;       /* Tempo total simulado */

	/* ---- Estado de UI ---- */
	bhs_hud_state_t hud; /* HUD: menus, seleção, etc */

	/* ---- [NOVO] Sistema de Marcadores de Órbita ---- */
	struct bhs_orbit_marker_system orbit_markers;

	/* ---- Timing / Profiling ---- */
	double last_frame_time; /* Timestamp do último frame */
	double phys_ms;		/* ms gastos em física */
	double render_ms;	/* ms gastos em rendering */
	int frame_count;	/* Contador de frames */

	/* ---- Flags de Controle ---- */
	bool should_quit; /* Hora de ir embora */

	/* ---- [NOVO] Estado de Persistência ---- */
	char current_workspace
		[256]; /* Caminho do arquivo carregado (para Recarregar) */
};

/* ============================================================================
 * API DE CICLO DE VIDA
 * ============================================================================
 */

/**
 * app_init - Inicializa TUDO: logs, gui, engine, scene, câmera
 * @app: Estado da aplicação (deve estar zerado)
 * @title: Título da janela
 * @width: Largura inicial
 * @height: Altura inicial
 *
 * Faz o boot completo do sistema em cascata. Se falhar em qualquer
 * ponto, limpa o que já foi alocado e retorna false.
 *
 * Ordem de inicialização:
 * 1. Logging
 * 2. Scene/Engine memory
 * 3. gui/UI (Window + Vulkan)
 * 4. Assets (texturas)
 * 5. Camera (valores padrão)
 * 6. HUD state
 *
 * Retorna: true se sucesso, false se algo explodiu
 */
bool app_init(struct app_state *app, const char *title, int width, int height);

/**
 * app_run - Loop principal da aplicação
 * @app: Estado inicializado
 *
 * Implementa o game loop com:
 * - Input polling
 * - Fixed timestep para física
 * - Rendering interpolado
 *
 * Retorna quando app->should_quit == true
 */
void app_run(struct app_state *app);

/**
 * app_shutdown - Limpa tudo na ordem correta
 * @app: Estado para destruir
 *
 * Ordem de cleanup (inversa do init):
 * 1. Texturas
 * 2. UI/gui
 * 3. Scene/Engine
 * 4. Logging
 */
void app_shutdown(struct app_state *app);

/* ============================================================================
 * API DE CONTROLE DE SIMULAÇÃO
 * ============================================================================
 */

/**
 * app_toggle_pause - Alterna entre pausado e rodando
 */
static inline void app_toggle_pause(struct app_state *app)
{
	if (!app)
		return;
	app->sim_status = (app->sim_status == APP_SIM_RUNNING)
				  ? APP_SIM_PAUSED
				  : APP_SIM_RUNNING;
}

/**
 * app_set_time_scale - Define escala de tempo
 * @scale: Multiplicador (1.0 = tempo real, 0.5 = metade, 2.0 = dobro)
 *
 * Clampado entre 0.1 e 100.0 pra não fazer merda
 */
static inline void app_set_time_scale(struct app_state *app, double scale)
{
	if (!app)
		return;
	if (scale < 0.1)
		scale = 0.1;
	/*
	 * MAX: 1e6 = ~694 dias/minuto = ~1.9 anos por minuto real.
	 * Suficiente pra ver órbitas planetárias em segundos.
	 * O limite anterior de 100x era completamente inútil pra astronomia.
	 */
	if (scale > 1.0e6)
		scale = 1.0e6;
	app->time_scale = scale;
}

#endif /* BHS_SRC_APP_STATE_H */
