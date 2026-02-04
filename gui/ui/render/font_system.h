/**
 * @file font_system.h
 * @brief Sistema de Fontes Dinâmico (FreeType + Fontconfig)
 * 
 * "Porque desenhar texto pixel por pixel é coisa de quem não tem o que fazer."
 */

#ifndef BHS_UX_UI_FONT_SYSTEM_H
#define BHS_UX_UI_FONT_SYSTEM_H

#include "gui/rhi/rhi.h"
#include "gui/ui/lib.h"

/* Forward declaration */
struct bhs_ui_ctx_impl;

/**
 * @brief Glyph info para o atlas
 */
struct bhs_glyph_info {
	float u0, v0, u1, v1; /* Coordenadas UV no atlas */
	int width, height;    /* Tamanho em pixels */
	int bearing_x, bearing_y;
	int advance;
};

/**
 * @brief Estado do sistema de fontes
 */
struct bhs_font_system {
	bhs_gpu_texture_t atlas_tex;
	struct bhs_glyph_info glyphs[256]; /* Cache para ASCII por enquanto */
	float atlas_width, atlas_height;
	bool initialized;
};

/**
 * @brief Inicializa o sistema de fontes
 */
int bhs_font_system_init(struct bhs_ui_ctx_impl *ctx);

/**
 * @brief Finaliza o sistema de fontes
 */
void bhs_font_system_shutdown(struct bhs_ui_ctx_impl *ctx);

/**
 * @brief Obtém informações de um glyph (ASCII)
 */
const struct bhs_glyph_info *
bhs_font_system_get_glyph(struct bhs_ui_ctx_impl *ctx, char c);

#endif /* BHS_UX_UI_FONT_SYSTEM_H */
