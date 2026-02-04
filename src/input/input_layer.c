/**
 * @file input_layer.c
 * @brief Implementação da Camada de Input
 *
 * "Se o input não funcionar, é bug do hardware, não do código."
 * (mentira, é sempre bug do código)
 *
 * Mapeamentos de teclas:
 *   WASD   - Movimento da câmera (frente/trás/strafe)
 *   QE     - Subir/Descer
 *   Mouse  - Rotação (click + drag)
 *   Space  - Toggle pause
 *   S      - QuickSave
 *   L      - QuickLoad
 *   ESC    - Quit (ou menu futuro)
 *   +/-    - Ajusta time scale
 *   Delete - Remove corpo selecionado
 */

#include "input_layer.h"
#include "src/app_state.h"
#include "src/ui/camera/camera_controller.h"
#include "src/simulation/components/sim_components.h" /* [NEW] For visual flags */

#include "engine/ecs/ecs.h"
#include "engine/scene/scene.h"
#include "gui/log.h"
#include "gui/ui/lib.h"
#include "math/units.h"
#include "simulation/scenario_mgr.h" /* [NEW] Persistence API */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * HELPERS DE PROJEÇÃO (para picking)
 * ============================================================================
 */

#include "math/mat4.h"
#include "src/ui/render/spacetime_renderer.h"
#include "src/ui/render/visual_utils.h"

/* ============================================================================
 * HANDLERS ESPECÍFICOS
 * ============================================================================
 */

/**
 * Processa inputs de câmera (WASD + mouse)
 */
static void handle_camera_input(struct app_state *app, double dt)
{
	/* Sincroniza estado do HUD com a Câmera */
	if (app->hud.top_down_view != app->camera.is_top_down_mode) {
		app->camera.is_top_down_mode = app->hud.top_down_view;

		if (app->camera.is_top_down_mode) {
			/* Entrando em Top-Down: Reset para altitude alta, olhando para baixo */
			app->camera.x = 0.0;
			app->camera.z = 0.0;
			app->camera.y = 1.0e13; /* ~66 UA - Boa visão geral */
			app->camera.pitch = -1.57079632679f; /* -90 graus */
			app->camera.yaw = 0.0f;
		} else {
			/* Saindo: Restaura pitch confortável */
			app->camera.pitch = -0.3f;
		}
	}

	/* Força trava de pitch se estiver no modo (segurança redundante) */
	if (app->camera.is_top_down_mode) {
		app->camera.pitch = -1.57079632679f;
		/* Não travamos X/Z para o usuário poder navegar (pan) */
	}

	/* Delega pro camera_controller existente (apenas se não estiver em modo foco) */
	if (!app->hud.fixed_planet_cam || app->hud.selected_body_index == -1) {
		bhs_camera_controller_update(&app->camera, app->ui, dt);
	}
}

/**
 * Processa controles de simulação (pause, time scale)
 */
static void handle_simulation_input(struct app_state *app, double dt)
{
	(void)dt; /* Agora não usado após remoção do grid control */
	/* Toggle pause com Space */
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_SPACE)) {
		app_toggle_pause(app);
		BHS_LOG_INFO("Simulação %s", app->sim_status == APP_SIM_PAUSED
						     ? "PAUSADA"
						     : "RODANDO");
	}

	/*
	 * Time scale com teclas numéricas
	 * Nota: BHS_KEY_PLUS e BHS_KEY_MINUS não existem no lib.h
	 * Usando 1-5 como atalhos de velocidade
	 */
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_1))
		app_set_time_scale(app, 0.25);
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_2))
		app_set_time_scale(app, 0.5);
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_3))
		app_set_time_scale(app, 1.0);
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_4))
		app_set_time_scale(app, 2.0);
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_5))
		app_set_time_scale(app, 4.0);
}

/**
 * Processa ações globais (save, load, quit)
 */
static void handle_global_input(struct app_state *app)
{
	/* QuickSave REMOVIDO a pedido do usuário */
	/* if (bhs_ui_key_pressed(app->ui, BHS_KEY_S) ... */

	/* QuickLoad */
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_L)) {
		BHS_LOG_INFO("Carregando mundo...");
		/* [FIX] Por enquanto mantém L mas aponta para arquivo de debug se houver. 
		   Botão de Reload Workspace é o caminho canônico agora. */
	}

	/* Quit com ESC */
	if (bhs_ui_key_pressed(app->ui, BHS_KEY_ESCAPE))
		app->should_quit = true;
}

