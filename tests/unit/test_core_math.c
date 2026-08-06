/**
 * @file test_core_math.c
 * @brief Testes unitários do core matemático
 *
 * "Se não testa, não funciona. Se funciona sem testar, foi sorte."
 *
 * Testa:
 * - Operações de vetor 4D
 * - Métrica de Minkowski
 * - Métricas Schwarzschild e Kerr
 * - Símbolos de Christoffel
 */

#define _GNU_SOURCE
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/math/vec4.h"
#include "core/spacetime/kerr.h"
#include "core/spacetime/schwarzschild.h"
#include "core/tensor/tensor.h"

/* ============================================================================
 * MACROS DE TESTE
 * ============================================================================
 */

#define TEST_EPSILON 1e-10

static int tests_passed = 0;
static int tests_failed = 0;

#define ASSERT_TRUE(cond, msg)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      printf("  ❌ FALHOU: %s\n", msg);                                        \
      tests_failed++;                                                          \
      return;                                                                  \
    }                                                                          \
    tests_passed++;                                                            \
  } while (0)

#define ASSERT_NEAR(a, b, eps, msg)                                            \
  do {                                                                         \
    if (fabs((a) - (b)) > (eps)) {                                             \
      printf("  ❌ FALHOU: %s (esperado %.10f, obtido %.10f)\n", msg,          \
             (double)(b), (double)(a));                                        \
      tests_failed++;                                                          \
      return;                                                                  \
    }                                                                          \
    tests_passed++;                                                            \
  } while (0)

/* ============================================================================
 * TESTES VEC4
 * ============================================================================
 */

static void test_vec4_operations(void) {
  printf("📐 Testando vec4 operações...\n");

  struct ri_vec4 a = ri_vec4_make(1.0, 2.0, 3.0, 4.0);
  struct ri_vec4 b = ri_vec4_make(5.0, 6.0, 7.0, 8.0);

  /* Soma */
  struct ri_vec4 c = ri_vec4_add(a, b);
  ASSERT_NEAR(c.t, 6.0, TEST_EPSILON, "vec4_add t");
  ASSERT_NEAR(c.x, 8.0, TEST_EPSILON, "vec4_add x");
  ASSERT_NEAR(c.y, 10.0, TEST_EPSILON, "vec4_add y");
  ASSERT_NEAR(c.z, 12.0, TEST_EPSILON, "vec4_add z");

  /* Escalar */
  struct ri_vec4 d = ri_vec4_scale(a, 2.0);
  ASSERT_NEAR(d.t, 2.0, TEST_EPSILON, "vec4_scale t");
  ASSERT_NEAR(d.x, 4.0, TEST_EPSILON, "vec4_scale x");

  printf("  ✅ vec4 operações OK\n");
}

static void test_vec4_minkowski(void) {
  printf("📐 Testando produto escalar Minkowski...\n");

  /* Vetor lightlike (nulo) */
  struct ri_vec4 photon = ri_vec4_make(1.0, 1.0, 0.0, 0.0);
  double norm2 = ri_vec4_norm2_minkowski(photon);
  ASSERT_NEAR(norm2, 0.0, TEST_EPSILON, "photon é null (ds²=0)");
  ASSERT_TRUE(ri_vec4_is_null(photon, 1e-6), "ri_vec4_is_null");

  /* Vetor timelike */
  struct ri_vec4 particle = ri_vec4_make(2.0, 1.0, 0.0, 0.0);
  double norm2_p = ri_vec4_norm2_minkowski(particle);
  ASSERT_TRUE(norm2_p < 0.0, "partícula é timelike (ds²<0)");
  ASSERT_NEAR(norm2_p, -3.0, TEST_EPSILON, "norma timelike");

  /* Vetor spacelike */
  struct ri_vec4 space = ri_vec4_make(0.0, 1.0, 1.0, 1.0);
  double norm2_s = ri_vec4_norm2_minkowski(space);
  ASSERT_TRUE(norm2_s > 0.0, "intervalo é spacelike (ds²>0)");

  printf("  ✅ Minkowski OK\n");
}

/* ============================================================================
 * TESTES VEC3
 * ============================================================================
 */

