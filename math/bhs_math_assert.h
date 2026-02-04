/**
 * @file assert.h
 * @brief Sistema de Asserções e Verificações Invariantes
 */

#ifndef BHS_CORE_ASSERT_H
#define BHS_CORE_ASSERT_H

#include <stdio.h>
#include <stdlib.h>

/**
 * BHS_ASSERT - Verifica uma condição e encerra se falhar.
 * "Se você passou um ponteiro nulo, você merece o crash."
 */
#define BHS_ASSERT(cond)                                                       \
	do {                                                                   \
		if (!(cond)) {                                                 \
			fprintf(stderr, "[ASSERT FALHOU] %s:%d: %s\n",         \
				__FILE__, __LINE__, #cond);                    \
			abort();                                               \
		}                                                              \
	} while (0)

/**
 * BHS_CHECK_UNREACHABLE - Marca código que nunca deveria ser executado.
 */
#define BHS_CHECK_UNREACHABLE()                                                \
	do {                                                                   \
		fprintf(stderr,                                                \
			"[FATAL] Código inalcançável atingido em %s:%d\n",     \
			__FILE__, __LINE__);                                   \
		abort();                                                       \
	} while (0)

#endif /* BHS_CORE_ASSERT_H */
