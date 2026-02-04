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
 *   #include "math/core.h"
 *
 *   struct bhs_kerr bh = { .M = 1.0, .a = 0.9 };
 *   double isco = bhs_kerr_isco(&bh, true);
 */

#ifndef BHS_CORE_LIB_H
#define BHS_CORE_LIB_H

/* ============================================================================
 * MATEMÁTICA FUNDAMENTAL
 * ============================================================================
 */

#include "math/bhs_math_assert.h"
#include "math/vec4.h"

/* ============================================================================
 * TENSORES
 * ============================================================================
 */

#include "math/tensor/tensor.h"

/* ============================================================================
 * ESPAÇO-TEMPO
 * ============================================================================
 */

#include "math/spacetime/kerr.h"
#include "math/spacetime/schwarzschild.h"

#endif /* BHS_CORE_LIB_H */
