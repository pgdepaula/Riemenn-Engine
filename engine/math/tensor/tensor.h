/**
 * @file tensor.h
 * @brief Tensores métricos e símbolos de Christoffel
 *
 * "Um tensor é algo que transforma como tensor."
 * — Definição circular mais honesta da física
 *
 * Este módulo implementa:
 * - Tensor métrico g_μν (covariante, 4x4 simétrico)
 * - Tensor métrico inverso g^μν (contravariante)
 * - Símbolos de Christoffel Γ^α_μν
 */

#ifndef RI_CORE_TENSOR_TENSOR_H
#define RI_CORE_TENSOR_TENSOR_H

#include <stdbool.h>

#include "engine/math/ri_math.h"
#include "engine/math/vec4.h"

/* ============================================================================
 * ESTRUTURAS
 * ============================================================================
 */

/**
 * struct ri_metric - Tensor métrico covariante g_μν
 *
 * Matriz 4x4 simétrica: g[μ][ν] = g[ν][μ]
 * Índices: 0=t, 1=x/r, 2=y/θ, 3=z/φ
 *
 * Alinhamento 16 bytes para compatibilidade com GPU (std140/std430)
 */
struct ri_metric {
	RI_ALIGN(16) real_t g[4][4];
};

/**
 * struct ri_christoffel - Símbolos de Christoffel Γ^α_μν
 *
 * Conexão de Levi-Civita, simétrica nos índices inferiores.
 * Γ[α][μ][ν] = Γ[α][ν][μ]
 */
struct ri_christoffel {
	RI_ALIGN(16) real_t gamma[4][4][4];
};

/* ============================================================================
 * CONSTANTES
 * ============================================================================
 */

/**
 * Métrica de Minkowski (espaço plano)
 */
extern const struct ri_metric RI_MINKOWSKI;

/* ============================================================================
 * OPERAÇÕES COM MÉTRICA
 * ============================================================================
 */

/**
 * ri_metric_zero - Métrica zerada
 */
struct ri_metric ri_metric_zero(void);

/**
 * ri_metric_minkowski - Retorna métrica de Minkowski
 *
 * η_μν = diag(-1, +1, +1, +1)
 */
struct ri_metric ri_metric_minkowski(void);

/**
 * ri_metric_diag - Cria métrica diagonal
 * @g00, g11, g22, g33: elementos da diagonal
 *
 * Útil pra métricas esféricas onde só a diagonal importa.
 */
struct ri_metric ri_metric_diag(real_t g00, real_t g11, real_t g22,
				  real_t g33);

/**
 * ri_metric_is_symmetric - Verifica simetria
 *
 * Retorna true se g[μ][ν] = g[ν][μ] para todos μ, ν.
 */
bool ri_metric_is_symmetric(const struct ri_metric *m, real_t tol);

/**
 * ri_metric_det - Determinante da métrica
 *
 * Retorna det(g_μν).
 * Para métricas diagonais: g00 * g11 * g22 * g33
 *
 * O determinante é usado para calcular elementos de volume:
 * dV = √|g| d⁴x
 */
real_t ri_metric_det(const struct ri_metric *m);

/**
 * ri_metric_invert - Inverte a métrica
 * @m: métrica covariante g_μν
 * @inv: [out] métrica contravariante g^μν
 *
 * Calcula g^μν tal que g^μα g_αν = δ^μ_ν
 *
 * Retorna:
 *   0 em sucesso
 *  -1 se matriz singular (det ≈ 0)
 */
int ri_metric_invert(const struct ri_metric *m, struct ri_metric *inv);

/* ============================================================================
 * OPERAÇÕES COM VETORES
 * ============================================================================
 */

/**
 * ri_metric_lower - Abaixa índice de vetor (contravariante → covariante)
 * @m: métrica g_μν
 * @v: vetor contravariante v^μ
 *
 * Retorna: v_μ = g_μν v^ν
 */
struct ri_vec4 ri_metric_lower(const struct ri_metric *m, struct ri_vec4 v);

/**
 * ri_metric_raise - Levanta índice de vetor (covariante → contravariante)
 * @m_inv: métrica inversa g^μν
 * @v: vetor covariante v_μ
 *
 * Retorna: v^μ = g^μν v_ν
 */
struct ri_vec4 ri_metric_raise(const struct ri_metric *m_inv,
				 struct ri_vec4 v);

/**
 * ri_metric_dot - Produto interno com métrica geral
 * @m: métrica g_μν
 * @a, @b: vetores contravariantes
 *
 * Retorna: g_μν a^μ b^ν
 */
real_t ri_metric_dot(const struct ri_metric *m, struct ri_vec4 a,
		      struct ri_vec4 b);

/* ============================================================================
 * SÍMBOLOS DE CHRISTOFFEL
 * ============================================================================
 */

#ifndef RI_SHADER_COMPILER
/**
 * Ponteiro de função para métrica parametrizada
 *
 * @coords: coordenadas (t, r/x, θ/y, φ/z) dependendo do sistema
 * @userdata: parâmetros adicionais (massa, spin, etc.)
 * @out: métrica calculada nesse ponto
 */
typedef void (*ri_metric_func)(struct ri_vec4 coords, void *userdata,
				struct ri_metric *out);
#endif

#ifndef RI_SHADER_COMPILER
/**
 * ri_christoffel_compute - Calcula símbolos de Christoffel numericamente
 * @metric_fn: função que retorna a métrica em um ponto
 * @coords: ponto onde calcular
 * @userdata: parâmetros passados para metric_fn
 * @h: tamanho do passo para diferença finita
 * @out: [out] símbolos de Christoffel Γ^α_μν
 *
 * Usa diferença central: ∂_μ g ≈ [g(x+h) - g(x-h)] / (2h)
 *
 * Retorna:
 *   0 em sucesso
 *  -1 se falhou (métrica singular)
 */
int ri_christoffel_compute(ri_metric_func metric_fn, struct ri_vec4 coords,
			    void *userdata, real_t h,
			    struct ri_christoffel *out);
#endif

/**
 * ri_christoffel_zero - Christoffel zerado (espaço plano)
 */
struct ri_christoffel ri_christoffel_zero(void);

/**
 * ri_geodesic_accel - Calcula aceleração geodésica
 * @chris: símbolos de Christoffel
 * @vel: 4-velocidade u^μ = dx^μ/dλ
 *
 * Retorna: a^α = -Γ^α_μν u^μ u^ν
 *
 * Esta é a aceleração que aparece na equação de geodésica:
 * d²x^α/dλ² = -Γ^α_μν (dx^μ/dλ)(dx^ν/dλ)
 */
struct ri_vec4 ri_geodesic_accel(const struct ri_christoffel *chris,
				   struct ri_vec4 vel);

#endif /* RI_CORE_TENSOR_TENSOR_H */
