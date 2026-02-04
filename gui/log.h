/**
 * @file log.h
 * @brief Sistema de Logging da Engine
 *
 * "printf é pra amadores. Aqui a gente faz log de verdade."
 *
 * Características:
 * - Níveis de log (TRACE, DEBUG, INFO, WARN, ERROR, FATAL)
 * - Canais por subsistema (PHYSICS, RENDER, UI, etc)
 * - File/Line automático
 * - Cores ANSI no terminal
 * - TRACE/DEBUG somem em Release (custo zero)
 * - Thread-safe (mutex interno)
 *
 * Uso:
 *   BHS_LOG_INFO("Janela criada: %dx%d", width, height);
 *   BHS_LOG_ERROR("Vulkan explodiu: %s", vk_result_str(res));
 *   BHS_LOG_TRACE("Entrando em bhs_scene_update()");
 */

#ifndef BHS_GUI_FRAMEWORK_LOG_H
#define BHS_GUI_FRAMEWORK_LOG_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * NÍVEIS DE LOG
 * ============================================================================
 */

typedef enum {
	BHS_LOG_LEVEL_TRACE = 0, /* Spam absoluto. Só pra debug hardcore. */
	BHS_LOG_LEVEL_DEBUG = 1, /* Info de desenvolvimento. */
	BHS_LOG_LEVEL_INFO = 2,	 /* Eventos importantes. */
	BHS_LOG_LEVEL_WARN = 3,	 /* Algo suspeito, mas não fatal. */
	BHS_LOG_LEVEL_ERROR = 4, /* Erro real. Algo quebrou. */
	BHS_LOG_LEVEL_FATAL = 5, /* Morreu. Abort iminente. */
} bhs_log_level;

/* ============================================================================
 * CANAIS DE LOG (Bitmask)
 * ============================================================================
 */

typedef enum {
	BHS_LOG_CHANNEL_CORE = (1 << 0),
	BHS_LOG_CHANNEL_PLATFORM = (1 << 1),
	BHS_LOG_CHANNEL_RENDER = (1 << 2),
	BHS_LOG_CHANNEL_UI = (1 << 3),
	BHS_LOG_CHANNEL_PHYSICS = (1 << 4),
	BHS_LOG_CHANNEL_ECS = (1 << 5),
	BHS_LOG_CHANNEL_SCENE = (1 << 6),
	BHS_LOG_CHANNEL_ASSETS = (1 << 7),
} bhs_log_channel;

#define BHS_LOG_CHANNEL_ALL 0xFFFFFFFFU

/* ============================================================================
 * CORES ANSI
 * ============================================================================
 */

#define BHS_COLOR_RESET "\x1b[0m"
#define BHS_COLOR_RED "\x1b[31m"
#define BHS_COLOR_GREEN "\x1b[32m"
#define BHS_COLOR_YELLOW "\x1b[33m"
#define BHS_COLOR_BLUE "\x1b[34m"
#define BHS_COLOR_MAGENTA "\x1b[35m"
#define BHS_COLOR_CYAN "\x1b[36m"
#define BHS_COLOR_WHITE "\x1b[37m"
#define BHS_COLOR_GRAY "\x1b[90m"

/* ============================================================================
 * CONFIGURAÇÃO
 * ============================================================================
 */

/**
 * bhs_log_init - Inicializa o sistema de logs
 *
 * Chame isso antes de qualquer log. Configura mutex interno.
 */
void bhs_log_init(void);

/**
 * bhs_log_shutdown - Finaliza o sistema de logs
 *
 * Libera recursos. Flush de buffers.
 */
void bhs_log_shutdown(void);

/**
 * bhs_log_set_level - Define nível mínimo de log
 * @level: Logs abaixo desse nível são ignorados
 *
 * Padrão: BHS_LOG_LEVEL_INFO em Release, BHS_LOG_LEVEL_TRACE em Debug.
 */
void bhs_log_set_level(bhs_log_level level);

