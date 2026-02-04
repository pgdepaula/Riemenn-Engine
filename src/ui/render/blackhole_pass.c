/**
 * @file blackhole_pass.c
 * @brief Implementação do Compute Pass do Buraco Negro
 */

#include "blackhole_pass.h"
#include <malloc.h>
#include <math.h>
#include <string.h>
#include "engine/assets/image_loader.h" /* Para utils de IO de arquivo se precisar */
#include "gui/log.h"

/* ============================================================================
 * ESTRUTURAS PRIVADAS
 * ============================================================================
 */

/* Mirrors shader PushConstants */
struct blackhole_params {
	float time;
	float mass;
	float spin;
	float camera_dist;
	float camera_angle;
	float camera_incl;
	float res_x;
	float res_y;
	int render_mode;
};

struct bhs_blackhole_pass {
	bhs_gpu_device_t device;

	/* Pipeline */
	bhs_gpu_shader_t shader;
	bhs_gpu_pipeline_t pipeline;

	/* Post Process */
	bhs_gpu_shader_t pp_shader;
	bhs_gpu_pipeline_t pp_pipeline;

	/* Resources */
	bhs_gpu_texture_t output_tex;

	/* State */
	int width;
	int height;
};

/* ============================================================================
 * HELPER: Carregar Shader
 * ============================================================================
 */

