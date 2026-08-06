/**
 * @file ri_math.h
 * @brief Definições matemáticas fundamentais e macros de plataforma
 *
 * "A matemática é a linguagem com a qual Deus escreveu o universo."
 * — Galileu (provavelmente pensando em float64)
 */

#ifndef RI_LIB_MATH_RI_MATH_H
#define RI_LIB_MATH_RI_MATH_H

#ifndef RI_SHADER_COMPILER
#include <math.h>
#include <stdalign.h>
#endif

/* ============================================================================
 * MACROS DE PORTABILIDADE (CPU/GPU)
 * ============================================================================
 */

#ifdef RI_SHADER_COMPILER
/* Compilando como OpenCL C (via -x cl) */
/* Habilita double precision se suportado */
#pragma OPENCL EXTENSION cl_khr_fp64 : enable

#define RI_GPU_KERNEL __kernel
#define RI_DEVICE_FUNC
#define RI_ALIGN(x) __attribute__((aligned(x)))
#else
/* Compilando como C/C++ padrão (Host) */
#define RI_GPU_KERNEL
#define RI_DEVICE_FUNC
#define RI_ALIGN(x) alignas(x)
#endif

/* ============================================================================
 * PRECISÃO NUMÉRICA (real_t)
 * ============================================================================
 */

/*
 * Por padrão usamos double para CPU simular física (precisão buraco negro).
 * Shaders podem definir RI_USE_FLOAT se o hardware não aguentar fp64.
 */
#if defined(RI_USE_FLOAT)
typedef float real_t;
#define RI_EPSILON 1e-5f
#define ri_abs fabsf
#define ri_sqrt sqrtf
#define ri_pow powf
#define ri_sin sinf
#define ri_cos cosf
#define ri_tan tanf
#define ri_acos acosf
#define ri_atan2 atan2f
#else
typedef double real_t;
#define RI_EPSILON 1e-9
#define ri_abs fabs
#define ri_sqrt sqrt
#define ri_pow pow
#define ri_sin sin
#define ri_cos cos
#define ri_tan tan
#define ri_acos acos
#define ri_atan2 atan2
#endif

/* Consistência Pi */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#endif /* RI_LIB_MATH_RI_MATH_H */
