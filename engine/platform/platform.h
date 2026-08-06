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

#ifndef RI_UX_PLATFORM_LIB_H
#define RI_UX_PLATFORM_LIB_H

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
typedef struct ri_window_impl *ri_window_t;

/**
 * Handle opaco para o contexto da plataforma.
 * Gerencia estado global (display connection, app delegate, etc).
 */
typedef struct ri_platform_impl *ri_platform_t;

/* ============================================================================
 * CÓDIGOS DE ERRO
 * ============================================================================
 */

enum ri_platform_error {
	RI_PLATFORM_OK = 0,
	RI_PLATFORM_ERR_NOMEM = -1,	   /* Sem memória */
	RI_PLATFORM_ERR_INIT = -2,	   /* Falha na inicialização */
	RI_PLATFORM_ERR_WINDOW = -3,	   /* Falha ao criar janela */
	RI_PLATFORM_ERR_INVALID = -4,	   /* Parâmetro inválido */
	RI_PLATFORM_ERR_UNSUPPORTED = -5, /* Operação não suportada */
};

/* ============================================================================
 * TIPOS DE EVENTO
 * ============================================================================
 */

enum ri_event_type {
	RI_EVENT_NONE = 0,

	/* Janela */
	RI_EVENT_WINDOW_CLOSE,
	RI_EVENT_WINDOW_RESIZE,
	RI_EVENT_WINDOW_FOCUS,
	RI_EVENT_WINDOW_BLUR,

	/* Mouse */
	RI_EVENT_MOUSE_MOVE,
	RI_EVENT_MOUSE_DOWN,
	RI_EVENT_MOUSE_UP,
	RI_EVENT_MOUSE_SCROLL,

	/* Teclado */
	RI_EVENT_KEY_DOWN,
	RI_EVENT_KEY_UP,
	RI_EVENT_KEY_REPEAT,

	/* Sistema */
	RI_EVENT_QUIT, /* Usuário quer fechar a aplicação */
};

/**
 * Botões do mouse
 */
enum ri_mouse_button {
	RI_MOUSE_LEFT = 0,
	RI_MOUSE_RIGHT = 1,
	RI_MOUSE_MIDDLE = 2,
	RI_MOUSE_EXTRA1 = 3,
	RI_MOUSE_EXTRA2 = 4,
};

/**
 * Modificadores de teclado (bitmask)
 */
enum ri_key_mod {
	RI_MOD_NONE = 0,
	RI_MOD_SHIFT = (1 << 0),
	RI_MOD_CTRL = (1 << 1),
	RI_MOD_ALT = (1 << 2),
	RI_MOD_SUPER = (1 << 3), /* Cmd no Mac, Win no Windows */
	RI_MOD_CAPS = (1 << 4),
};

/**
 * Formas de cursor do sistema
 */
enum ri_cursor_shape {
	RI_CURSOR_DEFAULT = 0,
	RI_CURSOR_TEXT,
	RI_CURSOR_POINTER,
	RI_CURSOR_CROSSHAIR,
	RI_CURSOR_RESIZE_H,
	RI_CURSOR_RESIZE_V,
	RI_CURSOR_RESIZE_NWSE,
	RI_CURSOR_RESIZE_NESW,
	RI_CURSOR_GRAB,
	RI_CURSOR_GRABBING,
	RI_CURSOR_HIDDEN, /* Escondido (útil pra jogos) */
};

/**
 * Evento unificado de plataforma.
 *
 * Usa union pra economizar memória - cada tipo de evento
 * usa apenas os campos relevantes.
 */
struct ri_event {
	enum ri_event_type type;
	uint32_t mods;	       /* ri_key_mod bitmask */
	uint64_t timestamp_ns; /* Timestamp em nanosegundos */

	union {
		/* RI_EVENT_WINDOW_RESIZE */
		struct {
			int32_t width;
			int32_t height;
		} resize;

