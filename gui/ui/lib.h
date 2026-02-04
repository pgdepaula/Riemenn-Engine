/**
 * @file lib.h
 * @brief Biblioteca UI unificada - o casamento feliz de platform + renderer
 *
 * Aqui a gente junta o Wayland/Cocoa/Win32 com o Vulkan/Metal/DX e finge
 * que tudo funciona de primeira. Spoiler: não funciona, mas a gente tenta.
 *
 * A ideia é simples: você quer uma janela com botões? Não precisa saber
 * que por baixo tem 2000 linhas de Vulkan. Só chama bhs_ui_button() e
 * torce pro universo cooperar.
 *
 * Estrutura:
 * - bhs_ui_ctx: Contexto que agrupa tudo (janela, GPU, input, widgets)
 * - Frame loop: begin_frame() -> desenha coisas -> end_frame()
 * - Widgets: Immediate mode UI (tipo Dear ImGui, mas pior e feito em C)
 *
 * Invariantes:
 * - Um contexto = uma janela = um swapchain
 * - Widgets só existem durante o frame (immediate mode)
 * - Se crashar, é culpa do driver gráfico (mentira, é minha)
 */

#ifndef BHS_UX_UI_LIB_H
#define BHS_UX_UI_LIB_H

#include <stdbool.h>
#include <stddef.h> /* [FIX] For size_t */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TIPOS OPACOS
 * ============================================================================
 */

/**
 * Contexto UI - o grande chefão que gerencia tudo.
 *
 * Internamente tem: platform, window, gpu device, swapchain, input state,
 * widget state, render batch... basicamente a cozinha inteira.
 */
typedef struct bhs_ui_ctx_impl *bhs_ui_ctx_t;

/**
 * Handle para Atlas de Fontes (Texture + Glyphs)
 */
typedef struct bhs_font_atlas_impl *bhs_font_atlas_t;

/* ============================================================================
 * CÓDIGOS DE ERRO
 * ============================================================================
 */

enum bhs_ui_error {
	BHS_UI_OK = 0,
	BHS_UI_SKIP = 1,	 /* [FIX] Frame deve ser pulado (ex: resize) */
	BHS_UI_ERR_NOMEM = -1,	 /* Sem memória (CPU ou GPU) */
	BHS_UI_ERR_INIT = -2,	 /* Falha na inicialização */
	BHS_UI_ERR_WINDOW = -3,	 /* Falha ao criar janela */
	BHS_UI_ERR_GPU = -4,	 /* Falha no device gráfico */
	BHS_UI_ERR_INVALID = -5, /* Parâmetro inválido */
}; // isso significa que alguem (vulgo eu) vai chorar no banho, quem mandou
// mexer com C?

/* ============================================================================
 * CONFIGURAÇÃO
 * ============================================================================
 */

/**
 * Configuração para criar o contexto UI.
 *
 * Basicamente: "que janela você quer?"
 */
struct bhs_ui_config {
	const char *title; /* Título da janela */
	int32_t width;	   /* Largura inicial */
	int32_t height;	   /* Altura inicial */
	bool resizable;	   /* Permite redimensionar? */
	bool vsync;	   /* VSync habilitado? */
	bool debug;	   /* Validation layers e logs verbosos */
};

/* ============================================================================
 * CORES
 * ============================================================================
 */

/**
 * Cor RGBA normalizada (0.0 - 1.0).
 *
 * Por que float? Porque é o século 21 e 8 bits por canal é coisa de 1995.
 */
struct bhs_ui_color {
	float r, g, b, a;
};