static void test_vec3_spherical(void) {
  printf("🌐 Testando coordenadas esféricas...\n");

  /* Ponto no eixo z+ */
  struct ri_vec3 z_axis = ri_vec3_make(0.0, 0.0, 5.0);
  double r, theta, phi;
  ri_vec3_to_spherical(z_axis, &r, &theta, &phi);

  ASSERT_NEAR(r, 5.0, TEST_EPSILON, "r no eixo z");
  ASSERT_NEAR(theta, 0.0, TEST_EPSILON, "theta no eixo z");

  /* Ponto no eixo x+ */
  struct ri_vec3 x_axis = ri_vec3_make(3.0, 0.0, 0.0);
  ri_vec3_to_spherical(x_axis, &r, &theta, &phi);

  ASSERT_NEAR(r, 3.0, TEST_EPSILON, "r no eixo x");
  ASSERT_NEAR(theta, M_PI / 2.0, TEST_EPSILON, "theta no eixo x");
  ASSERT_NEAR(phi, 0.0, TEST_EPSILON, "phi no eixo x");

  /* Ida e volta */
  struct ri_vec3 original = ri_vec3_make(1.0, 2.0, 3.0);
  ri_vec3_to_spherical(original, &r, &theta, &phi);
  struct ri_vec3 back = ri_vec3_from_spherical(r, theta, phi);

  ASSERT_NEAR(back.x, original.x, TEST_EPSILON, "roundtrip x");
  ASSERT_NEAR(back.y, original.y, TEST_EPSILON, "roundtrip y");
  ASSERT_NEAR(back.z, original.z, TEST_EPSILON, "roundtrip z");

  printf("  ✅ Esféricas OK\n");
}

/* ============================================================================
 * TESTES TENSOR
 * ============================================================================
 */

static void test_metric_minkowski(void) {
  printf("📊 Testando métrica Minkowski...\n");

  struct ri_metric eta = ri_metric_minkowski();

  ASSERT_NEAR(eta.g[0][0], -1.0, TEST_EPSILON, "η_tt = -1");
  ASSERT_NEAR(eta.g[1][1], 1.0, TEST_EPSILON, "η_xx = 1");
  ASSERT_NEAR(eta.g[2][2], 1.0, TEST_EPSILON, "η_yy = 1");
  ASSERT_NEAR(eta.g[3][3], 1.0, TEST_EPSILON, "η_zz = 1");

  /* Determinante */
  double det = ri_metric_det(&eta);
  ASSERT_NEAR(det, -1.0, TEST_EPSILON, "det(η) = -1");

  /* Inversa = ela mesma */
  struct ri_metric eta_inv;
  int ret = ri_metric_invert(&eta, &eta_inv);
  ASSERT_TRUE(ret == 0, "inversão OK");
  ASSERT_NEAR(eta_inv.g[0][0], -1.0, TEST_EPSILON, "η^tt = -1");

  printf("  ✅ Minkowski OK\n");
}

static void test_metric_product(void) {
  printf("📊 Testando produto com métrica...\n");

  struct ri_metric eta = ri_metric_minkowski();
  struct ri_vec4 v = ri_vec4_make(1.0, 1.0, 1.0, 1.0);

  /* η_μν v^μ v^ν = -1 + 1 + 1 + 1 = 2 */
  double dot = ri_metric_dot(&eta, v, v);
  ASSERT_NEAR(dot, 2.0, TEST_EPSILON, "produto com Minkowski");

  printf("  ✅ Produto OK\n");
}

/* ============================================================================
 * TESTES SCHWARZSCHILD
 * ============================================================================
 */

static void test_schwarzschild_metric(void) {
  printf("🕳️ Testando métrica Schwarzschild...\n");

  struct ri_schwarzschild bh = {.M = 1.0};

  /* Raios críticos */
  ASSERT_NEAR(ri_schwarzschild_rs(&bh), 2.0, TEST_EPSILON, "rs = 2M");
  ASSERT_NEAR(ri_schwarzschild_isco(&bh), 6.0, TEST_EPSILON, "ISCO = 6M");
  ASSERT_NEAR(ri_schwarzschild_photon_sphere(&bh), 3.0, TEST_EPSILON,
              "Photon sphere = 3M");

  /* Métrica em r = 10M, θ = π/2 */
  struct ri_metric g;
  double r = 10.0;
  double theta = M_PI / 2.0;
  ri_schwarzschild_metric(&bh, r, theta, &g);

  double rs = 2.0;
  double f = 1.0 - rs / r; /* 0.8 */

  ASSERT_NEAR(g.g[0][0], -f, TEST_EPSILON, "g_tt = -(1-rs/r)");
  ASSERT_NEAR(g.g[1][1], 1.0 / f, TEST_EPSILON, "g_rr = 1/(1-rs/r)");
  ASSERT_NEAR(g.g[2][2], r * r, TEST_EPSILON, "g_θθ = r²");
  ASSERT_NEAR(g.g[3][3], r * r, TEST_EPSILON, "g_φφ = r² (no equador)");

  /* Deve ser diagonal */
  ASSERT_NEAR(g.g[0][1], 0.0, TEST_EPSILON, "off-diagonal = 0");

  printf("  ✅ Schwarzschild OK\n");
}

/* ============================================================================
 * TESTES KERR
 * ============================================================================
 */

