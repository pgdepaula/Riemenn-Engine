/**
 * @file context.c
 * @brief Implementação do contexto UI
 *
 * Agora mais enxuto graças ao window.c que tirou a gordura.
 * É como uma dieta low-carb de código.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/ui/internal.h"
#include "engine/ui/ui.h"

/* ============================================================================
 * API PRINCIPAL
 * ============================================================================
 */

int ri_ui_create(const struct ri_ui_config *config, ri_ui_ctx_t *ctx)
{
	if (!config || !ctx)
		return RI_UI_ERR_INVALID;

	/* Aloca contexto */
	struct ri_ui_ctx_impl *c = calloc(1, sizeof(*c));
	if (!c)
		return RI_UI_ERR_NOMEM;

	int ret;

	/* === Inicializa Window (via internal wrapper) === */
	ret = ri_ui_window_init_internal(c, config);
	if (ret != RI_UI_OK) {
		free(c);
		return ret; /* Erro já logado no window.c */
	}

	/* === Inicializa GPU === */
	/* Pipeline interno para renderização 2D */
	struct ri_gpu_device_config gpu_config = {
		.preferred_backend = RI_GPU_BACKEND_AUTO,
		.enable_validation = config->debug,
		.prefer_discrete_gpu = true,
	};

	ret = ri_gpu_device_create(&gpu_config, &c->device);
	if (ret != RI_GPU_OK) {
		fprintf(stderr, "[ui] erro: falha ao criar device GPU (%d)\n",
			ret);
		ri_ui_window_shutdown_internal(c);
		free(c);
		return RI_UI_ERR_GPU;
	}

	/* === Cria Swapchain === */
	struct ri_gpu_swapchain_config swap_config = {
		.native_display = ri_platform_get_native_display(c->platform),
		.native_window = ri_window_get_native_handle(c->window),
		.native_layer = ri_window_get_native_layer(c->window),
		.width = (uint32_t)c->width,
		.height = (uint32_t)c->height,
		.format = RI_FORMAT_BGRA8_SRGB,
		.buffer_count = 2,
		.vsync = config->vsync,
	};

	ret = ri_gpu_swapchain_create(c->device, &swap_config, &c->swapchain);
	if (ret != RI_GPU_OK) {
		fprintf(stderr, "[ui] erro: falha ao criar swapchain (%d)\n",
			ret);
		ri_gpu_device_destroy(c->device);
		ri_ui_window_shutdown_internal(c);
		free(c);
		return RI_UI_ERR_GPU;
	}

	/* === Cria Command Buffer === */
	ret = ri_gpu_cmd_buffer_create(c->device, &c->cmd);
	if (ret != RI_GPU_OK) {
		fprintf(stderr,
			"[ui] erro: falha ao criar command buffer (%d)\n", ret);
		ri_gpu_swapchain_destroy(c->swapchain);
		ri_gpu_device_destroy(c->device);
		ri_ui_window_shutdown_internal(c);
		free(c);
		return RI_UI_ERR_GPU;
	}

	/* === Cria Fence de Frame === */
	ret = ri_gpu_fence_create(c->device, &c->fence_frame);
	if (ret != RI_GPU_OK) {
		fprintf(stderr,
			"[ui] erro: falha ao criar fence de frame (%d)\n", ret);
		ri_gpu_cmd_buffer_destroy(c->cmd);
		ri_gpu_swapchain_destroy(c->swapchain);
		ri_gpu_device_destroy(c->device);
		ri_ui_window_shutdown_internal(c);
		free(c);
		return RI_UI_ERR_GPU;
	}

	/* === Inicializa Render 2D === */
	/* === Inicializa Render 2D === */
	ret = ri_ui_render_init_internal(c);
	if (ret != RI_UI_OK) {
		fprintf(stderr,
			"[ui] erro: falha ao inicializar renderer 2D (%d)\n",
			ret);
		/* Cleanup fences, cmd, swapchain, device, window */
		/* Cleanup completo dos recursos */
		return ret;
	}

	*ctx = c;
	return RI_UI_OK;
}

void ri_ui_quit(ri_ui_ctx_t ctx)
{
	if (ctx)
		ctx->should_close = true;
}

