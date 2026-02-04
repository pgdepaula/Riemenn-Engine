/**
 * @file test_ui.c
 * @brief Teste de integração da biblioteca UI
 *
 * Se você chegou até aqui, já passou pelos campos minados do Wayland,
 * sobreviveu às profundezas do Vulkan, e agora está pronto pra ver
 * um botão na tela.
 *
 * Esse teste mostra:
 * - Criação de contexto UI (janela + GPU integrados)
 * - Frame loop com widgets immediate mode
 * - Interação básica (botão, slider, checkbox)
 *
 * Se funcionar, vá tomar uma água. Você merece.
 */

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include "ux/lib/lib.h"
#include "ux/ui/app.h"

int main(void) {
  printf("=== Teste de Integração: UI ===\n\n");

  /* Cria contexto */
  struct bhs_ui_config config = {
      .title = "Black Hole Simulator - UI Test",
      .width = 800,
      .height = 600,
      .resizable = true,
      .vsync = true,
      .debug = true,
  };

  bhs_ui_ctx_t ctx;
  int ret = bhs_ui_create(&config, &ctx);
  if (ret != BHS_UI_OK) {
    fprintf(stderr, "ERRO: Falha ao criar contexto UI (%d)\n", ret);
    return 1;
  }
  printf("Contexto UI criado com sucesso!\n");

  /* Estado dos widgets */
  float slider_value = 0.5f;
  bool checkbox_checked = false;
  int click_count = 0;

  /* Frame loop */
  printf("Loop iniciado. Feche a janela ou pressione ESC para sair.\n\n");

  /* Inicializa o App */
  void bhs_app_init(void); /* Declarado em app.h, mas aqui vamos sem include pra
                              simplificar ou incluir app.h */
  // Preferível incluir app.h

  /* ... (main loop) ... */
  while (!bhs_ui_should_close(ctx)) {
    /* Início do frame */
    ret = bhs_ui_begin_frame(ctx);
    if (ret != BHS_UI_OK)
      break;

    /* Verifica ESC */
    if (bhs_ui_key_pressed(ctx, BHS_KEY_ESCAPE))
      break;

    /* Roda a Aplicação */
    bhs_app_update(ctx);

    /* Fim do frame */
    ret = bhs_ui_end_frame(ctx);
    if (ret != BHS_UI_OK)
      break;
  }

  /* Cleanup */
  printf("\nFinalizando...\n");
  bhs_ui_destroy(ctx);
  printf("Teste concluído! Clicks: %d, Slider: %.2f, Checkbox: %s\n",
         click_count, slider_value, checkbox_checked ? "ON" : "OFF");

  return 0;
}