static void test_kerr_limits(void) {
  printf("🌀 Testando limites Kerr...\n");

  /* Kerr com a=0 deve ser Schwarzschild */
  struct ri_kerr kerr_s = {.M = 1.0, .a = 0.0};

  ASSERT_NEAR(ri_kerr_horizon_outer(&kerr_s), 2.0, TEST_EPSILON,
              "r+ = 2M para a=0");
  ASSERT_NEAR(ri_kerr_horizon_inner(&kerr_s), 0.0, TEST_EPSILON,
              "r- = 0 para a=0");
  ASSERT_NEAR(ri_kerr_isco(&kerr_s, true), 6.0, TEST_EPSILON,
              "ISCO = 6M para a=0");

  /* Kerr extremo a=M */
  struct ri_kerr kerr_e = {.M = 1.0, .a = 1.0};

  ASSERT_NEAR(ri_kerr_horizon_outer(&kerr_e), 1.0, TEST_EPSILON,
              "r+ = M para a=M");
  ASSERT_NEAR(ri_kerr_horizon_inner(&kerr_e), 1.0, TEST_EPSILON,
              "r- = M para a=M");

  /* Ergoesfera no equador = 2M (mesmo para Kerr extremo) */
  double ergo_eq = ri_kerr_ergosphere(&kerr_e, M_PI / 2.0);
  ASSERT_NEAR(ergo_eq, 2.0, TEST_EPSILON, "Ergoesfera equatorial = 2M");

  printf("  ✅ Limites Kerr OK\n");
}

static void test_kerr_metric_reduces_to_schwarzschild(void) {
  printf("🌀 Testando Kerr → Schwarzschild para a=0...\n");

  struct ri_kerr kerr = {.M = 1.0, .a = 0.0};
  struct ri_schwarzschild sch = {.M = 1.0};

  double r = 10.0;
  double theta = M_PI / 2.0;

  struct ri_metric g_kerr, g_sch;
  ri_kerr_metric(&kerr, r, theta, &g_kerr);
  ri_schwarzschild_metric(&sch, r, theta, &g_sch);

  ASSERT_NEAR(g_kerr.g[0][0], g_sch.g[0][0], TEST_EPSILON, "g_tt coincide");
  ASSERT_NEAR(g_kerr.g[1][1], g_sch.g[1][1], TEST_EPSILON, "g_rr coincide");
  ASSERT_NEAR(g_kerr.g[2][2], g_sch.g[2][2], TEST_EPSILON, "g_θθ coincide");
  ASSERT_NEAR(g_kerr.g[3][3], g_sch.g[3][3], TEST_EPSILON, "g_φφ coincide");
  ASSERT_NEAR(g_kerr.g[0][3], 0.0, TEST_EPSILON, "g_tφ = 0 para a=0");

  printf("  ✅ Kerr → Schwarzschild OK\n");
}

static void test_kerr_frame_dragging(void) {
  printf("🌀 Testando frame dragging...\n");

  struct ri_kerr kerr = {.M = 1.0, .a = 0.9};

  double r = 5.0;
  double theta = M_PI / 2.0;

  /* Frame dragging deve ser não-zero para a≠0 */
  double omega = ri_kerr_omega_frame(&kerr, r, theta);
  ASSERT_TRUE(omega > 0.0, "ω > 0 para a > 0");

  /* E deve decair com r */
  double omega_far = ri_kerr_omega_frame(&kerr, 100.0, theta);
  ASSERT_TRUE(omega_far < omega, "ω decresce com r");

  printf("  ✅ Frame dragging OK\n");
}

/* ============================================================================
 * MAIN
 * ============================================================================
 */

int main(void) {
  printf("\n");
  printf("╔══════════════════════════════════════════╗\n");
  printf("║  🧪 Testes Unitários - Core Matemático   ║\n");
  printf("╚══════════════════════════════════════════╝\n\n");

  /* Vec4 */
  test_vec4_operations();
  test_vec4_minkowski();

  /* Vec3 */
  test_vec3_spherical();

  /* Tensor */
  test_metric_minkowski();
  test_metric_product();

  /* Schwarzschild */
  test_schwarzschild_metric();

  /* Kerr */
  test_kerr_limits();
  test_kerr_metric_reduces_to_schwarzschild();
  test_kerr_frame_dragging();

  /* Resumo */
  printf("\n");
  printf("═══════════════════════════════════════════\n");
  if (tests_failed == 0) {
    printf("🎉 TODOS OS TESTES PASSARAM! (%d asserts)\n", tests_passed);
  } else {
    printf("💀 FALHAS: %d (passou: %d)\n", tests_failed, tests_passed);
  }
  printf("═══════════════════════════════════════════\n\n");

  return tests_failed > 0 ? 1 : 0;
}