/* Cores pré-definidas pra preguiçoso (tipo eu) */
#define BHS_UI_COLOR_WHITE ((struct bhs_ui_color){ 1.0f, 1.0f, 1.0f, 1.0f })
#define BHS_UI_COLOR_BLACK ((struct bhs_ui_color){ 0.0f, 0.0f, 0.0f, 1.0f })
#define BHS_UI_COLOR_RED ((struct bhs_ui_color){ 1.0f, 0.0f, 0.0f, 1.0f })
#define BHS_UI_COLOR_GREEN ((struct bhs_ui_color){ 0.0f, 1.0f, 0.0f, 1.0f })
#define BHS_UI_COLOR_BLUE ((struct bhs_ui_color){ 0.0f, 0.0f, 1.0f, 1.0f })
#define BHS_UI_COLOR_GRAY ((struct bhs_ui_color){ 0.5f, 0.5f, 0.5f, 1.0f })
#define BHS_UI_COLOR_TRANSPARENT                                               \
	((struct bhs_ui_color){ 0.0f, 0.0f, 0.0f, 0.0f })

/* ============================================================================
 * RETÂNGULO
 * ============================================================================
 */

/**
 * Retângulo com posição e tamanho.
 *
 * Coordenadas: (0,0) é canto superior esquerdo. Y cresce pra baixo.
 * Sim, igual HTML. Não, não foi minha escolha.
 */
struct bhs_ui_rect {
	float x, y;
	float width, height;
};

/* ============================================================================
 * API PRINCIPAL
 * ============================================================================
 */

/**
 * bhs_ui_create - Cria contexto UI com janela e tudo mais
 *
 * Isso aqui faz MUITA coisa por baixo:
 * 1. Inicializa platform (Wayland/Cocoa/Win32)
 * 2. Cria janela
 * 3. Inicializa GPU (Vulkan/Metal/DX)
 * 4. Cria swapchain
 * 5. Prepara sistema de input
 * 6. Inicializa batching de widgets
 *
 * Se qualquer etapa falhar, limpa tudo e retorna erro.
 * Tipo um foguete: ou decola perfeito ou explode espetacularmente.
 *
 * @config: Configuração da janela
 * @ctx: Ponteiro para receber o contexto
 *
 * Retorna: BHS_UI_OK ou código de erro
 */
int bhs_ui_create(const struct bhs_ui_config *config, bhs_ui_ctx_t *ctx);

/**
 * bhs_ui_destroy - Destrói contexto e libera tudo
 *
 * Faz o cleanup na ordem inversa da criação.
 * Depois disso, @ctx é inválido. Não usa mais.
 */
void bhs_ui_destroy(bhs_ui_ctx_t ctx);
void bhs_ui_quit(bhs_ui_ctx_t ctx);

/**
 * bhs_ui_should_close - Verifica se deve fechar
 *
 * Retorna true se o usuário clicou no X ou pressionou Alt+F4.
 * Use no loop: while (!bhs_ui_should_close(ctx)) { ... }
 */
bool bhs_ui_should_close(bhs_ui_ctx_t ctx);
/**
 * bhs_ui_set_vsync - Habilita/Desabilita VSync em tempo de execução
 * Requer recriar o swapchain, que ocorrerá no próximo frame.
 */
void bhs_ui_set_vsync(bhs_ui_ctx_t ctx, bool enabled);

/**
 * bhs_ui_begin_frame - Inicia um frame
 *
 * Faz poll de eventos, atualiza input state, prepara batching.
 * DEVE ser chamado antes de qualquer widget ou desenho.
 *
 * Retorna: BHS_UI_OK ou BHS_UI_ERR_* se swapchain morreu
 */
int bhs_ui_begin_frame(bhs_ui_ctx_t ctx);

/**
 * bhs_ui_end_frame - Finaliza e apresenta frame
 *
 * Submete todos os comandos de desenho e faz present.
 * DEVE ser chamado após todos os widgets.
 */
int bhs_ui_end_frame(bhs_ui_ctx_t ctx);

/**
 * bhs_ui_get_size - Obtém tamanho da janela
 */
void bhs_ui_get_size(bhs_ui_ctx_t ctx, int32_t *width, int32_t *height);

/**
 * bhs_ui_get_gpu_device - Obtém o device GPU (opaque handle)
 *
 * Necessário para inicializar o motor de física na mesma GPU.
 */
/* ============================================================================
 * UTILS
 * ============================================================================
 */

/**
 * bhs_ui_cmd_begin - Inicia gravação do command buffer (Reset + Begin)
 * Deve ser chamado antes de qualquer operação de GPU no frame.
 */