/**
 * Processa interação com objetos (seleção e deleção)
 */
/* Helper duplicado: Gravity Depth removido (Agora em visual_utils.h) */

static void handle_object_interaction(struct app_state *app)
{
	int win_w, win_h;
	bhs_ui_get_size(app->ui, &win_w, &win_h);

	/* Deleção de corpo selecionado */
	if (app->hud.req_delete_body) {
		if (app->hud.selected_body_index != -1) {
			bhs_scene_remove_body(app->scene,
					      app->hud.selected_body_index);
			app->hud.selected_body_index = -1;
		}
		app->hud.req_delete_body = false;
	}

	/* Injeção de novo corpo (via HUD) */
	if (app->hud.req_add_body_type != -1 ||
	    app->hud.req_add_registry_entry) {
		float spawn_dist = 20.0f;
		struct bhs_vec3 pos = {
			.x = app->camera.x + sinf(app->camera.yaw) * spawn_dist,
			.y = 0.0f,
			.z = app->camera.z + cosf(app->camera.yaw) * spawn_dist
		};

		struct bhs_body new_body = { 0 };

		/* Caso A: Do Registry */
		if (app->hud.req_add_registry_entry) {
			const struct bhs_planet_desc desc =
				app->hud.req_add_registry_entry->getter();

			new_body = bhs_body_create_from_desc(&desc, pos);

			/* Usa Conversão Canônica de Unidades (SI -> Sim) - REMOVIDO para Escala SI */
			new_body.state.mass = new_body.state.mass;
			new_body.state.radius = new_body.state.radius;

			if (new_body.state.mass < 1e-10)
				new_body.state.mass = 1e-10;

			app->hud.req_add_registry_entry = NULL;
		}
		/* Caso B: Tipo hardcoded */
		else {
			struct bhs_vec3 col = { (float)rand() / (float)RAND_MAX,
						(float)rand() / (float)RAND_MAX,
						(float)rand() /
							(float)RAND_MAX };
			double mass = 0.1;
			double radius = 0.5;

			if (app->hud.req_add_body_type == BHS_BODY_STAR) {
				mass = 2.0e30;	/* Massa Solar */
				radius = 7.0e8; /* Raio Solar */
				col = (struct bhs_vec3){ 1.0, 0.8, 0.2 };
			} else if (app->hud.req_add_body_type ==
				   BHS_BODY_BLACKHOLE) {
				mass = 1.0e31; /* 5 Massas Solares */
				radius =
					30000.0; /* Raio de Schwarzschild aprox */
				col = (struct bhs_vec3){ 0.0, 0.0, 0.0 };
			}

			new_body = bhs_body_create_planet_simple(pos, mass,
								 radius, col);
			new_body.type = app->hud.req_add_body_type;
		}

		/* Auto-orbit para planetas */
		if (new_body.type == BHS_BODY_PLANET) {
			int n_bodies;
			const struct bhs_body *bodies =
				bhs_scene_get_bodies(app->scene, &n_bodies);
			double central_mass = 0;

			for (int i = 0; i < n_bodies; i++) {
				if (bodies[i].type == BHS_BODY_BLACKHOLE ||
				    bodies[i].type == BHS_BODY_STAR) {
					central_mass += bodies[i].state.mass;
				}
			}

			if (central_mass > 0) {
				double r = sqrt(pos.x * pos.x + pos.z * pos.z);
				if (r > 0.1) {
					double v_orb = sqrt(central_mass / r);
					new_body.state.vel.x =
						-pos.z / r * v_orb;
					new_body.state.vel.z =
						pos.x / r * v_orb;
				}
			}
		}

		/* Nome default */
		if (new_body.name[0] == '\0') {
			if (new_body.type == BHS_BODY_PLANET)
				strncpy(new_body.name, "Planeta", 31);
			else if (new_body.type == BHS_BODY_STAR)
				strncpy(new_body.name, "Estrela", 31);
			else if (new_body.type == BHS_BODY_BLACKHOLE)
				strncpy(new_body.name, "Black Hole", 31);
		}

		bhs_scene_add_body_struct(app->scene, new_body);
		app->hud.req_add_body_type = -1;
	}

	/* Picking com click */
	if (bhs_ui_mouse_clicked(app->ui, 0)) {
		int mx, my;
		bhs_ui_mouse_pos(app->ui, &mx, &my);

		/* Verifica se click foi na HUD */
		if (bhs_hud_is_mouse_over(app->ui, &app->hud, mx, my, win_w,
					  win_h))
			return; /* Ignora picking */

		int n_bodies;
		const struct bhs_body *bodies =
			bhs_scene_get_bodies(app->scene, &n_bodies);

		int best_idx = -1;
		float best_dist = 1e9f;

		for (int i = 0; i < n_bodies; i++) {
			const struct bhs_body *b = &bodies[i];

			/* Filter types */
			if (b->type != BHS_BODY_PLANET &&
			    b->type != BHS_BODY_STAR &&
			    b->type != BHS_BODY_MOON)
				continue;

			/* [NOVO] Checagem de Isolamento */
			if (app->hud.isolate_view &&
			    app->hud.selected_body_index != -1) {
				if (i != app->hud.selected_body_index)
					continue;
			}

			/* [FIX] Aplica Escala Visual usando Helper Compartilhado */
			float visual_x, visual_y, visual_z, visual_radius;
			bhs_visual_calculate_transform(
				b, bodies, n_bodies, app->hud.visual_mode,
				&visual_x, &visual_y, &visual_z,
				&visual_radius);

			/* Projeta usando coordenadas absolutas */
			float sx, sy;
			bhs_project_point(&app->camera, visual_x, visual_y,
					  visual_z, (float)win_w, (float)win_h,
					  &sx, &sy);

			/* Só processa se estiver na tela */
			if (sx < -100 || sx > win_w + 100 || sy < -100 ||
			    sy > win_h + 100)
				continue;

			/* Tamanho Visual / Distância */
			float dx = visual_x - app->camera.x;
			float dy = visual_y - app->camera.y;
			float dz = visual_z - app->camera.z;
			float dist_to_cam = sqrtf(dx * dx + dy * dy + dz * dz);

			/* Raio de Picking do Corpo */
			float s_radius = 2.0f;
			if (dist_to_cam > 0.1f) {
				s_radius = (visual_radius / dist_to_cam) *
					   app->camera.fov;
			}
			float pick_radius =
				s_radius *
				1.5f; /* Largura um pouco maior pra facilitar */
			if (pick_radius < 15.0f)
				pick_radius = 15.0f;
			if (pick_radius > 200.0f)
				pick_radius = 200.0f;

			/* Checagem de Distância (Esfera) */
			float d2 = (sx - (float)mx) * (sx - (float)mx) +
				   (sy - (float)my) * (sy - (float)my);
			float radius_sq = pick_radius * pick_radius;

			bool hit = (d2 < radius_sq);

			/* Picking de Label de Texto */
			if (!hit) {
				const char *label =
					(b->name[0]) ? b->name : "Planet";
				float font_size = 15.0f; /* Match renderer */
				float tw = bhs_ui_measure_text(app->ui, label,
							       font_size);

				/* Retângulo de Texto: Centralizado em sx, abaixo por s_radius + 5.0f */
				float tx = sx - tw * 0.5f;
				float ty = sy + s_radius + 5.0f;
				float th = font_size;

				/* Hit Test Box (com padding extra) */
				if (mx >= tx - 5 && mx <= tx + tw + 5 &&
				    my >= ty - 5 && my <= ty + th + 5) {
					hit = true;
				}
			}

			if (hit && d2 < best_dist) {
				best_dist = d2;
				best_idx = i;
			}
		}

		/* [NOVO] Picking de Marcadores de Órbita (se não clicou em corpo próximo) */
		int best_marker = bhs_orbit_markers_get_at_screen(
			&app->orbit_markers, (float)mx, (float)my, &app->camera,
			win_w, win_h);

		/* Prioridade: se clicou em corpo, limpa marcador. Se clicou em marcador e não em corpo, seleciona. */
		if (best_idx != -1) {
			app->hud.selected_body_index = best_idx;
			app->hud.selected_marker_index = -1;
		} else if (best_marker != -1) {
			app->hud.selected_marker_index = best_marker;
			app->hud.selected_body_index = -1;
		} else {
			/* Clicou no vazio: limpa ambos */
			app->hud.selected_body_index = -1;
			app->hud.selected_marker_index = -1;
		}
	}

	/* Atualiza cache do corpo selecionado - MOVIDO PARA APP_STATE (Frame update) */
	/* if (app->hud.selected_body_index != -1) ... */
}

