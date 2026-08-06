/**
 * @file font_system.h
 * @brief Sistema de Fontes Dinâmico (FreeType + Fontconfig)
 * 
 * "Porque desenhar texto pixel por pixel é coisa de quem não tem o que fazer."
 */

#ifndef RI_UX_UI_FONT_SYSTEM_H
#define RI_UX_UI_FONT_SYSTEM_H

#include "engine/rhi/rhi.h"
#include "engine/ui/ui.h"

/* Forward declaration */
struct ri_ui_ctx_impl;

/**
 * @brief Glyph info para o atlas
 */
struct ri_glyph_info {
	float u0, v0, u1, v1; /* Coordenadas UV no atlas */
	int width, height;    /* Tamanho em pixels */
	int bearing_x, bearing_y;
	int advance;
};

/**
 * @brief Estado do sistema de fontes
 */
struct ri_font_system {
	ri_gpu_texture_t atlas_tex;
	struct ri_glyph_info glyphs[256]; /* Cache para ASCII por enquanto */
	float atlas_width, atlas_height;
	bool initialized;
};

/**
 * @brief Inicializa o sistema de fontes
 */
int ri_font_system_init(struct ri_ui_ctx_impl *ctx);

/**
 * @brief Finaliza o sistema de fontes
 */
void ri_font_system_shutdown(struct ri_ui_ctx_impl *ctx);

/**
 * @brief Obtém informações de um glyph (ASCII)
 */
const struct ri_glyph_info *
ri_font_system_get_glyph(struct ri_ui_ctx_impl *ctx, char c);

#endif /* RI_UX_UI_FONT_SYSTEM_H */
