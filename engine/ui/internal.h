/**
 * @file internal.h
 * @brief Coisas sujas que a gente esconde da API pública
 *
 * Aqui fica a struct do contexto descomunal e outras tralhas internas.
 * Se você não é um arquivo .c dentro de src/cmd/ui/, SAIA DAQUI.
 *
 * "Abandon all hope, ye who enter here."
 */

#ifndef RI_UX_UI_INTERNAL_H
#define RI_UX_UI_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>

#include "engine/platform/platform.h"
#include "engine/ui/ui.h"
#include "engine/ui/render/font_system.h"

/* ============================================================================
 * CONSTANTES INTERNAS
 * ============================================================================
 */

#define RI_UI_MAX_KEYS 256
#define RI_UI_MAX_BUTTONS 8

/* === Layout Engine State === */
#define RI_MAX_LAYOUT_STACK 64

struct ri_layout_node {
	struct ri_ui_rect rect;       /* Área total do container */
	struct ri_ui_rect cursor_pos; /* Onde estamos desenhando agora */
	struct ri_layout_style style;
	ri_layout_dir_t dir;
	float max_cross_size; /* Maior item no eixo cruzado */
};

struct ri_layout_ctx {
	struct ri_layout_node stack[RI_MAX_LAYOUT_STACK];
	int stack_ptr;
};

/* ============================================================================
 * ESTRUTURA DO CONTEXTO (EXPOSTA INTERNAMENTE)
 * ============================================================================
 */

struct ri_ui_ctx_impl {
	/* === Platform === */
	ri_platform_t platform;
	ri_window_t window;

	/* === Renderer === */
	ri_gpu_device_t device;
	ri_gpu_swapchain_t swapchain;
	ri_gpu_cmd_buffer_t cmd;

	/* Pipeline 2D (Fase 3) */
	ri_gpu_pipeline_t pipeline_2d;
	ri_gpu_texture_t white_texture;
	ri_gpu_sampler_t default_sampler;

	/* Batching state */
	ri_gpu_buffer_t vertex_buffer;
	ri_gpu_buffer_t index_buffer;
	void *mapped_vertices;
	void *mapped_indices;
	uint32_t vertex_count;
	uint32_t index_count;
	struct {
		ri_gpu_texture_t texture;
		uint32_t offset;
		uint32_t count;
	} current_batch;

	/* Sincronização por frame */
	ri_gpu_fence_t fence_frame;
	ri_gpu_texture_t current_texture; /* Textura do frame atual */
	ri_gpu_texture_t depth_texture; /* Textura de profundidade (para 3D) */

	/* === Estado da janela === */
	int32_t width;
	int32_t height;
	bool should_close;
	bool resize_pending; /* [FIX] Frame deve ser pulado após resize */
	bool vsync_pending;  /* [NEW] Request VSync change */
	bool vsync_target;   /* [NEW] Target VSync state */

	/* === Input state === */
	struct {
		bool keys[RI_UI_MAX_KEYS];	       /* Estado atual */
		bool keys_prev[RI_UI_MAX_KEYS];       /* Frame anterior */
		bool buttons[RI_UI_MAX_BUTTONS];      /* Mouse buttons */
		bool buttons_prev[RI_UI_MAX_BUTTONS]; /* Mouse buttons */
		int32_t mouse_x;
		int32_t mouse_y;
		float scroll_y; /* Vertical scroll delta */
		bool input_blocked; /* [NEW] Global input lock (modals) */
	} input;

	/* === Widget state (immediate mode) === */
	struct {
		uint64_t hot_id;    /* Widget sob o mouse */
		uint64_t active_id; /* Widget sendo clicado */
	} widget;

	/* === Font state === */
	struct ri_font_system font;

	/* === Layout state === */
	struct ri_layout_ctx layout;

	/* === Frame state === */
	bool in_frame;
	uint64_t frame_count;
};

/* ============================================================================
 * FUNÇÕES INTERNAS (MÓDULOS)
 * ============================================================================
 */

/* window/window.c */
int ri_ui_window_init_internal(ri_ui_ctx_t ctx,
				const struct ri_ui_config *config);
void ri_ui_window_shutdown_internal(ri_ui_ctx_t ctx);
void ri_ui_window_poll_events(ri_ui_ctx_t ctx);

/* render/render2d.c */
int ri_ui_render_init_internal(ri_ui_ctx_t ctx);
void ri_ui_render_shutdown_internal(ri_ui_ctx_t ctx);
void ri_ui_render_begin(ri_ui_ctx_t ctx); /* Setup viewport, pipeline */
void ri_ui_render_end(ri_ui_ctx_t ctx);

#endif /* RI_UX_UI_INTERNAL_H */
