/**
 * @file layout.h
 * @brief Engine de Layout (Flexbox-lite)
 *
 * Chega de calcular pixel na mão. Bem-vindo ao século 21.
 */

#ifndef RI_UX_LIB_LAYOUT_H
#define RI_UX_LIB_LAYOUT_H

#include "engine/ui/ui.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ENUMS
 * ============================================================================
 */

typedef enum { RI_LAYOUT_ROW, RI_LAYOUT_COLUMN } ri_layout_dir_t;

typedef enum {
	RI_ALIGN_START,
	RI_ALIGN_CENTER,
	RI_ALIGN_END,
	RI_ALIGN_STRETCH
} ri_align_t;

typedef enum {
	RI_JUSTIFY_START,
	RI_JUSTIFY_CENTER,
	RI_JUSTIFY_END,
	RI_JUSTIFY_SPACE_BETWEEN,
	RI_JUSTIFY_SPACE_AROUND
} ri_justify_t;

/* ============================================================================
 * ESTRUTURAS
 * ============================================================================
 */

/**
 * Configuração de estilo de layout.
 */
struct ri_layout_style {
	float width; /* < 0 = auto/flex, > 0 = fixo */
	float height;
	float padding[4]; /* top, right, bottom, left */
	float margin[4];
	float gap; /* Espaço entre itens */

	ri_align_t align_items;
	ri_justify_t justify_content;
	float flex_grow; /* 0 = não cresce, 1 = cresce */
};

/* ============================================================================
 * API
 * ============================================================================
 */

/**
 * ri_layout_begin - Inicia um container de layout
 *
 * @dir: Direção (ROW ou COLUMN)
 * @style: Estilo do container
 * @rect: Retângulo disponível (se NULL, usa o pai ou janela inteira)
 */
void ri_layout_begin(ri_ui_ctx_t ctx, ri_layout_dir_t dir,
		      const struct ri_layout_style *style);

/**
 * ri_layout_end - Fecha o container atual
 */
void ri_layout_end(ri_ui_ctx_t ctx);

/**
 * ri_layout_next - Obtém o retângulo para o próximo item
 *
 * Calcula onde o próximo widget deve ficar baseado no layout atual.
 */
struct ri_ui_rect ri_layout_next(ri_ui_ctx_t ctx, float width, float height);

#ifdef __cplusplus
}
#endif

#endif /* RI_UX_LIB_LAYOUT_H */
