/**
 * @file input_layer.h
 * @brief Camada de Input - Bridge entre Teclado e Ações
 *
 * "WASD: as quatro teclas sagradas que movem mundos."
 *
 * Esta camada traduz eventos de input (teclado, mouse) em ações
 * lógicas da aplicação (mover câmera, pausar, salvar, etc).
 *
 * A ideia é que NENHUM outro lugar do código precise saber
 * qual tecla faz o quê. Mapeamentos ficam centralizados aqui.
 */

#ifndef BHS_SRC_INPUT_INPUT_LAYER_H
#define BHS_SRC_INPUT_INPUT_LAYER_H

/* Forward declaration */
struct app_state;

/* ============================================================================
 * API PRINCIPAL
 * ============================================================================
 */

/**
 * input_process_frame - Processa todos os inputs do frame atual
 * @app: Estado da aplicação
 * @dt: Delta time do frame (segundos)
 *
 * Deve ser chamado uma vez por frame, após begin_frame.
 * Atualiza:
 * - Posição e rotação da câmera (WASD, mouse)
 * - Estado de pausa (Space)
 * - Time scale (+/-)
 * - Ações globais (Save, Load, Quit)
 * - Interação com objetos (click selection, delete)
 */
void input_process_frame(struct app_state *app, double dt);

#endif /* BHS_SRC_INPUT_INPUT_LAYER_H */
