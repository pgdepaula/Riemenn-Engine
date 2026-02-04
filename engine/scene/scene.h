#ifndef BHS_ENGINE_SCENE_NEW_H
#define BHS_ENGINE_SCENE_NEW_H

/* Typedef precisa estar visível */
typedef struct bhs_scene_impl *bhs_scene_t;

#include "engine/ecs/ecs.h"
#include "engine/engine.h"
#include "math/bhs_math.h"

/* ============================================================================
 * VIEW DTOs (Adaptador Legado para UI/Renderizador)
 * ============================================================================
 */

enum bhs_body_type {
	BHS_BODY_PLANET,
	BHS_BODY_MOON,
	BHS_BODY_STAR,
	BHS_BODY_BLACKHOLE,
	BHS_BODY_ASTEROID,
};

enum bhs_matter_state {
	BHS_STATE_SOLID,
	BHS_STATE_LIQUID,
	BHS_STATE_GAS,
	BHS_STATE_PLASMA
};

enum bhs_shape_type {
	BHS_SHAPE_SPHERE,
	BHS_SHAPE_ELLIPSOID,
	BHS_SHAPE_IRREGULAR
};

enum bhs_star_stage {
	BHS_STAR_MAIN_SEQUENCE,
	BHS_STAR_GIANT,
	BHS_STAR_WHITE_DWARF,
	BHS_STAR_NEUTRON
};

struct bhs_planet_data {
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

struct bhs_star_data {
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

struct bhs_blackhole_data {
	double spin_factor;
	double event_horizon_r;
	double ergososphere_r; // Typo fix? Keep legacy name.
	double accretion_disk_mass;
	double accretion_rate;
};

struct bhs_body_state {
	struct bhs_vec3 pos;
	struct bhs_vec3 vel;
	struct bhs_vec3 acc;
	struct bhs_vec3 rot_axis;
	double rot_speed;
	double moment_inertia;
	double mass;
	double radius;
	double current_rotation_angle; /* rad - acumulado */
	int shape;
};

/* Orbit Trail - buffer circular de posições históricas */
#define BHS_MAX_TRAIL_POINTS                                                   \
	2000000 /* 2M points * 1h sampling ~ 228 years history */

struct bhs_body {
	struct bhs_body_state state;
	enum bhs_body_type type;
	union {
		struct bhs_planet_data planet;
		struct bhs_star_data star;
		struct bhs_blackhole_data bh;
	} prop;
	struct bhs_vec3 color;
	bool is_fixed;
	bool is_alive;
	char name[32];
	bhs_entity_id entity_id; /* [NOVO] Back-reference para ECS */
	uint32_t visual_flags;	 /* [NOVO] Flags visuais (Trail, Markers...) */

	/* [NEW] Orbit Trail Data */
	float (*trail_positions)[3]; /* x, y, z - Dynamic Allocation */
	int trail_head;		     /* Próximo índice a escrever */
	int trail_count; /* Quantos pontos válidos (max = BHS_MAX_TRAIL_POINTS) */
};

/* API */
bhs_scene_t bhs_scene_create(void);
void bhs_scene_destroy(bhs_scene_t scene);
void bhs_scene_init_default(bhs_scene_t scene);
void bhs_scene_update(bhs_scene_t scene, double dt);
/* Accessors */
bhs_world_handle bhs_scene_get_world(bhs_scene_t scene);
const struct bhs_body *bhs_scene_get_bodies(bhs_scene_t scene, int *count);
bhs_entity_id bhs_scene_add_body_struct(bhs_scene_t scene, struct bhs_body b);
bhs_entity_id bhs_scene_add_body(bhs_scene_t scene, enum bhs_body_type type,
				 struct bhs_vec3 pos, struct bhs_vec3 vel,
				 double mass, double radius,
				 struct bhs_vec3 color);
bhs_entity_id bhs_scene_add_body_named(bhs_scene_t scene,
				       enum bhs_body_type type,
				       struct bhs_vec3 pos, struct bhs_vec3 vel,
				       double mass, double radius,
				       struct bhs_vec3 color, const char *name);

void bhs_scene_remove_body(bhs_scene_t scene, int index);
void bhs_scene_reset_counters(void);
void bhs_scene_clear_legacy_cache(void); /* [NOVO] Limpa trails antigos */

/* Factories (Legado/Shim) */
struct bhs_planet_desc;
struct bhs_body bhs_body_create_planet_simple(struct bhs_vec3 pos, double mass,
					      double radius,
					      struct bhs_vec3 color);
struct bhs_body bhs_body_create_star_simple(struct bhs_vec3 pos, double mass,
					    double radius,
					    struct bhs_vec3 color);
struct bhs_body bhs_body_create_blackhole_simple(struct bhs_vec3 pos,
						 double mass, double radius);

struct bhs_body bhs_body_create_from_desc(const struct bhs_planet_desc *desc,
					  struct bhs_vec3 pos);

/* Novas Factories Especializadas */
#include "src/simulation/data/blackhole.h"
#include "src/simulation/data/sun.h"

struct bhs_body bhs_body_create_from_sun_desc(const struct bhs_sun_desc *desc,
					      struct bhs_vec3 pos);
struct bhs_body
bhs_body_create_from_bh_desc(const struct bhs_blackhole_desc *desc,
			     struct bhs_vec3 pos);

#endif
