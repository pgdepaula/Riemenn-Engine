/**
 * @file epa.h (Engine platform abstraction)
 * @brief Abstração de plataforma - janelas, eventos, input
 *
 * Essa API define a interface comum entre todas as plataformas:
 * - Cocoa (macOS)
 * - X11 (Linux)
 * - Wayland (Linux moderno)
 * - Win32 (Windows)
 *
 * Cada backend implementa essas funções. A seleção é feita em tempo
 * de compilação via preprocessor ou em runtime via vtable.
 *
 * Invariantes:
 * - Invariante: Single-window application design.
 * - Eventos são processados no thread principal
 * - Ponteiros retornados por _create devem ser liberados com _destroy
 */

#ifndef BHS_UX_PLATFORM_LIB_H
#define BHS_UX_PLATFORM_LIB_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TIPOS OPACOS
 * ============================================================================
 */

/**
 * Handle opaco para a janela da plataforma.
 * Internamente contém NSWindow, HWND, Window, etc.
 */
typedef struct bhs_window_impl *bhs_window_t;

/**
 * Handle opaco para o contexto da plataforma.
 * Gerencia estado global (display connection, app delegate, etc).
 */
typedef struct bhs_platform_impl *bhs_platform_t;

/* ============================================================================
 * CÓDIGOS DE ERRO
 * ============================================================================
 */

enum bhs_platform_error {
	BHS_PLATFORM_OK = 0,
	BHS_PLATFORM_ERR_NOMEM = -1,	   /* Sem memória */
	BHS_PLATFORM_ERR_INIT = -2,	   /* Falha na inicialização */
	BHS_PLATFORM_ERR_WINDOW = -3,	   /* Falha ao criar janela */
	BHS_PLATFORM_ERR_INVALID = -4,	   /* Parâmetro inválido */
	BHS_PLATFORM_ERR_UNSUPPORTED = -5, /* Operação não suportada */
};

/* ============================================================================
 * TIPOS DE EVENTO
 * ============================================================================
 */

enum bhs_event_type {
	BHS_EVENT_NONE = 0,

	/* Janela */
	BHS_EVENT_WINDOW_CLOSE,
	BHS_EVENT_WINDOW_RESIZE,
	BHS_EVENT_WINDOW_FOCUS,
	BHS_EVENT_WINDOW_BLUR,

	/* Mouse */
	BHS_EVENT_MOUSE_MOVE,
	BHS_EVENT_MOUSE_DOWN,
	BHS_EVENT_MOUSE_UP,
	BHS_EVENT_MOUSE_SCROLL,

	/* Teclado */
	BHS_EVENT_KEY_DOWN,
	BHS_EVENT_KEY_UP,
	BHS_EVENT_KEY_REPEAT,

	/* Sistema */
	BHS_EVENT_QUIT, /* Usuário quer fechar a aplicação */
};

/**
 * Botões do mouse
 */
enum bhs_mouse_button {
	BHS_MOUSE_LEFT = 0,
	BHS_MOUSE_RIGHT = 1,
	BHS_MOUSE_MIDDLE = 2,
	BHS_MOUSE_EXTRA1 = 3,
	BHS_MOUSE_EXTRA2 = 4,
};

/**
 * Modificadores de teclado (bitmask)
 */
enum bhs_key_mod {
	BHS_MOD_NONE = 0,
	BHS_MOD_SHIFT = (1 << 0),
	BHS_MOD_CTRL = (1 << 1),
	BHS_MOD_ALT = (1 << 2),
	BHS_MOD_SUPER = (1 << 3), /* Cmd no Mac, Win no Windows */
	BHS_MOD_CAPS = (1 << 4),
};

/**
 * Formas de cursor do sistema
 */
enum bhs_cursor_shape {
	BHS_CURSOR_DEFAULT = 0,
	BHS_CURSOR_TEXT,
	BHS_CURSOR_POINTER,
	BHS_CURSOR_CROSSHAIR,
	BHS_CURSOR_RESIZE_H,
	BHS_CURSOR_RESIZE_V,
	BHS_CURSOR_RESIZE_NWSE,
	BHS_CURSOR_RESIZE_NESW,
	BHS_CURSOR_GRAB,
	BHS_CURSOR_GRABBING,
	BHS_CURSOR_HIDDEN, /* Escondido (útil pra jogos) */
};

/**
 * Evento unificado de plataforma.
 *
 * Usa union pra economizar memória - cada tipo de evento
 * usa apenas os campos relevantes.
 */
