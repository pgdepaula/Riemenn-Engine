/**
 * @file kerr.c
 * @brief Implementação da métrica de Kerr
 *
 * "Roy Kerr derivou essa solução em 1963. Demorou 47 anos
 * desde Schwarzschild para alguém resolver o caso com rotação.
 * Relatividade geral não é pra qualquer um."
 *
 * Referência principal:
 * - Bardeen, Press & Teukolsky (1972) - Rotating Black Holes
 * - Chandrasekhar - Mathematical Theory of Black Holes
 */

#include "kerr.h"
#include <math.h>

/* ============================================================================
 * RAIOS CRÍTICOS
 * ============================================================================
 */

double bhs_kerr_horizon_outer(const struct bhs_kerr *bh)
{
	/*
   * r+ = M + √(M² - a²)
   *
   * Raiz maior de Δ = 0.
   */
	double M = bh->M;
	double a = bh->a;
	double disc = M * M - a * a;

	if (disc < 0.0)
		return 0.0; /* Kerr super-extremo (não é buraco negro) */

	return M + sqrt(disc);
}

double bhs_kerr_horizon_inner(const struct bhs_kerr *bh)
{
	/*
   * r- = M - √(M² - a²)
   */
	double M = bh->M;
	double a = bh->a;
	double disc = M * M - a * a;

	if (disc < 0.0)
		return 0.0;

	return M - sqrt(disc);
}

double bhs_kerr_ergosphere(const struct bhs_kerr *bh, double theta)
{
	/*
   * r_ergo(θ) = M + √(M² - a² cos²θ)
   *
   * Onde g_tt = 0 (superfície de limite estacionário).
   */
	double M = bh->M;
	double a = bh->a;
	double cos_theta = cos(theta);
	double disc = M * M - a * a * cos_theta * cos_theta;

	if (disc < 0.0)
		return 0.0; /* Nunca acontece para |a| ≤ M */

	return M + sqrt(disc);
}

double bhs_kerr_isco(const struct bhs_kerr *bh, bool prograde)
{
	/*
   * ISCO para Kerr (Bardeen, Press & Teukolsky 1972):
   *
   * r_isco = M { 3 + Z₂ ∓ √[(3 - Z₁)(3 + Z₁ + 2Z₂)] }
   *
   * onde:
   * Z₁ = 1 + (1 - a²/M²)^(1/3) [(1 + a/M)^(1/3) + (1 - a/M)^(1/3)]
   * Z₂ = √(3a²/M² + Z₁²)
   *
   * O sinal ∓ é - para prograde, + para retrógrado.
   */
	double M = bh->M;
	double a = bh->a;
	double chi = a / M; /* Spin adimensional */

	/* Caso especial: Schwarzschild (evita divisões por zero) */
	if (fabs(chi) < 1e-10)
		return 6.0 * M;

	double chi2 = chi * chi;

	/* Z₁ */
	double cbrt_factor = cbrt(1.0 - chi2);
	double cbrt_plus = cbrt(1.0 + chi);
	double cbrt_minus = cbrt(1.0 - chi);
	double Z1 = 1.0 + cbrt_factor * (cbrt_plus + cbrt_minus);

	/* Z₂ */
	double Z2 = sqrt(3.0 * chi2 + Z1 * Z1);

	/* ISCO */
	double inner = (3.0 - Z1) * (3.0 + Z1 + 2.0 * Z2);

	if (inner < 0.0)
		inner = 0.0; /* Proteção numérica */

	double sqrt_inner = sqrt(inner);

	if (prograde)
		return M * (3.0 + Z2 - sqrt_inner);
	else
		return M * (3.0 + Z2 + sqrt_inner);
}

/* ============================================================================
 * FRAME DRAGGING
 * ============================================================================
 */

double bhs_kerr_omega_frame(const struct bhs_kerr *bh, double r, double theta)
{
	/*
   * ω = -g_tφ / g_φφ
   *
   * Usando as expressões explícitas:
   * g_tφ = -2Mar sin²θ / Σ
   * g_φφ = [(r² + a²)² - a²Δ sin²θ] sin²θ / Σ
   *
   * ω = 2Mar / [(r² + a²)² - a²Δ sin²θ]
   */
	double M = bh->M;
	double a = bh->a;
	double sin_theta = sin(theta);
	double sin2 = sin_theta * sin_theta;

	double r2 = r * r;
	double a2 = a * a;
	double sum = r2 + a2;
	double Delta = bhs_kerr_Delta(bh, r);

	double denom = sum * sum - a2 * Delta * sin2;

	if (fabs(denom) < 1e-15)
		return 0.0; /* Evita divisão por zero */

	return 2.0 * M * a * r / denom;
}

/* ============================================================================
 * MÉTRICA
 * ============================================================================
 */

