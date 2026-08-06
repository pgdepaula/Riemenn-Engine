/**
 * @file mat4.h
 * @brief Biblioteca de Matrizes 4x4 (Row-Major Storage / Column-Major Logic compatible)
 *
 * Implementação leve e focada em gráficos 3D (Vulkan/OpenGL convention).
 * Layout de memória:
 * [ 0  1  2  3 ]
 * [ 4  5  6  7 ]
 * [ 8  9 10 11 ]
 * [12 13 14 15 ]
 */

#ifndef RI_MATH_MAT4_H
#define RI_MATH_MAT4_H

/* #include "vec4.h" -- Physics vector not compatible with Graphics Standard */
#include <math.h>
#include <string.h>

/* Use float for GPU compatibility */
typedef struct ri_mat4 {
	float m[16];
} ri_mat4_t;

struct ri_v4 {
	float x, y, z, w;
};
struct ri_v3 {
	float x, y, z;
};

/* ============================================================================
 * CONSTRUTORES & BÁSICO
 * ============================================================================
 */

/* Retorna matriz Identidade */
ri_mat4_t ri_mat4_identity(void);

/* Retorna matriz Zero */
ri_mat4_t ri_mat4_zero(void);

/* Multiplicação: R = A * B */
ri_mat4_t ri_mat4_mul(ri_mat4_t a, ri_mat4_t b);

/* Multiplicação Vetor-Matriz: v_out = M * v_in */
/* Nota: Assume vetor coluna para transformação padrão (OpenGL style) */
struct ri_v4 ri_mat4_mul_v4(ri_mat4_t m, struct ri_v4 v);

/* ============================================================================
 * TRANSFORMAÇÕES (AFFINE)
 * ============================================================================
 */

/* Cria matriz de Translação */
ri_mat4_t ri_mat4_translate(float x, float y, float z);

/* Cria matriz de Escala */
ri_mat4_t ri_mat4_scale(float x, float y, float z);

/* Cria matriz de Rotação (Eixo X) - radianos */
ri_mat4_t ri_mat4_rotate_x(float rad);

/* Cria matriz de Rotação (Eixo Y) - radianos */
ri_mat4_t ri_mat4_rotate_y(float rad);

/* Cria matriz de Rotação (Eixo Z) - radianos */
ri_mat4_t ri_mat4_rotate_z(float rad);

/* Cria matriz de Rotação em torno de eixo arbitrário */
ri_mat4_t ri_mat4_rotate_axis(struct ri_v3 axis, float angle_rad);

/* ============================================================================
 * CÂMERA E PROJEÇÃO
 * ============================================================================
 */

/*
 * Cria matriz de Projeção Perspectiva (Right Handed, 0-1 Depth for Vulkan)
 * fov_y_rad: Campo de visão vertical em radianos
 * aspect: width / height
 * near_plane, far_plane: planos de corte
 */
ri_mat4_t ri_mat4_perspective(float fov_y_rad, float aspect, float near_plane,
				float far_plane);

/*
 * Cria matriz de View (LookAt) - Right Handed
 * eye: Posição da câmera
 * center: Ponto para onde olha
 * up: Vetor 'cima' (normalmente 0,1,0)
 */
ri_mat4_t ri_mat4_lookat(struct ri_v3 eye, struct ri_v3 center,
			   struct ri_v3 up);

#endif /* RI_MATH_MAT4_H */
