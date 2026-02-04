/**
 * @file log.c
 * @brief Implementação do Sistema de Logging
 *
 * Thread-safe via mutex. Cores ANSI. File/Line context.
 * Tudo que printf deveria ser mas não é.
 */

#define _POSIX_C_SOURCE 200809L

#include "gui/log.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * ESTADO GLOBAL
 * ============================================================================
 */

static struct {
	bhs_log_level min_level;
	uint32_t active_channels;
	FILE *file_output;
	bool colors_enabled;
	bool initialized;
	pthread_mutex_t mutex;
} g_log = {
	.min_level = BHS_LOG_LEVEL_INFO,
	.active_channels = BHS_LOG_CHANNEL_ALL,
	.file_output = NULL,
	.colors_enabled = true,
	.initialized = false,
};

/* ============================================================================
 * STRINGS DE NÍVEL
 * ============================================================================
 */

static const char *level_strings[] = { "TRACE", "DEBUG", "INFO",
				       "WARN",	"ERROR", "FATAL" };

static const char *level_colors[] = {
	BHS_COLOR_GRAY,	   /* TRACE */
	BHS_COLOR_CYAN,	   /* DEBUG */
	BHS_COLOR_GREEN,   /* INFO */
	BHS_COLOR_YELLOW,  /* WARN */
	BHS_COLOR_RED,	   /* ERROR */
	BHS_COLOR_MAGENTA, /* FATAL */
};

static const char *channel_strings[] = {
	"CORE",	    /* 0 */
	"PLATFORM", /* 1 */
	"RENDER",   /* 2 */
	"UI",	    /* 3 */
	"PHYSICS",  /* 4 */
	"ECS",	    /* 5 */
	"SCENE",    /* 6 */
	"ASSETS",   /* 7 */
};

/* ============================================================================
 * HELPERS
 * ============================================================================
 */

static const char *get_channel_name(bhs_log_channel ch)
{
	/* Encontra o primeiro bit setado */
	for (int i = 0; i < 8; i++) {
		if (ch & (1 << i)) {
			return channel_strings[i];
		}
	}
	return "???";
}

static const char *get_filename(const char *path)
{
	/* Pega só o nome do arquivo, sem o caminho todo */
	const char *slash = strrchr(path, '/');
	if (slash)
		return slash + 1;
	slash = strrchr(path, '\\'); /* Windows */
	if (slash)
		return slash + 1;
	return path;
}

/* ============================================================================
 * IMPLEMENTAÇÃO
 * ============================================================================
 */

void bhs_log_init(void)
{
	if (g_log.initialized)
		return;

	pthread_mutex_init(&g_log.mutex, NULL);
	g_log.initialized = true;

#ifdef NDEBUG
	g_log.min_level = BHS_LOG_LEVEL_INFO;
#else
	g_log.min_level = BHS_LOG_LEVEL_TRACE;
#endif
}

void bhs_log_shutdown(void)
{
	if (!g_log.initialized)
		return;

	if (g_log.file_output) {
		fflush(g_log.file_output);
		fclose(g_log.file_output);
		g_log.file_output = NULL;
	}

	pthread_mutex_destroy(&g_log.mutex);
	g_log.initialized = false;
}

void bhs_log_set_level(bhs_log_level level)
{
	g_log.min_level = level;
}

void bhs_log_set_channels(uint32_t channels)
{
	g_log.active_channels = channels;
}

void bhs_log_set_file(const char *path)
{
	pthread_mutex_lock(&g_log.mutex);

	if (g_log.file_output) {
		fclose(g_log.file_output);
		g_log.file_output = NULL;
	}

	if (path) {
		g_log.file_output = fopen(path, "a");
		if (!g_log.file_output) {
			fprintf(stderr, "[LOG] Falha ao abrir arquivo: %s\n",
				path);
		}
	}

	pthread_mutex_unlock(&g_log.mutex);
}

void bhs_log_set_colors(bool enabled)
{
	g_log.colors_enabled = enabled;
}

void bhs_log_output(bhs_log_level level, bhs_log_channel channel,
		    const char *file, int line, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	bhs_log_output_v(level, channel, file, line, fmt, args);
	va_end(args);
}

void bhs_log_output_v(bhs_log_level level, bhs_log_channel channel,
		      const char *file, int line, const char *fmt, va_list args)
{
	/* Filtros */
	if (level < g_log.min_level)
		return;
	if (!(channel & g_log.active_channels))
		return;

	/* Auto-init se esqueceram de chamar bhs_log_init() */
	if (!g_log.initialized) {
		bhs_log_init();
	}

	pthread_mutex_lock(&g_log.mutex);

	/* Timestamp */
	time_t now = time(NULL);
	struct tm *t = localtime(&now);
	char timestamp[16];
	strftime(timestamp, sizeof(timestamp), "%H:%M:%S", t);

	/* Formata a mensagem do usuário */
	char message[2048];
	vsnprintf(message, sizeof(message), fmt, args);

	/* Monta a linha completa */
	const char *filename = get_filename(file);
	const char *lvl_str = level_strings[level];
	const char *ch_str = get_channel_name(channel);

	/* Saída para stdout (com cores) */
	if (g_log.colors_enabled) {
		fprintf(stderr, "%s%s %s[%-5s]%s %s[%-8s]%s %s[%s:%d]%s %s\n",
			BHS_COLOR_GRAY, timestamp, level_colors[level], lvl_str,
			BHS_COLOR_RESET, BHS_COLOR_BLUE, ch_str,
			BHS_COLOR_RESET, BHS_COLOR_GRAY, filename, line,
			BHS_COLOR_RESET, message);
	} else {
		fprintf(stderr, "%s [%-5s] [%-8s] [%s:%d] %s\n", timestamp,
			lvl_str, ch_str, filename, line, message);
	}

	/* Saída para arquivo (sem cores) */
	if (g_log.file_output) {
		fprintf(g_log.file_output, "%s [%-5s] [%-8s] [%s:%d] %s\n",
			timestamp, lvl_str, ch_str, filename, line, message);
		fflush(g_log.file_output);
	}

	/* FATAL = abort */
	if (level == BHS_LOG_LEVEL_FATAL) {
		pthread_mutex_unlock(&g_log.mutex);
		abort();
	}

	pthread_mutex_unlock(&g_log.mutex);
}
