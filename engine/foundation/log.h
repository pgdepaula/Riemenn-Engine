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
 *   RI_LOG_INFO("Janela criada: %dx%d", width, height);
 *   RI_LOG_ERROR("Vulkan explodiu: %s", vk_result_str(res));
 *   RI_LOG_TRACE("Entrando em ri_scene_update()");
 */

#ifndef RI_GUI_FRAMEWORK_LOG_H
#define RI_GUI_FRAMEWORK_LOG_H

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
	RI_LOG_LEVEL_TRACE = 0, /* Spam absoluto. Só pra debug hardcore. */
	RI_LOG_LEVEL_DEBUG = 1, /* Info de desenvolvimento. */
	RI_LOG_LEVEL_INFO = 2,	 /* Eventos importantes. */
	RI_LOG_LEVEL_WARN = 3,	 /* Algo suspeito, mas não fatal. */
	RI_LOG_LEVEL_ERROR = 4, /* Erro real. Algo quebrou. */
	RI_LOG_LEVEL_FATAL = 5, /* Morreu. Abort iminente. */
} ri_log_level;

/* ============================================================================
 * CANAIS DE LOG (Bitmask)
 * ============================================================================
 */

typedef enum {
	RI_LOG_CHANNEL_CORE = (1 << 0),
	RI_LOG_CHANNEL_PLATFORM = (1 << 1),
	RI_LOG_CHANNEL_RENDER = (1 << 2),
	RI_LOG_CHANNEL_UI = (1 << 3),
	RI_LOG_CHANNEL_PHYSICS = (1 << 4),
	RI_LOG_CHANNEL_ECS = (1 << 5),
	RI_LOG_CHANNEL_SCENE = (1 << 6),
	RI_LOG_CHANNEL_ASSETS = (1 << 7),
} ri_log_channel;

#define RI_LOG_CHANNEL_ALL 0xFFFFFFFFU

/* ============================================================================
 * CORES ANSI
 * ============================================================================
 */

#define RI_COLOR_RESET "\x1b[0m"
#define RI_COLOR_RED "\x1b[31m"
#define RI_COLOR_GREEN "\x1b[32m"
#define RI_COLOR_YELLOW "\x1b[33m"
#define RI_COLOR_BLUE "\x1b[34m"
#define RI_COLOR_MAGENTA "\x1b[35m"
#define RI_COLOR_CYAN "\x1b[36m"
#define RI_COLOR_WHITE "\x1b[37m"
#define RI_COLOR_GRAY "\x1b[90m"

/* ============================================================================
 * CONFIGURAÇÃO
 * ============================================================================
 */

/**
 * ri_log_init - Inicializa o sistema de logs
 *
 * Chame isso antes de qualquer log. Configura mutex interno.
 */
void ri_log_init(void);

/**
 * ri_log_shutdown - Finaliza o sistema de logs
 *
 * Libera recursos. Flush de buffers.
 */
void ri_log_shutdown(void);

/**
 * ri_log_set_level - Define nível mínimo de log
 * @level: Logs abaixo desse nível são ignorados
 *
 * Padrão: RI_LOG_LEVEL_INFO em Release, RI_LOG_LEVEL_TRACE em Debug.
 */
void ri_log_set_level(ri_log_level level);

/**
 * ri_log_set_channels - Define quais canais estão ativos
 * @channels: Bitmask de canais (ex: RI_LOG_CHANNEL_PHYSICS | RI_LOG_CHANNEL_RENDER)
 *
 * Padrão: RI_LOG_CHANNEL_ALL
 */
void ri_log_set_channels(uint32_t channels);

/**
 * ri_log_set_file - Redireciona logs para arquivo
 * @path: Caminho do arquivo (NULL = só stdout)
 *
 * Logs vão para stdout E arquivo simultaneamente.
 */
void ri_log_set_file(const char *path);

/**
 * ri_log_set_colors - Habilita/desabilita cores ANSI
 * @enabled: true = cores, false = texto puro
 */
void ri_log_set_colors(bool enabled);

/* ============================================================================
 * FUNÇÃO CORE (Não chame diretamente, use as macros)
 * ============================================================================
 */

void ri_log_output(ri_log_level level, ri_log_channel channel,
		    const char *file, int line, const char *fmt, ...);

