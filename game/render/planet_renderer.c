/**
 * @file planet_renderer.c
 * @brief Renderizador de Planetas 3D
 */

#include "planet_renderer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "engine/assets/image_loader.h" /* Utils if needed */
#include "engine/geometry/mesh_gen.h"
#include "engine/foundation/log.h"
#include "engine/math/mat4.h"
#include "visual_utils.h"

#ifndef PI
#define PI 3.14159265359f
#endif

#define MAX_LINES 16384 /* Max linhas por frame */

struct line_vertex {
	float pos[3];
	float color[4];
};

struct ri_planet_pass {
	ri_ui_ctx_t ctx;

	/* Planet Pipeline */
	ri_gpu_pipeline_t pipeline;
	ri_gpu_shader_t vs, fs;
	ri_gpu_buffer_t vbo, ibo;
	ri_gpu_sampler_t sampler;
	uint32_t index_count;

	/* Line Pipeline */
	ri_gpu_pipeline_t line_pipeline;
	ri_gpu_shader_t line_vs, line_fs;
	ri_gpu_buffer_t line_vbo;
	struct line_vertex *line_cpu_buffer;
	uint32_t line_count;
};

/* Push Constants matching shader */
/* Push Constants correspondendo ao shader (PACKED em 128 bytes) */
struct planet_pc {
	ri_mat4_t viewProj;   /* 64 bytes */
	float modelParams[4];  /* xyz=pos, w=radius */
	float rotParams[4];    /* xyz=axis, w=angle */
	float lightAndStar[4]; /* xyz=lightPos, w=isStar */
	float colorParams[4];  /* xyz=color, w=pad */
}; /* Total 128 bytes */

struct line_pc {
	ri_mat4_t viewProj;
};

/* Helper Privado: Carrega código de Shader */
static int load_shader(ri_gpu_device_t dev, const char *rel_path,
		       enum ri_gpu_shader_stage stage, ri_gpu_shader_t *out)
{
	/* Tenta múltiplos prefixos para achar o shader */
	const char *prefixes[] = {
		"",	      /* Se rodando do bin/ ou assets no CWD */
		"build/bin/", /* Se rodando da raiz e shaders no build */
		"../",	      /* Se rodando de algum subdir */
		"bin/",	      /* Genérico */
		"assets/"     /* Redundante */
	};

	FILE *f = NULL;
	char full_path[512];

	for (int i = 0; i < 5; i++) {
		snprintf(full_path, sizeof(full_path), "%s%s", prefixes[i],
			 rel_path);
		f = fopen(full_path, "rb");
		if (f) {
			RI_LOG_INFO("Shader found at: %s", full_path);
			break;
		}
	}