void bhs_ui_cmd_begin(bhs_ui_ctx_t ctx);

void *bhs_ui_get_gpu_device(bhs_ui_ctx_t ctx);

/**
 * bhs_ui_get_current_cmd - Obtém o buffer de comando atual (void* casting
 * necessario)
 *
 * Útil para injetar comandos customizados (compute, transfer) antes da
 * renderização.
 */
void *bhs_ui_get_current_cmd(bhs_ui_ctx_t ctx);

/**
 * bhs_ui_flush - Força o envio do batch atual para a GPU
 * Útil antes de mudar o pipeline manualmente.
 */
void bhs_ui_flush(bhs_ui_ctx_t ctx);

/**
 * bhs_ui_reset_render_state - Restaura o pipeline e estado da UI
 * Útil após desenhar coisas customizadas que alteram o pipeline.
 */
void bhs_ui_reset_render_state(bhs_ui_ctx_t ctx);

/**
 * bhs_ui_begin_drawing - Inicia explicitamente o Render Pass
 *
 * Se você usar isso, bhs_ui_begin_frame NÃO iniciará o render pass
 * automaticamente. Isso permite rodar Compute Shaders antes de desenhar.
 */
void bhs_ui_begin_drawing(bhs_ui_ctx_t ctx);

/* ============================================================================
 * API DE INPUT
 * ============================================================================
 */

/**
 * bhs_ui_key_down - Tecla está pressionada agora?
 */
bool bhs_ui_key_down(bhs_ui_ctx_t ctx, uint32_t keycode);

/**
 * bhs_ui_key_pressed - Tecla foi pressionada NESTE frame?
 *
 * Diferente de key_down: só retorna true uma vez por pressionamento.
 */
bool bhs_ui_key_pressed(bhs_ui_ctx_t ctx, uint32_t keycode);

/**
 * bhs_ui_mouse_pos - Posição atual do mouse
 */
void bhs_ui_mouse_pos(bhs_ui_ctx_t ctx, int32_t *x, int32_t *y);

/**
 * bhs_ui_mouse_down - Botão do mouse está pressionado?
 */
bool bhs_ui_mouse_down(bhs_ui_ctx_t ctx, int button);

/**
 * bhs_ui_mouse_clicked - Botão foi clicado NESTE frame?
 */
bool bhs_ui_mouse_clicked(bhs_ui_ctx_t ctx, int button);

/**
 * bhs_ui_mouse_scroll - Obtém delta do scroll vertical NESTE frame
 */
float bhs_ui_mouse_scroll(bhs_ui_ctx_t ctx);

/**
 * bhs_ui_set_input_blocked - Bloqueia/Desbloqueia input globalmente
 *
 * Útil para modais. Quando bloqueado:
 * - mouse_pos retorna (-9999, -9999)
 * - clicks e keys retornam false
 */
void bhs_ui_set_input_blocked(bhs_ui_ctx_t ctx, bool blocked);

/* ============================================================================
 * API DE DESENHO 2D
 * ============================================================================
 */

/**
 * bhs_ui_clear - Limpa tela com cor
 */
void bhs_ui_clear(bhs_ui_ctx_t ctx, struct bhs_ui_color color);

/**
 * bhs_ui_draw_rect - Desenha retângulo preenchido
 */
void bhs_ui_draw_rect(bhs_ui_ctx_t ctx, struct bhs_ui_rect rect,
		      struct bhs_ui_color color);

/**
 * bhs_ui_draw_rect_outline - Desenha borda de retângulo
 */
void bhs_ui_draw_rect_outline(bhs_ui_ctx_t ctx, struct bhs_ui_rect rect,
			      struct bhs_ui_color color, float thickness);

/**
 * bhs_ui_draw_line - Desenha uma linha entre dois pontos
 *
 * Implementada como um quad rotacionado para permitir espessura controlada.
 */
void bhs_ui_draw_line(bhs_ui_ctx_t ctx, float x1, float y1, float x2, float y2,
		      struct bhs_ui_color color, float thickness);

