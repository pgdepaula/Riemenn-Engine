/**
 * @file telemetry.h
 * @brief Dashboard de Debug em Terminal (Real-time Physics Monitor)
 */

#ifndef RI_CMD_DEBUG_TELEMETRY_H
#define RI_CMD_DEBUG_TELEMETRY_H

#include "engine/scene/scene.h"

/**
 * @brief Imprime o estado atual da simulação no terminal.
 * Usa ANSI escape codes para tentar sobrescrever ou limpar tela se desejado.
 *
 * @param scene Cena contendo os corpos
 * @param time Tempo total de simulação
 */
/* Dashboard style (Clears screen) */
void ri_telemetry_print_scene(ri_scene_t scene, double time, double phys_ms,
			       double render_ms);

/* Scrolling Log style (Append) - Good for history analysis */
void ri_telemetry_log_orbits(ri_scene_t scene, double time);

#endif /* RI_CMD_DEBUG_TELEMETRY_H */
