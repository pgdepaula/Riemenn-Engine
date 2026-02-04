/**
 * @file mat4.c
 * @brief Implementação de Matrizes 4x4
 */

#include "mat4.h"
#include <math.h>
#include <string.h>

/* ============================================================================
 * IMPLEMENTAÇÃO
 * ============================================================================
 */

bhs_mat4_t bhs_mat4_identity(void)
{
	bhs_mat4_t out;
	memset(out.m, 0, sizeof(float) * 16);
	out.m[0] = 1.0f;
	out.m[5] = 1.0f;
	out.m[10] = 1.0f;
	out.m[15] = 1.0f;
	return out;
}

bhs_mat4_t bhs_mat4_zero(void)
{
	bhs_mat4_t out;
	memset(out.m, 0, sizeof(float) * 16);
	return out;
}

bhs_mat4_t bhs_mat4_mul(bhs_mat4_t a, bhs_mat4_t b)
{
	bhs_mat4_t out;
	/* Coluna 0 */
	out.m[0] = a.m[0] * b.m[0] + a.m[4] * b.m[1] + a.m[8] * b.m[2] +
		   a.m[12] * b.m[3];
	out.m[1] = a.m[1] * b.m[0] + a.m[5] * b.m[1] + a.m[9] * b.m[2] +
		   a.m[13] * b.m[3];
	out.m[2] = a.m[2] * b.m[0] + a.m[6] * b.m[1] + a.m[10] * b.m[2] +
		   a.m[14] * b.m[3];
	out.m[3] = a.m[3] * b.m[0] + a.m[7] * b.m[1] + a.m[11] * b.m[2] +
		   a.m[15] * b.m[3];

	/* Coluna 1 */
	out.m[4] = a.m[0] * b.m[4] + a.m[4] * b.m[5] + a.m[8] * b.m[6] +
		   a.m[12] * b.m[7];
	out.m[5] = a.m[1] * b.m[4] + a.m[5] * b.m[5] + a.m[9] * b.m[6] +
		   a.m[13] * b.m[7];
	out.m[6] = a.m[2] * b.m[4] + a.m[6] * b.m[5] + a.m[10] * b.m[6] +
		   a.m[14] * b.m[7];
	out.m[7] = a.m[3] * b.m[4] + a.m[7] * b.m[5] + a.m[11] * b.m[6] +
		   a.m[15] * b.m[7];

	/* Coluna 2 */
	out.m[8] = a.m[0] * b.m[8] + a.m[4] * b.m[9] + a.m[8] * b.m[10] +
		   a.m[12] * b.m[11];
	out.m[9] = a.m[1] * b.m[8] + a.m[5] * b.m[9] + a.m[9] * b.m[10] +
		   a.m[13] * b.m[11];
	out.m[10] = a.m[2] * b.m[8] + a.m[6] * b.m[9] + a.m[10] * b.m[10] +
		    a.m[14] * b.m[11];
	out.m[11] = a.m[3] * b.m[8] + a.m[7] * b.m[9] + a.m[11] * b.m[10] +
		    a.m[15] * b.m[11];

	/* Coluna 3 */
	out.m[12] = a.m[0] * b.m[12] + a.m[4] * b.m[13] + a.m[8] * b.m[14] +
		    a.m[12] * b.m[15];
	out.m[13] = a.m[1] * b.m[12] + a.m[5] * b.m[13] + a.m[9] * b.m[14] +
		    a.m[13] * b.m[15];
	out.m[14] = a.m[2] * b.m[12] + a.m[6] * b.m[13] + a.m[10] * b.m[14] +
		    a.m[14] * b.m[15];
	out.m[15] = a.m[3] * b.m[12] + a.m[7] * b.m[13] + a.m[11] * b.m[14] +
		    a.m[15] * b.m[15];

	return out;
}

struct bhs_v4 bhs_mat4_mul_v4(bhs_mat4_t m, struct bhs_v4 v)
{
	struct bhs_v4 out;
	/* v é tratado como coluna [x, y, z, w]^T */
	/* Column Major Matrix means:
	   x_out = m[0]*x + m[4]*y + m[8]*z + m[12]*w
	*/
	out.x = m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12] * v.w;
	out.y = m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13] * v.w;
	out.z = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w;
	out.w = m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w;
	return out;
}

/* ============================================================================
 * TRANSFORMAÇÕES
 * ============================================================================
 */

bhs_mat4_t bhs_mat4_translate(float x, float y, float z)
{
	bhs_mat4_t m = bhs_mat4_identity();
	/* Coluna 3 (Translação) */
	m.m[12] = x;
	m.m[13] = y;
	m.m[14] = z;
	return m;
}

bhs_mat4_t bhs_mat4_scale(float x, float y, float z)
{
	bhs_mat4_t m = bhs_mat4_zero();
	m.m[0] = x;
	m.m[5] = y;
	m.m[10] = z;
	m.m[15] = 1.0f;
	return m;
}

