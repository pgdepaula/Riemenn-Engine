/**
 * @file tensor.c
 * @brief Implementação de operações tensoriais
 *
 * "Tensores são objetos geométricos que existem independentemente
 * de coordenadas. Componentes são só números que dependem da sua
 * escolha de base. Não confunda o mapa com o território."
 */

#include "tensor.h"
#include <math.h>
#include <string.h>

/* ============================================================================
 * CONSTANTES
 * ============================================================================
 */

const struct bhs_metric BHS_MINKOWSKI = { .g = {
						  { -1.0, 0.0, 0.0, 0.0 },
						  { 0.0, 1.0, 0.0, 0.0 },
						  { 0.0, 0.0, 1.0, 0.0 },
						  { 0.0, 0.0, 0.0, 1.0 },
					  } };

/* ============================================================================
 * OPERAÇÕES COM MÉTRICA
 * ============================================================================
 */

struct bhs_metric bhs_metric_zero(void)
{
	struct bhs_metric m;
	memset(&m, 0, sizeof(m));
	return m;
}

struct bhs_metric bhs_metric_minkowski(void)
{
	return BHS_MINKOWSKI;
}

struct bhs_metric bhs_metric_diag(real_t g00, real_t g11, real_t g22,
				  real_t g33)
{
	struct bhs_metric m = bhs_metric_zero();
	m.g[0][0] = g00;
	m.g[1][1] = g11;
	m.g[2][2] = g22;
	m.g[3][3] = g33;
	return m;
}

bool bhs_metric_is_symmetric(const struct bhs_metric *m, real_t tol)
{
	for (int i = 0; i < 4; i++) {
		for (int j = i + 1; j < 4; j++) {
			if (bhs_abs(m->g[i][j] - m->g[j][i]) > tol)
				return false;
		}
	}
	return true;
}

real_t bhs_metric_det(const struct bhs_metric *m)
{
	/*
   * Determinante 4x4 por expansão de Laplace.
   * Isso é O(n!) mas n=4 então tanto faz.
   *
   * Para métrica diagonal: det = g00 * g11 * g22 * g33
   * Mas implementamos o caso geral.
   */
	const real_t(*g)[4] = m->g;

	/* Subdeterminantes 3x3 pela primeira linha */
	real_t det = 0.0;

	/* Cofator de g[0][0] */
	real_t c00 = g[1][1] * (g[2][2] * g[3][3] - g[2][3] * g[3][2]) -
		     g[1][2] * (g[2][1] * g[3][3] - g[2][3] * g[3][1]) +
		     g[1][3] * (g[2][1] * g[3][2] - g[2][2] * g[3][1]);

	/* Cofator de g[0][1] */
	real_t c01 = g[1][0] * (g[2][2] * g[3][3] - g[2][3] * g[3][2]) -
		     g[1][2] * (g[2][0] * g[3][3] - g[2][3] * g[3][0]) +
		     g[1][3] * (g[2][0] * g[3][2] - g[2][2] * g[3][0]);

	/* Cofator de g[0][2] */
	real_t c02 = g[1][0] * (g[2][1] * g[3][3] - g[2][3] * g[3][1]) -
		     g[1][1] * (g[2][0] * g[3][3] - g[2][3] * g[3][0]) +
		     g[1][3] * (g[2][0] * g[3][1] - g[2][1] * g[3][0]);

	/* Cofator de g[0][3] */
	real_t c03 = g[1][0] * (g[2][1] * g[3][2] - g[2][2] * g[3][1]) -
		     g[1][1] * (g[2][0] * g[3][2] - g[2][2] * g[3][0]) +
		     g[1][2] * (g[2][0] * g[3][1] - g[2][1] * g[3][0]);

	det = g[0][0] * c00 - g[0][1] * c01 + g[0][2] * c02 - g[0][3] * c03;

	return det;
}

