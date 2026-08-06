/**
 * @file lib.h
 * @brief Biblioteca UI unificada - o casamento feliz de platform + renderer
 *
 * Aqui a gente junta o Wayland/Cocoa/Win32 com o Vulkan/Metal/DX e finge
 * que tudo funciona de primeira. Spoiler: não funciona, mas a gente tenta.
 *
 * A ideia é simples: você quer uma janela com botões? Não precisa saber
 * que por baixo tem 2000 linhas de Vulkan. Só chama ri_ui_button() e
 * torce pro universo cooperar.
 *
 * Estrutura:
 * - ri_ui_ctx: Contexto que agrupa tudo (janela, GPU, input, widgets)
 * - Frame loop: begin_frame() -> desenha coisas -> end_frame()
 * - Widgets: Immediate mode UI (tipo Dear ImGui, mas pior e feito em C)
 *
 * Invariantes:
 * - Um contexto = uma janela = um swapchain
 * - Widgets só existem durante o frame (immediate mode)
 * - Se crashar, é culpa do driver gráfico (mentira, é minha)
 */

#ifndef RI_UX_UI_LIB_H
#define RI_UX_UI_LIB_H

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
typedef struct ri_ui_ctx_impl *ri_ui_ctx_t;

/**
 * Handle para Atlas de Fontes (Texture + Glyphs)
 */
typedef struct ri_font_atlas_impl *ri_font_atlas_t;

/* ============================================================================
 * CÓDIGOS DE ERRO
 * ============================================================================
 */

