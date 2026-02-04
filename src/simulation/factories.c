/**
 * @file body_factory.c
 * @brief Fábrica de Corpos Celestes (Updated for Kernel Style)
 */

#include <string.h>
#include "engine/scene/scene.h"
#include "src/simulation/data/blackhole.h"
#include "src/simulation/data/planet.h"
#include "src/simulation/data/sun.h"

struct bhs_body bhs_body_create_planet_simple(struct bhs_vec3 pos, double mass,
					      double radius,
					      struct bhs_vec3 color)
{
	struct bhs_body b = { 0 };
	strncpy(b.name, "Planet", sizeof(b.name) - 1);

	/* Universal State */
	b.state.pos = pos;
	b.state.mass =
		(mass > 0.0) ? mass : 0.01; /* Earth-like relative to BH */
	b.state.radius = radius;
	b.state.acc = (struct bhs_vec3){ 0, 0, 0 };
	b.state.vel = (struct bhs_vec3){ 0, 0, 0 };
	b.state.rot_axis = (struct bhs_vec3){ 0, 1, 0 };
	b.state.rot_speed = 7.27e-5; /* Earth rotation rad/s */
	b.state.moment_inertia =
		0.4 * b.state.mass * radius * radius; /* Sphere */
	b.state.shape = BHS_SHAPE_SPHERE;

	/* Type Specifics */
	b.type = BHS_BODY_PLANET;
	b.prop.planet =
		(struct bhs_planet_data){ .physical_state = BHS_STATE_SOLID,
					  .density = 5514.0, /* Earth kg/m3 */
					  .surface_pressure = 1.0,   /* Atm */
					  .atmosphere_mass = 5.1e18, /* kg */
					  .composition = "N2 78%, O2 21%",
					  .temperature = 288.0, /* 15 C */
					  .albedo = 0.306,
					  .axis_tilt = 0.409, /* 23.4 deg */
					  .has_atmosphere = true,
					  .has_magnetic_field = true };

	/* Visuals */
	b.color = color;
	return b;
}

struct bhs_body bhs_body_create_star_simple(struct bhs_vec3 pos, double mass,
					    double radius,
					    struct bhs_vec3 color)
{
	struct bhs_body b = { 0 };
	strncpy(b.name, "Star", sizeof(b.name) - 1);

	/* Universal State */
	b.state.pos = pos;
	b.state.mass = (mass > 0.0) ? mass : 2.0; /* Sol-like */
	b.state.radius = radius;
	b.state.rot_axis = (struct bhs_vec3){ 0, 1, 0 };
	b.state.rot_speed = 2.9e-6; /* Sun rotation rad/s */
	b.state.moment_inertia =
		0.07 * b.state.mass * radius * radius; /* Central condensed */
	b.state.shape = BHS_SHAPE_SPHERE;

	/* Type Specifics */
	b.type = BHS_BODY_STAR;
	b.prop.star =
		(struct bhs_star_data){ .luminosity = 3.828e26,	  /* Watts */
					.temp_effective = 5772.0, /* Kelvin */
					.age = 4.6e9,		  /* Years */
					.stage = BHS_STAR_MAIN_SEQUENCE,
					.metallicity = 0.0122,
					.spectral_type = "G2V" };

	/* Visuals */
	b.color = color;
	return b;
}

struct bhs_body bhs_body_create_blackhole_simple(struct bhs_vec3 pos,
						 double mass, double radius)
{
	struct bhs_body b = { 0 };

	/* Universal State */
	b.state.pos = pos;
	b.state.mass = (mass > 0.0) ? mass : 10.0; /* 5x Sol mass BH */
	b.state.radius = radius;		   /* Visual Horizon */
	b.state.rot_axis = (struct bhs_vec3){ 0, 1, 0 };
	b.state.shape = BHS_SHAPE_SPHERE;

	/* Type Specifics */
	b.type = BHS_BODY_BLACKHOLE;
	b.prop.bh = (struct bhs_blackhole_data){
		.spin_factor = 0.9,
		.event_horizon_r = 2.0 * b.state.mass,
		.ergososphere_r =
			2.0 *
			b.state.mass, /* Simplified (static limit at equator) */
		.accretion_disk_mass = 0.01 * b.state.mass
	};

	/* Visuals */
	b.color = (struct bhs_vec3){ 0.0, 0.0, 0.0 };
	return b;
}

struct bhs_body bhs_body_create_from_desc(const struct bhs_planet_desc *desc,
					  struct bhs_vec3 pos)
{
	struct bhs_body b = { 0 };

	strncpy(b.name, desc->name, sizeof(b.name) - 1);

	/* Universal State */
	b.state.pos = pos;
	b.state.mass = desc->mass;
	b.state.radius = desc->radius;
	b.state.acc = (struct bhs_vec3){ 0, 0, 0 };
	b.state.vel = (struct bhs_vec3){ 0, 0, 0 };

