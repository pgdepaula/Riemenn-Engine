/**
 * @file test_disk.c
 * @brief Testes unitários do módulo de disco de acreção
 *
 * "Testando física que levou décadas pra ser derivada.
 * Em segundos. Computadores são legais."
 */

#define _GNU_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/spacetime/kerr.h"
#include "engine/disk/disk.h"

/* ============================================================================
 * MACROS DE TESTE
 * ============================================================================
 */

#define TEST_EPSILON 1e-6

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
      printf("  ❌ FALHOU: %s (esperado %.6f, obtido %.6f)\n", msg,            \
             (double)(b), (double)(a));                                        \
      tests_failed++;                                                          \
      return;                                                                  \
    }                                                                          \
    tests_passed++;                                                            \
  } while (0)

/* ============================================================================
 * TESTES
 * ============================================================================
 */

static void test_disk_isco(void) {
  printf("🌀 Testando ISCO do disco...\n");

  struct bhs_kerr kerr_s = {.M = 1.0, .a = 0.0};
  struct bhs_kerr kerr_spin = {.M = 1.0, .a = 0.9};

  /* Schwarzschild: ISCO = 6M */
  double isco_s = bhs_disk_isco(&kerr_s);
  ASSERT_NEAR(isco_s, 6.0, 0.01, "ISCO Schwarzschild = 6M");

  /* Com spin, ISCO é menor (prograde) */
  double isco_spin = bhs_disk_isco(&kerr_spin);
  ASSERT_TRUE(isco_spin < isco_s, "ISCO com spin < ISCO sem spin");
  ASSERT_TRUE(isco_spin > 1.0, "ISCO > horizonte");

  printf("  ✅ ISCO OK (Schw=%.2f, Kerr 0.9=%.2f)\n", isco_s, isco_spin);
}

static void test_disk_temperature(void) {
  printf("🔥 Testando temperatura do disco...\n");

  struct bhs_kerr bh = {.M = 1.0, .a = 0.5};
  struct bhs_disk disk = {
      .inner_radius = bhs_disk_isco(&bh),
      .outer_radius = 15.0,
      .mdot = 0.1,
      .inclination = M_PI / 4.0,
  };

  /* Temperatura zero fora do disco */
  double temp_inside =
      bhs_disk_temperature(&bh, &disk, disk.inner_radius - 0.1);
  ASSERT_NEAR(temp_inside, 0.0, TEST_EPSILON, "Temp=0 dentro do ISCO");

  double temp_outside =
      bhs_disk_temperature(&bh, &disk, disk.outer_radius + 1.0);
  ASSERT_NEAR(temp_outside, 0.0, TEST_EPSILON, "Temp=0 fora do disco");

  /* Temperatura: pico em r_peak = 1.5 * ISCO, depois decresce */
  double r_peak = disk.inner_radius * 1.5;
  double temp_peak = bhs_disk_temperature(&bh, &disk, r_peak);
  double temp_mid = bhs_disk_temperature(&bh, &disk, 8.0);
  double temp_outer = bhs_disk_temperature(&bh, &disk, 14.0);

  ASSERT_TRUE(temp_peak > temp_mid, "Temp decresce: peak > mid");
  ASSERT_TRUE(temp_mid > temp_outer, "Temp decresce: mid > outer");

  printf("  \u2705 Temperatura OK (peak=%.3f, mid=%.3f, outer=%.3f)\n",
         temp_peak, temp_mid, temp_outer);
}

static void test_disk_orbital_velocity(void) {
  printf("🚀 Testando velocidades orbitais...\n");

  struct bhs_kerr bh = {.M = 1.0, .a = 0.5};

  /* Velocidade Kepleriana */
  double omega_10 = bhs_disk_omega_kepler(&bh, 10.0);
  double omega_20 = bhs_disk_omega_kepler(&bh, 20.0);

  ASSERT_TRUE(omega_10 > omega_20, "Ω_K decresce com r");
  ASSERT_TRUE(omega_10 > 0.0, "Ω_K > 0 (prograde)");

  /* Velocidade tangencial */
  double v_phi = bhs_disk_velocity_phi(&bh, 10.0);
  ASSERT_TRUE(v_phi > 0.0, "v_φ > 0");

  printf("  ✅ Velocidades OK (Ω_K(10)=%.4f, v_φ=%.4f)\n", omega_10, v_phi);
}

static void test_disk_redshift(void) {
  printf("🔴🔵 Testando redshift Doppler...\n");

  struct bhs_kerr bh = {.M = 1.0, .a = 0.5};
  double r = 10.0;
  double incl = M_PI / 4.0; /* 45 graus */

  /* Redshift em diferentes posições azimutais */
  /* phi = 0: lado longe (z ~ 0)
   * phi = π/2: lado se aproximando (blueshift, z < 0)
   * phi = 3π/2: lado se afastando (redshift, z > 0)
   */
  double z_far = bhs_disk_redshift_total(&bh, r, 0.0, incl);
  double z_approach = bhs_disk_redshift_total(&bh, r, M_PI / 2.0, incl);
  double z_recede = bhs_disk_redshift_total(&bh, r, 3.0 * M_PI / 2.0, incl);

  /* Redshift gravitacional sempre positivo */
  ASSERT_TRUE(z_far > 0.0, "z gravitacional > 0");

  /* Os dois lados devem ter redshifts diferentes devido ao Doppler */
  ASSERT_TRUE(fabs(z_approach - z_recede) > 0.01,
              "Lados opostos tem z diferente");

  printf("  \u2705 Redshift OK (longe=%.3f, lado1=%.3f, lado2=%.3f)\n", z_far,
         z_approach, z_recede);
}

static void test_blackbody_colors(void) {
  printf("🌈 Testando cores de corpo negro...\n");

  struct bhs_color_rgb cold = bhs_blackbody_color(0.1);
  struct bhs_color_rgb hot = bhs_blackbody_color(0.5);
  struct bhs_color_rgb very_hot = bhs_blackbody_color(0.9);

  /* Frio: mais vermelho */
  ASSERT_TRUE(cold.r > cold.b, "Frio: mais vermelho");

  /* Muito quente: mais branco/azulado */
  ASSERT_TRUE(very_hot.b > cold.b, "Quente: mais azul");
  ASSERT_TRUE(very_hot.g > cold.g, "Quente: mais verde");

  printf("  ✅ Cores OK (frio: R%.2f G%.2f B%.2f, quente: R%.2f G%.2f B%.2f)\n",
         cold.r, cold.g, cold.b, very_hot.r, very_hot.g, very_hot.b);
}

/* ============================================================================
 * MAIN
 * ============================================================================
 */

int main(void) {
  printf("\n");
  printf("╔══════════════════════════════════════════╗\n");
  printf("║  🧪 Testes Unitários - Disco de Acreção  ║\n");
  printf("╚══════════════════════════════════════════╝\n\n");

  test_disk_isco();
  test_disk_temperature();
  test_disk_orbital_velocity();
  test_disk_redshift();
  test_blackbody_colors();

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