void ri_ui_destroy(ri_ui_ctx_t ctx)
{
	if (!ctx)
		return;

	/* Cleanup na ordem inversa */
	if (ctx->cmd)
		ri_gpu_cmd_buffer_destroy(ctx->cmd);
	if (ctx->swapchain)
		ri_gpu_swapchain_destroy(ctx->swapchain);
	if (ctx->device)
		ri_gpu_device_destroy(ctx->device);

	/* Cleanup window */
	ri_ui_window_shutdown_internal(ctx);

	free(ctx);
}

bool ri_ui_should_close(ri_ui_ctx_t ctx)
{
	if (!ctx)
		return true;
	return ctx
		->should_close; /* Agora window wrapper atualiza isso via ctx */
}

int ri_ui_begin_frame(ri_ui_ctx_t ctx)
{
	if (!ctx || ctx->in_frame)
		return RI_UI_ERR_INVALID;

	/* Wait for previous frame (se não for o primeiro) */
	if (ctx->frame_count > 0) {
		ri_gpu_fence_wait(ctx->fence_frame, 1000000000); // 1s timeout
		ri_gpu_fence_reset(ctx->fence_frame);
	}

	/* Incrementa frame count no início */
	ctx->frame_count++;

	/* Salva estado anterior do input */
	memcpy(ctx->input.keys_prev, ctx->input.keys, sizeof(ctx->input.keys));
	memcpy(ctx->input.buttons_prev, ctx->input.buttons,
	       sizeof(ctx->input.buttons));

	/* Reset per-frame deltas */
	ctx->input.scroll_y = 0.0f;

	/* Processa eventos */
	ri_ui_window_poll_events(ctx);

	/* [FIX] Se houve resize ou mudança de VSync, recria recursos agora */
	if (ctx->resize_pending || ctx->vsync_pending) {
		bool vsync_changed = ctx->vsync_pending;

		ctx->resize_pending = false;
		ctx->vsync_pending = false;

		/* Se mudou VSync, precisamos destruir e recriar, pois resize pode não suportar troca de modo */
		if (vsync_changed) {
			if (ctx->swapchain) {
				ri_gpu_swapchain_destroy(ctx->swapchain);
				ctx->swapchain = NULL;
			}

			struct ri_gpu_swapchain_config swap_config = {
				.native_display =
					ri_platform_get_native_display(
						ctx->platform),
				.native_window = ri_window_get_native_handle(
					ctx->window),
				.native_layer = ri_window_get_native_layer(
					ctx->window),
				.width = (uint32_t)ctx->width,
				.height = (uint32_t)ctx->height,
				.format = RI_FORMAT_BGRA8_SRGB,
				.buffer_count = 2,
				.vsync = ctx->vsync_target,
			};

			int r = ri_gpu_swapchain_create(
				ctx->device, &swap_config, &ctx->swapchain);
			if (r != RI_GPU_OK) {
				fprintf(stderr,
					"[ui] erro: falha ao recriar swapchain "
					"com novo VSync! (%d)\n",
					r);
				/* Tenta fallback */
			}
		} else {
			/* Apenas resize */
			if (ctx->swapchain) {
				ri_gpu_swapchain_resize(ctx->swapchain,
							 (uint32_t)ctx->width,
							 (uint32_t)ctx->height);
			}
		}

		/* Recria depth texture */
		if (ctx->depth_texture && ctx->device) {
			ri_gpu_texture_destroy(ctx->depth_texture);
			ctx->depth_texture = NULL;

			struct ri_gpu_texture_config depth_cfg = {
				.width = (uint32_t)ctx->width,
				.height = (uint32_t)ctx->height,
				.depth = 1,
				.format = RI_FORMAT_DEPTH32_FLOAT,
				.usage = RI_TEXTURE_DEPTH_STENCIL,
				.mip_levels = 1,
				.array_layers = 1,
				.label = "UI Depth Buffer (Resized)",
			};
			ri_gpu_texture_create(ctx->device, &depth_cfg,
					       &ctx->depth_texture);
		}

		return RI_UI_SKIP; /* [FIX] Retorna SKIP para não tentar desenhar sem imagem */
	}

	/* Adquire imagem */
	ri_gpu_texture_t tex = NULL;
	int ret = ri_gpu_swapchain_next_texture(ctx->swapchain, &tex);

	if (ret != 0) {
		/* Pula frame */
		return RI_UI_OK;
	}
	ctx->current_texture = tex;

	/* Reseta widget state */
	ctx->widget.hot_id = 0;

	ctx->in_frame = true;
	return RI_UI_OK;
}

