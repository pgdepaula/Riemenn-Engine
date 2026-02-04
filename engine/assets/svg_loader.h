#ifndef BHS_ASSETS_SVG_LOADER_H
#define BHS_ASSETS_SVG_LOADER_H

#include "image_loader.h" /* Para bhs_image_t */

/**
 * @file svg_loader.h
 * @brief Carregador e Rasterizador SVG (C Puro)
 *
 * Implementação minimalista de um subconjunto de SVG 1.1.
 * Foco em parsing de paths (d="M..."), formas básicas e preenchimento sólido.
 */

/* ========================================================================= */
/*                              ESTRUTURAS                                   */
/* ========================================================================= */

/**
 * @struct bhs_svg
 * @brief Representação vetorial parseada do SVG.
 * 
 * Contém a lista de shapes (caminhos, retângulos, etc) prontos para rasterização.
 * Mantenha como handle opaco se possível, mas aqui expomos o mínimo para debug.
 */
typedef struct bhs_vec2 {
	float x, y;
} bhs_vec2_t;

/* Um "path" é uma sequencia de pontos conectados formando um poligono (aberto ou fechado) */
struct bhs_path {
	bhs_vec2_t *pts;
	int n_pts;
	int closed;
	struct bhs_path *next;
};
typedef struct bhs_path bhs_path_t;

/* Um "shape" contém atributos visuais e uma lista de paths decompostos */
struct bhs_shape {
	uint32_t fill_color;   /* ABGR packed */
	uint32_t stroke_color; /* ABGR packed */
	float stroke_width;
	float opacity;
	int has_fill;
	int has_stroke;

	bhs_path_t *paths;
	struct bhs_shape *next;
};
typedef struct bhs_shape bhs_shape_t;

struct bhs_svg {
	float width;
	float height;
	bhs_shape_t *shapes;
};
typedef struct bhs_svg bhs_svg_t;

/* ========================================================================= */
/*                                API PÚBLICA                                */
/* ========================================================================= */

/**
 * @brief Carrega, parseia e processa um arquivo SVG do disco.
 * 
 * Lê o XML, processa os paths e prepara para rasterização rápida.
 * Ignora tags e atributos não suportados sem reclamar (muito).
 * 
 * @param path Caminho absoluto ou relativo para o arquivo .svg
 * @return Ponteiro para estrutura SVG opaca, ou NULL em erro.
 */
bhs_svg_t *bhs_svg_load(const char *path);

/**
 * @brief Carrega SVG a partir de um buffer em memória (string null-terminated).
 * 
 * @param buffer Conteúdo XML do SVG
 * @return Ponteiro para estrutura SVG opaca, ou NULL em erro.
 */
bhs_svg_t *bhs_svg_parse(const char *buffer);

/**
 * @brief Libera a memória da estrutura SVG.
 */
void bhs_svg_free(bhs_svg_t *svg);

/**
 * @brief Rasteriza o vetor em uma imagem de pixels.
 * 
 * @param svg O vetor carregado.
 * @param scale Fator de escala (1.0 = tamanho original definido no SVG).
 *              Aumente para super-resolution.
 * @return Imagem RGBA (bhs_image_t) alocada. Deve ser liberada com bhs_image_free.
 */
bhs_image_t bhs_svg_rasterize(const bhs_svg_t *svg, float scale);

/**
 * @brief Rasteriza com tamanho alvo fixo (faz fit/contain).
 * 
 * @param width Largura desejada em pixels.
 * @param height Altura desejada em pixels.
 */
bhs_image_t bhs_svg_rasterize_fit(const bhs_svg_t *svg, int width, int height);

#endif /* BHS_ASSETS_SVG_LOADER_H */