/**
 * bhs_log_set_channels - Define quais canais estão ativos
 * @channels: Bitmask de canais (ex: BHS_LOG_CHANNEL_PHYSICS | BHS_LOG_CHANNEL_RENDER)
 *
 * Padrão: BHS_LOG_CHANNEL_ALL
 */
void bhs_log_set_channels(uint32_t channels);

/**
 * bhs_log_set_file - Redireciona logs para arquivo
 * @path: Caminho do arquivo (NULL = só stdout)
 *
 * Logs vão para stdout E arquivo simultaneamente.
 */
void bhs_log_set_file(const char *path);

/**
 * bhs_log_set_colors - Habilita/desabilita cores ANSI
 * @enabled: true = cores, false = texto puro
 */
void bhs_log_set_colors(bool enabled);

/* ============================================================================
 * FUNÇÃO CORE (Não chame diretamente, use as macros)
 * ============================================================================
 */

void bhs_log_output(bhs_log_level level, bhs_log_channel channel,
		    const char *file, int line, const char *fmt, ...);

void bhs_log_output_v(bhs_log_level level, bhs_log_channel channel,
		      const char *file, int line, const char *fmt,
		      va_list args);

/* ============================================================================
 * MACROS DE LOGGING
 * ============================================================================
 *
 * Usam __FILE__ e __LINE__ automaticamente.
 * Os canais são opcionais (default = CORE).
 */

