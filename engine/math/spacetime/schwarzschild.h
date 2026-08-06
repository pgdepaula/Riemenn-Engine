/**
 * @file schwarzschild.h
 * @brief Métrica de Schwarzschild - Buraco negro estático
 *
 * "A solução mais simples das equações de Einstein.
 * E ainda assim, ela esconde um horizonte de eventos."
 *
 * Coordenadas: Boyer-Lindquist (t, r, θ, φ)
 * Assinatura: (-,+,+,+) mostly plus
 */

#ifndef RI_CORE_SPACETIME_SCHWARZSCHILD_H
#define RI_CORE_SPACETIME_SCHWARZSCHILD_H

#include "engine/math/tensor/tensor.h"

/* ============================================================================
 * PARÂMETROS
 * ============================================================================
 */

/**
 * struct ri_schwarzschild - Parâmetros do buraco negro de Schwarzschild
 * @M: Massa do buraco negro (em unidades geométricas: G = c = 1)
 *
 * Raio de Schwarzschild: rs = 2M
 *
 * Em unidades SI:
 * rs = 2GM/c² ≈ 2.95 km * (M/M_sol)
 */
struct ri_schwarzschild {
	double M;
};

/* ============================================================================
 * RAIOS CRÍTICOS
 * ============================================================================
 */

/**
 * ri_schwarzschild_rs - Raio de Schwarzschild (horizonte de eventos)
 *
 * rs = 2M
 *
 * Nada escapa de r < rs. Nem luz, nem esperança.
 */
static inline double ri_schwarzschild_rs(const struct ri_schwarzschild *bh)
{
	return 2.0 * bh->M;
}

/**
 * ri_schwarzschild_isco - Innermost Stable Circular Orbit
 *
 * r_isco = 6M = 3rs
 *
 * Órbitas circulares são instáveis para r < r_isco.
 * É o limite interno do disco de acreção.
 */
static inline double ri_schwarzschild_isco(const struct ri_schwarzschild *bh)
{
	return 6.0 * bh->M;
}

/**
 * ri_schwarzschild_photon_sphere - Esfera de fótons
 *
 * r_ph = 3M = 1.5 rs
 *
 * Fótons podem orbitar aqui (instável). Órbitas circulares de luz.
 */
static inline double
ri_schwarzschild_photon_sphere(const struct ri_schwarzschild *bh)
{
	return 3.0 * bh->M;
}

/* ============================================================================
 * MÉTRICA
 * ============================================================================
 */

/**
 * ri_schwarzschild_metric - Calcula tensor métrico
 * @bh: parâmetros do buraco negro
 * @r: coordenada radial (deve ser > rs para evitar singularidade)
 * @theta: ângulo polar [0, π]
 * @out: [out] tensor métrico g_μν
 *
 * Linha de mundo:
 * ds² = -(1 - rs/r) dt² + (1 - rs/r)^(-1) dr² + r² dθ² + r² sin²θ dφ²
 *
 * Componentes:
 * g_tt = -(1 - rs/r)
 * g_rr = 1/(1 - rs/r)
 * g_θθ = r²
 * g_φφ = r² sin²θ
 */
void ri_schwarzschild_metric(const struct ri_schwarzschild *bh, double r,
			      double theta, struct ri_metric *out);

/**
 * ri_schwarzschild_metric_inverse - Calcula métrica inversa g^μν
 */
void ri_schwarzschild_metric_inverse(const struct ri_schwarzschild *bh,
				      double r, double theta,
				      struct ri_metric *out);

/**
 * ri_schwarzschild_redshift - Fator de redshift gravitacional
 * @bh: parâmetros
 * @r: coordenada radial
 *
 * Retorna: z = 1/√(1 - rs/r) - 1
 *
 * Luz emitida em r chega ao infinito com frequência reduzida por (1 + z).
 * No horizonte (r = rs): z → ∞ (redshift infinito)
 */
double ri_schwarzschild_redshift(const struct ri_schwarzschild *bh, double r);

/**
 * ri_schwarzschild_escape_velocity - Velocidade de escape
 * @bh: parâmetros
 * @r: coordenada radial
 *
 * Retorna: v_esc = √(rs/r) = √(2M/r)
 *
 * No horizonte: v_esc = c (1.0 em unidades naturais)
 */
double ri_schwarzschild_escape_velocity(const struct ri_schwarzschild *bh,
					 double r);

/* ============================================================================
 * FUNÇÃO DE MÉTRICA (para Christoffel)
 * ============================================================================
 */

/**
 * ri_schwarzschild_metric_func - Wrapper para ri_christoffel_compute
 *
 * Use como ri_metric_func com userdata = struct ri_schwarzschild*
 *
 * Coordenadas em vec4: (t, r, θ, φ)
 */
void ri_schwarzschild_metric_func(struct ri_vec4 coords, void *userdata,
				   struct ri_metric *out);

#endif /* RI_CORE_SPACETIME_SCHWARZSCHILD_H */
