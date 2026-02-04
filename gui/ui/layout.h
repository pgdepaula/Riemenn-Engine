/**
 * @file layout.h
 * @brief Engine de Layout (Flexbox-lite)
 *
 * Chega de calcular pixel na mão. Bem-vindo ao século 21.
 */

#ifndef BHS_UX_LIB_LAYOUT_H
#define BHS_UX_LIB_LAYOUT_H

#include "gui/ui/lib.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * ENUMS
 * ============================================================================
 */

typedef enum { BHS_LAYOUT_ROW, BHS_LAYOUT_COLUMN } bhs_layout_dir_t;

typedef enum {
	BHS_ALIGN_START,
	BHS_ALIGN_CENTER,
	BHS_ALIGN_END,
	BHS_ALIGN_STRETCH
} bhs_align_t;

typedef enum {
	BHS_JUSTIFY_START,
	BHS_JUSTIFY_CENTER,
	BHS_JUSTIFY_END,
	BHS_JUSTIFY_SPACE_BETWEEN,
	BHS_JUSTIFY_SPACE_AROUND
} bhs_justify_t;

/* ============================================================================
 * ESTRUTURAS
 * ============================================================================
 */

/**
 * Configuração de estilo de layout.
 */
struct bhs_layout_style {
	float width; /* < 0 = auto/flex, > 0 = fixo */
	float height;
	float padding[4]; /* top, right, bottom, left */
	float margin[4];
	float gap; /* Espaço entre itens */

	bhs_align_t align_items;
	bhs_justify_t justify_content;
	float flex_grow; /* 0 = não cresce, 1 = cresce */
};

/* ============================================================================
 * API
 * ============================================================================
 */

/**
 * bhs_layout_begin - Inicia um container de layout
 *
 * @dir: Direção (ROW ou COLUMN)
 * @style: Estilo do container
 * @rect: Retângulo disponível (se NULL, usa o pai ou janela inteira)
 */
void bhs_layout_begin(bhs_ui_ctx_t ctx, bhs_layout_dir_t dir,
		      const struct bhs_layout_style *style);

/**
 * bhs_layout_end - Fecha o container atual
 */
void bhs_layout_end(bhs_ui_ctx_t ctx);

/**
 * bhs_layout_next - Obtém o retângulo para o próximo item
 *
 * Calcula onde o próximo widget deve ficar baseado no layout atual.
 */
struct bhs_ui_rect bhs_layout_next(bhs_ui_ctx_t ctx, float width, float height);

#ifdef __cplusplus
}
#endif

#endif /* BHS_UX_LIB_LAYOUT_H */
