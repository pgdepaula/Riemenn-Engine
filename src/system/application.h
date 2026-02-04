/**
 * @file application.h
 * @brief Startup e Ciclo de Vida da Aplicação
 */

#ifndef BHS_SYSTEM_APPLICATION_H
#define BHS_SYSTEM_APPLICATION_H

#include "engine/scene/scene.h"
#include "gui/rhi/rhi.h"
#include "gui/ui/lib.h"
#include "src/ui/screens/hud.h"

typedef struct {
	bhs_scene_t scene;
	bhs_ui_ctx_t ui;
	bhs_gpu_texture_t bg_tex;
	bhs_gpu_texture_t sphere_tex;
	bhs_hud_state_t hud_state;
	// ... add more if needed
} bhs_app_t;

/**
 * bhs_app_init - Inicializa TUDO (Logs, gui, Engine, Simulação)
 * Retorna true se sucesso.
 */
bool bhs_app_init(bhs_app_t *app);

/**
 * bhs_app_shutdown - Limpa a bagunça.
 */
void bhs_app_shutdown(bhs_app_t *app);

#endif
