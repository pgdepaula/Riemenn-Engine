/**
 * @file vec4.c
 * @brief Implementação de operações de vetores 4D
 *
 * "Matemática é a linguagem em que Deus escreveu o universo."
 * — Galileu (e eu concordo, principalmente na parte do espaço-tempo)
 */

#include "vec4.h"
#include <math.h>

/* ============================================================================
 * OPERAÇÕES ALGÉBRICAS - VEC4
 * ============================================================================
 */

struct bhs_vec4 bhs_vec4_add(struct bhs_vec4 a, struct bhs_vec4 b)
{
	return (struct bhs_vec4){
		.t = a.t + b.t,
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
	};
}

struct bhs_vec4 bhs_vec4_sub(struct bhs_vec4 a, struct bhs_vec4 b)
{
	return (struct bhs_vec4){
		.t = a.t - b.t,
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}

struct bhs_vec4 bhs_vec4_scale(struct bhs_vec4 v, real_t s)
{
	return (struct bhs_vec4){
		.t = v.t * s,
		.x = v.x * s,
		.y = v.y * s,
		.z = v.z * s,
	};
}

struct bhs_vec4 bhs_vec4_neg(struct bhs_vec4 v)
{
	return (struct bhs_vec4){
		.t = -v.t,
		.x = -v.x,
		.y = -v.y,
		.z = -v.z,
	};
}

/* ============================================================================
 * PRODUTOS INTERNOS
 * ============================================================================
 */

real_t bhs_vec4_dot_minkowski(struct bhs_vec4 a, struct bhs_vec4 b)
{
	/*
   * Métrica de Minkowski: η_μν = diag(-1, +1, +1, +1)
   *
   * η_μν a^μ b^ν = -a^0 b^0 + a^1 b^1 + a^2 b^2 + a^3 b^3
   *              = -t1*t2 + x1*x2 + y1*y2 + z1*z2
   */
	return -a.t * b.t + a.x * b.x + a.y * b.y + a.z * b.z;
}

real_t bhs_vec4_norm2_minkowski(struct bhs_vec4 v)
{
	return bhs_vec4_dot_minkowski(v, v);
}

bool bhs_vec4_is_null(struct bhs_vec4 v, real_t epsilon)
{
	real_t norm2 = bhs_vec4_norm2_minkowski(v);
	return bhs_abs(norm2) < epsilon;
}

bool bhs_vec4_is_timelike(struct bhs_vec4 v)
{
	/* Timelike: ds² < 0 (nossa convenção mostly plus) */
	return bhs_vec4_norm2_minkowski(v) < 0.0;
}

bool bhs_vec4_is_spacelike(struct bhs_vec4 v)
{
	/* Spacelike: ds² > 0 */
	return bhs_vec4_norm2_minkowski(v) > 0.0;
}

/* ============================================================================
 * OPERAÇÕES ALGÉBRICAS - VEC3
 * ============================================================================
 */

struct bhs_vec3 bhs_vec3_add(struct bhs_vec3 a, struct bhs_vec3 b)
{
	return (struct bhs_vec3){
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
	};
}

struct bhs_vec3 bhs_vec3_sub(struct bhs_vec3 a, struct bhs_vec3 b)
{
	return (struct bhs_vec3){
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}

struct bhs_vec3 bhs_vec3_scale(struct bhs_vec3 v, real_t s)
{
	return (struct bhs_vec3){
		.x = v.x * s,
		.y = v.y * s,
		.z = v.z * s,
	};
}

real_t bhs_vec3_dot(struct bhs_vec3 a, struct bhs_vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

struct bhs_vec3 bhs_vec3_cross(struct bhs_vec3 a, struct bhs_vec3 b)
{
	/*
   * Produto vetorial:
   * (a × b)_i = ε_ijk a_j b_k
   *
   * Expandido:
   * x = a_y * b_z - a_z * b_y
   * y = a_z * b_x - a_x * b_z
   * z = a_x * b_y - a_y * b_x
   */
	return (struct bhs_vec3){
		.x = a.y * b.z - a.z * b.y,
		.y = a.z * b.x - a.x * b.z,
		.z = a.x * b.y - a.y * b.x,
	};
}

real_t bhs_vec3_norm(struct bhs_vec3 v)
{
	return bhs_sqrt(bhs_vec3_dot(v, v));
}

real_t bhs_vec3_norm2(struct bhs_vec3 v)
{
	return bhs_vec3_dot(v, v);
}

struct bhs_vec3 bhs_vec3_normalize(struct bhs_vec3 v)
{
	real_t n = bhs_vec3_norm(v);

	/* Evita divisão por zero - retorna zero ao invés de explodir */
	if (n < 1e-15)
		return bhs_vec3_zero();

	real_t inv_n = 1.0 / n;
	return bhs_vec3_scale(v, inv_n);
}

/* ============================================================================
 * COORDENADAS ESFÉRICAS
 * ============================================================================
 */

void bhs_vec3_to_spherical(struct bhs_vec3 v, real_t *r, real_t *theta,
			   real_t *phi)
{
	/*
   * Conversão cartesianas → esféricas:
   * r     = √(x² + y² + z²)
   * θ     = arccos(z/r)        [0, π]
   * φ     = atan2(y, x)        [-π, π]
   */
	*r = bhs_vec3_norm(v);

	if (*r < 1e-15) {
		/* Origem: θ e φ indefinidos, escolhemos zero */
		*theta = 0.0;
		*phi = 0.0;
		return;
	}

	*theta = bhs_acos(v.z / *r);
	*phi = bhs_atan2(v.y, v.x);
}

struct bhs_vec3 bhs_vec3_from_spherical(real_t r, real_t theta, real_t phi)
{
	/*
   * Conversão esféricas → cartesianas:
   * x = r * sin(θ) * cos(φ)
   * y = r * sin(θ) * sin(φ)
   * z = r * cos(θ)
   */
	real_t sin_theta = bhs_sin(theta);

	return (struct bhs_vec3){
		.x = r * sin_theta * bhs_cos(phi),
		.y = r * sin_theta * bhs_sin(phi),
		.z = r * bhs_cos(theta),
	};
}