enum ri_ui_error {
	RI_UI_OK = 0,
	RI_UI_SKIP = 1,	 /* [FIX] Frame deve ser pulado (ex: resize) */
	RI_UI_ERR_NOMEM = -1,	 /* Sem memória (CPU ou GPU) */
	RI_UI_ERR_INIT = -2,	 /* Falha na inicialização */
	RI_UI_ERR_WINDOW = -3,	 /* Falha ao criar janela */
	RI_UI_ERR_GPU = -4,	 /* Falha no device gráfico */
	RI_UI_ERR_INVALID = -5, /* Parâmetro inválido */
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
struct ri_ui_config {
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
struct ri_ui_color {
	float r, g, b, a;
};

/* Cores pré-definidas pra preguiçoso (tipo eu) */
#define RI_UI_COLOR_WHITE ((struct ri_ui_color){ 1.0f, 1.0f, 1.0f, 1.0f })
#define RI_UI_COLOR_BLACK ((struct ri_ui_color){ 0.0f, 0.0f, 0.0f, 1.0f })
#define RI_UI_COLOR_RED ((struct ri_ui_color){ 1.0f, 0.0f, 0.0f, 1.0f })
#define RI_UI_COLOR_GREEN ((struct ri_ui_color){ 0.0f, 1.0f, 0.0f, 1.0f })
#define RI_UI_COLOR_BLUE ((struct ri_ui_color){ 0.0f, 0.0f, 1.0f, 1.0f })
#define RI_UI_COLOR_GRAY ((struct ri_ui_color){ 0.5f, 0.5f, 0.5f, 1.0f })
#define RI_UI_COLOR_TRANSPARENT                                               \
	((struct ri_ui_color){ 0.0f, 0.0f, 0.0f, 0.0f })

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
struct ri_ui_rect {
	float x, y;
	float width, height;
};

/* ============================================================================
 * API PRINCIPAL
 * ============================================================================
 */

/**
 * ri_ui_create - Cria contexto UI com janela e tudo mais
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
 * Retorna: RI_UI_OK ou código de erro
 */
int ri_ui_create(const struct ri_ui_config *config, ri_ui_ctx_t *ctx);

/**
 * ri_ui_destroy - Destrói contexto e libera tudo
 *
 * Faz o cleanup na ordem inversa da criação.
 * Depois disso, @ctx é inválido. Não usa mais.
 */
void ri_ui_destroy(ri_ui_ctx_t ctx);
void ri_ui_quit(ri_ui_ctx_t ctx);

/**
 * ri_ui_should_close - Verifica se deve fechar
 *
 * Retorna true se o usuário clicou no X ou pressionou Alt+F4.
 * Use no loop: while (!ri_ui_should_close(ctx)) { ... }
 */
bool ri_ui_should_close(ri_ui_ctx_t ctx);
/**
 * ri_ui_set_vsync - Habilita/Desabilita VSync em tempo de execução
 * Requer recriar o swapchain, que ocorrerá no próximo frame.
 */
void ri_ui_set_vsync(ri_ui_ctx_t ctx, bool enabled);

/**
 * ri_ui_begin_frame - Inicia um frame
 *
 * Faz poll de eventos, atualiza input state, prepara batching.
 * DEVE ser chamado antes de qualquer widget ou desenho.
 *
 * Retorna: RI_UI_OK ou RI_UI_ERR_* se swapchain morreu
 */
int ri_ui_begin_frame(ri_ui_ctx_t ctx);

/**
 * ri_ui_end_frame - Finaliza e apresenta frame
 *
 * Submete todos os comandos de desenho e faz present.
 * DEVE ser chamado após todos os widgets.
 */
int ri_ui_end_frame(ri_ui_ctx_t ctx);

/**
 * ri_ui_get_size - Obtém tamanho da janela
 */
void ri_ui_get_size(ri_ui_ctx_t ctx, int32_t *width, int32_t *height);

/**
 * ri_ui_get_gpu_device - Obtém o device GPU (opaque handle)
 *
 * Necessário para inicializar o motor de física na mesma GPU.
 */
/* ============================================================================
 * UTILS
 * ============================================================================
 */

/**
 * ri_ui_cmd_begin - Inicia gravação do command buffer (Reset + Begin)
 * Deve ser chamado antes de qualquer operação de GPU no frame.
 */
void ri_ui_cmd_begin(ri_ui_ctx_t ctx);

void *ri_ui_get_gpu_device(ri_ui_ctx_t ctx);

/**
 * ri_ui_get_current_cmd - Obtém o buffer de comando atual (void* casting
 * necessario)
 *
 * Útil para injetar comandos customizados (compute, transfer) antes da
 * renderização.
 */
void *ri_ui_get_current_cmd(ri_ui_ctx_t ctx);

/**
 * ri_ui_flush - Força o envio do batch atual para a GPU
 * Útil antes de mudar o pipeline manualmente.
 */
void ri_ui_flush(ri_ui_ctx_t ctx);

/**
 * ri_ui_reset_render_state - Restaura o pipeline e estado da UI
 * Útil após desenhar coisas customizadas que alteram o pipeline.
 */
void ri_ui_reset_render_state(ri_ui_ctx_t ctx);

/**
 * ri_ui_begin_drawing - Inicia explicitamente o Render Pass
 *
 * Se você usar isso, ri_ui_begin_frame NÃO iniciará o render pass
 * automaticamente. Isso permite rodar Compute Shaders antes de desenhar.
 */
void ri_ui_begin_drawing(ri_ui_ctx_t ctx);

/* ============================================================================
 * API DE INPUT
 * ============================================================================
 */

/**
 * ri_ui_key_down - Tecla está pressionada agora?
 */
bool ri_ui_key_down(ri_ui_ctx_t ctx, uint32_t keycode);

/**
 * ri_ui_key_pressed - Tecla foi pressionada NESTE frame?
 *
 * Diferente de key_down: só retorna true uma vez por pressionamento.
 */
bool ri_ui_key_pressed(ri_ui_ctx_t ctx, uint32_t keycode);

/**
 * ri_ui_mouse_pos - Posição atual do mouse
 */
void ri_ui_mouse_pos(ri_ui_ctx_t ctx, int32_t *x, int32_t *y);

/**
 * ri_ui_mouse_down - Botão do mouse está pressionado?
 */
bool ri_ui_mouse_down(ri_ui_ctx_t ctx, int button);

/**
 * ri_ui_mouse_clicked - Botão foi clicado NESTE frame?
 */
bool ri_ui_mouse_clicked(ri_ui_ctx_t ctx, int button);

/**
 * ri_ui_mouse_scroll - Obtém delta do scroll vertical NESTE frame
 */
float ri_ui_mouse_scroll(ri_ui_ctx_t ctx);

/**
 * ri_ui_set_input_blocked - Bloqueia/Desbloqueia input globalmente
 *
 * Útil para modais. Quando bloqueado:
 * - mouse_pos retorna (-9999, -9999)
 * - clicks e keys retornam false
 */
void ri_ui_set_input_blocked(ri_ui_ctx_t ctx, bool blocked);

/* ============================================================================
 * API DE DESENHO 2D
 * ============================================================================
 */

/**
 * ri_ui_clear - Limpa tela com cor
 */
void ri_ui_clear(ri_ui_ctx_t ctx, struct ri_ui_color color);

/**
 * ri_ui_draw_rect - Desenha retângulo preenchido
 */
void ri_ui_draw_rect(ri_ui_ctx_t ctx, struct ri_ui_rect rect,
		      struct ri_ui_color color);

/**
 * ri_ui_draw_rect_outline - Desenha borda de retângulo
 */
void ri_ui_draw_rect_outline(ri_ui_ctx_t ctx, struct ri_ui_rect rect,
			      struct ri_ui_color color, float thickness);

/**
 * ri_ui_draw_line - Desenha uma linha entre dois pontos
 *
 * Implementada como um quad rotacionado para permitir espessura controlada.
 */
void ri_ui_draw_line(ri_ui_ctx_t ctx, float x1, float y1, float x2, float y2,
		      struct ri_ui_color color, float thickness);

/**
 * ri_ui_draw_circle_fill - Desenha um círculo preenchido
 *
 * Aproximação por polígono (Triangle Fan).
 */
void ri_ui_draw_circle_fill(ri_ui_ctx_t ctx, float cx, float cy, float radius,
			     struct ri_ui_color color);

/**
 * ri_ui_draw_text - Desenha texto
 *
 * Fonte: monospace builtin. Não pergunta, só aceita.
 */
void ri_ui_draw_text(ri_ui_ctx_t ctx, const char *text, float x, float y,
		      float size, struct ri_ui_color color);

/**
 * @brief Mede as dimensões de um texto com o sistema de fontes atual
 */
float ri_ui_measure_text(ri_ui_ctx_t ctx, const char *text, float size);

/**
 * ri_ui_draw_texture - Desenha textura (quad texturizado)
 *
 * Fundamental para o Viewport do simulador.
 * @texture: Se NULL, desenha retângulo branco (equivalente a draw_rect)
 */
void ri_ui_draw_texture(ri_ui_ctx_t ctx,
			 /* ri_gpu_texture_t */ void *texture, float x,
			 float y, float w, float h, struct ri_ui_color color);

/**
 * @brief Desenha textura com coordenadas UV controladas
 * Permite scrolling, tiling e atlas.
 */
void ri_ui_draw_texture_uv(ri_ui_ctx_t ctx, void *texture, float x, float y,
			    float w, float h, float u0, float v0, float u1,
			    float v1, struct ri_ui_color color);

/**
 * @brief Cria uma textura a partir de dados RGBA em memória.
 * @param width Largura da imagem
 * @param height Altura da imagem
 * @param data Ponteiro para dados RGBA (uint8_t), row-major.
 * @return Handle opaco da textura ou NULL em erro.
 */
void *ri_ui_create_texture_from_rgba(ri_ui_ctx_t ctx, int width, int height,
				      const void *data);

/**
 * @brief Desenha um quad com UVs arbitrários para cada vértice (TL, TR, BR, BL)
 * Essencial para distorções complexas (esferas, etc).
 */
void ri_ui_draw_quad_uv(ri_ui_ctx_t ctx, void *texture, float x0, float y0,
			 float u0, float v0,			 /* TL */
			 float x1, float y1, float u1, float v1, /* TR */
			 float x2, float y2, float u2, float v2, /* BR */
			 float x3, float y3, float u3, float v3, /* BL */
			 struct ri_ui_color color);

/* ============================================================================
 * ÍCONES E SÍMBOLOS
 * ============================================================================
 */

enum ri_ui_icon {
	RI_ICON_NONE = 0,
	RI_ICON_GEAR,	  /* Configurações */
	RI_ICON_PHYSICS, /* Parâmetros físicos */
	RI_ICON_CAMERA,  /* Parâmetros de visão */
	RI_ICON_INFO,	  /* Sobre / Ajuda */
	RI_ICON_CLOSE,	  /* Fechar modal */
};

/* ============================================================================
 * API DE WIDGETS (IMMEDIATE MODE)
 * ============================================================================
 */

/**
 * ri_ui_button - Desenha botão e retorna se foi clicado
 *
 * Immediate mode: chama todo frame, retorna true quando clicado.
 */
bool ri_ui_button(ri_ui_ctx_t ctx, const char *label,
		   struct ri_ui_rect rect);

/**
 * ri_ui_icon_button - Botão circular com ícone
 *
 * Ideal para controles flutuantes no canto da tela.
 * Retorna true se clicado.
 */
bool ri_ui_icon_button(ri_ui_ctx_t ctx, enum ri_ui_icon icon, float x,
			float y, float size);

/**
 * ri_ui_label - Desenha label (texto estático)
 */
void ri_ui_label(ri_ui_ctx_t ctx, const char *text, float x, float y);

/**
 * ri_ui_panel - Desenha painel (background + borda)
 *
 * Use pra agrupar widgets visualmente.
 */
void ri_ui_panel(ri_ui_ctx_t ctx, struct ri_ui_rect rect,
		  struct ri_ui_color bg, struct ri_ui_color border);

/**
 * ri_ui_panel_begin - Inicia um painel modal centralizado
 *
 * Cria um overlay escurecido sobre a tela e centraliza uma janela.
 * Use ri_ui_panel_end() para fechar o escopo.
 */
void ri_ui_panel_begin(ri_ui_ctx_t ctx, const char *title, float width,
			float height);

/**
 * ri_ui_panel_end - Finaliza o painel modal
 */
void ri_ui_panel_end(ri_ui_ctx_t ctx);

/**
 * ri_ui_slider - Slider horizontal
 */
bool ri_ui_slider(ri_ui_ctx_t ctx, struct ri_ui_rect rect, float *value);

/**
 * ri_ui_checkbox - Checkbox
 */
bool ri_ui_checkbox(ri_ui_ctx_t ctx, const char *label,
		     struct ri_ui_rect rect, bool *checked);

/**
 * ri_ui_text_field - Campo de Texto
 *
 * @focused: Estado de foco gerido externamente (ou internamente se NULL)
 * Returns true se o texto mudou.
 */
bool ri_ui_text_field(ri_ui_ctx_t ctx, struct ri_ui_rect rect, char *buf,
		       size_t max_len, bool *focused);

/* ============================================================================
 * KEYCODES
 * ============================================================================
 */

/* Alguns keycodes comuns (baseado em USB HID, tipo o que todo mundo usa) */
enum ri_ui_key {
	RI_KEY_ESCAPE = 1,
	RI_KEY_1 = 2,
	RI_KEY_2 = 3,
	RI_KEY_3 = 4,
	RI_KEY_4 = 5,
	RI_KEY_5 = 6,
	RI_KEY_6 = 7,
	RI_KEY_7 = 8,
	RI_KEY_8 = 9,
	RI_KEY_9 = 10,
	RI_KEY_0 = 11,
	RI_KEY_Q = 16,
	RI_KEY_W = 17,
	RI_KEY_E = 18,
	RI_KEY_R = 19,
	RI_KEY_T = 20,
	RI_KEY_Y = 21,
	RI_KEY_U = 22,
	RI_KEY_I = 23,
	RI_KEY_O = 24,
	RI_KEY_P = 25,
	RI_KEY_A = 30,
	RI_KEY_S = 31,
	RI_KEY_D = 32,
	RI_KEY_F = 33,
	RI_KEY_G = 34,
	RI_KEY_H = 35,
	RI_KEY_J = 36,
	RI_KEY_K = 37,
	RI_KEY_L = 38,
	RI_KEY_Z = 44,
	RI_KEY_X = 45,
	RI_KEY_C = 46,
	RI_KEY_V = 47,
	RI_KEY_B = 48,
	RI_KEY_N = 49,
	RI_KEY_M = 50,
	RI_KEY_SPACE = 57,
	RI_KEY_ENTER = 28,
	RI_KEY_UP = 103,
	RI_KEY_DOWN = 108,
	RI_KEY_LEFT = 105,
	RI_KEY_RIGHT = 106,
	RI_KEY_LEFTSHIFT = 42,
	RI_KEY_RIGHTSHIFT = 54,
};

/* Mouse buttons: usa RI_MOUSE_LEFT/RIGHT/MIDDLE de platform/platform.h */

/* ============================================================================
 * MODULOS ADICIONAIS
 * ============================================================================
 */

#include "engine/ui/layout.h"
#include "engine/ui/theme.h"

#ifdef __cplusplus
}
#endif

#endif /* RI_UX_UI_LIB_H */

// Eu terminaria isso mais rapido se só escrevesse código e não ficasse meia
// hora escrevendo comentarios que ninguem vai ler? Talvez. Você tá lendo isso?
// Tem alguem lendo isso?