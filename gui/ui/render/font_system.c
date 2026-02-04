/**
 * @file font_system.c
 * @brief Implementação do Sistema de Fontes (FreeType + Fontconfig)
 */

#include "font_system.h"
#include <fontconfig/fontconfig.h>
#include <ft2build.h>
#include "gui/log.h"
#include "gui/ui/internal.h"
#include FT_FREETYPE_H
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * HELPERS
 * ============================================================================
 */

static char *find_system_font(const char *pattern_str)
{
	FcConfig *config = FcInitLoadConfigAndFonts();
	FcPattern *pat = FcNameParse((const FcChar8 *)pattern_str);
	FcConfigSubstitute(config, pat, FcMatchPattern);
	FcDefaultSubstitute(pat);

	FcResult result;
	FcPattern *font = FcFontMatch(config, pat, &result);
	char *file = NULL;

	if (font) {
		FcChar8 *filename = NULL;
		if (FcPatternGetString(font, FC_FILE, 0, &filename) ==
		    FcResultMatch) {
			file = strdup((const char *)filename);
		}
		FcPatternDestroy(font);
	}

	FcPatternDestroy(pat);
	FcConfigDestroy(config);
	FcFini();

	return file;
}

/* ============================================================================
 * API PRINCIPAL
 * ============================================================================
 */

int bhs_font_system_init(struct bhs_ui_ctx_impl *ctx)
{
	if (!ctx || ctx->font.initialized)
		return BHS_UI_OK;

	BHS_LOG_INFO("Inicializando Sistema de Fontes Riemann...");

	/* 1. Localiza a fonte */
	char *font_path = find_system_font("sans-serif");
	if (!font_path) {
		BHS_LOG_ERROR("Falha ao encontrar fonte do sistema!");
		return BHS_UI_ERR_INIT;
	}
	BHS_LOG_INFO("  > Usando fonte: %s", font_path);

	/* 2. Inicializa FreeType */
	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		BHS_LOG_ERROR("Falha ao inicializar FreeType!");
		free(font_path);
		return BHS_UI_ERR_INIT;
	}

	FT_Face face;
	/* Usando tamanho fixo de 64px para o atlas de alta qualidade */
	if (FT_New_Face(ft, font_path, 0, &face)) {
		BHS_LOG_ERROR("Falha ao carregar face da fonte!");
		FT_Done_FreeType(ft);
		free(font_path);
		return BHS_UI_ERR_INIT;
	}

	FT_Set_Pixel_Sizes(face, 0, 64);

	/* 3. Cria Atlas (Simplificado: Grid 16x16 para os 128 primeiros ASCII) */
	/* Tamanho total: 1024x1024 (64 * 16 = 1024) */
	uint32_t atlas_dim = 1024;
	/* Usando RGBA8 para compatibilidade direta com o shader ui.frag */
	uint8_t *atlas_data = calloc(atlas_dim * atlas_dim * 4, 1);

	ctx->font.atlas_width = (float)atlas_dim;
	ctx->font.atlas_height = (float)atlas_dim;

	for (int c = 32; c < 256; c++) {
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			continue;

		FT_GlyphSlot g = face->glyph;
		int row = (c - 32) / 16;
		int col = (c - 32) % 16;
		int x_offset = col * 64;
		int y_offset = row * 64;

		/* Copia bitmap para o atlas (RGBA8) */
		for (uint32_t y = 0; y < g->bitmap.rows; y++) {
			for (uint32_t x = 0; x < g->bitmap.width; x++) {
				uint8_t alpha =
					g->bitmap.buffer[y * g->bitmap.width +
							 x];
				size_t idx = ((y_offset + y) * atlas_dim +
					      (x_offset + x)) *
					     4;
				atlas_data[idx + 0] = 255;   /* R */
				atlas_data[idx + 1] = 255;   /* G */
				atlas_data[idx + 2] = 255;   /* B */
				atlas_data[idx + 3] = alpha; /* A */
			}
		}

		/* Salva info do glyph */
		struct bhs_glyph_info *info = &ctx->font.glyphs[c];
		info->u0 = (float)x_offset / atlas_dim;
		info->v0 = (float)y_offset / atlas_dim;
		info->u1 = (float)(x_offset + g->bitmap.width) / atlas_dim;
		info->v1 = (float)(y_offset + g->bitmap.rows) / atlas_dim;
		info->width = g->bitmap.width;
		info->height = g->bitmap.rows;
		info->bearing_x = g->bitmap_left;
		info->bearing_y = g->bitmap_top;
		info->advance = g->advance.x >> 6;
	}

	/* 4. Upload para GPU */
	struct bhs_gpu_texture_config tex_cfg = {
		.width = atlas_dim,
		.height = atlas_dim,
		.depth = 1,
		.format = BHS_FORMAT_RGBA8_UNORM,
		.usage = BHS_TEXTURE_SAMPLED | BHS_TEXTURE_TRANSFER_DST,
		.mip_levels = 1,
		.array_layers = 1,
		.label = "Font Atlas"
	};

	if (bhs_gpu_texture_create(ctx->device, &tex_cfg,
				   &ctx->font.atlas_tex) != BHS_GPU_OK) {
		BHS_LOG_ERROR("Falha ao criar textura de atlas de fonte!");
	} else {
		bhs_gpu_texture_upload(ctx->font.atlas_tex, 0, 0, atlas_data,
				       atlas_dim * atlas_dim * 4);
	}

	/* Cleanup */
	free(atlas_data);
	FT_Done_Face(face);
	FT_Done_FreeType(ft);
	free(font_path);

	ctx->font.initialized = true;
	BHS_LOG_INFO("Sistema de Fontes pronto. Visual premium ativado.");

	return BHS_UI_OK;
}

void bhs_font_system_shutdown(struct bhs_ui_ctx_impl *ctx)
{
	if (!ctx || !ctx->font.initialized)
		return;

	if (ctx->font.atlas_tex) {
		bhs_gpu_texture_destroy(ctx->font.atlas_tex);
	}
	ctx->font.initialized = false;
}

const struct bhs_glyph_info *
bhs_font_system_get_glyph(struct bhs_ui_ctx_impl *ctx, char c)
{
	if (!ctx || !ctx->font.initialized)
		return NULL;
	uint8_t idx = (uint8_t)c;
	if (idx < 32)
		return NULL;
	return &ctx->font.glyphs[idx];
}
