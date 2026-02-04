/**
 * @file telemetry.h
 * @brief Dashboard de Debug em Terminal (Real-time Physics Monitor)
 */

#ifndef BHS_CMD_DEBUG_TELEMETRY_H
#define BHS_CMD_DEBUG_TELEMETRY_H

#include "engine/scene/scene.h"

/**
 * @brief Imprime o estado atual da simulação no terminal.
 * Usa ANSI escape codes para tentar sobrescrever ou limpar tela se desejado.
 *
 * @param scene Cena contendo os corpos
 * @param time Tempo total de simulação
 */
/* Dashboard style (Clears screen) */
void bhs_telemetry_print_scene(bhs_scene_t scene, double time, double phys_ms,
			       double render_ms);

/* Scrolling Log style (Append) - Good for history analysis */
void bhs_telemetry_log_orbits(bhs_scene_t scene, double time);

#endif /* BHS_CMD_DEBUG_TELEMETRY_H */
