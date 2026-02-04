/**
 * @file application.c
 * @brief Implementação do Ciclo de Vida da Aplicação
 */

#include "src/system/application.h"
#include <stdio.h>
#include <stdlib.h>
#include "engine/assets/image_loader.h"
#include "gui/log.h"
#include "gui/rhi/rhi.h"
#include "src/simulation/simulation_init.h"

bool bhs_app_init(bhs_app_t *app)
{
	if (!app)
		return false;

	// 0. Logging (FIRST)
	bhs_log_init();
	bhs_log_set_level(BHS_LOG_LEVEL_INFO);
	BHS_LOG_INFO("=== Black Hole Simulator Init ===");

	// 1. Scene / Engine Memory
	BHS_LOG_INFO("Allocating Engine Memory...");
	app->scene = bhs_scene_create();
	if (!app->scene) {
		BHS_LOG_FATAL("Failed to create scene!");
		return false;
	}

	// 2. Simulation Logic
	BHS_LOG_INFO("Initializing Simulation...");
	bhs_simulation_init(app->scene);

	// 3. UI gui (Window + Vulkan)
	BHS_LOG_INFO("Initializing gui/UI...");
	struct bhs_ui_config config = {
		.title = "Black Hole Simulator - Spacetime View",
		.width = 1280,
		.height = 720,
		.resizable = true,
		.vsync = true,
		.debug = true
	};

	int ret = bhs_ui_create(&config, &app->ui);
	if (ret != BHS_UI_OK) {
		BHS_LOG_FATAL("Failed to create UI: %d", ret);
		bhs_scene_destroy(app->scene);
		return false;
	}

	// 4. Auxiliary Systems (HUD, Textures)
	bhs_hud_init(&app->hud_state);

	// 4.1 Skybox
	BHS_LOG_INFO("Loading Assets...");
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
		BHS_LOG_WARN("Skybox texture missing.");
	}

	// 4.2 Sphere Impostor
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

	BHS_LOG_INFO("Initialization Complete. Systems Online.");
	return true;
}

void bhs_app_shutdown(bhs_app_t *app)
{
	if (!app)
		return;

	BHS_LOG_INFO("Shutting down...");
	if (app->bg_tex)
		bhs_gpu_texture_destroy(app->bg_tex);
	if (app->sphere_tex)
		bhs_gpu_texture_destroy(app->sphere_tex);
	if (app->ui)
		bhs_ui_destroy(app->ui);
	if (app->scene)
		bhs_scene_destroy(app->scene);

	bhs_log_shutdown();
}
