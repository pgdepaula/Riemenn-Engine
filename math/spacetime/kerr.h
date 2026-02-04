/**
 * @file kerr.h
 * @brief Métrica de Kerr - Buraco negro rotativo
 *
 * "O Gargantua do Interestelar usa essa métrica.
 * Se você entende isso, você entende Hollywood melhor que físicos."
 *
 * A métrica de Kerr descreve o espaço-tempo ao redor de um buraco negro
 * com momento angular (spin). É a solução fisicamente relevante para
 * buracos negros astrofísicos.
 *
 * Coordenadas: Boyer-Lindquist (t, r, θ, φ)
 * Assinatura: (-,+,+,+) mostly plus
 */

#ifndef BHS_CORE_SPACETIME_KERR_H
#define BHS_CORE_SPACETIME_KERR_H

#include <math.h>
#include "math/tensor/tensor.h"

/* ============================================================================
 * PARÂMETROS
 * ============================================================================
 */

/**
 * struct bhs_kerr - Parâmetros do buraco negro de Kerr
 * @M: Massa do buraco negro (em unidades geométricas, G = c = 1)
 * @a: Parâmetro de spin: a = J / (Mc), onde J é momento angular
 *
 * Restrição física: |a| ≤ M
 * - a = 0: Schwarzschild (sem rotação)
 * - a = M: Kerr extremo (rotação máxima)
 * - |a| > M: Kerr super-extremo (singularidade nua, não é buraco negro)
 *
 * O Gargantua do Interestelar tem a/M ≈ 0.998 (quase extremo).
 */
struct bhs_kerr {
	double M; /* Massa */
	double a; /* Spin */
};

/* ============================================================================
 * FUNÇÕES AUXILIARES (Boyer-Lindquist)
 * ============================================================================
 */

/**
 * bhs_kerr_Sigma - Função Σ(r, θ)
 *
 * Σ = r² + a² cos²θ
 *
 * Aparece em toda parte na métrica de Kerr.
 */
static inline double bhs_kerr_Sigma(const struct bhs_kerr *bh, double r,
				    double theta)
{
	double cos_theta = cos(theta);
	return r * r + bh->a * bh->a * cos_theta * cos_theta;
}

/**
 * bhs_kerr_Delta - Função Δ(r)
 *
 * Δ = r² - 2Mr + a²
 *
 * Zeros de Δ: horizontes de eventos (externo e interno).
 */
static inline double bhs_kerr_Delta(const struct bhs_kerr *bh, double r)
{
	return r * r - 2.0 * bh->M * r + bh->a * bh->a;
}

/* ============================================================================
 * RAIOS CRÍTICOS
 * ============================================================================
 */

/**
 * bhs_kerr_horizon_outer - Horizonte de eventos externo r+
 *
 * r+ = M + √(M² - a²)
 *
 * Este é O horizonte de eventos. Nada escapa de r < r+.
 * Para a = 0: r+ = 2M (Schwarzschild)
 * Para a = M: r+ = M (Kerr extremo)
 */
double bhs_kerr_horizon_outer(const struct bhs_kerr *bh);

/**
 * bhs_kerr_horizon_inner - Horizonte de Cauchy interno r-
 *
 * r- = M - √(M² - a²)
 *
 * Horizonte interno. Instável classicamente.
 * Para a = 0: r- = 0
 * Para a = M: r- = M (coincide com r+)
 */
double bhs_kerr_horizon_inner(const struct bhs_kerr *bh);

/**
 * bhs_kerr_ergosphere - Raio da ergoesfera
 *
 * r_ergo(θ) = M + √(M² - a² cos²θ)
 *
 * Na ergoesfera (r+ < r < r_ergo), observadores estáticos são impossíveis.
 * Tudo é forçado a co-rotar com o buraco negro (frame dragging).
 *
 * Máxima extensão no equador (θ = π/2): r_ergo = 2M
 * Coincide com r+ nos polos (θ = 0, π)
 */
double bhs_kerr_ergosphere(const struct bhs_kerr *bh, double theta);

/**
 * bhs_kerr_isco - ISCO (Innermost Stable Circular Orbit)
 * @prograde: true para órbita prograde (co-rotação), false para retrógrada
 *
 * Para Kerr, o ISCO depende da direção da órbita:
 * - Prograde: r_isco decresce com spin (até r = M para a = M)
 * - Retrógrada: r_isco aumenta com spin (até r = 9M para a = M)
 *
 * Para a = 0: r_isco = 6M (mesmo que Schwarzschild)
 */
double bhs_kerr_isco(const struct bhs_kerr *bh, bool prograde);

/* ============================================================================
 * FRAME DRAGGING
 * ============================================================================
 */

/**
 * bhs_kerr_omega_frame - Velocidade angular do frame dragging
 *
 * ω = -g_tφ / g_φφ = 2Mar / [(r² + a²)² - a²Δ sin²θ]
 *
 * É a velocidade angular com que um observador ZAMO (Zero Angular Momentum
 * Observer) é arrastado pelo espaço-tempo rotativo.
 *
 * No horizonte: ω_H = a / (2Mr+) - velocidade angular do buraco negro.
 */
double bhs_kerr_omega_frame(const struct bhs_kerr *bh, double r, double theta);

/* ============================================================================
 * MÉTRICA
 * ============================================================================
 */

/**
 * bhs_kerr_metric - Calcula tensor métrico de Kerr
 * @bh: parâmetros do buraco negro
 * @r: coordenada radial (deve ser > r+ fora do horizonte)
 * @theta: ângulo polar [0, π]
 * @out: [out] tensor métrico g_μν
 *
 * Linha de mundo em Boyer-Lindquist:
 * ds² = -(1 - 2Mr/Σ)dt² - (4Mar sin²θ/Σ) dt dφ
 *       + (Σ/Δ)dr² + Σ dθ² + [(r²+a²)² - a²Δ sin²θ]/Σ sin²θ dφ²
 *
 * Componentes não-diagonais: g_tφ ≠ 0 (frame dragging!)
 */
void bhs_kerr_metric(const struct bhs_kerr *bh, double r, double theta,
		     struct bhs_metric *out);

/**
 * bhs_kerr_metric_inverse - Calcula métrica inversa g^μν
 */
void bhs_kerr_metric_inverse(const struct bhs_kerr *bh, double r, double theta,
			     struct bhs_metric *out);

/* ============================================================================
 * REDSHIFT E EFEITOS RELATIVÍSTICOS
 * ============================================================================
 */

/**
 * bhs_kerr_redshift_zamo - Redshift para observador ZAMO
 * @bh: parâmetros
 * @r: coordenada radial
 * @theta: ângulo polar
 *
 * O observador ZAMO é "estacionário" no espaço-tempo rotativo
 * (momento angular zero relativo ao infinito).
 */
double bhs_kerr_redshift_zamo(const struct bhs_kerr *bh, double r,
			      double theta);

/* ============================================================================
 * FUNÇÃO DE MÉTRICA (para Christoffel)
 * ============================================================================
 */

/**
 * bhs_kerr_metric_func - Wrapper para bhs_christoffel_compute
 *
 * Use como bhs_metric_func com userdata = struct bhs_kerr*
 *
 * Coordenadas em vec4: (t, r, θ, φ)
 */
void bhs_kerr_metric_func(struct bhs_vec4 coords, void *userdata,
			  struct bhs_metric *out);

#endif /* BHS_CORE_SPACETIME_KERR_H */
