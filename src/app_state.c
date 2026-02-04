/**
 * @file app_state.c
 * @brief Implementação do Ciclo de Vida da Aplicação
 *.
 * "O código que faz o circo funcionar sem que a plateia veja os palhaços."
 *
 * Este arquivo implementa o boot, loop principal e shutdown da aplicação.
 * Se você está aqui, provavelmente algo quebrou. Boa sorte.
 */

#include "app_state.h"
#include "simulation/scenario_mgr.h"
#include "simulation/systems/systems.h" // [NOVO] Sistemas Lógicos
#include "system/config.h"		/* [NOVO] Configuração do Sistema */

#include <math.h> /* [NOVO] para powf/fabs */
#include "engine/assets/image_loader.h"
#include "gui/log.h"
#include "gui/rhi/rhi.h"
#include "src/simulation/data/planet.h" /* Registry is here */

#include "src/debug/telemetry.h"
#include "src/input/input_layer.h"
#include "src/ui/render/blackhole_pass.h" /* [NOVO] */
#include "src/ui/render/visual_utils.h"	  /* [NOVO] Transformadas Visuais */
#include "src/ui/screens/start_screen.h"  /* [NOVO] */
#include "src/ui/screens/view_spacetime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* 
 * Timestep fixo pra física - 60 Hz (aprox 16.6ms) ou similar logic.
 * No original estava PHYSICS_DT 60.0, mas geralmente isso é 1/60.
 * Vou manter como estava, mas traduzido.
 * "Fixed timestep for phyiscs"
 */
#define PHYSICS_DT 60.0
#define MAX_FRAME_TIME 0.25 /* Evita espiral da morte (death spiral) */

/* ============================================================================
 * HELPERS
 * ============================================================================
 */

