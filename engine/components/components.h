/**
 * @file components.h
 * @brief Definição dos Componentes do Sistema (ECS)
 */

#ifndef RI_ENGINE_COMPONENTS_H
#define RI_ENGINE_COMPONENTS_H

#include <stdbool.h>
#include <stdint.h>
#include "engine/math/vec4.h"

/* ============================================================================
 * COMPONENT IDs
 * ============================================================================
 * IDs manuais para evitar complexidade de hashing em Runtime.
 */
typedef enum {
	RI_COMP_TRANSFORM = 0,
	RI_COMP_PHYSICS,
	RI_COMP_METRIC, // Cria distorcao no espaco-tempo (Buraco Negro, Estrela)
	RI_COMP_RENDER, // Mesh/Material
	RI_COMP_TAG,	 // Nome/Type tag
	RI_COMP_METADATA, // [NEW] Global simulation metadata (Time, Scenario, etc)
	RI_COMP_COUNT
} ri_component_type_id;

/* ============================================================================
 * DATA STRUCTS
 * ============================================================================
 */

/**
 * struct ri_transform - Posicionamento no espaco
 */
typedef struct {
	struct ri_vec3 position;
	struct ri_vec4 rotation; // Quat
	struct ri_vec3 scale;
	// Cache de matriz de transformacao?
	// struct ri_mat4 world_matrix;
} ri_transform_t;

/**
 * struct ri_physics - Dados para integracao de movimento
 */
typedef struct {
	struct ri_vec3 velocity;
	struct ri_vec3 acceleration;
	struct ri_vec3 force_accumulator;
	double mass;
	double inverse_mass; // 0 se infinito (static)
	bool is_static;
} ri_physics_t;

/**
 * enum ri_metric_type - Tipo de distorcao
 */
typedef enum {
	RI_METRIC_SCHWARZSCHILD,
	RI_METRIC_KERR,
	RI_METRIC_MINKOWSKI
} ri_metric_type;

/**
 * struct ri_metric - Componente que define que a entidade deforma o espaco
 */
typedef struct {
	ri_metric_type type;
	double mass_parameter; // M (geometric units) or GM
	double spin_parameter; // a (Kerr)
	double event_horizon_radius;
} ri_metric_t;

/**
 * struct ri_tag - Metadados para debug/busca
 */
typedef struct {
	char name[32];
	uint32_t type_flags;
} ri_tag_t;

#endif /* RI_ENGINE_COMPONENTS_H */