/**
 * bhs_ui_draw_circle_fill - Desenha um círculo preenchido
 *
 * Aproximação por polígono (Triangle Fan).
 */
void bhs_ui_draw_circle_fill(bhs_ui_ctx_t ctx, float cx, float cy, float radius,
			     struct bhs_ui_color color);

/**
 * bhs_ui_draw_text - Desenha texto
 *
 * Fonte: monospace builtin. Não pergunta, só aceita.
 */
void bhs_ui_draw_text(bhs_ui_ctx_t ctx, const char *text, float x, float y,
		      float size, struct bhs_ui_color color);

/**
 * @brief Mede as dimensões de um texto com o sistema de fontes atual
 */
float bhs_ui_measure_text(bhs_ui_ctx_t ctx, const char *text, float size);

/**
 * bhs_ui_draw_texture - Desenha textura (quad texturizado)
 *
 * Fundamental para o Viewport do simulador.
 * @texture: Se NULL, desenha retângulo branco (equivalente a draw_rect)
 */
void bhs_ui_draw_texture(bhs_ui_ctx_t ctx,
			 /* bhs_gpu_texture_t */ void *texture, float x,
			 float y, float w, float h, struct bhs_ui_color color);

/**
 * @brief Desenha textura com coordenadas UV controladas
 * Permite scrolling, tiling e atlas.
 */
void bhs_ui_draw_texture_uv(bhs_ui_ctx_t ctx, void *texture, float x, float y,
			    float w, float h, float u0, float v0, float u1,
			    float v1, struct bhs_ui_color color);

/**
 * @brief Cria uma textura a partir de dados RGBA em memória.
 * @param width Largura da imagem
 * @param height Altura da imagem
 * @param data Ponteiro para dados RGBA (uint8_t), row-major.
 * @return Handle opaco da textura ou NULL em erro.
 */
void *bhs_ui_create_texture_from_rgba(bhs_ui_ctx_t ctx, int width, int height,
				      const void *data);

/**
 * @brief Desenha um quad com UVs arbitrários para cada vértice (TL, TR, BR, BL)
 * Essencial para distorções complexas (esferas, etc).
 */
void bhs_ui_draw_quad_uv(bhs_ui_ctx_t ctx, void *texture, float x0, float y0,
			 float u0, float v0,			 /* TL */
			 float x1, float y1, float u1, float v1, /* TR */
			 float x2, float y2, float u2, float v2, /* BR */
			 float x3, float y3, float u3, float v3, /* BL */
			 struct bhs_ui_color color);

/* ============================================================================
 * ÍCONES E SÍMBOLOS
 * ============================================================================
 */

enum bhs_ui_icon {
	BHS_ICON_NONE = 0,
	BHS_ICON_GEAR,	  /* Configurações */
	BHS_ICON_PHYSICS, /* Parâmetros físicos */
	BHS_ICON_CAMERA,  /* Parâmetros de visão */
	BHS_ICON_INFO,	  /* Sobre / Ajuda */
	BHS_ICON_CLOSE,	  /* Fechar modal */
};

/* ============================================================================
 * API DE WIDGETS (IMMEDIATE MODE)
 * ============================================================================
 */

/**
 * bhs_ui_button - Desenha botão e retorna se foi clicado
 *
 * Immediate mode: chama todo frame, retorna true quando clicado.
 */
bool bhs_ui_button(bhs_ui_ctx_t ctx, const char *label,
		   struct bhs_ui_rect rect);

/**
 * bhs_ui_icon_button - Botão circular com ícone
 *
 * Ideal para controles flutuantes no canto da tela.
 * Retorna true se clicado.
 */
bool bhs_ui_icon_button(bhs_ui_ctx_t ctx, enum bhs_ui_icon icon, float x,
			float y, float size);

/**
 * bhs_ui_label - Desenha label (texto estático)
 */
void bhs_ui_label(bhs_ui_ctx_t ctx, const char *text, float x, float y);

/**
 * bhs_ui_panel - Desenha painel (background + borda)
 *
 * Use pra agrupar widgets visualmente.
 */
