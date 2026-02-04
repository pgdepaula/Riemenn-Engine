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

#include "gui/ui/internal.h"
#include "gui/ui/lib.h"

/* ============================================================================
 * API PRINCIPAL
 * ============================================================================
 */

int bhs_ui_create(const struct bhs_ui_config *config, bhs_ui_ctx_t *ctx)
{
	if (!config || !ctx)
		return BHS_UI_ERR_INVALID;

	/* Aloca contexto */
	struct bhs_ui_ctx_impl *c = calloc(1, sizeof(*c));
	if (!c)
		return BHS_UI_ERR_NOMEM;

	int ret;

	/* === Inicializa Window (via internal wrapper) === */
	ret = bhs_ui_window_init_internal(c, config);
	if (ret != BHS_UI_OK) {
		free(c);
		return ret; /* Erro já logado no window.c */
	}

	/* === Inicializa GPU === */
	/* Pipeline interno para renderização 2D */
	struct bhs_gpu_device_config gpu_config = {
		.preferred_backend = BHS_GPU_BACKEND_AUTO,
		.enable_validation = config->debug,
		.prefer_discrete_gpu = true,
	};

	ret = bhs_gpu_device_create(&gpu_config, &c->device);
	if (ret != BHS_GPU_OK) {
		fprintf(stderr, "[ui] erro: falha ao criar device GPU (%d)\n",
			ret);
		bhs_ui_window_shutdown_internal(c);
		free(c);
		return BHS_UI_ERR_GPU;
	}

	/* === Cria Swapchain === */
	struct bhs_gpu_swapchain_config swap_config = {
		.native_display = bhs_platform_get_native_display(c->platform),
		.native_window = bhs_window_get_native_handle(c->window),
		.native_layer = bhs_window_get_native_layer(c->window),
		.width = (uint32_t)c->width,
		.height = (uint32_t)c->height,
		.format = BHS_FORMAT_BGRA8_SRGB,
		.buffer_count = 2,
		.vsync = config->vsync,
	};

	ret = bhs_gpu_swapchain_create(c->device, &swap_config, &c->swapchain);
	if (ret != BHS_GPU_OK) {
		fprintf(stderr, "[ui] erro: falha ao criar swapchain (%d)\n",
			ret);
		bhs_gpu_device_destroy(c->device);
		bhs_ui_window_shutdown_internal(c);
		free(c);
		return BHS_UI_ERR_GPU;
	}

	/* === Cria Command Buffer === */
	ret = bhs_gpu_cmd_buffer_create(c->device, &c->cmd);
	if (ret != BHS_GPU_OK) {
		fprintf(stderr,
			"[ui] erro: falha ao criar command buffer (%d)\n", ret);
		bhs_gpu_swapchain_destroy(c->swapchain);
		bhs_gpu_device_destroy(c->device);
		bhs_ui_window_shutdown_internal(c);
		free(c);
		return BHS_UI_ERR_GPU;
	}

	/* === Cria Fence de Frame === */
	ret = bhs_gpu_fence_create(c->device, &c->fence_frame);
	if (ret != BHS_GPU_OK) {
		fprintf(stderr,
			"[ui] erro: falha ao criar fence de frame (%d)\n", ret);
		bhs_gpu_cmd_buffer_destroy(c->cmd);
		bhs_gpu_swapchain_destroy(c->swapchain);
		bhs_gpu_device_destroy(c->device);
		bhs_ui_window_shutdown_internal(c);
		free(c);
		return BHS_UI_ERR_GPU;
	}

	/* === Inicializa Render 2D === */
	/* === Inicializa Render 2D === */
	ret = bhs_ui_render_init_internal(c);
	if (ret != BHS_UI_OK) {
		fprintf(stderr,
			"[ui] erro: falha ao inicializar renderer 2D (%d)\n",
			ret);
		/* Cleanup fences, cmd, swapchain, device, window */
		/* Cleanup completo dos recursos */
		return ret;
	}

	*ctx = c;
	return BHS_UI_OK;
}

void bhs_ui_quit(bhs_ui_ctx_t ctx)
{
	if (ctx)
		ctx->should_close = true;
}

