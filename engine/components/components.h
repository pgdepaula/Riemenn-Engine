/**
 * @file components.h
 * @brief Definição dos Componentes do Sistema (ECS)
 */

#ifndef BHS_ENGINE_COMPONENTS_H
#define BHS_ENGINE_COMPONENTS_H

#include <stdbool.h>
#include <stdint.h>
#include "math/vec4.h"

/* ============================================================================
 * COMPONENT IDs
 * ============================================================================
 * IDs manuais para evitar complexidade de hashing em Runtime.
 */
typedef enum {
	BHS_COMP_TRANSFORM = 0,
	BHS_COMP_PHYSICS,
	BHS_COMP_METRIC, // Cria distorcao no espaco-tempo (Buraco Negro, Estrela)
	BHS_COMP_RENDER, // Mesh/Material
	BHS_COMP_TAG,	 // Nome/Type tag
	BHS_COMP_METADATA, // [NEW] Global simulation metadata (Time, Scenario, etc)
	BHS_COMP_COUNT
} bhs_component_type_id;

/* ============================================================================
 * DATA STRUCTS
 * ============================================================================
 */

/**
 * struct bhs_transform - Posicionamento no espaco
 */
typedef struct {
	struct bhs_vec3 position;
	struct bhs_vec4 rotation; // Quat
	struct bhs_vec3 scale;
	// Cache de matriz de transformacao?
	// struct bhs_mat4 world_matrix;
} bhs_transform_t;

/**
 * struct bhs_physics - Dados para integracao de movimento
 */
typedef struct {
	struct bhs_vec3 velocity;
	struct bhs_vec3 acceleration;
	struct bhs_vec3 force_accumulator;
	double mass;
	double inverse_mass; // 0 se infinito (static)
	bool is_static;
} bhs_physics_t;

/**
 * enum bhs_metric_type - Tipo de distorcao
 */
typedef enum {
	BHS_METRIC_SCHWARZSCHILD,
	BHS_METRIC_KERR,
	BHS_METRIC_MINKOWSKI
} bhs_metric_type;

/**
 * struct bhs_metric - Componente que define que a entidade deforma o espaco
 */
typedef struct {
	bhs_metric_type type;
	double mass_parameter; // M (geometric units) or GM
	double spin_parameter; // a (Kerr)
	double event_horizon_radius;
} bhs_metric_t;

/**
 * struct bhs_tag - Metadados para debug/busca
 */
typedef struct {
	char name[32];
	uint32_t type_flags;
} bhs_tag_t;

#endif /* BHS_ENGINE_COMPONENTS_H */