	if (!f) {
		RI_LOG_ERROR("Shader NOT found: %s (checked common paths)",
			      rel_path);
		return RI_GPU_ERR_INVALID;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	void *code = malloc(size);
	if (!code) {
		fclose(f);
		return RI_GPU_ERR_NOMEM;
	}
	fread(code, 1, size, f);
	fclose(f);

	struct ri_gpu_shader_config conf = { .stage = stage,
					      .code = code,
					      .code_size = size,
					      .entry_point = "main" };

	int res = ri_gpu_shader_create(dev, &conf, out);
	free(code);

	if (res != 0) {
		RI_LOG_ERROR("Shader compilation/create failed for: %s",
			      full_path);
	}

	return res;
}

int ri_planet_pass_create(ri_ui_ctx_t ctx, ri_planet_pass_t *out_pass)
{
	ri_planet_pass_t p = calloc(1, sizeof(*p));
	if (!p)
		return -1;

	p->ctx = ctx;
	ri_gpu_device_t dev = ri_ui_get_gpu_device(ctx);

	/* 1. Carrega Shaders de Planeta */
	if (load_shader(dev, "assets/shaders/planet.vert.spv",
			RI_SHADER_VERTEX, &p->vs) != 0) {
		RI_LOG_ERROR("Failed to load planet.vert.spv");
		free(p);
		return -1;
	}
	if (load_shader(dev, "assets/shaders/planet.frag.spv",
			RI_SHADER_FRAGMENT, &p->fs) != 0) {
		RI_LOG_ERROR("Failed to load planet.frag.spv");
		free(p);
		return -1;
	}

	/* 1b. Carrega Shaders de Linha */
	/* Dependemos do build system para compilar .vert -> .vert.spv */
	if (load_shader(dev, "assets/shaders/line.vert.spv", RI_SHADER_VERTEX,
			&p->line_vs) != 0) {
		RI_LOG_WARN("Failed to load line.vert.spv - Lines will fail");
	}
	if (load_shader(dev, "assets/shaders/line.frag.spv",
			RI_SHADER_FRAGMENT, &p->line_fs) != 0) {
		RI_LOG_WARN("Failed to load line.frag.spv - Lines will fail");
	}

	/* 2. Cria Mesh da Esfera */
	ri_mesh_t mesh = ri_mesh_gen_sphere(32, 32);
	p->index_count = mesh.index_count;

	/* VBO */
	struct ri_gpu_buffer_config vbo_conf = {
		.size = mesh.vertex_count * sizeof(ri_vertex_3d_t),
		.usage = RI_BUFFER_VERTEX,
		.memory = RI_MEMORY_CPU_TO_GPU,
	};
	ri_gpu_buffer_create(dev, &vbo_conf, &p->vbo);
	ri_gpu_buffer_upload(p->vbo, 0, mesh.vertices, vbo_conf.size);

	/* IBO */
	struct ri_gpu_buffer_config ibo_conf = {
		.size = mesh.index_count * sizeof(uint16_t),
		.usage = RI_BUFFER_INDEX,
		.memory = RI_MEMORY_CPU_TO_GPU,
	};
	ri_gpu_buffer_create(dev, &ibo_conf, &p->ibo);
	ri_gpu_buffer_upload(p->ibo, 0, mesh.indices, ibo_conf.size);

	ri_mesh_free(mesh);

	/* 2b. Cria Buffer de Linhas */
	p->line_cpu_buffer = malloc(MAX_LINES * 2 * sizeof(struct line_vertex));
	struct ri_gpu_buffer_config line_vbo_conf = {
		.size = MAX_LINES * 2 * sizeof(struct line_vertex),
		.usage = RI_BUFFER_VERTEX,
		.memory = RI_MEMORY_CPU_TO_GPU,
	};
	ri_gpu_buffer_create(dev, &line_vbo_conf, &p->line_vbo);

	/* 3. Sampler */
	struct ri_gpu_sampler_config samp_conf = {
		.min_filter = RI_FILTER_LINEAR,
		.mag_filter = RI_FILTER_LINEAR,
		.address_u = RI_ADDRESS_REPEAT,
		.address_v = RI_ADDRESS_CLAMP_TO_EDGE,
	};
	ri_gpu_sampler_create(dev, &samp_conf, &p->sampler);

	enum ri_gpu_texture_format color_fmt = RI_FORMAT_BGRA8_SRGB;

	/* 4. Config Pipeline do Planeta */
	struct ri_gpu_vertex_attr attrs[] = {
		{ .location = 0,
		  .binding = 0,
		  .format = RI_FORMAT_RGB32_FLOAT,
		  .offset = offsetof(ri_vertex_3d_t, pos) },
		{ .location = 1,
		  .binding = 0,
		  .format = RI_FORMAT_RGB32_FLOAT,
		  .offset = offsetof(ri_vertex_3d_t, normal) },
		{ .location = 2,
		  .binding = 0,
		  .format = RI_FORMAT_RG32_FLOAT,
		  .offset = offsetof(ri_vertex_3d_t, uv) }
	};
	struct ri_gpu_vertex_binding binding = { .binding = 0,
						  .stride = sizeof(
							  ri_vertex_3d_t),
						  .per_instance = false };

	struct ri_gpu_pipeline_config pipe_conf = {
		.vertex_shader = p->vs,
		.fragment_shader = p->fs,
		.vertex_attrs = attrs,
		.vertex_attr_count = 3,
		.vertex_bindings = &binding,
		.vertex_binding_count = 1,
		.primitive = RI_PRIMITIVE_TRIANGLES,
		.cull_mode = RI_CULL_BACK,
		.front_ccw = true,
		.depth_test = true,
		.depth_write = true,
		.depth_compare = RI_COMPARE_LESS_EQUAL,
		.color_formats = &color_fmt,
		.color_format_count = 1,
		.depth_format = RI_FORMAT_DEPTH32_FLOAT,
		.label = "Planet Pipeline"
	};

	if (ri_gpu_pipeline_create(dev, &pipe_conf, &p->pipeline) != 0) {
		RI_LOG_ERROR("Failed to create Planet Pipeline");
		free(p->line_cpu_buffer); /* Cleanup */
		free(p);
		return -1;
	}

	/* 4b. Config Pipeline de Linha */
	if (p->line_vs && p->line_fs) {
		struct ri_gpu_vertex_attr line_attrs[] = {
			{ .location = 0,
			  .binding = 0,
			  .format = RI_FORMAT_RGB32_FLOAT,
			  .offset = offsetof(struct line_vertex, pos) },
			{ .location = 1,
			  .binding = 0,
			  .format = RI_FORMAT_RGBA32_FLOAT,
			  .offset = offsetof(struct line_vertex, color) }
		};
		struct ri_gpu_vertex_binding line_binding = {
			.binding = 0,
			.stride = sizeof(struct line_vertex),
			.per_instance = false
		};

		struct ri_gpu_pipeline_config line_pipe_conf = {
			.vertex_shader = p->line_vs,
			.fragment_shader = p->line_fs,
			.vertex_attrs = line_attrs,
			.vertex_attr_count = 2,
			.vertex_bindings = &line_binding,
			.vertex_binding_count = 1,
			.primitive = RI_PRIMITIVE_LINES,
			.cull_mode = RI_CULL_NONE,
			.front_ccw = true,
			.depth_test = true,
			.depth_write =
				false, /* Linhas geralmente transparentes ou debug */
			.depth_compare = RI_COMPARE_LESS_EQUAL,
			.color_formats = &color_fmt,
			.color_format_count = 1,
			.depth_format = RI_FORMAT_DEPTH32_FLOAT,
			.label = "Line Pipeline"
		};

		if (ri_gpu_pipeline_create(dev, &line_pipe_conf,
					    &p->line_pipeline) != 0) {
			RI_LOG_ERROR("Failed to create Line Pipeline");
		}
	}

	*out_pass = p;
	return 0;
}

void ri_planet_pass_destroy(ri_planet_pass_t pass)
{
	if (!pass)
		return;
	ri_gpu_pipeline_destroy(pass->pipeline);
	ri_gpu_shader_destroy(pass->vs);
	ri_gpu_shader_destroy(pass->fs);
	ri_gpu_buffer_destroy(pass->vbo);
	ri_gpu_buffer_destroy(pass->ibo);
	ri_gpu_sampler_destroy(pass->sampler);

	ri_gpu_pipeline_destroy(pass->line_pipeline);
	ri_gpu_shader_destroy(pass->line_vs);
	ri_gpu_shader_destroy(pass->line_fs);
	ri_gpu_buffer_destroy(pass->line_vbo);
	if (pass->line_cpu_buffer)
		free(pass->line_cpu_buffer);

	free(pass);
}

/* Helper para ordenação */
struct render_item {
	int index;
	float dist_sq;
	float vis_x, vis_y, vis_z;
	float vis_radius;
};

static int compare_items(const void *a, const void *b)
{
	const struct render_item *ia = (const struct render_item *)a;
	const struct render_item *ib = (const struct render_item *)b;
	/* Ordena LONGE -> PERTO (Distância descendente) */
	if (ia->dist_sq > ib->dist_sq)
		return -1;
	if (ia->dist_sq < ib->dist_sq)
		return 1;
	return 0;
}

void ri_planet_pass_submit_line(ri_planet_pass_t pass, float x1, float y1,
				 float z1, float x2, float y2, float z2,
				 float r, float g, float b, float a)
{
	if (!pass || !pass->line_cpu_buffer)
		return;
	if (pass->line_count >= MAX_LINES)
		return;

	int idx = pass->line_count * 2;

	pass->line_cpu_buffer[idx].pos[0] = x1;
	pass->line_cpu_buffer[idx].pos[1] = y1;
	pass->line_cpu_buffer[idx].pos[2] = z1;
	pass->line_cpu_buffer[idx].color[0] = r;
	pass->line_cpu_buffer[idx].color[1] = g;
	pass->line_cpu_buffer[idx].color[2] = b;
	pass->line_cpu_buffer[idx].color[3] = a;

	pass->line_cpu_buffer[idx + 1].pos[0] = x2;
	pass->line_cpu_buffer[idx + 1].pos[1] = y2;
	pass->line_cpu_buffer[idx + 1].pos[2] = z2;
	pass->line_cpu_buffer[idx + 1].color[0] = r;
	pass->line_cpu_buffer[idx + 1].color[1] = g;
	pass->line_cpu_buffer[idx + 1].color[2] = b;
	pass->line_cpu_buffer[idx + 1].color[3] = a;

	pass->line_count++;
}

void ri_planet_pass_draw(ri_planet_pass_t pass, ri_gpu_cmd_buffer_t cmd,
			  ri_scene_t scene, const ri_camera_t *cam,
			  const ri_view_assets_t *assets,
			  ri_visual_mode_t mode, float output_width,
			  float output_height)
{
	if (!pass || !cmd || !scene)
		return;

	/* Define Viewport/Scissor explicitamente (Estado Dinâmico) */
	ri_gpu_cmd_set_viewport(cmd, 0, 0, output_width, output_height, 0.0f,
				 1.0f);
	ri_gpu_cmd_set_scissor(cmd, 0, 0, (uint32_t)output_width,
				(uint32_t)output_height);

	/* --- CÁLCULO COMUM DE MATRIZ --- */
	float cy = cosf(cam->yaw);
	float sy = sinf(cam->yaw);
	float cp = cosf(cam->pitch);
	float sp = sinf(cam->pitch);

	/* Rotação Yaw (Eixo Y) */
	ri_mat4_t m_yaw = ri_mat4_identity();
	m_yaw.m[0] = cy;  /* Row 0, Col 0: x->x */
	m_yaw.m[2] = sy;  /* Row 2, Col 0: x->z (z' = x*sy...) */
	m_yaw.m[8] = -sy; /* Row 0, Col 2: z->x (x' = ... - z*sy) */
	m_yaw.m[10] = cy; /* Row 2, Col 2: z->z */

	/* Rotação Pitch (Eixo X) */
	ri_mat4_t m_pitch = ri_mat4_identity();
	m_pitch.m[5] = cp;  /* Row 1, Col 1: y->y */
	m_pitch.m[6] = sp;  /* Row 2, Col 1: y->z (z' = y*sp...) */
	m_pitch.m[9] = -sp; /* Row 1, Col 2: z->y (y' = ... - z*sp) */
	m_pitch.m[10] = cp; /* Row 2, Col 2: z->z */

	ri_mat4_t mat_view = ri_mat4_mul(m_pitch, m_yaw);

	/* Matriz Proj & Configs de Escala */
	float focal_length = cam->fov;
	if (focal_length < 1.0f)
		focal_length = 1.0f;

	float fov_y = 2.0f * atanf((output_height * 0.5f) / focal_length);
	float aspect = output_width / output_height;

	ri_mat4_t mat_proj =
		ri_mat4_perspective(fov_y, aspect, 1.0e7f, 1.0e14f);
	ri_mat4_t mat_vp = ri_mat4_mul(mat_proj, mat_view);

	/* --- DESENHO DE PLANETAS (OPACO) --- */
	ri_gpu_cmd_set_pipeline(cmd, pass->pipeline);
	ri_gpu_cmd_set_vertex_buffer(cmd, 0, pass->vbo, 0);
	ri_gpu_cmd_set_index_buffer(cmd, pass->ibo, 0, false); /* 16 bit */

	/* Coleta Corpos */
	int count = 0;
	const struct ri_body *bodies = ri_scene_get_bodies(scene, &count);

	/* Aloca Array de Ordenação */
	struct render_item sort_list[128];
	int render_count = 0;

	/* Usa Helper para Transformada */
	for (int i = 0; i < count; i++) {
		const struct ri_body *b = &bodies[i];
		if (b->type != RI_BODY_PLANET && b->type != RI_BODY_STAR &&
		    b->type != RI_BODY_MOON)
			continue;
		if (render_count >= 128)
			break;
		if (assets && assets->isolated_body_index >= 0) {
			/* Show: Self, Attractor, or if I am a moon of Self */
			bool visible = false;
			if (i == assets->isolated_body_index) visible = true;
			if (i == assets->attractor_index) visible = true;
			
			if (!visible) {
				/* Check if I am a moon of the isolated body */
				int parent = ri_visual_find_parent(i, bodies, count, NULL);
				if (parent == assets->isolated_body_index) {
					visible = true;
				}
			}

			if (!visible) {
				continue;
			}
		}

		float vx, vy, vz, vrad;
		/* CHAMADA LÓGICA COMPARTILHADA */
		ri_visual_calculate_transform(b, bodies, count, mode, &vx, &vy,
					       &vz, &vrad);

		double rel_x = vx - cam->x;
		double rel_y = vy - cam->y;
		double rel_z = vz - cam->z;

		float dist_sq =
			(float)(rel_x * rel_x + rel_y * rel_y + rel_z * rel_z);

		sort_list[render_count].index = i;
		sort_list[render_count].dist_sq = dist_sq;
		sort_list[render_count].vis_x = vx;
		sort_list[render_count].vis_y = vy;
		sort_list[render_count].vis_z = vz;
		sort_list[render_count].vis_radius = vrad;
		render_count++;
	}

	/* Ordena (Painter's Algorithm: Longe -> Perto) */
	qsort(sort_list, render_count, sizeof(struct render_item),
	      compare_items);

	/* Desenha Ordenado */
	float light_pos[3] = { 0.0f, 0.0f, 0.0f };

	for (int k = 0; k < render_count; k++) {
		int i = sort_list[k].index;
		const struct ri_body *b = &bodies[i];

		/* Textura ... */
		ri_gpu_texture_t tex = assets->sphere_texture;
		if (assets->tex_cache) {
			for (int t = 0; t < assets->tex_cache_count; t++) {
				if (strcmp(assets->tex_cache[t].name,
					   b->name) == 0) {
					tex = assets->tex_cache[t].tex;
					break;
				}
			}
		}
		ri_gpu_cmd_bind_texture(cmd, 0, 0, tex, pass->sampler);

		float b_px = sort_list[k].vis_x;
		float b_py = sort_list[k].vis_y;
		float b_pz = sort_list[k].vis_z;

		double rel_x = b_px - cam->x;
		double rel_y = b_py - cam->y;
		double rel_z = b_pz - cam->z;

		float tx = (float)rel_x;
		float ty = (float)rel_y;
		float tz = (float)rel_z;

		float radius = sort_list[k].vis_radius;
		float angle = (float)b->state.current_rotation_angle;
		if (assets) {
			angle +=
				(float)(b->state.rot_speed * assets->sim_alpha);
		}

		float ax = (float)b->state.rot_axis.x;
		float ay = (float)b->state.rot_axis.y;
		float az = (float)b->state.rot_axis.z;

		if (fabsf(ax) + fabsf(ay) + fabsf(az) < 0.01f) {
			ax = 0.0f;
			ay = 1.0f;
			az = 0.0f;
		}

		struct planet_pc pc;
		pc.viewProj = mat_vp;

		pc.modelParams[0] = tx;
		pc.modelParams[1] = ty;
		pc.modelParams[2] = tz;
		pc.modelParams[3] = radius;

		pc.rotParams[0] = ax;
		pc.rotParams[1] = ay;
		pc.rotParams[2] = az;
		pc.rotParams[3] = angle;

		pc.lightAndStar[0] = light_pos[0] - (float)cam->x;
		pc.lightAndStar[1] = light_pos[1] - (float)cam->y;
		pc.lightAndStar[2] = light_pos[2] - (float)cam->z;
		pc.lightAndStar[3] = (b->type == RI_BODY_STAR) ? 1.0f : 0.0f;

		pc.colorParams[0] = (float)b->color.x;
		pc.colorParams[1] = (float)b->color.y;
		pc.colorParams[2] = (float)b->color.z;
		pc.colorParams[3] = 0.0f;

		ri_gpu_cmd_push_constants(cmd, 0, &pc, sizeof(pc));
		ri_gpu_cmd_draw_indexed(cmd, pass->index_count, 1, 0, 0, 0);
	}

	/* --- DESENHO DE LINHAS (TRANSPARENTE/DEBUG) --- */
	if (pass->line_count > 0 && pass->line_pipeline) {
		ri_gpu_cmd_set_pipeline(cmd, pass->line_pipeline);

		/* Upload Linhas */
		ri_gpu_buffer_upload(pass->line_vbo, 0, pass->line_cpu_buffer,
				      pass->line_count * 2 *
					      sizeof(struct line_vertex));

		ri_gpu_cmd_set_vertex_buffer(cmd, 0, pass->line_vbo, 0);

		/* Push Constant para Linhas (Apenas VP) */
		/* Push Constant for Lines (Just VP) */
		/* struct line_pc lpc; -- Unused */

		/* Lines are in world space (relative to cam? No, we submit world coords usually or relative?)
		   Wait, `spacetime_renderer` calculates `visual_transform`. That output is WORLD space.
		   But `mat_vp` expects coordinates relative to camera?
		   No, `mat_view` translates by `cam`. 
		   Wait, `mat_view` rotation DOES NOT include translation.
		   In planet draw, we manually calc `rel_x`. `pc.modelParams` is relative to cam.
		   So `mat_vp` handles rotation + proj. Translation is done manually in shader via `modelParams`.
		   
		   For Lines, we should probably pass a VP matrix that handles translation too?
		   Or we pre-translate lines to be relative to camera before submission?
		   
		   Let's pre-translate inside the loop below or assume submission is in CAM relative coords?
		   If I submit lines, I should submit them in World coords and have the shader handle it?
		   But floats lose precision far from 0.
		   Better: Submit lines relative to Camera. 
		   The `submit_line` caller should act like `planet_params`: coords relative to camera.
		   
		   My `planet_renderer` uses `rel_x = vx - cam->x`.
		   So `vx` is Absolute. `rel_x` is Relative.
		   
		   Start simple: Logic in `submit_line` takes ABSOLUTE? 
		   Then we subtract cam pos here?
		   Or `submit_line` takes RELATIVE?
		   
		   Let's check `spacetime_renderer` usage. It calculates `px, py...`.
		   These are Absolute World Coords.
		   So `submit_line` receives Absolute.
		   Inside `draw_lines` (here), we should adjust `viewProj` or adjust vertices.
		   
		   Adjusting vertices on CPU (during upload? no, during submission!)
		   Let's change implementation of `submit_line` to subtract cam? 
		   But `pass` doesn't know cam in `submit_line`.
		   Only in `draw`.
		   
		   So we must store Absolute in `cpu_buffer`, then at `draw` time, we create a View Matrix that includes Translation?
		   Standard `lookAt` includes translation.
		   Here `mat_view` is ONLY rotation.
		   
		   Let's construct full View Matrix including translation `(-cam.x, -cam.y, -cam.z)`.
		   `ri_mat4_translate` ...
		   
		   BUT `cam->x` is double. Matrix is float. Precision loss!!!!
		   That's why `planet.vert` takes `modelParams` (high prec offset maybe? No, `vec4`).
		   
		   Solução: Deslocar vértices relativos à Câmera na CPU antes do upload. 
		   Itera `line_buffer`, subtrai `cam`, upload.
		*/

		for (uint32_t i = 0; i < pass->line_count * 2; ++i) {
			pass->line_cpu_buffer[i].pos[0] -= (float)cam->x;
			pass->line_cpu_buffer[i].pos[1] -= (float)cam->y;
			pass->line_cpu_buffer[i].pos[2] -= (float)cam->z;
		}

		/* Re-upload deslocado */
		ri_gpu_buffer_upload(pass->line_vbo, 0, pass->line_cpu_buffer,
				      pass->line_count * 2 *
					      sizeof(struct line_vertex));

		ri_gpu_cmd_push_constants(cmd, 0, &mat_vp, sizeof(mat_vp));

		ri_gpu_cmd_draw(cmd, pass->line_count * 2, 1, 0, 0);

		/* Reseta contagem para próximo frame */
		pass->line_count = 0;
	}
}
