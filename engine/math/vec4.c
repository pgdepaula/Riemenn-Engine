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

struct ri_vec4 ri_vec4_add(struct ri_vec4 a, struct ri_vec4 b)
{
	return (struct ri_vec4){
		.t = a.t + b.t,
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
	};
}

struct ri_vec4 ri_vec4_sub(struct ri_vec4 a, struct ri_vec4 b)
{
	return (struct ri_vec4){
		.t = a.t - b.t,
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}

struct ri_vec4 ri_vec4_scale(struct ri_vec4 v, real_t s)
{
	return (struct ri_vec4){
		.t = v.t * s,
		.x = v.x * s,
		.y = v.y * s,
		.z = v.z * s,
	};
}

struct ri_vec4 ri_vec4_neg(struct ri_vec4 v)
{
	return (struct ri_vec4){
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

real_t ri_vec4_dot_minkowski(struct ri_vec4 a, struct ri_vec4 b)
{
	/*
   * Métrica de Minkowski: η_μν = diag(-1, +1, +1, +1)
   *
   * η_μν a^μ b^ν = -a^0 b^0 + a^1 b^1 + a^2 b^2 + a^3 b^3
   *              = -t1*t2 + x1*x2 + y1*y2 + z1*z2
   */
	return -a.t * b.t + a.x * b.x + a.y * b.y + a.z * b.z;
}

real_t ri_vec4_norm2_minkowski(struct ri_vec4 v)
{
	return ri_vec4_dot_minkowski(v, v);
}

bool ri_vec4_is_null(struct ri_vec4 v, real_t epsilon)
{
	real_t norm2 = ri_vec4_norm2_minkowski(v);
	return ri_abs(norm2) < epsilon;
}

bool ri_vec4_is_timelike(struct ri_vec4 v)
{
	/* Timelike: ds² < 0 (nossa convenção mostly plus) */
	return ri_vec4_norm2_minkowski(v) < 0.0;
}

bool ri_vec4_is_spacelike(struct ri_vec4 v)
{
	/* Spacelike: ds² > 0 */
	return ri_vec4_norm2_minkowski(v) > 0.0;
}

/* ============================================================================
 * OPERAÇÕES ALGÉBRICAS - VEC3
 * ============================================================================
 */

struct ri_vec3 ri_vec3_add(struct ri_vec3 a, struct ri_vec3 b)
{
	return (struct ri_vec3){
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
	};
}

struct ri_vec3 ri_vec3_sub(struct ri_vec3 a, struct ri_vec3 b)
{
	return (struct ri_vec3){
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
	};
}

struct ri_vec3 ri_vec3_scale(struct ri_vec3 v, real_t s)
{
	return (struct ri_vec3){
		.x = v.x * s,
		.y = v.y * s,
		.z = v.z * s,
	};
}

real_t ri_vec3_dot(struct ri_vec3 a, struct ri_vec3 b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

struct ri_vec3 ri_vec3_cross(struct ri_vec3 a, struct ri_vec3 b)
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
	return (struct ri_vec3){
		.x = a.y * b.z - a.z * b.y,
		.y = a.z * b.x - a.x * b.z,
		.z = a.x * b.y - a.y * b.x,
	};
}

real_t ri_vec3_norm(struct ri_vec3 v)
{
	return ri_sqrt(ri_vec3_dot(v, v));
}

real_t ri_vec3_norm2(struct ri_vec3 v)
{
	return ri_vec3_dot(v, v);
}

struct ri_vec3 ri_vec3_normalize(struct ri_vec3 v)
{
	real_t n = ri_vec3_norm(v);

	/* Evita divisão por zero - retorna zero ao invés de explodir */
	if (n < 1e-15)
		return ri_vec3_zero();

	real_t inv_n = 1.0 / n;
	return ri_vec3_scale(v, inv_n);
}

/* ============================================================================
 * COORDENADAS ESFÉRICAS
 * ============================================================================
 */

void ri_vec3_to_spherical(struct ri_vec3 v, real_t *r, real_t *theta,
			   real_t *phi)
{
	/*
   * Conversão cartesianas → esféricas:
   * r     = √(x² + y² + z²)
   * θ     = arccos(z/r)        [0, π]
   * φ     = atan2(y, x)        [-π, π]
   */
	*r = ri_vec3_norm(v);

	if (*r < 1e-15) {
		/* Origem: θ e φ indefinidos, escolhemos zero */
		*theta = 0.0;
		*phi = 0.0;
		return;
	}

	*theta = ri_acos(v.z / *r);
	*phi = ri_atan2(v.y, v.x);
}

struct ri_vec3 ri_vec3_from_spherical(real_t r, real_t theta, real_t phi)
{
	/*
   * Conversão esféricas → cartesianas:
   * x = r * sin(θ) * cos(φ)
   * y = r * sin(θ) * sin(φ)
   * z = r * cos(θ)
   */
	real_t sin_theta = ri_sin(theta);

	return (struct ri_vec3){
		.x = r * sin_theta * ri_cos(phi),
		.y = r * sin_theta * ri_sin(phi),
		.z = r * ri_cos(theta),
	};
}
