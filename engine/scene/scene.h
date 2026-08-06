#ifndef RI_ENGINE_SCENE_NEW_H
#define RI_ENGINE_SCENE_NEW_H

/* Typedef precisa estar visível */
typedef struct ri_scene_impl *ri_scene_t;

#include "engine/ecs/ecs.h"
#include "engine/engine.h"
#include "engine/math/ri_math.h"

/* ============================================================================
 * VIEW DTOs (Adaptador Legado para UI/Renderizador)
 * ============================================================================
 */

enum ri_body_type {
	RI_BODY_PLANET,
	RI_BODY_MOON,
	RI_BODY_STAR,
	RI_BODY_BLACKHOLE,
	RI_BODY_ASTEROID,
};

enum ri_matter_state {
	RI_STATE_SOLID,
	RI_STATE_LIQUID,
	RI_STATE_GAS,
	RI_STATE_PLASMA
};

enum ri_shape_type {
	RI_SHAPE_SPHERE,
	RI_SHAPE_ELLIPSOID,
	RI_SHAPE_IRREGULAR
};

enum ri_star_stage {
	RI_STAR_MAIN_SEQUENCE,
	RI_STAR_GIANT,
	RI_STAR_WHITE_DWARF,
	RI_STAR_NEUTRON
};

struct ri_planet_data {
	double density;
	double axis_tilt;	// [NOVO] Obliquidade em radianos
	double rotation_period; // [NOVO] Período de rotação sideral em segundos
	double j2; /* [NOVO] Harmônico J2 */ // Added field
	double albedo;
	bool has_atmosphere;
	double surface_pressure;
	double atmosphere_mass;
	char composition[64];
	double temperature;
	double heat_capacity;
	double energy_flux;
	int physical_state; // Enum simplificado
	bool has_magnetic_field;
};

struct ri_star_data {
	double luminosity;
	double temp_effective;
	double age;
	double density;
	double hydrogen_frac;
	double helium_frac;
	double metals_frac;
	int stage; // Simplificado
	double metallicity;
	char spectral_type[8];
};

struct ri_blackhole_data {
	double spin_factor;
	double event_horizon_r;
	double ergososphere_r; // Typo fix? Keep legacy name.
	double accretion_disk_mass;
	double accretion_rate;
};

struct ri_body_state {
	struct ri_vec3 pos;
	struct ri_vec3 vel;
	struct ri_vec3 acc;
	struct ri_vec3 rot_axis;
	double rot_speed;
	double moment_inertia;
	double mass;
	double radius;
	double current_rotation_angle; /* rad - acumulado */
	int shape;
};

/* Orbit Trail - buffer circular de posições históricas */
#define RI_MAX_TRAIL_POINTS                                                   \
	2000000 /* 2M points * 1h sampling ~ 228 years history */

struct ri_body {
	struct ri_body_state state;
	enum ri_body_type type;
	union {
		struct ri_planet_data planet;
		struct ri_star_data star;
		struct ri_blackhole_data bh;
	} prop;
	struct ri_vec3 color;
	bool is_fixed;
	bool is_alive;
	char name[32];
	ri_entity_id entity_id; /* [NOVO] Back-reference para ECS */
	uint32_t visual_flags;	 /* [NOVO] Flags visuais (Trail, Markers...) */

	/* [NEW] Orbit Trail Data */
	float (*trail_positions)[3]; /* x, y, z - Dynamic Allocation */
	int trail_head;		     /* Próximo índice a escrever */
	int trail_count; /* Quantos pontos válidos (max = RI_MAX_TRAIL_POINTS) */
};

/* API */
ri_scene_t ri_scene_create(void);
void ri_scene_destroy(ri_scene_t scene);
void ri_scene_init_default(ri_scene_t scene);
void ri_scene_update(ri_scene_t scene, double dt);
/* Accessors */
ri_world_handle ri_scene_get_world(ri_scene_t scene);
const struct ri_body *ri_scene_get_bodies(ri_scene_t scene, int *count);
ri_entity_id ri_scene_add_body_struct(ri_scene_t scene, struct ri_body b);
ri_entity_id ri_scene_add_body(ri_scene_t scene, enum ri_body_type type,
				 struct ri_vec3 pos, struct ri_vec3 vel,
				 double mass, double radius,
				 struct ri_vec3 color);
ri_entity_id ri_scene_add_body_named(ri_scene_t scene,
				       enum ri_body_type type,
				       struct ri_vec3 pos, struct ri_vec3 vel,
				       double mass, double radius,
				       struct ri_vec3 color, const char *name);

void ri_scene_remove_body(ri_scene_t scene, int index);
void ri_scene_reset_counters(void);
void ri_scene_clear_legacy_cache(void); /* [NOVO] Limpa trails antigos */

/* Factories (Legado/Shim) */
struct ri_planet_desc;
struct ri_body ri_body_create_planet_simple(struct ri_vec3 pos, double mass,
					      double radius,
					      struct ri_vec3 color);
struct ri_body ri_body_create_star_simple(struct ri_vec3 pos, double mass,
					    double radius,
					    struct ri_vec3 color);
struct ri_body ri_body_create_blackhole_simple(struct ri_vec3 pos,
						 double mass, double radius);

struct ri_body ri_body_create_from_desc(const struct ri_planet_desc *desc,
					  struct ri_vec3 pos);

/* Forward declarations para factories especializadas */
struct ri_sun_desc;
struct ri_blackhole_desc;

struct ri_body ri_body_create_from_sun_desc(const struct ri_sun_desc *desc,
					      struct ri_vec3 pos);
struct ri_body
ri_body_create_from_bh_desc(const struct ri_blackhole_desc *desc,
			     struct ri_vec3 pos);

#endif