void ri_ui_cmd_begin(ri_ui_ctx_t ctx)
{
	if (ctx && ctx->cmd) {
		ri_gpu_cmd_reset(ctx->cmd);
		ri_gpu_cmd_begin(ctx->cmd);
	}
}

void ri_ui_begin_drawing(ri_ui_ctx_t ctx)
{
	if (ctx && ctx->in_frame) {
		ri_ui_render_begin(ctx);
	}
}

void *ri_ui_get_current_cmd(ri_ui_ctx_t ctx)
{
	if (!ctx)
		return NULL;
	/* Retorna ponteiro opaco para ri_gpu_cmd_buffer_t */
	return ctx->cmd;
}

int ri_ui_end_frame(ri_ui_ctx_t ctx)
{
	if (!ctx || !ctx->in_frame)
		return RI_UI_ERR_INVALID;

	/* Finaliza render pass e command buffer */
	ri_ui_render_end(ctx);

	/* Finaliza command buffer */
	ri_gpu_cmd_end(ctx->cmd);

	/* Submete com Fence (para sincronizar frames) */
	int ret = ri_gpu_swapchain_submit(ctx->swapchain, ctx->cmd,
					   ctx->fence_frame);
	if (ret != 0) {
		return RI_UI_ERR_INVALID;
	}

	/* Apresenta */
	ri_gpu_swapchain_present(ctx->swapchain);

	ctx->in_frame = false;
	return RI_UI_OK;
}

void ri_ui_get_size(ri_ui_ctx_t ctx, int32_t *width, int32_t *height)
{
	if (!ctx)
		return;
	if (width)
		*width = ctx->width;
	if (height)
		*height = ctx->height;
}

void *ri_ui_get_gpu_device(ri_ui_ctx_t ctx)
{
	return ctx ? ctx->device : NULL;
}

/* ============================================================================
 * API DE INPUT (Manteve-se igual, pois acessa struct interna)
 * ============================================================================
 */

bool ri_ui_key_down(ri_ui_ctx_t ctx, uint32_t keycode)
{
	if (!ctx || keycode >= RI_UI_MAX_KEYS)
		return false;
	if (ctx->input.input_blocked)
		return false;
	return ctx->input.keys[keycode];
}

bool ri_ui_key_pressed(ri_ui_ctx_t ctx, uint32_t keycode)
{
	if (!ctx || keycode >= RI_UI_MAX_KEYS)
		return false;
	if (ctx->input.input_blocked)
		return false;
	return ctx->input.keys[keycode] && !ctx->input.keys_prev[keycode];
}

void ri_ui_mouse_pos(ri_ui_ctx_t ctx, int32_t *x, int32_t *y)
{
	if (!ctx)
		return;
	if (ctx->input.input_blocked) {
		if (x) *x = -9999;
		if (y) *y = -9999;
		return;
	}
	if (x)
		*x = ctx->input.mouse_x;
	if (y)
		*y = ctx->input.mouse_y;
}

bool ri_ui_mouse_down(ri_ui_ctx_t ctx, int button)
{
	if (!ctx || button < 0 || button >= RI_UI_MAX_BUTTONS)
		return false;
	if (ctx->input.input_blocked)
		return false;
	return ctx->input.buttons[button];
}

bool ri_ui_mouse_clicked(ri_ui_ctx_t ctx, int button)
{
	if (!ctx || button < 0 || button >= RI_UI_MAX_BUTTONS)
		return false;
	if (ctx->input.input_blocked)
		return false;
	return ctx->input.buttons[button] && !ctx->input.buttons_prev[button];
}

float ri_ui_mouse_scroll(ri_ui_ctx_t ctx)
{
	if (ctx && ctx->input.input_blocked)
		return 0.0f;
	return ctx ? ctx->input.scroll_y : 0.0f;
}

void ri_ui_set_input_blocked(ri_ui_ctx_t ctx, bool blocked)
{
	if (ctx)
		ctx->input.input_blocked = blocked;
}

/* ============================================================================
 * STUBS (Drawing - vai tudo pra render2d.c depois)
 * ============================================================================
 */

/* Funções gráficas implementadas em render2d.c */

void ri_ui_set_vsync(ri_ui_ctx_t ctx, bool enabled)
{
	if (!ctx)
		return;
	ctx->vsync_target = enabled;
	ctx->vsync_pending = true;
}