bhs_mat4_t bhs_mat4_rotate_z(float rad)
{
	bhs_mat4_t m = bhs_mat4_identity();
	float c = cosf(rad);
	float s = sinf(rad);

	/* Rotação Z (Column-Major)
	   [ c  -s   0   0 ]
	   [ s   c   0   0 ]
	*/
	m.m[0] = c;
	m.m[1] = s;
	m.m[4] = -s;
	m.m[5] = c;
	return m;
}

bhs_mat4_t bhs_mat4_rotate_y(float rad)
{
	bhs_mat4_t m = bhs_mat4_identity();
	float c = cosf(rad);
	float s = sinf(rad);

	/* Rotação Y (Column-Major)
	   [ c   0   s   0 ]
	   [ 0   1   0   0 ]
	   [ -s  0   c   0 ]
	*/
	m.m[0] = c;
	m.m[2] = -s;
	m.m[8] = s;
	m.m[10] = c;
	return m;
}

bhs_mat4_t bhs_mat4_rotate_x(float rad)
{
	bhs_mat4_t m = bhs_mat4_identity();
	float c = cosf(rad);
	float s = sinf(rad);

	/* Rotação X (Column-Major)
	   [ 1   0   0   0 ]
	   [ 0   c  -s   0 ]
	   [ 0   s   c   0 ]
	*/
	m.m[5] = c;
	m.m[6] = s;
	m.m[9] = -s;
	m.m[10] = c;
	return m;
}

bhs_mat4_t bhs_mat4_rotate_axis(struct bhs_v3 axis, float angle_rad)
{
	bhs_mat4_t m = bhs_mat4_identity();
	float c = cosf(angle_rad);
	float s = sinf(angle_rad);
	float omc = 1.0f - c;

	/* Normalizar eixo */
	float len = sqrtf(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
	if (len < 1e-6f)
		return m;
	float x = axis.x / len;
	float y = axis.y / len;
	float z = axis.z / len;

	/* Column 0 */
	m.m[0] = x * x * omc + c;
	m.m[1] = y * x * omc + z * s;
	m.m[2] = x * z * omc - y * s;

	/* Column 1 */
	m.m[4] = x * y * omc - z * s;
	m.m[5] = y * y * omc + c;
	m.m[6] = y * z * omc + x * s;

	/* Column 2 */
	m.m[8] = x * z * omc + y * s;
	m.m[9] = y * z * omc - x * s;
	m.m[10] = z * z * omc + c;

	return m;
}

/* ============================================================================
 * PROJEÇÃO (VULKAN CLIP SPACE: Y DOWN, 0..1 Z)
 * ============================================================================
 */

bhs_mat4_t bhs_mat4_perspective(float fov_y_rad, float aspect, float n, float f)
{
	bhs_mat4_t m = bhs_mat4_zero();

	float tan_half_fov = tanf(fov_y_rad / 2.0f);

	/* Column 0 */
	m.m[0] = 1.0f / (aspect * tan_half_fov);

	/* Column 1 */
	/* Y-Flip para Vulkan */
	m.m[5] = -1.0f / tan_half_fov;

	/* Column 2 */
	m.m[10] = f / (f - n);
	m.m[11] = 1.0f; /* w = z_view */

	/* Column 3 */
	m.m[14] = -(f * n) / (f - n);
	m.m[15] = 0.0f;

	return m;
}

bhs_mat4_t bhs_mat4_lookat(struct bhs_v3 eye, struct bhs_v3 center,
			   struct bhs_v3 up)
{
	struct bhs_v3 f = { center.x - eye.x, center.y - eye.y,
			    center.z - eye.z };
	float len_f = sqrtf(f.x * f.x + f.y * f.y + f.z * f.z);
	if (len_f > 0) {
		f.x /= len_f;
		f.y /= len_f;
		f.z /= len_f;
	}

	struct bhs_v3 s = { f.y * up.z - f.z * up.y, f.z * up.x - f.x * up.z,
			    f.x * up.y - f.y * up.x };
	float len_s = sqrtf(s.x * s.x + s.y * s.y + s.z * s.z);
	if (len_s > 0) {
		s.x /= len_s;
		s.y /= len_s;
		s.z /= len_s;
	}

	struct bhs_v3 u = { s.y * f.z - s.z * f.y, s.z * f.x - s.x * f.z,
			    s.x * f.y - s.y * f.x };

	bhs_mat4_t m = bhs_mat4_identity();

	/* Col 0 */
	m.m[0] = s.x;
	m.m[1] = u.x;
	m.m[2] = -f.x;
	m.m[3] = 0.0f;

	/* Col 1 */
	m.m[4] = s.y;
	m.m[5] = u.y;
	m.m[6] = -f.y;
	m.m[7] = 0.0f;

	/* Col 2 */
	m.m[8] = s.z;
	m.m[9] = u.z;
	m.m[10] = -f.z;
	m.m[11] = 0.0f;

	/* Col 3 (Translation) */
	m.m[12] = -(s.x * eye.x + s.y * eye.y + s.z * eye.z);
	m.m[13] = -(u.x * eye.x + u.y * eye.y + u.z * eye.z);
	m.m[14] = (f.x * eye.x + f.y * eye.y + f.z * eye.z);
	m.m[15] = 1.0f;

	return m;
}