int bhs_metric_invert(const struct bhs_metric *m, struct bhs_metric *inv)
{
	/*
   * Inversão 4x4 usando matriz adjunta:
   * A^(-1) = adj(A) / det(A)
   *
   * Para métricas diagonais é trivial, mas fazemos o caso geral.
   */
	real_t det = bhs_metric_det(m);

	if (bhs_abs(det) < 1e-15)
		return -1; /* Matriz singular */

	real_t inv_det = 1.0 / det;
	const real_t(*g)[4] = m->g;

	/*
   * Matriz de cofatores (transposta já incluída).
   * Cada C[i][j] é o cofator de g[j][i] (note a troca de índices).
   */

	/* Linha 0 */
	inv->g[0][0] =
		inv_det * (g[1][1] * (g[2][2] * g[3][3] - g[2][3] * g[3][2]) -
			   g[1][2] * (g[2][1] * g[3][3] - g[2][3] * g[3][1]) +
			   g[1][3] * (g[2][1] * g[3][2] - g[2][2] * g[3][1]));

	inv->g[0][1] =
		-inv_det * (g[0][1] * (g[2][2] * g[3][3] - g[2][3] * g[3][2]) -
			    g[0][2] * (g[2][1] * g[3][3] - g[2][3] * g[3][1]) +
			    g[0][3] * (g[2][1] * g[3][2] - g[2][2] * g[3][1]));

	inv->g[0][2] =
		inv_det * (g[0][1] * (g[1][2] * g[3][3] - g[1][3] * g[3][2]) -
			   g[0][2] * (g[1][1] * g[3][3] - g[1][3] * g[3][1]) +
			   g[0][3] * (g[1][1] * g[3][2] - g[1][2] * g[3][1]));

	inv->g[0][3] =
		-inv_det * (g[0][1] * (g[1][2] * g[2][3] - g[1][3] * g[2][2]) -
			    g[0][2] * (g[1][1] * g[2][3] - g[1][3] * g[2][1]) +
			    g[0][3] * (g[1][1] * g[2][2] - g[1][2] * g[2][1]));

	/* Linha 1 */
	inv->g[1][0] =
		-inv_det * (g[1][0] * (g[2][2] * g[3][3] - g[2][3] * g[3][2]) -
			    g[1][2] * (g[2][0] * g[3][3] - g[2][3] * g[3][0]) +
			    g[1][3] * (g[2][0] * g[3][2] - g[2][2] * g[3][0]));

	inv->g[1][1] =
		inv_det * (g[0][0] * (g[2][2] * g[3][3] - g[2][3] * g[3][2]) -
			   g[0][2] * (g[2][0] * g[3][3] - g[2][3] * g[3][0]) +
			   g[0][3] * (g[2][0] * g[3][2] - g[2][2] * g[3][0]));

	inv->g[1][2] =
		-inv_det * (g[0][0] * (g[1][2] * g[3][3] - g[1][3] * g[3][2]) -
			    g[0][2] * (g[1][0] * g[3][3] - g[1][3] * g[3][0]) +
			    g[0][3] * (g[1][0] * g[3][2] - g[1][2] * g[3][0]));

	inv->g[1][3] =
		inv_det * (g[0][0] * (g[1][2] * g[2][3] - g[1][3] * g[2][2]) -
			   g[0][2] * (g[1][0] * g[2][3] - g[1][3] * g[2][0]) +
			   g[0][3] * (g[1][0] * g[2][2] - g[1][2] * g[2][0]));

	/* Linha 2 */
	inv->g[2][0] =
		inv_det * (g[1][0] * (g[2][1] * g[3][3] - g[2][3] * g[3][1]) -
			   g[1][1] * (g[2][0] * g[3][3] - g[2][3] * g[3][0]) +
			   g[1][3] * (g[2][0] * g[3][1] - g[2][1] * g[3][0]));

	inv->g[2][1] =
		-inv_det * (g[0][0] * (g[2][1] * g[3][3] - g[2][3] * g[3][1]) -
			    g[0][1] * (g[2][0] * g[3][3] - g[2][3] * g[3][0]) +
			    g[0][3] * (g[2][0] * g[3][1] - g[2][1] * g[3][0]));

	inv->g[2][2] =
		inv_det * (g[0][0] * (g[1][1] * g[3][3] - g[1][3] * g[3][1]) -
			   g[0][1] * (g[1][0] * g[3][3] - g[1][3] * g[3][0]) +
			   g[0][3] * (g[1][0] * g[3][1] - g[1][1] * g[3][0]));

	inv->g[2][3] =
		-inv_det * (g[0][0] * (g[1][1] * g[2][3] - g[1][3] * g[2][1]) -
			    g[0][1] * (g[1][0] * g[2][3] - g[1][3] * g[2][0]) +
			    g[0][3] * (g[1][0] * g[2][1] - g[1][1] * g[2][0]));

	/* Linha 3 */
	inv->g[3][0] =
		-inv_det * (g[1][0] * (g[2][1] * g[3][2] - g[2][2] * g[3][1]) -
			    g[1][1] * (g[2][0] * g[3][2] - g[2][2] * g[3][0]) +
			    g[1][2] * (g[2][0] * g[3][1] - g[2][1] * g[3][0]));

	inv->g[3][1] =
		inv_det * (g[0][0] * (g[2][1] * g[3][2] - g[2][2] * g[3][1]) -
			   g[0][1] * (g[2][0] * g[3][2] - g[2][2] * g[3][0]) +
			   g[0][2] * (g[2][0] * g[3][1] - g[2][1] * g[3][0]));

	inv->g[3][2] =
		-inv_det * (g[0][0] * (g[1][1] * g[3][2] - g[1][2] * g[3][1]) -
			    g[0][1] * (g[1][0] * g[3][2] - g[1][2] * g[3][0]) +
			    g[0][2] * (g[1][0] * g[3][1] - g[1][1] * g[3][0]));

	inv->g[3][3] =
		inv_det * (g[0][0] * (g[1][1] * g[2][2] - g[1][2] * g[2][1]) -
			   g[0][1] * (g[1][0] * g[2][2] - g[1][2] * g[2][0]) +
			   g[0][2] * (g[1][0] * g[2][1] - g[1][1] * g[2][0]));

	return 0;
}