/* Versão com canal explícito */
#define BHS_LOG_TRACE_CH(ch, fmt, ...)                                         \
	bhs_log_output(BHS_LOG_LEVEL_TRACE, (ch), __FILE__, __LINE__, fmt,     \
		       ##__VA_ARGS__)
#define BHS_LOG_DEBUG_CH(ch, fmt, ...)                                         \
	bhs_log_output(BHS_LOG_LEVEL_DEBUG, (ch), __FILE__, __LINE__, fmt,     \
		       ##__VA_ARGS__)
#define BHS_LOG_INFO_CH(ch, fmt, ...)                                          \
	bhs_log_output(BHS_LOG_LEVEL_INFO, (ch), __FILE__, __LINE__, fmt,      \
		       ##__VA_ARGS__)
#define BHS_LOG_WARN_CH(ch, fmt, ...)                                          \
	bhs_log_output(BHS_LOG_LEVEL_WARN, (ch), __FILE__, __LINE__, fmt,      \
		       ##__VA_ARGS__)
#define BHS_LOG_ERROR_CH(ch, fmt, ...)                                         \
	bhs_log_output(BHS_LOG_LEVEL_ERROR, (ch), __FILE__, __LINE__, fmt,     \
		       ##__VA_ARGS__)
#define BHS_LOG_FATAL_CH(ch, fmt, ...)                                         \
	bhs_log_output(BHS_LOG_LEVEL_FATAL, (ch), __FILE__, __LINE__, fmt,     \
		       ##__VA_ARGS__)

/* Versão simplificada (canal = CORE) */
#ifdef NDEBUG
/* Release: TRACE e DEBUG somem completamente (custo zero) */
#define BHS_LOG_TRACE(fmt, ...) ((void)0)
#define BHS_LOG_DEBUG(fmt, ...) ((void)0)
#else
/* Debug: Todos os níveis ativos */
#define BHS_LOG_TRACE(fmt, ...)                                                \
	bhs_log_output(BHS_LOG_LEVEL_TRACE, BHS_LOG_CHANNEL_CORE, __FILE__,    \
		       __LINE__, fmt, ##__VA_ARGS__)
#define BHS_LOG_DEBUG(fmt, ...)                                                \
	bhs_log_output(BHS_LOG_LEVEL_DEBUG, BHS_LOG_CHANNEL_CORE, __FILE__,    \
		       __LINE__, fmt, ##__VA_ARGS__)
#endif

#define BHS_LOG_INFO(fmt, ...)                                                 \
	bhs_log_output(BHS_LOG_LEVEL_INFO, BHS_LOG_CHANNEL_CORE, __FILE__,     \
		       __LINE__, fmt, ##__VA_ARGS__)
#define BHS_LOG_WARN(fmt, ...)                                                 \
	bhs_log_output(BHS_LOG_LEVEL_WARN, BHS_LOG_CHANNEL_CORE, __FILE__,     \
		       __LINE__, fmt, ##__VA_ARGS__)
#define BHS_LOG_ERROR(fmt, ...)                                                \
	bhs_log_output(BHS_LOG_LEVEL_ERROR, BHS_LOG_CHANNEL_CORE, __FILE__,    \
		       __LINE__, fmt, ##__VA_ARGS__)
#define BHS_LOG_FATAL(fmt, ...)                                                \
	bhs_log_output(BHS_LOG_LEVEL_FATAL, BHS_LOG_CHANNEL_CORE, __FILE__,    \
		       __LINE__, fmt, ##__VA_ARGS__)

/* ============================================================================
 * MACROS DE CONVENIÊNCIA POR SUBSISTEMA
 * ============================================================================
 */

/* Platform */
#define BHS_LOG_PLATFORM_INFO(fmt, ...)                                        \
	BHS_LOG_INFO_CH(BHS_LOG_CHANNEL_PLATFORM, fmt, ##__VA_ARGS__)
#define BHS_LOG_PLATFORM_ERROR(fmt, ...)                                       \
	BHS_LOG_ERROR_CH(BHS_LOG_CHANNEL_PLATFORM, fmt, ##__VA_ARGS__)

/* Render */
#define BHS_LOG_RENDER_INFO(fmt, ...)                                          \
	BHS_LOG_INFO_CH(BHS_LOG_CHANNEL_RENDER, fmt, ##__VA_ARGS__)
#define BHS_LOG_RENDER_WARN(fmt, ...)                                          \
	BHS_LOG_WARN_CH(BHS_LOG_CHANNEL_RENDER, fmt, ##__VA_ARGS__)
#define BHS_LOG_RENDER_ERROR(fmt, ...)                                         \
	BHS_LOG_ERROR_CH(BHS_LOG_CHANNEL_RENDER, fmt, ##__VA_ARGS__)

/* UI */
#define BHS_LOG_UI_INFO(fmt, ...)                                              \
	BHS_LOG_INFO_CH(BHS_LOG_CHANNEL_UI, fmt, ##__VA_ARGS__)
#define BHS_LOG_UI_WARN(fmt, ...)                                              \
	BHS_LOG_WARN_CH(BHS_LOG_CHANNEL_UI, fmt, ##__VA_ARGS__)

/* Physics */
#define BHS_LOG_PHYSICS_DEBUG(fmt, ...)                                        \
	BHS_LOG_DEBUG_CH(BHS_LOG_CHANNEL_PHYSICS, fmt, ##__VA_ARGS__)
#define BHS_LOG_PHYSICS_WARN(fmt, ...)                                         \
	BHS_LOG_WARN_CH(BHS_LOG_CHANNEL_PHYSICS, fmt, ##__VA_ARGS__)

/* ECS */
#define BHS_LOG_ECS_DEBUG(fmt, ...)                                            \
	BHS_LOG_DEBUG_CH(BHS_LOG_CHANNEL_ECS, fmt, ##__VA_ARGS__)

/* Scene */
#define BHS_LOG_SCENE_INFO(fmt, ...)                                           \
	BHS_LOG_INFO_CH(BHS_LOG_CHANNEL_SCENE, fmt, ##__VA_ARGS__)

/* Assets */
#define BHS_LOG_ASSETS_INFO(fmt, ...)                                          \
	BHS_LOG_INFO_CH(BHS_LOG_CHANNEL_ASSETS, fmt, ##__VA_ARGS__)
#define BHS_LOG_ASSETS_ERROR(fmt, ...)                                         \
	BHS_LOG_ERROR_CH(BHS_LOG_CHANNEL_ASSETS, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* BHS_GUI_FRAMEWORK_LOG_H */

#ifdef __clang__
#pragma clang diagnostic pop
#endif
