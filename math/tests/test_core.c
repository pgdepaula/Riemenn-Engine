/**
 * @file test_core.c
 * @brief Driver de teste CLI para libbhs_core
 *
 * "Confia, mas verifica."
 * — Provérbio russo (e regra pra quem mexe com métricas de Kerr)
 */

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "math/core.h"
#include "math/spacetime/schwarzschild.h"
#include "math/tensor/tensor.h"
#include "math/vec4.h"

#define TEST_PASS "[\033[32m PASS \033[0m]"
#define TEST_FAIL "[\033[31m FAIL \033[0m]"

static int tests_run = 0;
static int tests_failed = 0;

#define ASSERT_EPS(a, b, eps, msg)                                             \
	do {                                                                   \
		tests_run++;                                                   \
		double diff = (double)(a) - (double)(b);                       \
		if (diff < 0) diff = -diff;                                    \
		if (diff > (eps)) {                                            \
			fprintf(stderr, "%s %s: exp %f, got %f (eps %e)\n",    \
				TEST_FAIL, msg, (double)(b), (double)(a),      \
				(double)(eps));                                \
			tests_failed++;                                        \
		} else {                                                       \
			/* printf("%s %s\n", TEST_PASS, msg); */               \
		}                                                              \
	} while (0)

/* ============================================================================
 * TESTES: VEC4
 * ============================================================================
 */

void test_vec4_math()
{
	struct bhs_vec4 a = bhs_vec4_make(1, 2, 3, 4);
	struct bhs_vec4 b = bhs_vec4_make(10, 20, 30, 40);

	struct bhs_vec4 c = bhs_vec4_add(a, b);
	ASSERT_EPS(c.t, 11, 1e-10, "vec4_add.t");
	ASSERT_EPS(c.x, 22, 1e-10, "vec4_add.x");
	ASSERT_EPS(c.y, 33, 1e-10, "vec4_add.y");
	ASSERT_EPS(c.z, 44, 1e-10, "vec4_add.z");

	double dot = bhs_vec4_dot_minkowski(a, a);
	/* -1^2 + 2^2 + 3^2 + 4^2 = -1 + 4 + 9 + 16 = 28 */
	ASSERT_EPS(dot, 28, 1e-10, "vec4_dot_minkowski");
}

/* ============================================================================
 * TESTES: TENSOR
 * ============================================================================
 */

void test_metric_invert()
{
	/* Métrica de Minkowski */
	struct bhs_metric m = bhs_metric_minkowski();
	struct bhs_metric inv;

	int ret = bhs_metric_invert(&m, &inv);
	ASSERT_EPS(ret, 0, 0.1, "metric_invert status");

	/* Minkowski invertida deve ser ela mesma */
	ASSERT_EPS(inv.g[0][0], -1.0, 1e-10, "inv_minkowski[0][0]");
	ASSERT_EPS(inv.g[1][1], 1.0, 1e-10, "inv_minkowski[1][1]");

	/* Métrica diagonal arbitrária */
	struct bhs_metric diag = bhs_metric_diag(-2.0, 0.5, 4.0, 1.0);
	ret = bhs_metric_invert(&diag, &inv);
	ASSERT_EPS(ret, 0, 0.1, "metric_invert_diag status");
	ASSERT_EPS(inv.g[0][0], -0.5, 1e-10, "inv_diag[0][0]");
	ASSERT_EPS(inv.g[1][1], 2.0, 1e-10, "inv_diag[1][1]");
	ASSERT_EPS(inv.g[2][2], 0.25, 1e-10, "inv_diag[2][2]");
}

/* ============================================================================
 * TESTES: SCHWARZSCHILD
 * ============================================================================
 */

void test_schwarzschild()
{
	struct bhs_schwarzschild bh = { .M = 1.0 }; /* rs = 2.0 */
	struct bhs_metric g;

	/* r = 4.0, theta = PI/2 */
	bhs_schwarzschild_metric(&bh, 4.0, 1.57079632679, &g);

	/* f = 1 - 2/4 = 0.5 */
	/* g_tt = -0.5, g_rr = 1/0.5 = 2.0, g_thth = r^2 = 16, g_phph = r^2 *
   * sin^2(90) = 16 */
	ASSERT_EPS(g.g[0][0], -0.5, 1e-10, "schwarzschild_g_tt");
	ASSERT_EPS(g.g[1][1], 2.0, 1e-10, "schwarzschild_g_rr");
	ASSERT_EPS(g.g[2][2], 16.0, 1e-10, "schwarzschild_g_thth");
	ASSERT_EPS(g.g[3][3], 16.0, 1e-10, "schwarzschild_g_phph");

	double z = bhs_schwarzschild_redshift(&bh, 4.0);
	/* z = 1/sqrt(0.5) - 1 = sqrt(2) - 1 ≈ 0.41421356 */
	ASSERT_EPS(z, 0.41421356237, 1e-8, "schwarzschild_redshift");
}

/* ============================================================================
 * MAIN
 * ============================================================================
 */

int main()
{
	printf("=== [BHS CORE TEST SUITE] ===\n");

	test_vec4_math();
	test_metric_invert();
	test_schwarzschild();

	printf("\nResultados:\n");
	printf("  Rodados: %d\n", tests_run);
	printf("  Falhas:  %d\n", tests_failed);

	if (tests_failed == 0) {
		printf("\n\033[32mTODO CORE OPERANDO EM NÍVEL "
		       "KERNEL.\033[0m\n");
		return 0;
	} else {
		printf("\n\033[31mFALHAS DETECTADAS. AUDITE O "
		       "CÓDIGO!\033[0m\n");
		return 1;
	}
}