void bhs_ui_panel(bhs_ui_ctx_t ctx, struct bhs_ui_rect rect,
		  struct bhs_ui_color bg, struct bhs_ui_color border);

/**
 * bhs_ui_panel_begin - Inicia um painel modal centralizado
 *
 * Cria um overlay escurecido sobre a tela e centraliza uma janela.
 * Use bhs_ui_panel_end() para fechar o escopo.
 */
void bhs_ui_panel_begin(bhs_ui_ctx_t ctx, const char *title, float width,
			float height);

/**
 * bhs_ui_panel_end - Finaliza o painel modal
 */
void bhs_ui_panel_end(bhs_ui_ctx_t ctx);

/**
 * bhs_ui_slider - Slider horizontal
 */
bool bhs_ui_slider(bhs_ui_ctx_t ctx, struct bhs_ui_rect rect, float *value);

/**
 * bhs_ui_checkbox - Checkbox
 */
bool bhs_ui_checkbox(bhs_ui_ctx_t ctx, const char *label,
		     struct bhs_ui_rect rect, bool *checked);

/**
 * bhs_ui_text_field - Campo de Texto
 *
 * @focused: Estado de foco gerido externamente (ou internamente se NULL)
 * Returns true se o texto mudou.
 */
bool bhs_ui_text_field(bhs_ui_ctx_t ctx, struct bhs_ui_rect rect, char *buf,
		       size_t max_len, bool *focused);

/* ============================================================================
 * KEYCODES
 * ============================================================================
 */

/* Alguns keycodes comuns (baseado em USB HID, tipo o que todo mundo usa) */
enum bhs_ui_key {
	BHS_KEY_ESCAPE = 1,
	BHS_KEY_1 = 2,
	BHS_KEY_2 = 3,
	BHS_KEY_3 = 4,
	BHS_KEY_4 = 5,
	BHS_KEY_5 = 6,
	BHS_KEY_6 = 7,
	BHS_KEY_7 = 8,
	BHS_KEY_8 = 9,
	BHS_KEY_9 = 10,
	BHS_KEY_0 = 11,
	BHS_KEY_Q = 16,
	BHS_KEY_W = 17,
	BHS_KEY_E = 18,
	BHS_KEY_R = 19,
	BHS_KEY_T = 20,
	BHS_KEY_Y = 21,
	BHS_KEY_U = 22,
	BHS_KEY_I = 23,
	BHS_KEY_O = 24,
	BHS_KEY_P = 25,
	BHS_KEY_A = 30,
	BHS_KEY_S = 31,
	BHS_KEY_D = 32,
	BHS_KEY_F = 33,
	BHS_KEY_G = 34,
	BHS_KEY_H = 35,
	BHS_KEY_J = 36,
	BHS_KEY_K = 37,
	BHS_KEY_L = 38,
	BHS_KEY_Z = 44,
	BHS_KEY_X = 45,
	BHS_KEY_C = 46,
	BHS_KEY_V = 47,
	BHS_KEY_B = 48,
	BHS_KEY_N = 49,
	BHS_KEY_M = 50,
	BHS_KEY_SPACE = 57,
	BHS_KEY_ENTER = 28,
	BHS_KEY_UP = 103,
	BHS_KEY_DOWN = 108,
	BHS_KEY_LEFT = 105,
	BHS_KEY_RIGHT = 106,
	BHS_KEY_LEFTSHIFT = 42,
	BHS_KEY_RIGHTSHIFT = 54,
};

/* Mouse buttons: usa BHS_MOUSE_LEFT/RIGHT/MIDDLE de platform/platform.h */

/* ============================================================================
 * MODULOS ADICIONAIS
 * ============================================================================
 */

#include "gui/ui/layout.h"
#include "gui/ui/theme.h"

#ifdef __cplusplus
}
#endif

#endif /* BHS_UX_UI_LIB_H */

// Eu terminaria isso mais rapido se só escrevesse código e não ficasse meia
// hora escrevendo comentarios que ninguem vai ler? Talvez. Você tá lendo isso?
// Tem alguem lendo isso?