/**
 * @file layout.c
 * @brief Implementação do Layout Engine
 */

#include "engine/ui/layout.h"
#include <stdio.h>
#include <string.h>
#include "engine/foundation/assert.h"
#include "engine/ui/internal.h"

void ri_layout_begin(ri_ui_ctx_t ctx, ri_layout_dir_t dir,
		      const struct ri_layout_style *style)
{
	RI_ASSERT(ctx != NULL);

	struct ri_layout_ctx *lctx = &ctx->layout;

	if (lctx->stack_ptr >= RI_MAX_LAYOUT_STACK - 1) {
		fprintf(stderr, "[layout] erro: estouro de pilha de layout!\n");
		return;
	}

	int parent_idx = lctx->stack_ptr;
	lctx->stack_ptr++;
	struct ri_layout_node *node = &lctx->stack[lctx->stack_ptr];
	struct ri_layout_node *parent =
		(parent_idx >= 0) ? &lctx->stack[parent_idx] : NULL;

	node->dir = dir;
	node->style = *style;
	node->max_cross_size = 0;

	/* Determina retângulo inicial */
	if (parent) {
		/* Filhos herdam espaço do cursor do pai */
		node->rect.x = parent->cursor_pos.x;
		node->rect.y = parent->cursor_pos.y;

		/* Se width definido, usa. Se não, tenta pegar o resto do pai */
		node->rect.width =
			(style->width > 0)
				? style->width
				: parent->rect.width; // Placeholder logic
		node->rect.height = (style->height > 0) ? style->height
							: parent->rect.height;
	} else {
		/* Root container - pegar da janela */
		node->rect = (struct ri_ui_rect){ 0, 0, (float)ctx->width,
						   (float)ctx->height };
	}

	/* Aplica padding inicial no cursor */
	node->cursor_pos = node->rect;
	node->cursor_pos.x += style->padding[3]; // Left
	node->cursor_pos.y += style->padding[0]; // Top
	node->cursor_pos.width -= (style->padding[1] + style->padding[3]);
	node->cursor_pos.height -= (style->padding[0] + style->padding[2]);
}

void ri_layout_end(ri_ui_ctx_t ctx)
{
	RI_ASSERT(ctx != NULL);
	struct ri_layout_ctx *lctx = &ctx->layout;

	if (lctx->stack_ptr >= 0) {
		lctx->stack_ptr--;
	} else {
		fprintf(stderr, "[layout] aviso: layout_end sem layout_begin "
				"correspondente!\n");
	}
}

struct ri_ui_rect ri_layout_next(ri_ui_ctx_t ctx, float width, float height)
{
	if (!ctx)
		return (struct ri_ui_rect){ 0, 0, width, height };

	struct ri_layout_ctx *lctx = &ctx->layout;

	if (lctx->stack_ptr < 0) {
		/* Sem layout ativo, retorna 0,0 */
		return (struct ri_ui_rect){ 0, 0, width, height };
	}

	struct ri_layout_node *node = &lctx->stack[lctx->stack_ptr];
	struct ri_ui_rect result;

	result.x = node->cursor_pos.x;
	result.y = node->cursor_pos.y;
	result.width = width;
	result.height = height;

	/* Avança cursor */
	if (node->dir == RI_LAYOUT_ROW) {
		node->cursor_pos.x += width + node->style.gap;
		if (height > node->max_cross_size)
			node->max_cross_size = height;
	} else {
		node->cursor_pos.y += height + node->style.gap;
		if (width > node->max_cross_size)
			node->max_cross_size = width;
	}

	return result;
}
