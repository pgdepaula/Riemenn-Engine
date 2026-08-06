/**
 * @file schwarzschild.c
 * @brief Implementação da métrica de Schwarzschild
 *
 * "Em 1916, Karl Schwarzschild derivou essa solução
 * enquanto servia no front russo da Primeira Guerra.
 * Morreu meses depois. A física perdeu um gigante."
 */

#include "schwarzschild.h"
#include <math.h>

void ri_schwarzschild_metric(const struct ri_schwarzschild *bh, double r,
			      double theta, struct ri_metric *out)
{
	/*
   * Métrica de Schwarzschild em coordenadas (t, r, θ, φ):
   *
   * ds² = -(1 - rs/r) dt² + (1 - rs/r)^(-1) dr² + r² dθ² + r² sin²θ dφ²
   *
   * Zeros fora da diagonal (métrica diagonal).
   */

	double rs = ri_schwarzschild_rs(bh);
	double f = 1.0 - rs / r; /* Fator de Schwarzschild (1 - 2M/r) */
	double sin_theta = sin(theta);
	double r2 = r * r;

	*out = ri_metric_zero();

	out->g[0][0] = -f;			   /* g_tt */
	out->g[1][1] = 1.0 / f;			   /* g_rr */
	out->g[2][2] = r2;			   /* g_θθ */
	out->g[3][3] = r2 * sin_theta * sin_theta; /* g_φφ */
}

void ri_schwarzschild_metric_inverse(const struct ri_schwarzschild *bh,
				      double r, double theta,
				      struct ri_metric *out)
{
	/*
   * Para métrica diagonal, inversa é trivial:
   * g^μμ = 1/g_μμ
   */

	double rs = ri_schwarzschild_rs(bh);
	double f = 1.0 - rs / r;
	double sin_theta = sin(theta);
	double r2 = r * r;

	*out = ri_metric_zero();

	out->g[0][0] = -1.0 / f;			   /* g^tt */
	out->g[1][1] = f;				   /* g^rr */
	out->g[2][2] = 1.0 / r2;			   /* g^θθ */
	out->g[3][3] = 1.0 / (r2 * sin_theta * sin_theta); /* g^φφ */
}

double ri_schwarzschild_redshift(const struct ri_schwarzschild *bh, double r)
{
	/*
   * Redshift gravitacional:
   *
   * ν_∞ / ν_emit = √(g_tt(r)) / √(g_tt(∞))
   *              = √(1 - rs/r)
   *
   * z = λ_obs/λ_emit - 1 = 1/√(1 - rs/r) - 1
   */

	double rs = ri_schwarzschild_rs(bh);
	double f = 1.0 - rs / r;

	if (f <= 0.0)
		return 1e30; /* "Infinito" (dentro do horizonte) */

	return 1.0 / sqrt(f) - 1.0;
}

double ri_schwarzschild_escape_velocity(const struct ri_schwarzschild *bh,
					 double r)
{
	/*
   * Velocidade de escape (fração de c):
   * v_esc = √(2GM/r) = √(rs/r)
   *
   * Em c = 1: retorna valor entre 0 e 1.
   * No horizonte (r = rs): v_esc = 1 (velocidade da luz)
   */

	double rs = ri_schwarzschild_rs(bh);
	return sqrt(rs / r);
}

void ri_schwarzschild_metric_func(struct ri_vec4 coords, void *userdata,
				   struct ri_metric *out)
{
	/*
   * Wrapper para uso com ri_christoffel_compute.
   * coords = (t, r, θ, φ)
   */

	const struct ri_schwarzschild *bh = userdata;
	double r = coords.x;	 /* r está em x (índice 1) */
	double theta = coords.y; /* θ está em y (índice 2) */

	ri_schwarzschild_metric(bh, r, theta, out);
}
