/**
 * @file bhs_shader_std.h
 * @brief Biblioteca padrão para shaders escritos em C
 *
 * Mapeia tipos e funções do C para builtins de GPU (SPIR-V)
 * quando compilado com frontend Clang -> SPIR-V.
 */

#ifndef BHS_LIB_SHADER_STD_H
#define BHS_LIB_SHADER_STD_H

#include "math/bhs_math.h"
#include "math/tensor/tensor.h"
#include "math/vec4.h"

/* ============================================================================
 * TIPOS DE MEMÓRIA (ADDRESS SPACES)
 * ============================================================================
 */

#define BHS_GLOBAL __global
#define BHS_CONSTANT __constant
#define BHS_LOCAL __local

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
#define BHS_BUFFER(type, set_idx, binding_idx) BHS_GLOBAL type *

/* ============================================================================
 * BUILTINS DE COMPUTE SHADER
 * ============================================================================
 */

/* Redefinição de tipos básicos se necessário, mas -finclude-default-header já traz*/
// typedef unsigned int uint; // OpenCL já tem

/* Define bhs_get_global_id based on context */
#ifdef BHS_SHADER_COMPILER
/* GPU Implementation (OpenCL builtins) */
BHS_DEVICE_FUNC static inline uint bhs_get_global_id(uint dim)
{
	return get_global_id(dim);
}
#else
/* CPU Fallback / Intellisense Mock */
static inline uint bhs_get_global_id(uint dim)
{
	return 0;
}
#endif

#endif /* BHS_LIB_SHADER_STD_H */
