/**
 * @file engine.h
 * @brief Riemenn Engine - Public API
 *
 * This is the ONLY header that the Application (game/) should include.
 * It strictly hides the implementation details (ECS, Physics, Algorithms).
 *
 * Principios:
 * - Opaque definition (void*)
 * - Simple Life-cycle (Init, Update, Shutdown)
 * - Data-Oriented input (Scene loading)
 */

#ifndef RI_ENGINE_H
#define RI_ENGINE_H

#include <stdbool.h>
#include "engine/math/core.h"

/* ============================================================================
 * ENGINE LIFECYCLE
 * ============================================================================
 */

/**
 * ri_engine_init - Inicializa subsistemas (Memory, ECS, Physics)
 * 
 * Deve ser chamado antes de qualquer outra funcao da engine.
 */
void ri_engine_init(void);

/**
 * ri_engine_shutdown - Libera todos os recursos
 */
void ri_engine_shutdown(void);

/**
 * ri_engine_update - Avanca a simulacao
 * @dt: Delta time em segundos (step fixo recomendado para fisica)
 *
 * Executa:
 * 1. Physics System (Integrator)
 * 2. Spacetime System (Metric Updates)
 * 3. Script/Game Logic Systems
 */
void ri_engine_update(double dt);

/* ============================================================================
 * SCENE MANAGEMENT
 * ============================================================================
 */

/**
 * ri_scene_load - Carrega uma cena (planetas, config)
 * @path: Caminho para o arquivo ou string de definicao
 *
 * Por enquanto, hardcoded ou script simples.
 * Futuro: JSON/Binary serialization.
 */
void ri_scene_load(const char *path);

/* ============================================================================
 * ECS EXPOSURE (Optional/Advanced)
 * ============================================================================
 * A aplicacao pode precisar criar entidades manualmente.
 * Expomos handles opacos.
 */

// TODO: Decidir se expomos ECS diretamente aqui ou se mantemos encapsulado. 
// Provavelmente vamos manter encapsulado pra ninguém fazer besteira.
// Por enquanto, encapsulado. Use ri_scene_load.

#endif /* RI_ENGINE_H */
