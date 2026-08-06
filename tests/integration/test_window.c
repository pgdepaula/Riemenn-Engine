/**
 * @file test_window.c
 * @brief Teste de integração - abre uma janela Wayland e espera eventos
 *
 * Isso aqui é pra validar que o backend Wayland tá funcionando.
 * Abre uma janela, processa eventos por uns segundos, e fecha.
 *
 * Se você está vendo uma janela preta, parabéns, funcionou!
 * Se crashou, bem... boa sorte debugando Wayland (diss pra mim mesmo).
 */

#define _DEFAULT_SOURCE /* usleep */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ux/platform/platform.h"

static void event_callback(ri_window_t window, const struct ri_event *event,
                           void *userdata) {
  (void)window;
  (void)userdata;

  switch (event->type) {
  case RI_EVENT_WINDOW_CLOSE:
    printf("[evento] Janela fechada\n");
    break;
  case RI_EVENT_WINDOW_RESIZE:
    printf("[evento] Resize: %dx%d\n", event->resize.width,
           event->resize.height);
    break;
  case RI_EVENT_KEY_DOWN:
    printf("[evento] Tecla: keycode=%u text='%s'\n", event->key.keycode,
           event->key.text);
    break;
  case RI_EVENT_MOUSE_MOVE:
    /* Mouse move é muito verboso, ignora */
    break;
  case RI_EVENT_MOUSE_DOWN:
    printf("[evento] Mouse down: x=%d y=%d button=%d\n", event->mouse_button.x,
           event->mouse_button.y, event->mouse_button.button);
    break;
  default:
    break;
  }
}

int main(void) {
  printf("=== Teste de Integração: Wayland ===\n\n");

  /* Inicializa plataforma */
  printf("Inicializando plataforma...\n");
  ri_platform_t platform;
  int ret = ri_platform_init(&platform);
  if (ret != RI_PLATFORM_OK) {
    fprintf(stderr, "ERRO: Falha ao inicializar plataforma (%d)\n", ret);
    fprintf(stderr, "Dica: Você está rodando em sessão Wayland?\n");
    return 1;
  }
  printf("  OK!\n");

  /* Cria janela */
  printf("Criando janela...\n");
  struct ri_window_config config = {
      .title = "Black Hole Simulator - Teste",
      .width = 800,
      .height = 600,
      .x = RI_WINDOW_POS_CENTERED,
      .y = RI_WINDOW_POS_CENTERED,
      .flags = RI_WINDOW_RESIZABLE,
  };

  ri_window_t window;
  ret = ri_window_create(platform, &config, &window);
  if (ret != RI_PLATFORM_OK) {
    fprintf(stderr, "ERRO: Falha ao criar janela (%d)\n", ret);
    ri_platform_shutdown(platform);
    return 1;
  }
  printf("  OK! Janela criada: %dx%d\n", config.width, config.height);

  /* Registra callback de eventos */
  ri_window_set_event_callback(window, event_callback, NULL);

  /* Loop principal */
  printf("\nLoop de eventos iniciado. Pressione Ctrl+C ou feche a janela.\n");
  printf("Interaja com a janela para ver eventos...\n\n");

  int frames = 0;
  while (!ri_window_should_close(window)) {
    ri_platform_poll_events(platform);

    /* Só pra mostrar que tá vivo */
    frames++;
    if (frames % 1000 == 0) {
      printf("[loop] frame %d\n", frames);
    }

    /* Não queima CPU */
    usleep(1000); /* 1ms */
  }

  /* Cleanup */
  printf("\nFinalizando...\n");
  ri_window_destroy(window);
  ri_platform_shutdown(platform);
  printf("Teste concluído com sucesso!\n");

  return 0;
}
