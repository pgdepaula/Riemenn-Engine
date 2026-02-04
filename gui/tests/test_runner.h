/**
 * @file test_runner.h
 * @brief Mini-gui de testes unitários
 *
 * "Rodar testes é como ir ao dentista: ninguém gosta, mas quem não vai
 * acaba com os dentes podres."
 *
 * Uso:
 *   BHS_TEST_BEGIN("Minha Suite");
 *   BHS_TEST_ASSERT(1 + 1 == 2, "Matemática quebrou");
 *   BHS_TEST_ASSERT(ptr != NULL, "Ponteiro nulo");
 *   BHS_TEST_END();
 */

#ifndef BHS_TEST_RUNNER_H
#define BHS_TEST_RUNNER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Cores ANSI */
#define TEST_RED "\x1b[31m"
#define TEST_GREEN "\x1b[32m"
#define TEST_YELLOW "\x1b[33m"
#define TEST_RESET "\x1b[0m"

/* Estado global do teste (feio mas funcional) */
static int g_test_passed = 0;
static int g_test_failed = 0;
static const char *g_test_suite = NULL;

/* ============================================================================
 * MACROS
 * ============================================================================
 */

/**
 * BHS_TEST_BEGIN - Inicia uma suite de testes
 */
#define BHS_TEST_BEGIN(suite_name)                                             \
	do {                                                                   \
		g_test_suite = (suite_name);                                   \
		g_test_passed = 0;                                             \
		g_test_failed = 0;                                             \
		printf("\n%s========================================%s\n",     \
		       TEST_YELLOW, TEST_RESET);                               \
		printf("%s[SUITE]%s %s\n", TEST_YELLOW, TEST_RESET,            \
		       suite_name);                                            \
		printf("%s========================================%s\n",       \
		       TEST_YELLOW, TEST_RESET);                               \
	} while (0)

/**
 * BHS_TEST_ASSERT - Verifica uma condição
 */
#define BHS_TEST_ASSERT(cond, msg)                                             \
	do {                                                                   \
		if (cond) {                                                    \
			g_test_passed++;                                       \
			printf("  %s[PASS]%s %s\n", TEST_GREEN, TEST_RESET,    \
			       msg);                                           \
		} else {                                                       \
			g_test_failed++;                                       \
			printf("  %s[FAIL]%s %s\n", TEST_RED, TEST_RESET,      \
			       msg);                                           \
			printf("         @ %s:%d\n", __FILE__, __LINE__);      \
		}                                                              \
	} while (0)

/**
 * BHS_TEST_ASSERT_EQ - Verifica igualdade de inteiros
 */
#define BHS_TEST_ASSERT_EQ(a, b, msg)                                          \
	do {                                                                   \
		long long _a = (long long)(a);                                 \
		long long _b = (long long)(b);                                 \
		if (_a == _b) {                                                \
			g_test_passed++;                                       \
			printf("  %s[PASS]%s %s\n", TEST_GREEN, TEST_RESET,    \
			       msg);                                           \
		} else {                                                       \
			g_test_failed++;                                       \
			printf("  %s[FAIL]%s %s (esperado: %lld, obtido: "     \
			       "%lld)\n",                                      \
			       TEST_RED, TEST_RESET, msg, _b, _a);             \
			printf("         @ %s:%d\n", __FILE__, __LINE__);      \
		}                                                              \
	} while (0)

/**
 * BHS_TEST_ASSERT_NOT_NULL - Verifica ponteiro não-nulo
 */
#define BHS_TEST_ASSERT_NOT_NULL(ptr, msg)                                     \
	do {                                                                   \
		if ((ptr) != NULL) {                                           \
			g_test_passed++;                                       \
			printf("  %s[PASS]%s %s\n", TEST_GREEN, TEST_RESET,    \
			       msg);                                           \
		} else {                                                       \
			g_test_failed++;                                       \
			printf("  %s[FAIL]%s %s (ponteiro NULL)\n", TEST_RED,  \
			       TEST_RESET, msg);                               \
			printf("         @ %s:%d\n", __FILE__, __LINE__);      \
		}                                                              \
	} while (0)

/**
 * BHS_TEST_END - Finaliza a suite e retorna código de saída
 */
#define BHS_TEST_END()                                                         \
	do {                                                                   \
		printf("\n%s----------------------------------------%s\n",     \
		       TEST_YELLOW, TEST_RESET);                               \
		printf("[RESULTADO] Passou: %s%d%s | Falhou: %s%d%s\n",        \
		       TEST_GREEN, g_test_passed, TEST_RESET,                  \
		       (g_test_failed > 0) ? TEST_RED : TEST_GREEN,            \
		       g_test_failed, TEST_RESET);                             \
		printf("%s----------------------------------------%s\n\n",     \
		       TEST_YELLOW, TEST_RESET);                               \
		return (g_test_failed > 0) ? 1 : 0;                            \
	} while (0)

/**
 * BHS_TEST_SECTION - Marca uma seção dentro da suite
 */
#define BHS_TEST_SECTION(name)                                                 \
	printf("\n  %s>> %s%s\n", TEST_YELLOW, name, TEST_RESET)

#endif /* BHS_TEST_RUNNER_H */
