#ifndef BHS_SRC_UI_SCREENS_START_SCREEN_H
#define BHS_SRC_UI_SCREENS_START_SCREEN_H

#include "gui/ui/lib.h"

/* Forward declarations */
struct app_state;

/**
 * @brief Desenha a tela de boot estilizada (Riemann Engine)
 * 
 * @param app Ponteiro para o estado da aplicação (para ler/escrever cenário e estado)
 * @param ctx Contexto de UI para desenho
 * @param win_w Largura da janela
 * @param win_h Altura da janela
 */
void bhs_start_screen_draw(struct app_state *app, bhs_ui_ctx_t ctx, int win_w,
			   int win_h);

#endif /* BHS_SRC_UI_SCREENS_START_SCREEN_H */
