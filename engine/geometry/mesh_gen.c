/**
 * @file mesh_gen.c
 * @brief Gerador de Geometria
 */

#include "mesh_gen.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifndef PI
#define PI 3.14159265359f
#endif

bhs_mesh_t bhs_mesh_gen_sphere(int rings, int sectors)
{
	bhs_mesh_t mesh = { 0 };

	/* Computação de Vértices */
	mesh.vertex_count = (rings + 1) * (sectors + 1);
	mesh.vertices = malloc(mesh.vertex_count * sizeof(bhs_vertex_3d_t));

	if (!mesh.vertices)
		return mesh;

	int v_idx = 0;
	for (int r = 0; r <= rings;
	     r++) { /* Nota: Usar '<=' para fechar o loop? Ou verificar lógica */
		/* Lógica padrão geralmente repete o último vértice para fechar a costura UV */
		/* But index count logic uses squares. */
		/* If rings=10, we have 11 vertices in height? */
		/* Let's assume standard grid generation */
	}

	/* Reimplementando o loop de forma limpa */
	/* Rings+1 rows, Sectors+1 cols (for UV repeat) */

	for (int r = 0; r < rings + 1; r++) {		/* Altura */
		for (int s = 0; s < sectors + 1; s++) { /* Anel */
			/* Coordenadas normalizadas [0..1] */
			/* r vai de 0 a rings */
			float y_factor =
				(float)r /
				(float)rings; /* 0..1 (topo para base geralmente) */
			float x_factor =
				(float)s /
				(float)sectors; /* 0..1 (volta completa) */

			/* Coordenadas esféricas */
			/* Theta (em torno de Y): 0 a 2*PI */
			float theta = x_factor * 2.0f * PI;
			/* Phi (cima baixo): 0 a PI */
			float phi = (1.0f - y_factor) *
				    PI; /* 0 at bottom, PI at top? Wait. */
			/* Let's define: y_factor=0 -> Top (North Pole) */
			phi = y_factor * PI; /* 0 to PI */

			/* Cartesian (Radius 1)
                x = sin(phi) * cos(theta)
                z = sin(phi) * sin(theta)
                y = cos(phi)
             */

			float x = sinf(phi) * cosf(theta);
			float z = sinf(phi) * sinf(theta);
			float y = cosf(phi);

			mesh.vertices[v_idx].pos[0] = x;
			mesh.vertices[v_idx].pos[1] = y;
			mesh.vertices[v_idx].pos[2] = z;

			mesh.vertices[v_idx].normal[0] = x;
			mesh.vertices[v_idx].normal[1] = y;
			mesh.vertices[v_idx].normal[2] = z;

			mesh.vertices[v_idx].uv[0] = x_factor;
			mesh.vertices[v_idx].uv[1] = y_factor;

			v_idx++;
		}
	}

	/* Computação de índices */
	mesh.index_count = rings * sectors * 6;
	mesh.indices = malloc(mesh.index_count * sizeof(uint16_t));

	if (!mesh.indices) {
		free(mesh.vertices);
		mesh.vertices = NULL;
		return mesh;
	}

	int i_idx = 0;
	for (int r = 0; r < rings; r++) {
		for (int s = 0; s < sectors; s++) {
			int cur_row = r * (sectors + 1);
			int next_row = (r + 1) * (sectors + 1);

			/* Quad splitted into 2 triangles */
			/* T1: cur, next, cur+1 */
			/* T2: next, next+1, cur+1 */

			/* Ordem importa para culling (CCW) */
			/* Topo-Esq: cur_row + s */
			/* Topo-Dir: cur_row + s + 1 */
			/* Baixo-Esq: next_row + s */
			/* Baixo-Dir: next_row + s + 1 */

			int tl = cur_row + s;
			int tr = cur_row + s + 1;
			int bl = next_row + s;
			int br = next_row + s + 1;

			/* Tri 1: TL -> BL -> TR */
			mesh.indices[i_idx++] = tl;
			mesh.indices[i_idx++] = bl;
			mesh.indices[i_idx++] = tr;

			/* Tri 2: TR -> BL -> BR */
			mesh.indices[i_idx++] = tr;
			mesh.indices[i_idx++] = bl;
			mesh.indices[i_idx++] = br;
		}
	}

	return mesh;
}

void bhs_mesh_free(bhs_mesh_t mesh)
{
	if (mesh.vertices)
		free(mesh.vertices);
	if (mesh.indices)
		free(mesh.indices);
}