struct bhs_event {
	enum bhs_event_type type;
	uint32_t mods;	       /* bhs_key_mod bitmask */
	uint64_t timestamp_ns; /* Timestamp em nanosegundos */

	union {
		/* BHS_EVENT_WINDOW_RESIZE */
		struct {
			int32_t width;
			int32_t height;
		} resize;

		/* BHS_EVENT_MOUSE_MOVE */
		struct {
			int32_t x;
			int32_t y;
			int32_t dx;
			int32_t dy;
		} mouse_move;

		/* BHS_EVENT_MOUSE_DOWN, BHS_EVENT_MOUSE_UP */
		struct {
			int32_t x;
			int32_t y;
			enum bhs_mouse_button button;
			int click_count; /* 1 = single, 2 = double, etc */
		} mouse_button;

		/* BHS_EVENT_MOUSE_SCROLL */
		struct {
			int32_t x;
			int32_t y;
			float dx;	 /* Scroll horizontal */
			float dy;	 /* Scroll vertical */
			bool is_precise; /* Trackpad vs mouse wheel */
		} scroll;

		/* BHS_EVENT_KEY_DOWN, BHS_EVENT_KEY_UP, BHS_EVENT_KEY_REPEAT */
		struct {
			uint32_t keycode;  /* Código físico da tecla */
			uint32_t scancode; /* Scancode do sistema */
			/* UTF-8 do caractere, se aplicável */
			char text[8];
		} key;
	};
};

/* ============================================================================
 * CONFIGURAÇÃO DE JANELA
 * ============================================================================
 */

/**
 * Flags de janela (bitmask)
 */
enum bhs_window_flags {
	BHS_WINDOW_RESIZABLE = (1 << 0),
	BHS_WINDOW_BORDERLESS = (1 << 1),
	BHS_WINDOW_FULLSCREEN = (1 << 2),
	BHS_WINDOW_HIDDEN = (1 << 3),
	BHS_WINDOW_HIGH_DPI = (1 << 4), /* Retina/HiDPI */
};

/**
 * Configuração para criar uma janela.
 * Use inicialização designada: { .title = "Meu App", .width = 800, ... }
 */
struct bhs_window_config {
	const char *title;
	int32_t width;
	int32_t height;
	int32_t x; /* BHS_WINDOW_POS_CENTERED ou posição */
	int32_t y;
	uint32_t flags; /* bhs_window_flags */
};

#define BHS_WINDOW_POS_UNDEFINED (-1)
#define BHS_WINDOW_POS_CENTERED (-2)

/* ============================================================================
 * API DE PLATAFORMA
 * ============================================================================
 */

/**
 * bhs_platform_init - Inicializa o subsistema de plataforma
 *
 * Deve ser chamada antes de qualquer outra função.
 * Inicializa conexão com display server, app delegate, etc.
 *
 * @platform: Ponteiro para receber o handle (chamador libera)
 *
 * Retorna: BHS_PLATFORM_OK ou código de erro
 */
int bhs_platform_init(bhs_platform_t *platform);

/**
 * bhs_platform_shutdown - Finaliza o subsistema de plataforma
 *
 * Libera recursos e fecha conexões.
 * Após chamar, @platform é inválido.
 *
 * @platform: Handle da plataforma
 */
void bhs_platform_shutdown(bhs_platform_t platform);

/**
 * bhs_platform_poll_events - Processa eventos pendentes
 *
 * Não bloqueia. Processa todos os eventos na fila.
 * Use em game loops.
 *
 * @platform: Handle da plataforma
 */
void bhs_platform_poll_events(bhs_platform_t platform);

/**
 * bhs_platform_wait_events - Aguarda por eventos
 *
 * Bloqueia até que pelo menos um evento esteja disponível.
 * Use para aplicações event-driven (não games).
 *
 * @platform: Handle da plataforma
 */
void bhs_platform_wait_events(bhs_platform_t platform);

/* ============================================================================
 * API DE JANELA
 * ============================================================================
 */

/**
 * bhs_window_create - Cria uma nova janela
 *
 * @platform: Handle da plataforma
 * @config: Configuração da janela
 * @window: Ponteiro para receber o handle (chamador libera)
 *
 * Retorna: BHS_PLATFORM_OK ou código de erro
 */
int bhs_window_create(bhs_platform_t platform,
		      const struct bhs_window_config *config,
		      bhs_window_t *window);

/**
 * bhs_window_destroy - Destrói uma janela
 *
 * @window: Handle da janela (pode ser NULL)
 */
