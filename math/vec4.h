/**
 * @file vec4.h
 * @brief Vetor 4D para cálculos em espaço-tempo
 *
 * "No espaço-tempo, tempo é só mais uma dimensão.
 * Só que essa dimensão te mata se você errar o sinal."
 *
 * Convenção de sinatura métrica: (-,+,+,+) aka "mostly plus"
 * Coordenadas: (t, x, y, z) ou (t, r, θ, φ) dependendo do contexto.
 */

#ifndef BHS_CORE_MATH_VEC4_H
#define BHS_CORE_MATH_VEC4_H

#include <stdbool.h>
#include <stddef.h>

#include "math/bhs_math.h"

/* ============================================================================
 * ESTRUTURAS
 * ============================================================================
 */

/**
 * struct bhs_vec4 - Vetor 4D em espaço-tempo
 * @t: componente temporal (x^0)
 * @x: componente espacial x (x^1)
 * @y: componente espacial y (x^2)
 * @z: componente espacial z (x^3)
 *
 * Pode representar:
 * - Posição 4D (evento no espaço-tempo)
 * - 4-velocidade (u^μ = dx^μ/dτ)
 * - 4-momento (p^μ = m*u^μ)
 * - Qualquer 4-vetor contravariante
 */
struct bhs_vec4 {
	real_t t;
	real_t x;
	real_t y;
	real_t z;
};

/**
 * struct bhs_vec3 - Vetor 3D espacial (pra facilitar)
 *
 * Usado quando só a parte espacial importa.
 */
struct bhs_vec3 {
	real_t x;
	real_t y;
	real_t z;
};

/* ============================================================================
 * CONSTRUTORES
 * ============================================================================
 */

/**
 * bhs_vec4_make - Cria vetor 4D
 */
static inline struct bhs_vec4 bhs_vec4_make(real_t t, real_t x, real_t y,
					    real_t z)
{
	return (struct bhs_vec4){ .t = t, .x = x, .y = y, .z = z };
}

/**
 * bhs_vec4_zero - Vetor nulo
 */
static inline struct bhs_vec4 bhs_vec4_zero(void)
{
	return (struct bhs_vec4){ 0 };
}

/**
 * bhs_vec3_make - Cria vetor 3D
 */
static inline struct bhs_vec3 bhs_vec3_make(real_t x, real_t y, real_t z)
{
	return (struct bhs_vec3){ .x = x, .y = y, .z = z };
}

/**
 * bhs_vec3_zero - Vetor 3D nulo
 */
static inline struct bhs_vec3 bhs_vec3_zero(void)
{
	return (struct bhs_vec3){ 0 };
}

/* ============================================================================
 * OPERAÇÕES ALGÉBRICAS - VEC4
 * ============================================================================
 */

/**
 * bhs_vec4_add - Soma de vetores
 */
struct bhs_vec4 bhs_vec4_add(struct bhs_vec4 a, struct bhs_vec4 b);

/**
 * bhs_vec4_sub - Subtração de vetores
 */
struct bhs_vec4 bhs_vec4_sub(struct bhs_vec4 a, struct bhs_vec4 b);

/**
 * bhs_vec4_scale - Multiplicação por escalar
 */
struct bhs_vec4 bhs_vec4_scale(struct bhs_vec4 v, real_t s);

/**
 * bhs_vec4_neg - Negação (inverte sinal)
 */
struct bhs_vec4 bhs_vec4_neg(struct bhs_vec4 v);

/* ============================================================================
 * PRODUTOS INTERNOS
 * ============================================================================
 */

/**
 * bhs_vec4_dot_minkowski - Produto interno com métrica de Minkowski
 *
 * η_μν = diag(-1, +1, +1, +1)
 *
 * Retorna: -t1*t2 + x1*x2 + y1*y2 + z1*z2
 *
 * Para 4-velocidade u^μ de partícula com massa:
 *   η_μν u^μ u^ν = -1 (normalização)
 *
 * Para 4-vetor nulo (fótons):
 *   η_μν k^μ k^ν = 0
 */
real_t bhs_vec4_dot_minkowski(struct bhs_vec4 a, struct bhs_vec4 b);