void bhs_kerr_metric(const struct bhs_kerr *bh, double r, double theta,
		     struct bhs_metric *out)
{
	/*
   * Métrica de Kerr em Boyer-Lindquist:
   *
   * g_tt = -(1 - 2Mr/Σ)
   * g_tφ = g_φt = -2Mar sin²θ / Σ
   * g_rr = Σ / Δ
   * g_θθ = Σ
   * g_φφ = [(r² + a²)² - a²Δ sin²θ] sin²θ / Σ
   *
   * Onde:
   * Σ = r² + a² cos²θ
   * Δ = r² - 2Mr + a²
   */

	double M = bh->M;
	double a = bh->a;

	double sin_theta = sin(theta);
	double cos_theta = cos(theta);
	double sin2 = sin_theta * sin_theta;
	double cos2 = cos_theta * cos_theta;

	double r2 = r * r;
	double a2 = a * a;

	double Sigma = r2 + a2 * cos2;
	double Delta = r2 - 2.0 * M * r + a2;

	double sum = r2 + a2;
	double A =
		sum * sum -
		a2 * Delta * sin2; /* Chamado às vezes de (r²+a²)² - a²Δsin²θ */

	*out = bhs_metric_zero();

	/* g_tt */
	out->g[0][0] = -(1.0 - 2.0 * M * r / Sigma);

	/* g_tφ = g_φt (frame dragging!) */
	out->g[0][3] = -2.0 * M * a * r * sin2 / Sigma;
	out->g[3][0] = out->g[0][3]; /* Simetria */

	/* g_rr */
	out->g[1][1] = Sigma / Delta;

	/* g_θθ */
	out->g[2][2] = Sigma;

	/* g_φφ */
	out->g[3][3] = A * sin2 / Sigma;
}

void bhs_kerr_metric_inverse(const struct bhs_kerr *bh, double r, double theta,
			     struct bhs_metric *out)
{
	/*
   * Inversa da métrica de Kerr.
   *
   * Para bloco 2x2 (t, φ), inversão explícita:
   * [g_tt   g_tφ]^(-1)        1         [g_φφ   -g_tφ]
   * [g_φt   g_φφ]     =  ----------- *  [-g_φt   g_tt]
   *                      det(bloco)
   *
   * det = g_tt * g_φφ - g_tφ²
   */

	double M = bh->M;
	double a = bh->a;

	double sin_theta = sin(theta);
	double cos_theta = cos(theta);
	double sin2 = sin_theta * sin_theta;
	double cos2 = cos_theta * cos_theta;

	double r2 = r * r;
	double a2 = a * a;

	double Sigma = r2 + a2 * cos2;
	double Delta = r2 - 2.0 * M * r + a2;

	double sum = r2 + a2;
	double A = sum * sum - a2 * Delta * sin2;

	/* Componentes covariantes (precisamos deles) */
	double g_tt = -(1.0 - 2.0 * M * r / Sigma);
	double g_tphi = -2.0 * M * a * r * sin2 / Sigma;
	double g_phiphi = A * sin2 / Sigma;

	/* Determinante do bloco (t, φ) */
	double det_block = g_tt * g_phiphi - g_tphi * g_tphi;

	*out = bhs_metric_zero();

	if (fabs(det_block) > 1e-15) {
		double inv_det = 1.0 / det_block;
		out->g[0][0] = g_phiphi * inv_det; /* g^tt */
		out->g[0][3] = -g_tphi * inv_det;  /* g^tφ */
		out->g[3][0] = out->g[0][3];	   /* g^φt */
		out->g[3][3] = g_tt * inv_det;	   /* g^φφ */
	}

	/* Inversa dos elementos diagonais simples */
	out->g[1][1] = Delta / Sigma; /* g^rr */
	out->g[2][2] = 1.0 / Sigma;   /* g^θθ */
}

/* ============================================================================
 * REDSHIFT
 * ============================================================================
 */

double bhs_kerr_redshift_zamo(const struct bhs_kerr *bh, double r, double theta)
{
	/*
   * Redshift para observador ZAMO (Zero Angular Momentum Observer).
   *
   * O fator de redshift é dado pelo componente temporal da 4-velocidade
   * do ZAMO: u^t = 1 / √(-g_tt + ω² g_φφ)
   *
   * Ou usando a métrica inversa: u^t = √(-g^tt)
   *
   * z = 1/u^t - 1 = √(-g_tt_eff) - 1
   */

	struct bhs_metric g_inv;
	bhs_kerr_metric_inverse(bh, r, theta, &g_inv);

	double g_tt_inv = g_inv.g[0][0]; /* g^tt */

	if (g_tt_inv >= 0.0)
		return 1e30; /* Dentro da ergoesfera, g^tt ≥ 0 */

	double u_t = sqrt(-g_tt_inv);
	return 1.0 / u_t - 1.0;
}

/* ============================================================================
 * FUNÇÃO DE MÉTRICA
 * ============================================================================
 */

void bhs_kerr_metric_func(struct bhs_vec4 coords, void *userdata,
			  struct bhs_metric *out)
{
	const struct bhs_kerr *bh = userdata;
	double r = coords.x;	 /* r está em x (índice 1) */
	double theta = coords.y; /* θ está em y (índice 2) */

	bhs_kerr_metric(bh, r, theta, out);
}