	/* Rotação - Eixo Y (Up) inclinado por axis_tilt */
	/* Tilt 0 = (0,1,0). Tilt 90 = (1,0,0) (Deitado) */
	b.state.rot_axis = (struct bhs_vec3){ sin(desc->axis_tilt),
					      cos(desc->axis_tilt), 0.0 };
	/* Normalize axis roughly if needed, simplified here */

	if (desc->rotation_period != 0.0) {
		b.state.rot_speed = (2.0 * 3.14159) / desc->rotation_period;
	} else {
		b.state.rot_speed = 0.0;
	}

	b.state.shape = BHS_SHAPE_SPHERE;

	/* Type Mapping */
	switch (desc->type) {
	case BHS_STAR_MAIN_SEQ:
		b.type = BHS_BODY_STAR;
		b.prop.star.luminosity = 3.828e26; /* Default */
		b.prop.star.temp_effective = desc->mean_temperature;
		break;
	case BHS_BLACK_HOLE:
		b.type = BHS_BODY_BLACKHOLE;
		b.prop.bh.spin_factor = 0.9;
		break;
	default:
		b.type = BHS_BODY_PLANET;
		b.prop.planet.density = desc->density;
		b.prop.planet.axis_tilt = desc->axis_tilt;
		b.prop.planet.rotation_period =
			desc->rotation_period; /* [FIX] Copiar dado */
		b.prop.planet.albedo = desc->albedo;
		b.prop.planet.has_atmosphere = desc->has_atmosphere;
		b.prop.planet.surface_pressure = desc->surface_pressure;
		b.prop.planet.temperature = desc->mean_temperature;
		b.prop.planet.j2 = desc->j2; /* [NEW] Copiar J2 */
		break;
	}

	b.color = desc->base_color;
	b.is_alive = true;
	b.is_fixed = (b.type == BHS_BODY_STAR && pos.x == 0 && pos.y == 0);

	return b;
}

struct bhs_body bhs_body_create_from_sun_desc(const struct bhs_sun_desc *desc,
					      struct bhs_vec3 pos)
{
	struct bhs_body b = { 0 };

	strncpy(b.name, desc->name, sizeof(b.name) - 1);

	/* Universal State */
	b.state.pos = pos;
	b.state.mass = desc->mass;
	b.state.radius = desc->radius;
	b.state.rot_axis = (struct bhs_vec3){ sin(desc->axis_tilt), 0.0,
					      cos(desc->axis_tilt) };

	if (desc->rotation_period != 0.0) {
		b.state.rot_speed = (2.0 * 3.14159) / desc->rotation_period;
	} else {
		b.state.rot_speed = 0.0;
	}

	b.state.moment_inertia =
		0.07 * b.state.mass * desc->radius * desc->radius;
	b.state.shape = BHS_SHAPE_SPHERE;
	b.state.acc = (struct bhs_vec3){ 0, 0, 0 };
	b.state.vel = (struct bhs_vec3){ 0, 0, 0 };

	/* Type Specifics */
	b.type = BHS_BODY_STAR;
	b.prop.star.luminosity = desc->luminosity;
	b.prop.star.temp_effective = desc->temperature;
	b.prop.star.age = desc->age;
	b.prop.star.metallicity = desc->metallicity;
	strncpy(b.prop.star.spectral_type, desc->spectral_type, 7);

	/* Mapping enum from one header to another (could be unified but keeping explicit) */
	/* Assuming simplistic mapping for now */
	b.prop.star.stage = (int)desc->stage;

	/* Visuals */
	b.color = desc->base_color;
	b.is_alive = true;
	b.is_fixed = false; /* Caller determines if fixed, typically */

	return b;
}

struct bhs_body
bhs_body_create_from_bh_desc(const struct bhs_blackhole_desc *desc,
			     struct bhs_vec3 pos)
{
	struct bhs_body b = { 0 };

	strncpy(b.name, desc->name, sizeof(b.name) - 1);

	/* Universal State */
	b.state.pos = pos;
	b.state.mass = desc->mass;
	b.state.radius = desc->event_horizon_r; /* Visual horizon */
	b.state.rot_axis = (struct bhs_vec3){ 0, 1, 0 };
	b.state.rot_speed = 0.0; /* Complex for BH, simplified */

	b.state.moment_inertia = 0.4 * b.state.mass * desc->event_horizon_r *
				 desc->event_horizon_r;
	b.state.shape = BHS_SHAPE_SPHERE;
	b.state.acc = (struct bhs_vec3){ 0, 0, 0 };
	b.state.vel = (struct bhs_vec3){ 0, 0, 0 };

	/* Type Specifics */
	b.type = BHS_BODY_BLACKHOLE;
	b.prop.bh.spin_factor = desc->spin;
	b.prop.bh.event_horizon_r = desc->event_horizon_r;
	b.prop.bh.accretion_disk_mass = desc->accretion_disk_mass;

	/* Visuals */
	b.color = desc->base_color;
	b.is_alive = true;
	b.is_fixed = false;

	return b;
}