/**
 * Processa comandos vindos da HUD (Botões)
 */
static void handle_hud_commands(struct app_state *app)
{
	if (app->hud.req_save_snapshot) {
		if (scenario_save_snapshot(app)) {
			BHS_LOG_INFO("Snapshot salvo via HUD.");
		}
		app->hud.req_save_snapshot = false;
	}

	if (app->hud.req_reload_workspace) {
		if (scenario_reload_current(app)) {
			BHS_LOG_INFO("Workspace recarregado via HUD.");
		}
		app->hud.req_reload_workspace = false;
	}

	/* [FIX] Handle Visual Flag Toggle Request */
	if (app->hud.req_toggle_visual_bit) {
		/* Access ECS World directly to update component state */
		bhs_world_handle world = bhs_scene_get_world(app->scene);
		
		if (world && app->hud.req_visual_entity != BHS_ENTITY_INVALID) {
			bhs_celestial_component *comp = bhs_ecs_get_component(
				world, app->hud.req_visual_entity, BHS_COMP_CELESTIAL);
			
			if (comp) {
				/* Toggle bit on the persistent component */
				if (comp->visual_flags & app->hud.req_visual_mask) {
					comp->visual_flags &= ~app->hud.req_visual_mask;
				} else {
					comp->visual_flags |= app->hud.req_visual_mask;
				}
				BHS_LOG_INFO("Visual Flags updated for Entity %d: 0x%X", 
					app->hud.req_visual_entity, comp->visual_flags);
			} else {
				BHS_LOG_WARN("Failed to get Celestial Component for Ent %d", 
					app->hud.req_visual_entity);
			}
		}

		/* Still update transient cache for instant feedback in ONE frame 
		   (though next frame scene update will re-read from ECS anyway) */
		if (app->hud.selected_body_index != -1) {
			if (app->hud.selected_body_cache.entity_id == app->hud.req_visual_entity) {
				/* Update cache too */
				if (app->hud.selected_body_cache.visual_flags & app->hud.req_visual_mask) {
					app->hud.selected_body_cache.visual_flags &= ~app->hud.req_visual_mask;
				} else {
					app->hud.selected_body_cache.visual_flags |= app->hud.req_visual_mask;
				}
			}
		}
		
		app->hud.req_toggle_visual_bit = false;
	}
}