		/* RI_EVENT_MOUSE_MOVE */
		struct {
			int32_t x;
			int32_t y;
			int32_t dx;
			int32_t dy;
		} mouse_move;

		/* RI_EVENT_MOUSE_DOWN, RI_EVENT_MOUSE_UP */
		struct {
			int32_t x;
			int32_t y;
			enum ri_mouse_button button;
			int click_count; /* 1 = single, 2 = double, etc */
		} mouse_button;

		/* RI_EVENT_MOUSE_SCROLL */
		struct {
			int32_t x;
			int32_t y;
			float dx;	 /* Scroll horizontal */
			float dy;	 /* Scroll vertical */
			bool is_precise; /* Trackpad vs mouse wheel */
		} scroll;

		/* RI_EVENT_KEY_DOWN, RI_EVENT_KEY_UP, RI_EVENT_KEY_REPEAT */
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
enum ri_window_flags {
	RI_WINDOW_RESIZABLE = (1 << 0),
	RI_WINDOW_BORDERLESS = (1 << 1),
	RI_WINDOW_FULLSCREEN = (1 << 2),
	RI_WINDOW_HIDDEN = (1 << 3),
	RI_WINDOW_HIGH_DPI = (1 << 4), /* Retina/HiDPI */
};

/**
 * Configuração para criar uma janela.
 * Use inicialização designada: { .title = "Meu App", .width = 800, ... }
 */
struct ri_window_config {
	const char *title;
	int32_t width;
	int32_t height;
	int32_t x; /* RI_WINDOW_POS_CENTERED ou posição */
	int32_t y;
	uint32_t flags; /* ri_window_flags */
};

#define RI_WINDOW_POS_UNDEFINED (-1)
#define RI_WINDOW_POS_CENTERED (-2)

/* ============================================================================
 * API DE PLATAFORMA
 * ============================================================================
 */

/**
 * ri_platform_init - Inicializa o subsistema de plataforma
 *
 * Deve ser chamada antes de qualquer outra função.
 * Inicializa conexão com display server, app delegate, etc.
 *
 * @platform: Ponteiro para receber o handle (chamador libera)
 *
 * Retorna: RI_PLATFORM_OK ou código de erro
 */
int ri_platform_init(ri_platform_t *platform);

/**
 * ri_platform_shutdown - Finaliza o subsistema de plataforma
 *
 * Libera recursos e fecha conexões.
 * Após chamar, @platform é inválido.
 *
 * @platform: Handle da plataforma
 */
void ri_platform_shutdown(ri_platform_t platform);

/**
 * ri_platform_poll_events - Processa eventos pendentes
 *
 * Não bloqueia. Processa todos os eventos na fila.
 * Use em game loops.
 *
 * @platform: Handle da plataforma
 */
void ri_platform_poll_events(ri_platform_t platform);

/**
 * ri_platform_wait_events - Aguarda por eventos
 *
 * Bloqueia até que pelo menos um evento esteja disponível.
 * Use para aplicações event-driven (não games).
 *
 * @platform: Handle da plataforma
 */
void ri_platform_wait_events(ri_platform_t platform);

/* ============================================================================
 * API DE JANELA
 * ============================================================================
 */

/**
 * ri_window_create - Cria uma nova janela
 *
 * @platform: Handle da plataforma
 * @config: Configuração da janela
 * @window: Ponteiro para receber o handle (chamador libera)
 *
 * Retorna: RI_PLATFORM_OK ou código de erro
 */
int ri_window_create(ri_platform_t platform,
		      const struct ri_window_config *config,
		      ri_window_t *window);

/**
 * ri_window_destroy - Destrói uma janela
 *
 * @window: Handle da janela (pode ser NULL)
 */
void ri_window_destroy(ri_window_t window);

/**
 * ri_window_show - Mostra a janela
 */
void ri_window_show(ri_window_t window);

/**
 * ri_window_hide - Esconde a janela
 */
void ri_window_hide(ri_window_t window);

/**
 * ri_window_set_title - Define o título da janela
 */
void ri_window_set_title(ri_window_t window, const char *title);

/**
 * ri_window_get_size - Obtém o tamanho da janela
 *
 * @window: Handle da janela
 * @width: Ponteiro para receber largura (pode ser NULL)
 * @height: Ponteiro para receber altura (pode ser NULL)
 */
void ri_window_get_size(ri_window_t window, int32_t *width, int32_t *height);

/**
 * ri_window_get_framebuffer_size - Obtém tamanho do framebuffer
 *
 * Em displays HiDPI, isso difere do tamanho da janela.
 * Use para configurar viewport do renderer.
 */
void ri_window_get_framebuffer_size(ri_window_t window, int32_t *width,
				     int32_t *height);

/**
 * ri_window_should_close - Verifica se janela deve fechar
 *
 * Retorna true após receber RI_EVENT_WINDOW_CLOSE.
 */
bool ri_window_should_close(ri_window_t window);

/**
 * ri_window_set_should_close - Define flag de fechamento
 *
 * Use para forçar fechamento programático.
 */
void ri_window_set_should_close(ri_window_t window, bool should_close);

/**
 * ri_window_set_cursor - Define cursor do mouse
 */
void ri_window_set_cursor(ri_window_t window, enum ri_cursor_shape shape);

/**
 * ri_window_set_mouse_lock - Trava o mouse na janela (FPS style)
 *
 * Quando travado, eventos de MOUSE_MOVE trazem diffs ilimitados (dx/dy),
 * e o cursor fica invisível e centralizado.
 */
void ri_window_set_mouse_lock(ri_window_t window, bool locked);

/* ============================================================================
 * API DE EVENTOS (CALLBACK STYLE)
 * ============================================================================
 */

/**
 * Callback de evento.
 * @window: Janela que gerou o evento
 * @event: Dados do evento
 * @userdata: Ponteiro passado em ri_window_set_event_callback
 */
typedef void (*ri_event_callback_fn)(ri_window_t window,
				      const struct ri_event *event,
				      void *userdata);

/**
 * ri_window_set_event_callback - Define callback de eventos
 *
 * Apenas um callback por janela. Passar NULL remove o callback.
 */
void ri_window_set_event_callback(ri_window_t window,
				   ri_event_callback_fn callback,
				   void *userdata);

/* ============================================================================
 * API DE EVENTOS (POLL STYLE)
 * ============================================================================
 */

/**
 * ri_window_next_event - Obtém próximo evento da fila
 *
 * Estilo alternativo aos callbacks. Remove evento da fila.
 *
 * @window: Handle da janela
 * @event: Ponteiro para receber o evento
 *
 * Retorna: true se havia evento, false se fila vazia
 */
bool ri_window_next_event(ri_window_t window, struct ri_event *event);

/* ============================================================================
 * INTEGRAÇÃO COM RENDERER
 * ============================================================================
 */

/**
 * ri_window_get_native_handle - Obtém handle nativo da janela
 *
 * Retorna: void* que deve ser convertido para o tipo da plataforma
 * - macOS: NSWindow*
 * - Windows: HWND
 * - X11: Window (XID)
 * - Wayland: wl_surface*
 */
void *ri_window_get_native_handle(ri_window_t window);

/**
 * ri_window_get_native_layer - Obtém layer/surface para rendering
 *
 * Para Metal: CAMetalLayer*
 * Para Vulkan: use com vkCreateMacOSSurfaceKHR ou similar
 * Para OpenGL: contexto já está configurado na janela
 *
 * Retorna: void* específico da plataforma, ou NULL se não aplicável
 */
void *ri_window_get_native_layer(ri_window_t window);

/**
 * ri_platform_get_native_display - Obtém display nativo
 *
 * - Wayland: wl_display*
 * - X11: Display*
 * - Outros: NULL
 */
void *ri_platform_get_native_display(ri_platform_t platform);

#ifdef __cplusplus
}
#endif

#endif /* RI_UX_PLATFORM_LIB_H */
