/**
 * @file bhs_math.h
 * @brief Definições matemáticas fundamentais e macros de plataforma
 *
 * "A matemática é a linguagem com a qual Deus escreveu o universo."
 * — Galileu (provavelmente pensando em float64)
 */

#ifndef BHS_LIB_MATH_BHS_MATH_H
#define BHS_LIB_MATH_BHS_MATH_H

#ifndef BHS_SHADER_COMPILER
#include <math.h>
#include <stdalign.h>
#endif

/* ============================================================================
 * MACROS DE PORTABILIDADE (CPU/GPU)
 * ============================================================================
 */

#ifdef BHS_SHADER_COMPILER
/* Compilando como OpenCL C (via -x cl) */
/* Habilita double precision se suportado */
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define BHS_GPU_KERNEL __kernel
#define BHS_DEVICE_FUNC
#define BHS_ALIGN(x) __attribute__((aligned(x)))
#else
/* Compilando como C/C++ padrão (Host) */
#define BHS_GPU_KERNEL
#define BHS_DEVICE_FUNC
#define BHS_ALIGN(x) alignas(x)
#endif

/* ============================================================================
 * PRECISÃO NUMÉRICA (real_t)
 * ============================================================================
 */

/*
 * Por padrão usamos double para CPU simular física (precisão buraco negro).
 * Shaders podem definir BHS_USE_FLOAT se o hardware não aguentar fp64.
 */
#if defined(BHS_USE_FLOAT)
typedef float real_t;
#define BHS_EPSILON 1e-5f
#define bhs_abs fabsf
#define bhs_sqrt sqrtf
#define bhs_pow powf
#define bhs_sin sinf
#define bhs_cos cosf
#define bhs_tan tanf
#define bhs_acos acosf
#define bhs_atan2 atan2f
#else
typedef double real_t;
#define BHS_EPSILON 1e-9
#define bhs_abs fabs
#define bhs_sqrt sqrt
#define bhs_pow pow
#define bhs_sin sin
#define bhs_cos cos
#define bhs_tan tan
#define bhs_acos acos
#define bhs_atan2 atan2
#endif

/* Consistência Pi */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif /* BHS_LIB_MATH_BHS_MATH_H */