/* ============================================================================
 * API PÚBLICA
 * ============================================================================
 */

void input_process_frame(struct app_state *app, double dt)
{
	if (!app || !app->ui)
		return;

	/* 
	 * 1. Comandos Globais & Requests do HUD (Sempre ativo) 
	 * Mesmo com foco, queremos que ESC funcione (a menos que queiramos bloquear? 
	 * Por enquanto, mantém por segurança).
	 */
	handle_global_input(app);
	handle_hud_commands(app); /* Processa requests da UI (Save, etc) */

	/* 2. Checa Estado de Bloqueio da UI */
	/* Se um campo de texto tem foco, bloqueia COMPLETAMENTE comandos de simulação (WASD, etc) */
	if (app->hud.input_focused) {
		return;
	}

	/* 3. Checa Mouse Sobre UI para interações espaciais */
	int32_t mx, my, win_w, win_h;
	bhs_ui_mouse_pos(app->ui, &mx, &my);
	bhs_ui_get_size(app->ui, &win_w, &win_h);
	bool mouse_on_ui =
		bhs_hud_is_mouse_over(app->ui, &app->hud, mx, my, win_w, win_h);

	/* 
	 * 4. Controles de Simulação (Atalhos de Tempo, etc)
	 * Geralmente bloqueado se sobre UI para evitar mudanças de velocidade acidentais ao clicar em botões 
	 */
	if (!mouse_on_ui) {
		handle_simulation_input(app, dt);
	}

	/* 5. Interações Espaciais (Câmera & Seleção de objeto) */
	if (!mouse_on_ui) {
		handle_camera_input(app, dt);
		handle_object_interaction(app);
	} else {
		/* 
		 * Caso Especial: Mesmo se o mouse estiver na UI, podemos querer deixar a seleção 
		 * limpar (ex: handle_object_interaction chama bhs_hud_is_mouse_over internamente).
		 * Mas como já temos a flag, apenas pulamos.
		 */
	}
}
