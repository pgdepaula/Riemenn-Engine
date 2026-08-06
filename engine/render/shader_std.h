/**
 * @file ri_shader_std.h
 * @brief Biblioteca padrão para shaders escritos em C
 *
 * Mapeia tipos e funções do C para builtins de GPU (SPIR-V)
 * quando compilado com frontend Clang -> SPIR-V.
 */

#ifndef RI_LIB_SHADER_STD_H
#define RI_LIB_SHADER_STD_H

#include "engine/math/ri_math.h"
#include "engine/math/tensor/tensor.h"
#include "engine/math/vec4.h"

/* ============================================================================
 * TIPOS DE MEMÓRIA (ADDRESS SPACES)
 * ============================================================================
 */

#define RI_GLOBAL __global
#define RI_CONSTANT __constant
#define RI_LOCAL __local

/* ============================================================================
 * BUFFERS E BINDINGS
 * ============================================================================
 */

/* 
 * Layout de buffer std430 (regras de alinhamento de GPU)
 * Binding set=0, binding=N
 * Nota: Em OpenCL puro os bindings são por ordem de argumento, 
 * mas SPIR-V backend respeita atributos se passados corretamente.
 * Por simplicidade usamos argumentos diretos por enquanto.
 */
#define RI_BUFFER(type, set_idx, binding_idx) RI_GLOBAL type *

/* ============================================================================
 * BUILTINS DE COMPUTE SHADER
 * ============================================================================
 */

/* Redefinição de tipos básicos se necessário, mas -finclude-default-header já traz*/
// typedef unsigned int uint; // OpenCL já tem

/* Define ri_get_global_id based on context */
#ifdef RI_SHADER_COMPILER
/* GPU Implementation (OpenCL builtins) */
RI_DEVICE_FUNC static inline uint ri_get_global_id(uint dim)
{
	return get_global_id(dim);
}
#else
/* CPU Fallback / Intellisense Mock */
static inline uint ri_get_global_id(uint dim)
{
	return 0;
}
#endif

#endif /* RI_LIB_SHADER_STD_H */