void bhs_ui_destroy(bhs_ui_ctx_t ctx)
{
	if (!ctx)
		return;

	/* Cleanup na ordem inversa */
	if (ctx->cmd)
		bhs_gpu_cmd_buffer_destroy(ctx->cmd);
	if (ctx->swapchain)
		bhs_gpu_swapchain_destroy(ctx->swapchain);
	if (ctx->device)
		bhs_gpu_device_destroy(ctx->device);

	/* Cleanup window */
	bhs_ui_window_shutdown_internal(ctx);

	free(ctx);
}

bool bhs_ui_should_close(bhs_ui_ctx_t ctx)
{
	if (!ctx)
		return true;
	return ctx
		->should_close; /* Agora window wrapper atualiza isso via ctx */
}

int bhs_ui_begin_frame(bhs_ui_ctx_t ctx)
{
	if (!ctx || ctx->in_frame)
		return BHS_UI_ERR_INVALID;

	/* Wait for previous frame (se não for o primeiro) */
	if (ctx->frame_count > 0) {
		bhs_gpu_fence_wait(ctx->fence_frame, 1000000000); // 1s timeout
		bhs_gpu_fence_reset(ctx->fence_frame);
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
	bhs_ui_window_poll_events(ctx);

	/* [FIX] Se houve resize ou mudança de VSync, recria recursos agora */
	if (ctx->resize_pending || ctx->vsync_pending) {
		bool vsync_changed = ctx->vsync_pending;

		ctx->resize_pending = false;
		ctx->vsync_pending = false;

		/* Se mudou VSync, precisamos destruir e recriar, pois resize pode não suportar troca de modo */
		if (vsync_changed) {
			if (ctx->swapchain) {
				bhs_gpu_swapchain_destroy(ctx->swapchain);
				ctx->swapchain = NULL;
			}

			struct bhs_gpu_swapchain_config swap_config = {
				.native_display =
					bhs_platform_get_native_display(
						ctx->platform),
				.native_window = bhs_window_get_native_handle(
					ctx->window),
				.native_layer = bhs_window_get_native_layer(
					ctx->window),
				.width = (uint32_t)ctx->width,
				.height = (uint32_t)ctx->height,
				.format = BHS_FORMAT_BGRA8_SRGB,
				.buffer_count = 2,
				.vsync = ctx->vsync_target,
			};

			int r = bhs_gpu_swapchain_create(
				ctx->device, &swap_config, &ctx->swapchain);
			if (r != BHS_GPU_OK) {
				fprintf(stderr,
					"[ui] erro: falha ao recriar swapchain "
					"com novo VSync! (%d)\n",
					r);
				/* Tenta fallback */
			}
		} else {
			/* Apenas resize */
			if (ctx->swapchain) {
				bhs_gpu_swapchain_resize(ctx->swapchain,
							 (uint32_t)ctx->width,
							 (uint32_t)ctx->height);
			}
		}

		/* Recria depth texture */
		if (ctx->depth_texture && ctx->device) {
			bhs_gpu_texture_destroy(ctx->depth_texture);
			ctx->depth_texture = NULL;

			struct bhs_gpu_texture_config depth_cfg = {
				.width = (uint32_t)ctx->width,
				.height = (uint32_t)ctx->height,
				.depth = 1,
				.format = BHS_FORMAT_DEPTH32_FLOAT,
				.usage = BHS_TEXTURE_DEPTH_STENCIL,
				.mip_levels = 1,
				.array_layers = 1,
				.label = "UI Depth Buffer (Resized)",
			};
			bhs_gpu_texture_create(ctx->device, &depth_cfg,
					       &ctx->depth_texture);
		}

		return BHS_UI_SKIP; /* [FIX] Retorna SKIP para não tentar desenhar sem imagem */
	}

	/* Adquire imagem */
	bhs_gpu_texture_t tex = NULL;
	int ret = bhs_gpu_swapchain_next_texture(ctx->swapchain, &tex);

	if (ret != 0) {
		/* Pula frame */
		return BHS_UI_OK;
	}
	ctx->current_texture = tex;

	/* Reseta widget state */
	ctx->widget.hot_id = 0;

	ctx->in_frame = true;
	return BHS_UI_OK;
}

void bhs_ui_cmd_begin(bhs_ui_ctx_t ctx)
{
	if (ctx && ctx->cmd) {
		bhs_gpu_cmd_reset(ctx->cmd);
		bhs_gpu_cmd_begin(ctx->cmd);
	}
}

void bhs_ui_begin_drawing(bhs_ui_ctx_t ctx)
{
	if (ctx && ctx->in_frame) {
		bhs_ui_render_begin(ctx);
	}
}

void *bhs_ui_get_current_cmd(bhs_ui_ctx_t ctx)
{
	if (!ctx)
		return NULL;
	/* Retorna ponteiro opaco para bhs_gpu_cmd_buffer_t */
	return ctx->cmd;
}

int bhs_ui_end_frame(bhs_ui_ctx_t ctx)
{
	if (!ctx || !ctx->in_frame)
		return BHS_UI_ERR_INVALID;

	/* Finaliza render pass e command buffer */
	bhs_ui_render_end(ctx);

	/* Finaliza command buffer */
	bhs_gpu_cmd_end(ctx->cmd);

	/* Submete com Fence (para sincronizar frames) */
	int ret = bhs_gpu_swapchain_submit(ctx->swapchain, ctx->cmd,
					   ctx->fence_frame);
	if (ret != 0) {
		return BHS_UI_ERR_INVALID;
	}

	/* Apresenta */
	bhs_gpu_swapchain_present(ctx->swapchain);

	ctx->in_frame = false;
	return BHS_UI_OK;
}

void bhs_ui_get_size(bhs_ui_ctx_t ctx, int32_t *width, int32_t *height)
{
	if (!ctx)
		return;
	if (width)
		*width = ctx->width;
	if (height)
		*height = ctx->height;
}

void *bhs_ui_get_gpu_device(bhs_ui_ctx_t ctx)
{
	return ctx ? ctx->device : NULL;
}

/* ============================================================================
 * API DE INPUT (Manteve-se igual, pois acessa struct interna)
 * ============================================================================
 */

bool bhs_ui_key_down(bhs_ui_ctx_t ctx, uint32_t keycode)
{
	if (!ctx || keycode >= BHS_UI_MAX_KEYS)
		return false;
	if (ctx->input.input_blocked)
		return false;
	return ctx->input.keys[keycode];
}

bool bhs_ui_key_pressed(bhs_ui_ctx_t ctx, uint32_t keycode)
{
	if (!ctx || keycode >= BHS_UI_MAX_KEYS)
		return false;
	if (ctx->input.input_blocked)
		return false;
	return ctx->input.keys[keycode] && !ctx->input.keys_prev[keycode];
}

void bhs_ui_mouse_pos(bhs_ui_ctx_t ctx, int32_t *x, int32_t *y)
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

bool bhs_ui_mouse_down(bhs_ui_ctx_t ctx, int button)
{
	if (!ctx || button < 0 || button >= BHS_UI_MAX_BUTTONS)
		return false;
	if (ctx->input.input_blocked)
		return false;
	return ctx->input.buttons[button];
}

bool bhs_ui_mouse_clicked(bhs_ui_ctx_t ctx, int button)
{
	if (!ctx || button < 0 || button >= BHS_UI_MAX_BUTTONS)
		return false;
	if (ctx->input.input_blocked)
		return false;
	return ctx->input.buttons[button] && !ctx->input.buttons_prev[button];
}

float bhs_ui_mouse_scroll(bhs_ui_ctx_t ctx)
{
	if (ctx && ctx->input.input_blocked)
		return 0.0f;
	return ctx ? ctx->input.scroll_y : 0.0f;
}

void bhs_ui_set_input_blocked(bhs_ui_ctx_t ctx, bool blocked)
{
	if (ctx)
		ctx->input.input_blocked = blocked;
}

/* ============================================================================
 * STUBS (Drawing - vai tudo pra render2d.c depois)
 * ============================================================================
 */

/* Funções gráficas implementadas em render2d.c */

void bhs_ui_set_vsync(bhs_ui_ctx_t ctx, bool enabled)
{
	if (!ctx)
		return;
	ctx->vsync_target = enabled;
	ctx->vsync_pending = true;
}