/**
 * bhs_vec4_norm2_minkowski - Norma ao quadrado (Minkowski)
 *
 * Retorna: η_μν v^μ v^ν = -t² + x² + y² + z²
 *
 * Pode ser:
 *   < 0 : timelike (interval de tempo)
 *   = 0 : null/lightlike (fótons)
 *   > 0 : spacelike (intervalo espacial)
 */
real_t bhs_vec4_norm2_minkowski(struct bhs_vec4 v);

/**
 * bhs_vec4_is_null - Verifica se é vetor nulo (fóton)
 *
 * Tolerância: |norm²| < epsilon
 */
bool bhs_vec4_is_null(struct bhs_vec4 v, real_t epsilon);

/**
 * bhs_vec4_is_timelike - Verifica se é timelike
 */
bool bhs_vec4_is_timelike(struct bhs_vec4 v);

/**
 * bhs_vec4_is_spacelike - Verifica se é spacelike
 */
bool bhs_vec4_is_spacelike(struct bhs_vec4 v);

/* ============================================================================
 * OPERAÇÕES ALGÉBRICAS - VEC3
 * ============================================================================
 */

/**
 * bhs_vec3_add - Soma de vetores 3D
 */
struct bhs_vec3 bhs_vec3_add(struct bhs_vec3 a, struct bhs_vec3 b);

/**
 * bhs_vec3_sub - Subtração de vetores 3D
 */
struct bhs_vec3 bhs_vec3_sub(struct bhs_vec3 a, struct bhs_vec3 b);

/**
 * bhs_vec3_scale - Multiplicação por escalar
 */
struct bhs_vec3 bhs_vec3_scale(struct bhs_vec3 v, real_t s);

/**
 * bhs_vec3_dot - Produto escalar euclidiano
 */
real_t bhs_vec3_dot(struct bhs_vec3 a, struct bhs_vec3 b);

/**
 * bhs_vec3_cross - Produto vetorial
 */
struct bhs_vec3 bhs_vec3_cross(struct bhs_vec3 a, struct bhs_vec3 b);

/**
 * bhs_vec3_norm - Norma euclidiana (magnitude)
 */
real_t bhs_vec3_norm(struct bhs_vec3 v);

/**
 * bhs_vec3_norm2 - Norma ao quadrado (evita sqrt)
 */
real_t bhs_vec3_norm2(struct bhs_vec3 v);

/**
 * bhs_vec3_normalize - Retorna vetor unitário
 *
 * Se v = 0, retorna vetor zero (não explode).
 */
struct bhs_vec3 bhs_vec3_normalize(struct bhs_vec3 v);

/* ============================================================================
 * CONVERSÕES
 * ============================================================================
 */

/**
 * bhs_vec4_spatial - Extrai parte espacial de vec4
 */
static inline struct bhs_vec3 bhs_vec4_spatial(struct bhs_vec4 v)
{
	return (struct bhs_vec3){ .x = v.x, .y = v.y, .z = v.z };
}

/**
 * bhs_vec3_to_vec4 - Promove vec3 para vec4 com t dado
 */
static inline struct bhs_vec4 bhs_vec3_to_vec4(struct bhs_vec3 v, real_t t)
{
	return (struct bhs_vec4){ .t = t, .x = v.x, .y = v.y, .z = v.z };
}

/* ============================================================================
 * COORDENADAS ESFÉRICAS
 * ============================================================================
 */

/**
 * bhs_vec3_to_spherical - Cartesianas para esféricas
 * @v: vetor em coordenadas cartesianas (x, y, z)
 * @r: [out] raio
 * @theta: [out] ângulo polar (0 a π)
 * @phi: [out] ângulo azimutal (-π a π)
 */
void bhs_vec3_to_spherical(struct bhs_vec3 v, real_t *r, real_t *theta,
			   real_t *phi);

/**
 * bhs_vec3_from_spherical - Esféricas para cartesianas
 */
struct bhs_vec3 bhs_vec3_from_spherical(real_t r, real_t theta, real_t phi);

#endif /* BHS_CORE_MATH_VEC4_H */
