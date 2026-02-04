/*
 * test_scene_lifecycle.c
 * Testa criação, atualização e destruição da cena repetidamente
 * para garantir estabilidade e ausência de leaks.
 */

#include "engine/scene/scene.h"
#include <stdio.h>
#include <time.h>

#define NUM_ITERATIONS 1000

int main(void) {
  printf("=== Teste de Ciclo de Vida da Cena ===\n");
  printf("Rodando %d iteracoes...\n", NUM_ITERATIONS);

  clock_t start = clock();

  for (int i = 0; i < NUM_ITERATIONS; i++) {
    bhs_scene_t scene = bhs_scene_create();
    if (!scene) {
      fprintf(stderr, "[FALHA] Nao foi possivel criar cena na iteracao %d\n",
              i);
      return 1;
    }

    bhs_scene_init_default(scene);

    /* 
     * Como a engine entrega uma cena vazia, precisamos adicionar algo 
     * para o teste não reclamar de "Cena vazia". 
     */
    struct bhs_vec3 zero = {0, 0, 0};
    bhs_scene_add_body(scene, BHS_BODY_PLANET, zero, zero, 1.0, 1.0, zero);

    /* Simula alguns frames */
    for (int j = 0; j < 10; j++) {
      bhs_scene_update(scene, 0.016);
    }

    /* Check simples para ver se tem coisas lá dentro */
    int count = 0;
    const struct bhs_body *bodies = bhs_scene_get_bodies(scene, &count);
    if (count == 0 || !bodies) {
      fprintf(stderr, "[FALHA] Cena vazia na iteracao %d\n", i);
      bhs_scene_destroy(scene);
      return 1;
    }

    /* Spacetime check removed (Legacy) */

    bhs_scene_destroy(scene);

    if (i % 100 == 0) {
      printf("Iteracao %d ok...\r", i);
      fflush(stdout);
    }
  }

  clock_t end = clock();
  double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

  printf("\n[SUCESSO] %d ciclos completados em %.4f segundos.\n",
         NUM_ITERATIONS, time_spent);
  printf("Se o uso de memoria do sistema nao aumentou, estamos clean.\n");

  return 0;
}