void bhs_window_destroy(bhs_window_t window);

/**
 * bhs_window_show - Mostra a janela
 */
void bhs_window_show(bhs_window_t window);

/**
 * bhs_window_hide - Esconde a janela
 */
void bhs_window_hide(bhs_window_t window);

/**
 * bhs_window_set_title - Define o título da janela
 */
void bhs_window_set_title(bhs_window_t window, const char *title);

/**
 * bhs_window_get_size - Obtém o tamanho da janela
 *
 * @window: Handle da janela
 * @width: Ponteiro para receber largura (pode ser NULL)
 * @height: Ponteiro para receber altura (pode ser NULL)
 */
void bhs_window_get_size(bhs_window_t window, int32_t *width, int32_t *height);

/**
 * bhs_window_get_framebuffer_size - Obtém tamanho do framebuffer
 *
 * Em displays HiDPI, isso difere do tamanho da janela.
 * Use para configurar viewport do renderer.
 */
void bhs_window_get_framebuffer_size(bhs_window_t window, int32_t *width,
				     int32_t *height);

/**
 * bhs_window_should_close - Verifica se janela deve fechar
 *
 * Retorna true após receber BHS_EVENT_WINDOW_CLOSE.
 */
bool bhs_window_should_close(bhs_window_t window);

/**
 * bhs_window_set_should_close - Define flag de fechamento
 *
 * Use para forçar fechamento programático.
 */
void bhs_window_set_should_close(bhs_window_t window, bool should_close);

/**
 * bhs_window_set_cursor - Define cursor do mouse
 */
void bhs_window_set_cursor(bhs_window_t window, enum bhs_cursor_shape shape);

/**
 * bhs_window_set_mouse_lock - Trava o mouse na janela (FPS style)
 *
 * Quando travado, eventos de MOUSE_MOVE trazem diffs ilimitados (dx/dy),
 * e o cursor fica invisível e centralizado.
 */
void bhs_window_set_mouse_lock(bhs_window_t window, bool locked);

/* ============================================================================
 * API DE EVENTOS (CALLBACK STYLE)
 * ============================================================================
 */

/**
 * Callback de evento.
 * @window: Janela que gerou o evento
 * @event: Dados do evento
 * @userdata: Ponteiro passado em bhs_window_set_event_callback
 */
typedef void (*bhs_event_callback_fn)(bhs_window_t window,
				      const struct bhs_event *event,
				      void *userdata);

/**
 * bhs_window_set_event_callback - Define callback de eventos
 *
 * Apenas um callback por janela. Passar NULL remove o callback.
 */
void bhs_window_set_event_callback(bhs_window_t window,
				   bhs_event_callback_fn callback,
				   void *userdata);

/* ============================================================================
 * API DE EVENTOS (POLL STYLE)
 * ============================================================================
 */

/**
 * bhs_window_next_event - Obtém próximo evento da fila
 *
 * Estilo alternativo aos callbacks. Remove evento da fila.
 *
 * @window: Handle da janela
 * @event: Ponteiro para receber o evento
 *
 * Retorna: true se havia evento, false se fila vazia
 */
bool bhs_window_next_event(bhs_window_t window, struct bhs_event *event);

/* ============================================================================
 * INTEGRAÇÃO COM RENDERER
 * ============================================================================
 */

/**
 * bhs_window_get_native_handle - Obtém handle nativo da janela
 *
 * Retorna: void* que deve ser convertido para o tipo da plataforma
 * - macOS: NSWindow*
 * - Windows: HWND
 * - X11: Window (XID)
 * - Wayland: wl_surface*
 */
void *bhs_window_get_native_handle(bhs_window_t window);

/**
 * bhs_window_get_native_layer - Obtém layer/surface para rendering
 *
 * Para Metal: CAMetalLayer*
 * Para Vulkan: use com vkCreateMacOSSurfaceKHR ou similar
 * Para OpenGL: contexto já está configurado na janela
 *
 * Retorna: void* específico da plataforma, ou NULL se não aplicável
 */
void *bhs_window_get_native_layer(bhs_window_t window);

/**
 * bhs_platform_get_native_display - Obtém display nativo
 *
 * - Wayland: wl_display*
 * - X11: Display*
 * - Outros: NULL
 */
void *bhs_platform_get_native_display(bhs_platform_t platform);

#ifdef __cplusplus
}
#endif

#endif /* BHS_UX_PLATFORM_LIB_H */