void ri_log_output_v(ri_log_level level, ri_log_channel channel,
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
#define RI_LOG_TRACE_CH(ch, fmt, ...)                                         \
	ri_log_output(RI_LOG_LEVEL_TRACE, (ch), __FILE__, __LINE__, fmt,     \
		       ##__VA_ARGS__)
#define RI_LOG_DEBUG_CH(ch, fmt, ...)                                         \
	ri_log_output(RI_LOG_LEVEL_DEBUG, (ch), __FILE__, __LINE__, fmt,     \
		       ##__VA_ARGS__)
#define RI_LOG_INFO_CH(ch, fmt, ...)                                          \
	ri_log_output(RI_LOG_LEVEL_INFO, (ch), __FILE__, __LINE__, fmt,      \
		       ##__VA_ARGS__)
#define RI_LOG_WARN_CH(ch, fmt, ...)                                          \
	ri_log_output(RI_LOG_LEVEL_WARN, (ch), __FILE__, __LINE__, fmt,      \
		       ##__VA_ARGS__)
#define RI_LOG_ERROR_CH(ch, fmt, ...)                                         \
	ri_log_output(RI_LOG_LEVEL_ERROR, (ch), __FILE__, __LINE__, fmt,     \
		       ##__VA_ARGS__)
#define RI_LOG_FATAL_CH(ch, fmt, ...)                                         \
	ri_log_output(RI_LOG_LEVEL_FATAL, (ch), __FILE__, __LINE__, fmt,     \
		       ##__VA_ARGS__)

/* Versão simplificada (canal = CORE) */
#ifdef NDEBUG
/* Release: TRACE e DEBUG somem completamente (custo zero) */
#define RI_LOG_TRACE(fmt, ...) ((void)0)
#define RI_LOG_DEBUG(fmt, ...) ((void)0)
#else
/* Debug: Todos os níveis ativos */
#define RI_LOG_TRACE(fmt, ...)                                                \
	ri_log_output(RI_LOG_LEVEL_TRACE, RI_LOG_CHANNEL_CORE, __FILE__,    \
		       __LINE__, fmt, ##__VA_ARGS__)
#define RI_LOG_DEBUG(fmt, ...)                                                \
	ri_log_output(RI_LOG_LEVEL_DEBUG, RI_LOG_CHANNEL_CORE, __FILE__,    \
		       __LINE__, fmt, ##__VA_ARGS__)
#endif

#define RI_LOG_INFO(fmt, ...)                                                 \
	ri_log_output(RI_LOG_LEVEL_INFO, RI_LOG_CHANNEL_CORE, __FILE__,     \
		       __LINE__, fmt, ##__VA_ARGS__)
#define RI_LOG_WARN(fmt, ...)                                                 \
	ri_log_output(RI_LOG_LEVEL_WARN, RI_LOG_CHANNEL_CORE, __FILE__,     \
		       __LINE__, fmt, ##__VA_ARGS__)
#define RI_LOG_ERROR(fmt, ...)                                                \
	ri_log_output(RI_LOG_LEVEL_ERROR, RI_LOG_CHANNEL_CORE, __FILE__,    \
		       __LINE__, fmt, ##__VA_ARGS__)
#define RI_LOG_FATAL(fmt, ...)                                                \
	ri_log_output(RI_LOG_LEVEL_FATAL, RI_LOG_CHANNEL_CORE, __FILE__,    \
		       __LINE__, fmt, ##__VA_ARGS__)

/* ============================================================================
 * MACROS DE CONVENIÊNCIA POR SUBSISTEMA
 * ============================================================================
 */

/* Platform */
#define RI_LOG_PLATFORM_INFO(fmt, ...)                                        \
	RI_LOG_INFO_CH(RI_LOG_CHANNEL_PLATFORM, fmt, ##__VA_ARGS__)
#define RI_LOG_PLATFORM_ERROR(fmt, ...)                                       \
	RI_LOG_ERROR_CH(RI_LOG_CHANNEL_PLATFORM, fmt, ##__VA_ARGS__)

/* Render */
#define RI_LOG_RENDER_INFO(fmt, ...)                                          \
	RI_LOG_INFO_CH(RI_LOG_CHANNEL_RENDER, fmt, ##__VA_ARGS__)
#define RI_LOG_RENDER_WARN(fmt, ...)                                          \
	RI_LOG_WARN_CH(RI_LOG_CHANNEL_RENDER, fmt, ##__VA_ARGS__)
#define RI_LOG_RENDER_ERROR(fmt, ...)                                         \
	RI_LOG_ERROR_CH(RI_LOG_CHANNEL_RENDER, fmt, ##__VA_ARGS__)

/* UI */
#define RI_LOG_UI_INFO(fmt, ...)                                              \
	RI_LOG_INFO_CH(RI_LOG_CHANNEL_UI, fmt, ##__VA_ARGS__)
#define RI_LOG_UI_WARN(fmt, ...)                                              \
	RI_LOG_WARN_CH(RI_LOG_CHANNEL_UI, fmt, ##__VA_ARGS__)

/* Physics */
#define RI_LOG_PHYSICS_DEBUG(fmt, ...)                                        \
	RI_LOG_DEBUG_CH(RI_LOG_CHANNEL_PHYSICS, fmt, ##__VA_ARGS__)
#define RI_LOG_PHYSICS_WARN(fmt, ...)                                         \
	RI_LOG_WARN_CH(RI_LOG_CHANNEL_PHYSICS, fmt, ##__VA_ARGS__)

/* ECS */
#define RI_LOG_ECS_DEBUG(fmt, ...)                                            \
	RI_LOG_DEBUG_CH(RI_LOG_CHANNEL_ECS, fmt, ##__VA_ARGS__)

/* Scene */
#define RI_LOG_SCENE_INFO(fmt, ...)                                           \
	RI_LOG_INFO_CH(RI_LOG_CHANNEL_SCENE, fmt, ##__VA_ARGS__)

/* Assets */
#define RI_LOG_ASSETS_INFO(fmt, ...)                                          \
	RI_LOG_INFO_CH(RI_LOG_CHANNEL_ASSETS, fmt, ##__VA_ARGS__)
#define RI_LOG_ASSETS_ERROR(fmt, ...)                                         \
	RI_LOG_ERROR_CH(RI_LOG_CHANNEL_ASSETS, fmt, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* RI_GUI_FRAMEWORK_LOG_H */

#ifdef __clang__
#pragma clang diagnostic pop
#endif
