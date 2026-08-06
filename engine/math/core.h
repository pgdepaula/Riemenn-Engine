/**
 * @file lib.h
 * @brief Core Library - Fundação matemática do simulador
 *
 * "Se você não entende vetores 4D, volte pro Minecraft."
 *
 * Este header agrupa toda a infraestrutura matemática:
 * - Vetores 4D (espaço-tempo)
 * - Tensores métricos
 * - Métricas de Schwarzschild e Kerr
 *
 * Uso típico:
 *   #include "engine/math/core.h"
 *
 *   struct ri_kerr bh = { .M = 1.0, .a = 0.9 };
 *   double isco = ri_kerr_isco(&bh, true);
 */

#ifndef RI_CORE_LIB_H
#define RI_CORE_LIB_H

/* ============================================================================
 * MATEMÁTICA FUNDAMENTAL
 * ============================================================================
 */

#include "engine/foundation/assert.h"
#include "engine/math/vec4.h"

/* ============================================================================
 * TENSORES
 * ============================================================================
 */

#include "engine/math/tensor/tensor.h"

/* ============================================================================
 * ESPAÇO-TEMPO
 * ============================================================================
 */

#include "engine/math/spacetime/kerr.h"
#include "engine/math/spacetime/schwarzschild.h"

#endif /* RI_CORE_LIB_H */