/* ============================================================================
 * OPERAÇÕES COM VETORES
 * ============================================================================
 */

struct bhs_vec4 bhs_metric_lower(const struct bhs_metric *m, struct bhs_vec4 v)
{
	/* v_μ = g_μν v^ν */
	double components[4] = { v.t, v.x, v.y, v.z };
	real_t result[4] = { 0 };

	for (int mu = 0; mu < 4; mu++) {
		for (int nu = 0; nu < 4; nu++) {
			result[mu] += m->g[mu][nu] * components[nu];
		}
	}

	return bhs_vec4_make(result[0], result[1], result[2], result[3]);
}

struct bhs_vec4 bhs_metric_raise(const struct bhs_metric *m_inv,
				 struct bhs_vec4 v)
{
	/* v^μ = g^μν v_ν (mesmo código, só muda interpretação) */
	return bhs_metric_lower(m_inv, v);
}

real_t bhs_metric_dot(const struct bhs_metric *m, struct bhs_vec4 a,
		      struct bhs_vec4 b)
{
	/* g_μν a^μ b^ν */
	double a_comp[4] = { a.t, a.x, a.y, a.z };
	double b_comp[4] = { b.t, b.x, b.y, b.z };

	real_t result = 0.0;
	for (int mu = 0; mu < 4; mu++) {
		for (int nu = 0; nu < 4; nu++) {
			result += m->g[mu][nu] * a_comp[mu] * b_comp[nu];
		}
	}

	return result;
}

/* ============================================================================
 * SÍMBOLOS DE CHRISTOFFEL
 * ============================================================================
 */

struct bhs_christoffel bhs_christoffel_zero(void)
{
	struct bhs_christoffel c;
	memset(&c, 0, sizeof(c));
	return c;
}

