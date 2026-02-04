#ifndef BHS_ENGINE_GEOMETRY_MESH_GEN_H
#define BHS_ENGINE_GEOMETRY_MESH_GEN_H

#include <stdint.h>

/**
 * @struct bhs_vertex_3d
 * @brief Vertex format for 3D rendering
 */
typedef struct bhs_vertex_3d {
	float pos[3];
	float normal[3];
	float uv[2];
} bhs_vertex_3d_t;

/**
 * @struct bhs_mesh
 * @brief CPU mesh data
 */
typedef struct bhs_mesh {
	bhs_vertex_3d_t *vertices;
	uint32_t vertex_count;

	uint16_t *indices; /* Use 16-bit for simplicity/compatibility */
	uint32_t index_count;
} bhs_mesh_t;

/**
 * @brief Generate a UV Sphere
 * @param rings Number of latitude bands
 * @param sectors Number of longitude slices
 */
bhs_mesh_t bhs_mesh_gen_sphere(int rings, int sectors);

/**
 * @brief Free mesh data
 */
void bhs_mesh_free(bhs_mesh_t mesh);

#endif
