#ifndef RI_ENGINE_GEOMETRY_MESH_GEN_H
#define RI_ENGINE_GEOMETRY_MESH_GEN_H

#include <stdint.h>

/**
 * @struct ri_vertex_3d
 * @brief Vertex format for 3D rendering
 */
typedef struct ri_vertex_3d {
	float pos[3];
	float normal[3];
	float uv[2];
} ri_vertex_3d_t;

/**
 * @struct ri_mesh
 * @brief CPU mesh data
 */
typedef struct ri_mesh {
	ri_vertex_3d_t *vertices;
	uint32_t vertex_count;

	uint16_t *indices; /* Use 16-bit for simplicity/compatibility */
	uint32_t index_count;
} ri_mesh_t;

/**
 * @brief Generate a UV Sphere
 * @param rings Number of latitude bands
 * @param sectors Number of longitude slices
 */
ri_mesh_t ri_mesh_gen_sphere(int rings, int sectors);

/**
 * @brief Free mesh data
 */
void ri_mesh_free(ri_mesh_t mesh);

#endif