static double get_time_seconds(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* ============================================================================
 * INICIALIZAÇÃO
 * ============================================================================
 */

bool app_init(struct app_state *app, const char *title, int width, int height)
{
	if (!app)
		return false;

	/* ... */

	/* 0. Logging (PRIMEIRO - antes de qualquer log) */
	bhs_log_init();
	bhs_log_set_level(BHS_LOG_LEVEL_DEBUG); /* [DEBUG] Forçar Nível de Debug */
	BHS_LOG_INFO("=== BlackHole TensorRay - Inicializando ===");

	/* [NOVO] Carrega config do usuário */
	bhs_user_config_t user_cfg;
	bhs_config_load(&user_cfg,
			"data/user_config.bin"); /* Carrega arquivo ou padrões */

	/* 1. Scene / Engine Memory */
	BHS_LOG_INFO("Alocando memória da Engine...");
	app->scene = bhs_scene_create();
	if (!app->scene) {
		BHS_LOG_FATAL("Falha ao criar scene!");
		goto err_scene;
	}

	/* 2. gui/UI (Window + Vulkan) */
	BHS_LOG_INFO("Inicializando gui/UI...");
	struct bhs_ui_config config = {
		.title = title ? title : "BlackHole TensorRay",
		.width = width > 0 ? width : 1280,
		.height = height > 0 ? height : 720,
		.resizable = true,
		.vsync =
			user_cfg.vsync_enabled, /* [FIX] Usa config carregada */
		.debug = true
	};

	int ret = bhs_ui_create(&config, &app->ui);
	if (ret != BHS_UI_OK) {
		BHS_LOG_FATAL("Falha ao criar UI: %d", ret);
		goto err_ui;
	}

	/* 3. HUD State */
	bhs_hud_init(&app->hud);

	/* [NOVO] Aplica Config ao HUD */
	app->hud.vsync_enabled = user_cfg.vsync_enabled;
	app->hud.show_fps = user_cfg.show_fps;
	app->hud.time_scale_val = user_cfg.time_scale_val;

	app->hud.visual_mode = (bhs_visual_mode_t)user_cfg.visual_mode;
	app->hud.top_down_view = user_cfg.top_down_view;
	app->hud.show_gravity_line = user_cfg.show_gravity_line;
	app->hud.show_orbit_trail = user_cfg.show_orbit_trail;
	app->hud.show_satellite_orbits = user_cfg.show_satellite_orbits;
	app->hud.show_planet_markers = user_cfg.show_planet_markers;
	app->hud.show_moon_markers = user_cfg.show_moon_markers;

	/* 4. Assets - Skybox */
	BHS_LOG_INFO("Carregando assets...");
	bhs_image_t bg_img = bhs_image_load("src/assets/textures/space_bg.png");
	if (bg_img.data) {
		struct bhs_gpu_texture_config tex_conf = {
			.width = bg_img.width,
			.height = bg_img.height,
			.depth = 1,
			.mip_levels = 1,
			.array_layers = 1,
			.format = BHS_FORMAT_RGBA8_SRGB,
			.usage = BHS_TEXTURE_SAMPLED | BHS_TEXTURE_TRANSFER_DST,
			.label = "Skybox"
		};
		bhs_gpu_device_t dev = bhs_ui_get_gpu_device(app->ui);
		if (bhs_gpu_texture_create(dev, &tex_conf, &app->bg_tex) ==
		    BHS_GPU_OK) {
			bhs_gpu_texture_upload(app->bg_tex, 0, 0, bg_img.data,
					       bg_img.width * bg_img.height *
						       4);
		}
		bhs_image_free(bg_img);
	} else {
		BHS_LOG_WARN("Skybox não encontrado - usando fundo preto");
	}

	/* 4.1. Impostor de Esfera (Sphere Impostor) */
	{
		int size = 64;
		bhs_image_t sphere_img = bhs_image_gen_sphere(size);
		if (sphere_img.data) {
			struct bhs_gpu_texture_config conf = {
				.width = size,
				.height = size,
				.depth = 1,
				.mip_levels = 1,
				.array_layers = 1,
				.format = BHS_FORMAT_RGBA8_UNORM,
				.usage = BHS_TEXTURE_SAMPLED |
					 BHS_TEXTURE_TRANSFER_DST,
				.label = "Sphere Impostor"
			};
			bhs_gpu_device_t dev = bhs_ui_get_gpu_device(app->ui);
			bhs_gpu_texture_create(dev, &conf, &app->sphere_tex);
			if (app->sphere_tex) {
				bhs_gpu_texture_upload(app->sphere_tex, 0, 0,
						       sphere_img.data,
						       size * size * 4);
			}
			bhs_image_free(sphere_img);
		}
	}

	/* 4.1.5. [NOVO] Texturas Procedurais de Planetas (O Despertar) */
	{
		BHS_LOG_INFO("Gerando texturas procedurais dos planetas...");
		bhs_gpu_device_t dev = bhs_ui_get_gpu_device(app->ui);

		const struct bhs_planet_registry_entry *entry =
			bhs_planet_registry_get_head();
		int count = 0;

		while (entry && count < 32) {
			if (entry->getter) {
				struct bhs_planet_desc desc = entry->getter();
				BHS_LOG_INFO("  > Gerando surface: %s...",
					     desc.name);

				/* Gera Textura High-Res (1024x512) */
				/* Nota: Texturas grandes aumentam tempo de boot. Manter modesto por enquanto. */
				bhs_image_t img = bhs_image_gen_planet_texture(
					&desc, 1024,
					512); // Aumentar resolução aqui

				if (img.data) {
					struct bhs_gpu_texture_config conf = {
						.width = img.width,
						.height = img.height,
						.depth = 1,
						.mip_levels = 1,
						.array_layers = 1,
						.format =
							BHS_FORMAT_RGBA8_UNORM, /* Albedo */
						.usage =
							BHS_TEXTURE_SAMPLED |
							BHS_TEXTURE_TRANSFER_DST,
						.label = desc.name
					};

					bhs_gpu_texture_t tex = NULL;
					if (bhs_gpu_texture_create(dev, &conf,
								   &tex) ==
					    BHS_GPU_OK) {
						bhs_gpu_texture_upload(
							tex, 0, 0, img.data,
							img.width * img.height *
								4);

						/* Cache it */
						strncpy(app->tex_cache[count]
								.name,
							desc.name, 31);
						app->tex_cache[count].tex = tex;
						count++;
					}

					bhs_image_free(img);
				}
			}
			entry = entry->next;
		}
		app->tex_cache_count = count;
		BHS_LOG_INFO("Geradas %d texturas de planetas.", count);
	}

	/* 4.2. Passo Buraco Negro (Init) */
	{
		bhs_gpu_device_t dev = bhs_ui_get_gpu_device(app->ui);
		bhs_blackhole_pass_config_t bh_conf = { .width = width,
							.height = height };
		app->bh_pass = bhs_blackhole_pass_create(dev, &bh_conf);
		if (!app->bh_pass) {
			BHS_LOG_WARN("Compute Pass falhou ao iniciar - Shader "
				     "faltando?");
		}
	}

	/* 4.3. [NOVO] Passo Planetário 3D (Planet 3D Pass) */
	if (bhs_planet_pass_create(app->ui, &app->planet_pass) != 0) {
		BHS_LOG_ERROR("Falha ao inicializar renderer de planetas.");
	}

	/* 5. Câmera (valores padrão) */
	bhs_camera_init(&app->camera);

	/* 6. Padrões da Simulação */
	app->sim_status = APP_SIM_RUNNING;
	app->time_scale = 1.0;
	app->accumulated_time = 0.0;
	app->scenario = APP_SCENARIO_NONE;
	app->should_quit = false;

	/* 6.1. [NOVO] Inicializa sistema de marcadores de órbita */
	bhs_orbit_markers_init(&app->orbit_markers);

	/* 7. Temporização (Timing) */
	app->last_frame_time = get_time_seconds();
	app->frame_count = 0;
	app->phys_ms = 0.0;
	app->render_ms = 0.0;

	BHS_LOG_INFO("Inicialização completa. Sistemas online.");
	return true;

err_ui:
	bhs_scene_destroy(app->scene);
	app->scene = NULL;
err_scene:
	bhs_log_shutdown();
	return false;
}

/* ============================================================================
 * LOOP PRINCIPAL
 * ============================================================================
 */

void app_run(struct app_state *app)
{
	if (!app || !app->ui || !app->scene) {
		BHS_LOG_FATAL("app_run chamado com estado inválido");
		return;
	}

	BHS_LOG_INFO("Entrando no loop principal...");
	double accumulator = 0.0;

	while (!app->should_quit && !bhs_ui_should_close(app->ui)) {
		/* Temporização */
		double current_time = get_time_seconds();
		double frame_time = current_time - app->last_frame_time;
		app->last_frame_time = current_time;

		/* Evitar espiral da morte (death spiral) */
		if (frame_time > MAX_FRAME_TIME)
			frame_time = MAX_FRAME_TIME;

		/* Sincronizar Escala de Tempo do HUD PRIMEIRO - antes de acumular tempo */
		/* Formula: dias/min = 0.1 * 3650^val */
		/* 1 dia = 86400 segundos, 1 minuto real = 60 segundos */
		/* timescale = dias/min * 86400 / 60 = dias/min * 1440 */
		{
			float days_per_min =
				0.1f * powf(3650.0f, app->hud.time_scale_val);
			float target_timescale = days_per_min * 1440.0f;
			app_set_time_scale(app, (double)target_timescale);
		}

		/* [NOVO] Gerencia Request de VSync do HUD */
		if (app->hud.req_update_vsync) {
			bhs_ui_set_vsync(app->ui, app->hud.vsync_enabled);
			app->hud.req_update_vsync = false;
			BHS_LOG_INFO("VSync state update requested: %s",
				     app->hud.vsync_enabled ? "ON" : "OFF");
		}

		/* [NOVO] Gerencia Request de Pause do HUD */
		if (app->hud.req_toggle_pause) {
			app_toggle_pause(app);
			app->hud.req_toggle_pause = false;
		}

		/* [NOVO] Gerencia Request de Saída do HUD */
		if (app->hud.req_exit_to_menu) {
			scenario_unload(app);
			app->sim_status = APP_SIM_START_SCREEN;
			app->hud.req_exit_to_menu = false;

			/* Reset HUD state */
			app->hud.active_menu_index = -1;
			app->hud.add_menu_category = -1;
			app->hud.selected_body_index = -1;
		}

		/* Sync HUD state */
		app->hud.is_paused = (app->sim_status == APP_SIM_PAUSED);

		/* Acumular tempo para fixed timestep - MULTIPLICADO pelo time_scale! */
		/* [CRITICAL] Só acumula se estiver rodando, senão cria Death Spiral ao voltar */
		if (app->sim_status == APP_SIM_RUNNING) {
			accumulator += frame_time * app->time_scale;
		}

		/* Inicia quadro */
		if (bhs_ui_begin_frame(app->ui) != BHS_UI_OK)
			continue; /* Frame perdido, vida que segue */

		/* Processar input - Sempre processar para garantir comandos de HUD (Save, Toggle, etc) */
		input_process_frame(app, frame_time);

		/* Inicia gravação de comandos */
		bhs_ui_cmd_begin(app->ui);
		bhs_ui_begin_drawing(app->ui);

/* Timestep fixo para física */
/* Limita número de passos por frame para evitar death spiral */
#define MAX_PHYSICS_STEPS_PER_FRAME 1000
		int physics_steps = 0;

		double t0 = get_time_seconds();

		/* Loop de Física - Só roda se tivermos tempo acumulado suficiente */
		while (accumulator >= PHYSICS_DT &&
		       app->sim_status == APP_SIM_RUNNING &&
		       physics_steps < MAX_PHYSICS_STEPS_PER_FRAME) {
			/* [NOVO] Atualização dos Sistemas ECS (Leapfrog + 1PN) */
			bhs_world_handle world =
				bhs_scene_get_world(app->scene);

			/* Integração Física de Alta Fidelidade */
			physics_system_update(world, PHYSICS_DT);

			/* 3. Atualização da Engine (Colisão, Hierarquia de Transformadas, Sinc Espaço-Tempo) */
			bhs_scene_update(app->scene, PHYSICS_DT);

			/* 4. Gameplay/Atualização Celestial (Rotação, Eventos) */
			bhs_celestial_system_update(app->scene, PHYSICS_DT);

			/* [NOVO] Amostragem de Trilha de Órbita (Infinite History) */
			/* Sampling a cada 60 physic steps (1 min DT) = 1 hora simulada */
			static int trail_sample_counter = 0;
			trail_sample_counter++;
			/* [FIX] Always sample if counter hits, regardless of GLOBAL flag. 
			   Visibility logic in renderer handles the rest. */
			if (trail_sample_counter % 60 == 0) {
				int count = 0;
				struct bhs_body *bodies =
					(struct bhs_body *)bhs_scene_get_bodies(
						app->scene, &count);
				for (int i = 0; i < count; i++) {
					if (bodies[i].type != BHS_BODY_PLANET)
						continue;

					/* [FIX] Dynamic Allocation for Infinite Trails */
					if (!bodies[i].trail_positions) {
						size_t sz = BHS_MAX_TRAIL_POINTS * 3 * sizeof(float);
						bodies[i].trail_positions = malloc(sz);
						if (!bodies[i].trail_positions) continue; // Alloc failed
						memset(bodies[i].trail_positions, 0, sz);
					}

					/* Adiciona posição atual ao buffer circular */
					int idx = bodies[i].trail_head;
					bodies[i].trail_positions[idx][0] =
						(float)bodies[i].state.pos.x;
					bodies[i].trail_positions[idx][1] =
						(float)bodies[i].state.pos.y;
					bodies[i].trail_positions[idx][2] =
						(float)bodies[i].state.pos.z;

					bodies[i].trail_head =
						(idx + 1) %
						BHS_MAX_TRAIL_POINTS;
					if (bodies[i].trail_count <
					    BHS_MAX_TRAIL_POINTS) {
						bodies[i].trail_count++;
					}
				}
			}

			/* [NOVO] Atualiza sistema de marcadores de órbita */
			{
				int count = 0;
				const struct bhs_body *bodies =
					bhs_scene_get_bodies(app->scene,
							     &count);
				bhs_orbit_markers_update(&app->orbit_markers,
							 bodies, count,
							 app->accumulated_time);
			}

			accumulator -= PHYSICS_DT;
			app->accumulated_time += PHYSICS_DT;
			physics_steps++;
		}

		/* NOTA: Sync do time_scale foi movido para antes do acumulador */

		/* [FIX] Atualiza Cache do Inspetor de Objetos manualmente aqui (desacoplado do Input) */
		if (app->hud.selected_body_index != -1) {
			int count = 0;
			const struct bhs_body *bodies =
				bhs_scene_get_bodies(app->scene, &count);
			if (app->hud.selected_body_index < count) {
				app->hud.selected_body_cache =
					bodies[app->hud.selected_body_index];
			} else {
				app->hud.selected_body_index = -1;
			}
		}

		/* [NOVO] Gerenciamento de Estado da Câmera (Salva/Restaura no Toggle) */
		static bool last_fixed_cam = false;
		if (app->hud.fixed_planet_cam && !last_fixed_cam) {
			/* Entrando em Modo Fixo: Salva Estado */
			app->hud.saved_camera_state = app->camera;
			app->hud.has_saved_camera = true;
			// BHS_LOG_INFO("Camera saved.");
		} else if (!app->hud.fixed_planet_cam && last_fixed_cam) {
			/* Saindo do Modo Fixo: Restaura Estado */
			if (app->hud.has_saved_camera) {
				app->camera = app->hud.saved_camera_state;
				// BHS_LOG_INFO("Camera restored.");
			}
		}
		last_fixed_cam = app->hud.fixed_planet_cam;

		/* [MOVIDO] Lógica de Câmera Fixa em Planeta (Pós-Física) */
		if (app->hud.fixed_planet_cam &&
		    app->hud.selected_body_index != -1) {
			int count = 0;
			const struct bhs_body *bodies =
				bhs_scene_get_bodies(app->scene, &count);

			if (app->hud.selected_body_index < count) {
				const struct bhs_body *target =
					&bodies[app->hud.selected_body_index];

				/* 1. Acha o Sol (Força Maior) */
				int sun_idx = 0;
				double max_mass = -1.0;
				for (int i = 0; i < count; i++) {
					if ((bodies[i].type == BHS_BODY_STAR ||
					     bodies[i].type ==
						     BHS_BODY_BLACKHOLE) &&
					    bodies[i].state.mass > max_mass) {
						max_mass = bodies[i].state.mass;
						sun_idx = i;
					}
				}

				if (sun_idx != app->hud.selected_body_index) {
					const struct bhs_body *sun =
						&bodies[sun_idx];
					/* [FIX] Usa Coordenadas Visuais para casar com Escala de Renderização */
					float tvx, tvy, tvz, tv_rad;
					bhs_visual_calculate_transform(
						target, bodies, count,
						app->hud.visual_mode, &tvx,
						&tvy, &tvz, &tv_rad);

					/* Sun/Attractor Visual Pos */
					float svx, svy, svz, sv_rad;
					bhs_visual_calculate_transform(
						sun, bodies, count,
						app->hud.visual_mode, &svx,
						&svy, &svz, &sv_rad);

					/* Vector Sun(Vis) -> Planet(Vis) */
					double dx = tvx - svx;
					double dy = tvy - svy;
					double dz = tvz - svz;
					double dist = sqrt(dx * dx + dy * dy +
							   dz * dz);

					if (dist >
					    1.0) { /* Avoid division by zero */
						/* Direção Normalizada (Sol -> Planeta) */
						double nx = dx / dist;
						double ny = dy / dist;
						double nz = dz / dist;

						/* Basis Vectors */
						/* Right = Forward x Up = (-N) x (0,1,0) = (nz, 0, -nx) */
						double rx = nz;
						double rz = -nx;
						double r_len =
							sqrt(rx * rx + rz * rz);
						if (r_len > 0.001) {
							rx /= r_len;
							rz /= r_len;
						}

						/* Offsets baseados no RAIO VISUAL */
						/* [AJUSTE] Distância aumentada para 5.0x para melhor enquadramento */
						double offset_dist =
							tv_rad * 5.0;
						if (offset_dist < 20.0)
							offset_dist =
								20.0; /* Mínimo de sanidade */

						/* [AJUSTE] Sinal do deslocamento lateral invertido para colocar Planeta na ESQUERDA */
						double side_offset =
							-tv_rad *
							1.5; /* Sinal invertido */

						double up_offset = tv_rad * 0.3;

						/* Position */
						app->camera.x =
							tvx +
							(nx * offset_dist) +
							(rx * side_offset);
						app->camera.y =
							tvy +
							(ny * offset_dist) +
							up_offset;
						app->camera.z =
							tvz +
							(nz * offset_dist) +
							(rz * side_offset);

						/* Olhar para o Sol (Pos Visual) */
						double lx = svx - app->camera.x;
						double ly = svy - app->camera.y;
						double lz = svz - app->camera.z;

						app->camera.yaw = atan2(lx, lz);
						double gd =
							sqrt(lx * lx + lz * lz);
						app->camera.pitch =
							-atan2(ly, gd);
					}
				}
			}
		}

		/* [NOVO] Inspetor de Objetos: Calcula Atrator Mais Forte em todo o sistema */
		int attractor_idx = -1;
		if (app->hud.selected_body_index != -1) {
			int count = 0;
			const struct bhs_body *bodies =
				bhs_scene_get_bodies(app->scene, &count);

			// Validar índice
			if (app->hud.selected_body_index < count) {
				const struct bhs_body *me =
					&bodies[app->hud.selected_body_index];

				/* [FIX] Usa Lógica da Esfera de Hill para detecção de "Pais" em vez de Força bruta.
				   Força bruta diz Sol > Terra para a Lua (prox 2x), mas a Terra é o pai da Lua. */

				double best_hill_score =
					1.0e52; /* [FIX] Init maior que qualquer Hill R possível (era 1.0e50) */
				int parent_idx = -1;
				double best_dist = 0.0;

				/* Encontra o Atrator do Sistema (Sol/BN) para Cálculo de Hill */
				int sys_attractor = -1;
				double max_mass = 0.0; /* [FIX] Init em 0.0 */
				for (int k = 0; k < count; ++k) {
					if ((bodies[k].type == BHS_BODY_STAR ||
					     bodies[k].type ==
						     BHS_BODY_BLACKHOLE) &&
					    bodies[k].state.mass > max_mass) {
						max_mass = bodies[k].state.mass;
						sys_attractor = k;
					}
				}
				/* Fallback: Heaviest Object if no Star */
				if (sys_attractor == -1 && count > 0) {
					for (int k = 0; k < count; ++k) {
						if (bodies[k].state.mass >
						    max_mass) {
							max_mass =
								bodies[k]
									.state
									.mass;
							sys_attractor = k;
						}
					}
				}

				/* Safety: avoid div by zero if max_mass is 0 (empty scene) */
				if (max_mass < 1.0)
					max_mass = 1.0;

				for (int i = 0; i < count; i++) {
					if (i == app->hud.selected_body_index)
						continue;
					if (!bodies[i].is_alive)
						continue;

					/* Must be larger mass to be a parent */
					if (bodies[i].state.mass <=
					    me->state.mass)
						continue;

					double dx = bodies[i].state.pos.x -
						    me->state.pos.x;
					double dy = bodies[i].state.pos.y -
						    me->state.pos.y;
					double dz = bodies[i].state.pos.z -
						    me->state.pos.z;
					double dist_sq =
						dx * dx + dy * dy + dz * dz;
					double dist = sqrt(dist_sq);

					/* Calculate Hill Radius of candidate 'i' */
					double hill_r = 1.0e50;
					if (i != sys_attractor) {
						double dx_s =
							bodies[i].state.pos.x -
							bodies[sys_attractor]
								.state.pos.x;
						double dy_s =
							bodies[i].state.pos.y -
							bodies[sys_attractor]
								.state.pos.y;
						double dz_s =
							bodies[i].state.pos.z -
							bodies[sys_attractor]
								.state.pos.z;
						double d_sun =
							sqrt(dx_s * dx_s +
							     dy_s * dy_s +
							     dz_s * dz_s);
						hill_r =
							d_sun *
							pow(bodies[i].state.mass /
								    (3.0 *
								     max_mass),
							    0.333333);
					}

					/* Am I inside their Hill Sphere? */
					if (dist < hill_r) {
						/* Pick the 'tightest' parent (smallest Hill Sphere I fit in) */
						if (hill_r < best_hill_score) {
							best_hill_score =
								hill_r;
							parent_idx = i;
							best_dist = dist;
						}
					}
				}

				/* Fallback: If no parent found (e.g. I am the Sun), uses closest massive body? 
				   No, show nothing or "None". 
				   Actually, for the Sun, it shows nothing. Correct. */

				if (parent_idx != -1) {
					snprintf(app->hud.attractor_name, 64,
						 "%s", bodies[parent_idx].name);

					/* Surface-to-Surface Distance */
					double r_parent =
						bodies[parent_idx].state.radius;
					double r_self = me->state.radius;
					double surf_dist =
						best_dist - r_parent - r_self;
					if (surf_dist < 0)
						surf_dist = 0;

					app->hud.attractor_dist = surf_dist;
				} else {
					app->hud.attractor_name[0] = '\0';
				}
			}
		}

		/* Renderização */
		t0 = get_time_seconds();

		int win_w, win_h;
		bhs_ui_get_size(app->ui, &win_w, &win_h);

		if (app->sim_status == APP_SIM_START_SCREEN) {
			bhs_start_screen_draw(app, app->ui, win_w, win_h);
		} else {
			/* Limpa tela - preto absoluto */
			bhs_ui_clear(app->ui, (struct bhs_ui_color){
						      0.0f, 0.0f, 0.0f, 1.0f });

			/* [NOVO] Despacha Compute Pass */
			bhs_gpu_texture_t bh_tex = NULL;
			if (app->bh_pass &&
			    app->scenario == APP_SCENARIO_KERR_BLACKHOLE) {
				bhs_blackhole_pass_resize(app->bh_pass, win_w,
							  win_h);
				bhs_gpu_cmd_buffer_t cmd =
					bhs_ui_get_current_cmd(app->ui);
				bhs_blackhole_pass_dispatch(app->bh_pass, cmd,
							    app->scene,
							    &app->camera);
				bh_tex = bhs_blackhole_pass_get_output(
					app->bh_pass);
			}

			/* Desenha cena (Atualizado com Modo Visual) */
			bhs_view_assets_t assets = {
				.bg_texture = app->bg_tex,
				.sphere_texture = app->sphere_tex,
				.bh_texture = bh_tex,
				.tex_cache =
					(const struct bhs_planet_tex_entry *)
						app->tex_cache,
				.tex_cache_count = app->tex_cache_count,
				.render_3d_active = (app->planet_pass != NULL),
				/* Gravity Line: passa o toggle e o índice do body selecionado */
				.show_gravity_line = app->hud.show_gravity_line,
				.selected_body_index =
					app->hud.selected_body_index,
				/* Trilha de Órbita (Orbit Trail) */
				.show_orbit_trail = app->hud.show_orbit_trail,
				/* Órbitas de Satélites */
				.show_satellite_orbits =
					app->hud.show_satellite_orbits,
				/* [NOVO] Controle visual detalhado */
				.show_planet_markers =
					app->hud.show_planet_markers,
				.show_moon_markers = app->hud.show_moon_markers,
				/* [NOVO] Visão Isolada - propaga o índice se isolamento ativo */
				.isolated_body_index =
					app->hud.isolate_view
						? app->hud.selected_body_index
						: -1,
				/* [NOVO] Sistema de marcadores de órbita */
				.orbit_markers = &app->orbit_markers,
				/* [NOVO] Alpha de Interpolação */
				.sim_alpha = accumulator,
				/* [NOVO] Índice do Atrator para contexto visual */
				.attractor_index = attractor_idx
			};
			bhs_view_spacetime_draw(app->ui, app->scene,
						&app->camera, win_w, win_h,
						&assets, app->hud.visual_mode,
						app->planet_pass);

			/* HUD */
			/* [NOVO] Calcula FPS para Display (Filtro Passa-Baixa Simples) */
			float instantaneous_fps =
				(frame_time > 0.0001)
					? (1.0f / (float)frame_time)
					: 0.0f;
			static float avg_fps = 60.0f;
			avg_fps = (avg_fps * 0.9f) + (instantaneous_fps * 0.1f);

			app->hud.sim_time_seconds =
				app->accumulated_time; /* Sincronizar tempo J2000 */
			app->hud.current_fps = avg_fps;
			app->hud.orbit_markers_ptr =
				&app->orbit_markers; /* [NOVO] Passa marcadores */
			app->hud.current_scenario =
				(int)app->scenario; /* [NOVO] Sincronizar Tipo de Cenário */
			bhs_hud_draw(app->ui, &app->hud, win_w, win_h);

			/* Status bar */
			const char *status = app->sim_status == APP_SIM_PAUSED
						     ? "PAUSED"
						     : "Running";
			char status_buf[128];
			snprintf(status_buf, sizeof(status_buf),
				 "Status: %s | Time Scale: %.1fx | S=Save "
				 "L=Load "
				 "Space=Pause",
				 status, app->time_scale);
			bhs_ui_draw_text(app->ui, status_buf, 10,
					 (float)win_h - 30, 16.0f,
					 BHS_UI_COLOR_GRAY);
		}

		app->render_ms = (get_time_seconds() - t0) * 1000.0;

		/* Fim do quadro */
		bhs_ui_end_frame(app->ui);

		/* Telemetria periódica */
		app->frame_count++;
		if (app->frame_count % 30 == 0) {
			bhs_telemetry_print_scene(app->scene,
						  app->accumulated_time,
						  app->phys_ms, app->render_ms);
		}

		/* Registrar órbitas periodicamente para análise (Histórico rolável) */
		if (app->frame_count % 60 == 0) {
			bhs_telemetry_log_orbits(app->scene,
						 app->accumulated_time);
		}
	}

	BHS_LOG_INFO("Saindo do loop principal...");
}

/* ============================================================================
 * SHUTDOWN
 * ============================================================================
 */

void app_shutdown(struct app_state *app)
{
	if (!app)
		return;

	BHS_LOG_INFO("Desligando aplicação...");

	/* Ordem inversa da inicialização */
	if (app->bg_tex)
		bhs_gpu_texture_destroy(app->bg_tex);
	if (app->sphere_tex)
		bhs_gpu_texture_destroy(app->sphere_tex);

	/* Destroy cached textures */
	for (int i = 0; i < app->tex_cache_count; i++) {
		if (app->tex_cache[i].tex)
			bhs_gpu_texture_destroy(app->tex_cache[i].tex);
	}

	if (app->bh_pass)
		bhs_blackhole_pass_destroy(app->bh_pass);
	if (app->planet_pass)
		bhs_planet_pass_destroy(app->planet_pass);
	if (app->ui)
		bhs_ui_destroy(app->ui);
	if (app->scene)
		bhs_scene_destroy(app->scene);

	bhs_log_shutdown();

	BHS_LOG_INFO("Shutdown completo. Até a próxima.");
}