int bhs_christoffel_compute(bhs_metric_func metric_fn, struct bhs_vec4 coords,
			    void *userdata, real_t h,
			    struct bhs_christoffel *out)
{
	/*
   * Γ^α_μν = (1/2) g^αβ (∂_μ g_βν + ∂_ν g_βμ - ∂_β g_μν)
   *
   * Estratégia:
   * 1. Calcular métrica no ponto central
   * 2. Calcular derivadas por diferença central
   * 3. Inverter métrica
   * 4. Contrair com inversa
   */

	double coords_arr[4] = { coords.t, coords.x, coords.y, coords.z };

	/* 1. Métrica no ponto */
	struct bhs_metric g_center;
	metric_fn(coords, userdata, &g_center);

	/* 2. Inversa da métrica */
	struct bhs_metric g_inv;
	if (bhs_metric_invert(&g_center, &g_inv) != 0)
		return -1;

	/* 3. Derivadas parciais: dg[sigma][mu][nu] = ∂_sigma g_munu */
	real_t dg[4][4][4]; /* dg[sigma][mu][nu] */

	for (int sigma = 0; sigma < 4; sigma++) {
		struct bhs_vec4 coords_plus, coords_minus;
		real_t c_plus[4], c_minus[4];

		/* Copia coordenadas e perturba */
		for (int i = 0; i < 4; i++) {
			c_plus[i] = coords_arr[i];
			c_minus[i] = coords_arr[i];
		}
		c_plus[sigma] += h;
		c_minus[sigma] -= h;

		coords_plus = bhs_vec4_make(c_plus[0], c_plus[1], c_plus[2],
					    c_plus[3]);
		coords_minus = bhs_vec4_make(c_minus[0], c_minus[1], c_minus[2],
					     c_minus[3]);

		struct bhs_metric g_plus, g_minus;
		metric_fn(coords_plus, userdata, &g_plus);
		metric_fn(coords_minus, userdata, &g_minus);

		/* Diferença central */
		real_t inv_2h = 1.0 / (2.0 * h);
		for (int mu = 0; mu < 4; mu++) {
			for (int nu = 0; nu < 4; nu++) {
				dg[sigma][mu][nu] =
					(g_plus.g[mu][nu] - g_minus.g[mu][nu]) *
					inv_2h;
			}
		}
	}

	/* 4. Calcula Γ^α_μν */
	*out = bhs_christoffel_zero();

	for (int alpha = 0; alpha < 4; alpha++) {
		for (int mu = 0; mu < 4; mu++) {
			for (int nu = mu; nu < 4;
			     nu++) { /* Só metade, depois simetriza */
				real_t sum = 0.0;

				for (int beta = 0; beta < 4; beta++) {
					/* (1/2) g^αβ (∂_μ g_βν + ∂_ν g_βμ - ∂_β g_μν) */
					real_t term = dg[mu][beta][nu] +
						      dg[nu][beta][mu] -
						      dg[beta][mu][nu];
					sum += g_inv.g[alpha][beta] * term;
				}

				out->gamma[alpha][mu][nu] = 0.5 * sum;
				out->gamma[alpha][nu][mu] =
					out->gamma[alpha][mu]
						  [nu]; /* Simetria */
			}
		}
	}

	return 0;
}

struct bhs_vec4 bhs_geodesic_accel(const struct bhs_christoffel *chris,
				   struct bhs_vec4 vel)
{
	/*
   * Equação de geodésica:
   * d²x^α/dλ² = -Γ^α_μν (dx^μ/dλ)(dx^ν/dλ)
   *
   * Ou seja: a^α = -Γ^α_μν u^μ u^ν
   */
	double u[4] = { vel.t, vel.x, vel.y, vel.z };
	real_t a[4] = { 0 };

	for (int alpha = 0; alpha < 4; alpha++) {
		real_t sum = 0.0;
		for (int mu = 0; mu < 4; mu++) {
			for (int nu = 0; nu < 4; nu++) {
				sum += chris->gamma[alpha][mu][nu] * u[mu] *
				       u[nu];
			}
		}
		a[alpha] = -sum;
	}

	return bhs_vec4_make(a[0], a[1], a[2], a[3]);
}