static bhs_gpu_shader_t load_shader(bhs_gpu_device_t device, const char *path)
{
	/* Robust File Loading Logic */
	const char *prefixes[] = { "", "build/bin/", "../", "bin/" };

	FILE *f = NULL;
	char full_path[256];

	for (int i = 0; i < 4; i++) {
		snprintf(full_path, sizeof(full_path), "%s%s", prefixes[i],
			 path);
		f = fopen(full_path, "rb");
		if (f)
			break;
	}

	if (!f) {
		BHS_LOG_ERROR(
			"Falha ao abrir shader: %s (tentado em vários paths)",
			path);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	void *code = malloc(size);
	fread(code, 1, size, f);
	fclose(f);

	/* Cria shader module */
	struct bhs_gpu_shader_config conf = { .stage = BHS_SHADER_COMPUTE,
					      .code = code,
					      .code_size = size,
					      .entry_point = "main",
					      .label = "BlackHole Compute" };

	bhs_gpu_shader_t shader = NULL;
	if (bhs_gpu_shader_create(device, &conf, &shader) != BHS_GPU_OK) {
		BHS_LOG_ERROR("Falha ao criar shader module: %s", path);
		free(code);
		return NULL;
	}

	free(code);
	return shader;
}

/* ============================================================================
 * HELPER: Criar Textura
 * ============================================================================
 */

static bhs_gpu_texture_t create_storage_image(bhs_gpu_device_t device, int w,
					      int h)
{
	struct bhs_gpu_texture_config conf = {
		.width = w,
		.height = h,
		.depth = 1,
		.mip_levels = 1,
		.array_layers = 1,
		/* 
		 * RGBA8 é o que o shader atual espera (layout rgba8).
		 * Futuramente mudaremos para RGBA16F (HDR).
		 */
		.format = BHS_FORMAT_RGBA32_FLOAT,
		.usage = BHS_TEXTURE_STORAGE | BHS_TEXTURE_SAMPLED |
			 BHS_TEXTURE_TRANSFER_SRC,
		.label = "BlackHole Output"
	};

	bhs_gpu_texture_t tex = NULL;
	if (bhs_gpu_texture_create(device, &conf, &tex) != BHS_GPU_OK) {
		BHS_LOG_ERROR("Falha ao criar storage texture %dx%d", w, h);
		return NULL;
	}
	return tex;
}

/* ============================================================================
 * API
 * ============================================================================
 */

bhs_blackhole_pass_t
bhs_blackhole_pass_create(bhs_gpu_device_t device,
			  const bhs_blackhole_pass_config_t *config)
{
	bhs_blackhole_pass_t pass =
		calloc(1, sizeof(struct bhs_blackhole_pass));
	pass->device = device;
	pass->width = config->width;
	pass->height = config->height;

	/* 1. Carrega Shader */
	/* Assume que o CMake coloca blackhole.spv (resultante de .comp) aqui */
	pass->shader = load_shader(device, "assets/shaders/blackhole.spv");
	if (!pass->shader) {
		BHS_LOG_FATAL(
			"Impossível inicializar BlackHole Pass sem shader.");
		free(pass);
		return NULL;
	}

	/* 2. Cria Pipeline */
	struct bhs_gpu_compute_pipeline_config pipe_conf = {
		.compute_shader = pass->shader, .label = "BlackHole Pipeline"
	};
	if (bhs_gpu_pipeline_compute_create(device, &pipe_conf,
					    &pass->pipeline) != BHS_GPU_OK) {
		BHS_LOG_FATAL("Falha ao criar Compute Pipeline.");
		bhs_blackhole_pass_destroy(pass);
		return NULL;
	}

	/* 2.1 Post Process Pipeline */
	pass->pp_shader = load_shader(device, "assets/shaders/postprocess.spv");
	if (pass->pp_shader) {
		struct bhs_gpu_compute_pipeline_config pp_conf = {
			.compute_shader = pass->pp_shader,
			.label = "PostProcess Pipeline"
		};
		bhs_gpu_pipeline_compute_create(device, &pp_conf,
						&pass->pp_pipeline);
	} else {
		BHS_LOG_WARN("PostProcess shader não encontrado. HDR raw será "
			     "exibido.");
	}

	/* 3. Cria Output Texture */
	pass->output_tex =
		create_storage_image(device, pass->width, pass->height);
	if (!pass->output_tex) {
		bhs_blackhole_pass_destroy(pass);
		return NULL;
	}

	BHS_LOG_INFO("BlackHole Compute Pass inicializado (%dx%d)", pass->width,
		     pass->height);
	return pass;
}

void bhs_blackhole_pass_destroy(bhs_blackhole_pass_t pass)
{
	if (!pass)
		return;

	if (pass->output_tex)
		bhs_gpu_texture_destroy(pass->output_tex);
	if (pass->pipeline)
		bhs_gpu_pipeline_destroy(pass->pipeline);
	if (pass->shader)
		bhs_gpu_shader_destroy(pass->shader);

	if (pass->pp_pipeline)
		bhs_gpu_pipeline_destroy(pass->pp_pipeline);
	if (pass->pp_shader)
		bhs_gpu_shader_destroy(pass->pp_shader);

	free(pass);
}

void bhs_blackhole_pass_resize(bhs_blackhole_pass_t pass, int width, int height)
{
	if (!pass)
		return;
	if (pass->width == width && pass->height == height)
		return;

	pass->width = width;
	pass->height = height;

	/* Recria textura */
	if (pass->output_tex)
		bhs_gpu_texture_destroy(pass->output_tex);
	pass->output_tex = create_storage_image(pass->device, width, height);

	BHS_LOG_INFO("BlackHole Pass redimensionado para %dx%d", width, height);
}

bhs_gpu_texture_t bhs_blackhole_pass_get_output(bhs_blackhole_pass_t pass)
{
	return pass ? pass->output_tex : NULL;
}

void bhs_blackhole_pass_dispatch(bhs_blackhole_pass_t pass,
				 bhs_gpu_cmd_buffer_t cmd, bhs_scene_t scene,
				 const bhs_camera_t *cam)
{
	if (!pass || !pass->pipeline || !pass->output_tex)
		return;

	/* 1. Transição de layout para STORAGE (se necessário) */
	/* (Assumimos que o renderer lida com transições generalizadas ou o driver cuida) */

	/* 2. Bind Pipeline */
	bhs_gpu_cmd_set_pipeline(cmd, pass->pipeline);

	/* 3. Bind Resources (Set 0, Binding 0) */
	bhs_gpu_cmd_bind_compute_storage_texture(cmd, pass->pipeline, 0, 0,
						 pass->output_tex);

	/* 4. Push Constants */
	struct blackhole_params params = { 0 };

	/* Dados da câmera */
	params.camera_dist =
		sqrtf(cam->x * cam->x + cam->y * cam->y + cam->z * cam->z);
	params.time = 0.0f; /* TODO: Passar tempo real */
	params.res_x = (float)pass->width;
	params.res_y = (float)pass->height;
	params.render_mode = 0; /* Physics */

	/* Ângulos da câmera */
	/* 
	 * O shader espera ângulos esféricos para conversão:
	 * cam_dist * sin(incl) * cos(angle)
	 * Precisamos converter o Yaw/Pitch/Pos da camera 3D para isso,
	 * OU (Melhor) passar a posição e rotação da câmera diretamente.
	 * 
	 * O shader atual (v2.0) usa:
	 * if (mode != 2) reconstrói cam_pos de dist/angle/incl.
	 * 
	 * VAMOS MUDAR O SHADER DEPOIS PARA USAR CAM_POS DIRETO.
	 * Por enquanto, modo 2 hardcoded no shader usa posição fixa.
	 * Vamos tentar usar o modo 0 e enganar os angulos ou 
	 * vamos hackear o shader para aceitar Mode 3 (Free Camera).
	 * 
	 * Por ora, vamos preencher com valores dummy que funcionam
	 * para a câmera padrão do cenário Kerr.
	 */
	params.camera_angle = 1.0f; // cam->yaw;
	params.camera_incl = 1.57f; // cam->pitch + PI/2?

	/* Dados do Buraco Negro da Scene */
	if (scene) {
		int n = 0;
		const struct bhs_body *bodies = bhs_scene_get_bodies(scene, &n);
		/* Procura primeiro Buraco Negro */
		for (int i = 0; i < n; i++) {
			if (bodies[i].type == BHS_BODY_BLACKHOLE) {
				params.mass = bodies[i].state.mass;
				/* O campo 'radius' as vezes é usado como spin ou raio visual */
				/* Vamos assumir spin alto fixo por enquanto se não tiver no struct */
				/* Mas o user disse para usar spin alto. */
				params.spin = 0.998f;
				break;
			}
		}
	}
	if (params.mass == 0.0f)
		params.mass = 1.0f; /* Fallback */

	bhs_gpu_cmd_push_constants(cmd, 0, &params, sizeof(params));

	/* 5. Dispatch */
	/* Group size 16x16 */
	unsigned int groups_x = (pass->width + 15) / 16;
	unsigned int groups_y = (pass->height + 15) / 16;

	bhs_gpu_cmd_dispatch(cmd, groups_x, groups_y, 1);

	/* 6. Barrier de memória entre passes (WAR/RAW) */
	bhs_gpu_cmd_transition_texture(cmd, pass->output_tex);

	/* 7. Post Process Dispatch (In-Place) */
	if (pass->pp_pipeline) {
		bhs_gpu_cmd_set_pipeline(cmd, pass->pp_pipeline);
		bhs_gpu_cmd_bind_compute_storage_texture(
			cmd, pass->pp_pipeline, 0, 0, pass->output_tex);
		bhs_gpu_cmd_dispatch(cmd, groups_x, groups_y, 1);

		/* Final barrier for Fragment Shader read */
		bhs_gpu_cmd_transition_texture(cmd, pass->output_tex);
	}
}
