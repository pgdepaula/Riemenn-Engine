/**
 * @file presets.c
 * @brief Implementação dos Corpos Celestes Pré-Definidos
 *
 * "Criar um Sol é fácil. Manter os planetas em órbita é a parte difícil."
 *
 * Este arquivo usa o sistema de unidades definido em lib/units.h.
 * Todas as proporções físicas são preservadas.
 */

#include "presets.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "../data/planet.h"
#include "engine/ecs/ecs.h"
#include "engine/scene/scene.h"
#include "engine/math/units.h"
#include "game/simulation/components/sim_components.h"

/* ============================================================================
 * FUNÇÕES AUXILIARES
 * ============================================================================
 */

double ri_preset_orbital_velocity(double central_mass, double orbital_radius)
{
	/* Usa a função do sistema de unidades */
	return ri_orbital_velocity(central_mass, orbital_radius);
}

/**
 * create_body_from_module - Converte descritor de planeta em corpo simulável
 * @desc: Descritor com dados SI reais (de planets/)
 * @center_pos: Posição do corpo central (Sol)
 * @central_mass_sim: Massa do corpo central em unidades de simulação
 *
 * Aplica escalas para converter unidades SI em unidades de simulação.
 * 
 * ESCALAS (definidas no topo do arquivo):
 *   MASS:   1e29 kg → 1.0 unidade
 *   DIST:   1 AU    → 50 unidades
 *   RADIUS: R_sol   → 3.0 unidades
 *
 * PROPORÇÕES REAIS DOS RAIOS:
 *   Sol:     696,340 km → 3.00 unidades
 *   Júpiter:  69,911 km → 0.30 unidades (10x menor que Sol)
 *   Saturno:  58,232 km → 0.25 unidades
 *   Terra:     6,371 km → 0.027 unidades (109x menor que Sol)
 *   Mercúrio:  2,439 km → 0.011 unidades
 *
 * Usamos os valores REAIS sem modificação.
 */
/*
 * Helper: Solve Kepler's Equation M = E - e*sin(E) for E
 */
static double solve_kepler(double M, double e)
{
	double E = M;
	for (int i = 0; i < 10; i++) {
		double dE = (E - e * sin(E) - M) / (1.0 - e * cos(E));
		E -= dE;
		if (fabs(dE) < 1e-6)
			break;
	}
	return E;
}

/*
 * Keplerian Elements to Cartesian State Vectors (J2000 -> Engine)
 * Engine Coordinates: Y-up (Gravity down). J2000: Z-up.
 * Mapping: J2000(X, Y, Z) -> Engine(X, Z, Y) 
 * (Y in J2000 matches Z in engine for "depth/plane", Z in J2000 is "up" matches Y in engine)
 */
static void ri_kepler_to_cartesian(struct ri_planet_desc *d,
				    double central_mass,
				    struct ri_vec3 *out_pos,
				    struct ri_vec3 *out_vel)
{
	/* 1. Extract Elements & Convert to Radians */
	double a = d->semimajor_axis; // meters
	double e = d->eccentricity;
	double inc = d->inclination * (M_PI / 180.0);
	double Omega = d->long_asc_node * (M_PI / 180.0);
	double varpi = d->long_perihelion * (M_PI / 180.0);
	double L = d->mean_longitude * (M_PI / 180.0);

	/* Argument of Periapsis */
	double omega = varpi - Omega;

	/* Mean Anomaly */
	double M = L - varpi;

	/* 2. Solve Kepler Equation for Eccentric Anomaly (E) */
	double E = solve_kepler(M, e);

	/* 3. True Anomaly (nu) & Distance (r) */
	double cosE = cos(E);
	double sinE = sin(E);

	double x_orb = a * (cosE - e);
	double y_orb = a * sqrt(1.0 - e * e) * sinE;
	double r = sqrt(x_orb * x_orb + y_orb * y_orb);

	/* Orbital Velocity (Vis-viva derivative) */
	/* Mean motion n = sqrt(mu / a^3) */
	const double G = 6.67430e-11;
	/* FIX: Include own mass for 2-body stability (reduced mass correction equivalent) */
	/* If we neglect d->mass, v is too low, orbit shrinks, period decreases. */
	double mu = G * (central_mass + d->mass);
	double n = sqrt(mu / (a * a * a));

	double vx_orb = -(n * a * a / r) * sinE;
	double vy_orb = (n * a * a / r) * sqrt(1.0 - e * e) * cosE;

	/* 4. Rotate to Heliocentric Coordinates (J2000) */
	double cosO = cos(Omega);
	double sinO = sin(Omega);
	double cosw = cos(omega);
	double sinw = sin(omega);
	double cosi = cos(inc);
	double sini = sin(inc);

	/* Rotation Matrix Elements */
	double Px = cosO * cosw - sinO * sinw * cosi;
	double Py = sinO * cosw + cosO * sinw * cosi;
	double Pz = sinw * sini;

	double Qx = -cosO * sinw - sinO * cosw * cosi;
	double Qy = -sinO * sinw + cosO * cosw * cosi;
	double Qz = cosw * sini;

	/* J2000 Position */
	double X = x_orb * Px + y_orb * Qx;
	double Y = x_orb * Py + y_orb * Qy;
	double Z = x_orb * Pz + y_orb * Qz;

	/* J2000 Velocity */
	double VX = vx_orb * Px + vy_orb * Qx;
	double VY = vx_orb * Py + vy_orb * Qy;
	double VZ = vx_orb * Pz + vy_orb * Qz;

	/* 5. Map to Engine Coordinates (X -> X, Y -> Z, Z -> Y) */
	out_pos->x = X;
	out_pos->y = Z; /* Z_J2000 (up) -> Y_Engine (up) */
	out_pos->z = Y; /* Y_J2000 (plane) -> Z_Engine (plane) */

	out_vel->x = VX;
	out_vel->y = VZ;
	out_vel->z = VY;
}

static struct ri_body
create_body_from_module(struct ri_planet_desc desc, struct ri_vec3 center_pos,
			struct ri_vec3 center_vel, double central_mass_sim,
			ri_entity_id parent_id, ri_scene_t scene)
{
	struct ri_vec3 pos = { 0 };
	struct ri_vec3 vel = { 0 };

	(void)parent_id;
	(void)scene;

	/* Calculate realistic position/velocity IF we have orbital data */
	if (desc.semimajor_axis > 0.0) {
		ri_kepler_to_cartesian(&desc, central_mass_sim, &pos, &vel);

		/* Offset by central body position */
		pos.x += center_pos.x;
		pos.y += center_pos.y;
		pos.z += center_pos.z;
	} else {
		/* Fallback for Sun/Fixed bodies at 0 or manual placement */
		pos = center_pos;
	}

	/* Cria corpo base a partir do descritor */
	struct ri_body b = ri_body_create_from_desc(&desc, pos);

	/* Set calculated velocity + Parent Velocity */
	b.state.vel.x = vel.x + center_vel.x;
	b.state.vel.y = vel.y + center_vel.y;
	b.state.vel.z = vel.z + center_vel.z;

	/* Aplica escalas de massa e raio - REAL SCALE (SI) */
	b.state.mass = b.state.mass;
	b.state.radius = b.state.radius;

	printf("[PRESET] %s: M=%.2e, R=%.4f (real), a=%.2e m\n", desc.name,
	       b.state.mass, b.state.radius, desc.semimajor_axis);

	/* Create the body first to define it, avoiding chicken-egg? 
       No, we return the body struct to be added by caller? 
       Wait, create_body_from_module returns struct ri_body.
       But we need to attach the COMPONENT to the ENTITY.
       Typical flow: 
       1. Create struct body.
       2. Add to scene -> get ID.
       3. Add extra components manually if needed.
       
       OR we change this helper to ADD to scene itself?
       The callers (ri_preset_solar_system) call ri_scene_add_body_struct immediately after.
       
       Let's change this to create the entity directly?
       No, let's keep it returning struct for composability, BUT we can't attach components to a struct buffer cleanly unless we extend struct ri_body to hold generic components (it doesn't).
       
       BETTER: We return struct, and return the orbital data via out param? 
       OR just add the component AFTER adding body in the caller.
       
       Let's modify the caller to handle component attachment using a helper.
       Reverting signature change idea here.
    */
	return b;
}

/* Helper to attach orbital component */
static void attach_orbital_component(ri_scene_t scene, ri_entity_id entity,
				     ri_entity_id parent,
				     double semi_major_axis,
				     double eccentricity, double period,
				     bool tidal_lock)
{
	if (!scene || entity == RI_ENTITY_INVALID ||
	    parent == RI_ENTITY_INVALID)
		return;

	ri_world_handle world = ri_scene_get_world(scene);

	ri_orbital_component orb = {
		.parent = parent,
		.semi_major_axis = semi_major_axis,
		.eccentricity = eccentricity,
		.period = period,
		.flags = tidal_lock ? RI_ORBITAL_FLAG_TIDAL_LOCK : 0
	};

	ri_ecs_add_component(world, entity, RI_COMP_ORBITAL, sizeof(orb),
			      &orb);
}

/* ============================================================================
 * MAIN PRESET LOADER
 * ============================================================================
 */

void ri_preset_solar_system(ri_scene_t scene)
{
	if (!scene) {
		fprintf(stderr, "[PRESET] Erro: cena nula\n");
		return;
	}

	printf("[PRESET] Criando Sistema Solar Completo...\n");
	printf("[PRESET] Sistema de unidades: lib/units.h (G=1, M☉=20, R☉=3, "
	       "AU=50)\n");

	/* 1. SUN - Dados vindos de sun.c */
	struct ri_planet_desc d_sun = ri_sun_get_desc();

	printf("[PRESET] Sol (REAL): M=%.3e kg, R=%.3e m\n", d_sun.mass,
	       d_sun.radius);

	struct ri_body sun =
		ri_body_create_from_desc(&d_sun, (struct ri_vec3){ 0, 0, 0 });

	/* Aplica escalas usando units.h - REAL SCALE (SI) */
	/* No conversion */
	sun.state.mass = sun.state.mass;
	sun.state.radius = sun.state.radius;

	printf("[PRESET] Sol (SIM):  M=%.2f, R=%.2f\n", sun.state.mass,
	       sun.state.radius);
	fflush(stdout);

	/* Sol é fixo no centro */
	sun.is_fixed = true;

	ri_entity_id sun_id = ri_scene_add_body_struct(scene, sun);

	double M_sun = sun.state.mass;

	/* 2. PLANETS */
	struct ri_planet_desc (*planet_getters[])(void) = {
		ri_mercury_get_desc,
		ri_venus_get_desc,
		ri_earth_get_desc,
		ri_mars_get_desc,
		ri_jupiter_get_desc,
		ri_saturn_get_desc,
		ri_uranus_get_desc,
		ri_neptune_get_desc,
		ri_pluto_get_desc, /* Pluto acts as generic dwarf here */
		NULL
	};

	/* Store Earth for Moon creation */
	struct ri_body earth_body = { 0 };
	ri_entity_id earth_id = RI_ENTITY_INVALID;
	bool earth_found = false;

	for (int i = 0; planet_getters[i] != NULL; i++) {
		struct ri_planet_desc d = planet_getters[i]();

		/* [FIX] Pass 0 velocity for primary planets orbiting static Sun */
		struct ri_body b = create_body_from_module(
			d, sun.state.pos, (struct ri_vec3){ 0, 0, 0 }, M_sun,
			RI_ENTITY_INVALID, scene);
		ri_entity_id pid = ri_scene_add_body_struct(scene, b);

		/* Attach Orbital Info linking to SUN */
		attach_orbital_component(scene, pid, sun_id, d.semimajor_axis,
					 d.eccentricity, d.orbital_period,
					 false);

		if (strcmp(d.name, "Terra") == 0) {
			earth_body = b;
			earth_id = pid;
			earth_found = true;
		}
	}

	/* 3. MOON (If Earth exists) */
	if (earth_found && earth_id != RI_ENTITY_INVALID) {
		printf("[PRESET] Adicionando Lua corajosa...\n");
		struct ri_planet_desc d_moon = ri_moon_get_desc();
		struct ri_body moon = create_body_from_module(
			d_moon, earth_body.state.pos, earth_body.state.vel,
			earth_body.state.mass, earth_id, scene);
		ri_entity_id moon_id = ri_scene_add_body_struct(scene, moon);

		/* Attach Orbital Info linking to EARTH */
		/* Moon is tidally locked */
		attach_orbital_component(
			scene, moon_id, earth_id, d_moon.semimajor_axis,
			d_moon.eccentricity, d_moon.orbital_period, true);
	}

	printf("[PRESET] Sistema Solar Completo Carregado!\n");
}

void ri_preset_earth_moon_only(ri_scene_t scene)
{
	if (!scene)
		return;

	printf("[PRESET] Criando APENAS Terra e Lua (Sem Sol)...\n");

	/* 1. EARTH (Fixed at 0,0,0) - Anchor of this simulation */
	struct ri_planet_desc d_earth = ri_earth_get_desc();
	/* No central mass (Sun), or use Sun mass as 'phantom' if we want Earth to orbit something invisible? 
       No, user wants Earth & Moon. Earth should be the center. 
       We create Earth at 0,0,0 with 0 velocity. */
	struct ri_vec3 center = { 0, 0, 0 };
	/* We use Earth's own structure but positioned at origin */
	struct ri_body earth = ri_body_create_from_desc(&d_earth, center);

	earth.is_fixed =
		true; /* Fix Earth so it doesn't drift due to Moon's pull (optional, but good for "Study") */

	ri_scene_add_body_struct(scene, earth);

	/* 2. MOON */
	struct ri_planet_desc d_moon = ri_moon_get_desc();

	/* Moon orbits Earth.
       Center mass for orbital calc is Earth's mass.
       Center pos is Earth (0,0,0). 
       Center vel is Earth (0,0,0). */
	struct ri_body moon = create_body_from_module(
		d_moon, center, (struct ri_vec3){ 0, 0, 0 }, earth.state.mass,
		RI_ENTITY_INVALID, scene);

	ri_scene_add_body_struct(scene, moon);

	printf("[PRESET] Terra e Lua (Isolados) carregados.\n");
}

void ri_preset_earth_moon_sun(ri_scene_t scene)
{
	if (!scene)
		return;

	printf("[PRESET] Criando Sol, Terra e Lua (Escala Real)...\n");

	/* 1. SUN */
	struct ri_planet_desc d_sun = ri_sun_get_desc();
	struct ri_body sun =
		ri_body_create_from_desc(&d_sun, (struct ri_vec3){ 0, 0, 0 });

	/* Escalas - REAL SCALE (SI) */
	sun.state.mass = sun.state.mass;
	sun.state.radius = sun.state.radius;
	sun.is_fixed = true;

	ri_entity_id sun_id = ri_scene_add_body_struct(scene, sun);

	/* 2. EARTH */
	struct ri_planet_desc d_earth = ri_earth_get_desc();
	struct ri_body earth = create_body_from_module(
		d_earth, sun.state.pos, (struct ri_vec3){ 0, 0, 0 },
		sun.state.mass, RI_ENTITY_INVALID, scene);
	ri_entity_id earth_id = ri_scene_add_body_struct(scene, earth);
	attach_orbital_component(scene, earth_id, sun_id,
				 d_earth.semimajor_axis, d_earth.eccentricity,
				 d_earth.orbital_period, false);

	/* 3. MOON */
	struct ri_planet_desc d_moon = ri_moon_get_desc();
	/* Moon orbits Earth, so center_pos is Earth's position, VELOCITY is Earth's VELOCITY, and central_mass is Earth's mass */
	struct ri_body moon = create_body_from_module(
		d_moon, earth.state.pos, earth.state.vel, earth.state.mass,
		earth_id, scene);

	ri_entity_id moon_id = ri_scene_add_body_struct(scene, moon);
	attach_orbital_component(scene, moon_id, earth_id,
				 d_moon.semimajor_axis, d_moon.eccentricity,
				 d_moon.orbital_period, true);

	printf("[PRESET] Sol, Terra e Lua carregados.\n");
}

void ri_preset_jupiter_pluto_pull(ri_scene_t scene)
{
	if (!scene)
		return;

	printf("[PRESET] Criando Workspace Júpiter & Plutão Pull...\n");

	/* 1. SUN (Fixo na origem para referência gravitacional do sistema solar) */
	struct ri_planet_desc d_sun = ri_sun_get_desc();
	struct ri_body sun =
		ri_body_create_from_desc(&d_sun, (struct ri_vec3){ 0, 0, 0 });
	sun.is_fixed = true;
	ri_entity_id sun_id = ri_scene_add_body_struct(scene, sun);

	/* 2. JUPITER (Orbitando o Sol normalmente) */
	struct ri_planet_desc d_jup = ri_jupiter_get_desc();
	struct ri_body jup = create_body_from_module(
		d_jup, sun.state.pos, (struct ri_vec3){ 0, 0, 0 },
		sun.state.mass, sun_id, scene);
	ri_entity_id jup_id = ri_scene_add_body_struct(scene, jup);

	attach_orbital_component(scene, jup_id, sun_id, d_jup.semimajor_axis,
				 d_jup.eccentricity, d_jup.orbital_period,
				 false);

	/* 3. PLUTO (Puxado para Júpiter) */
	struct ri_planet_desc d_pluto = ri_pluto_get_desc();

	/* Posição: Perto de Júpiter. Vamos usar 20x o raio de Júpiter como distância inicial. */
	/* Radius Jup ~71k km. 20x ~ 1.4M km. 
	   Comparação: Luas de Júpiter: Io ~421k, Europa ~671k, Ganymede ~1M, Callisto ~1.8M.
	   Então 20x raio coloca Plutão entre Ganymede e Callisto. Perfeito para "pull". */

	double offset_dist = jup.state.radius * 20.0;

	/* Offset vector: apenas em X para simplificar visualização */
	struct ri_vec3 offset = { offset_dist, 0, 0 };

	/* Posição final: Júpiter Pos + Offset */
	struct ri_vec3 pluto_pos = { jup.state.pos.x + offset.x,
				      jup.state.pos.y + offset.y,
				      jup.state.pos.z + offset.z };

	/* Velocidade: IGUAL a Júpiter. 
	   Se a velocidade for igual, eles estão em "repouso relativo".
	   A única força atuando relativamente será a gravidade mútua (e maré solar, mas Júpiter domina aqui).
	   Isso fará Plutão "cair" em direção a Júpiter. */
	struct ri_vec3 pluto_vel = jup.state.vel;

	struct ri_body pluto = ri_body_create_from_desc(&d_pluto, pluto_pos);
	pluto.state.vel = pluto_vel;

	/* Adicionar à cena */
	ri_scene_add_body_struct(scene, pluto);

	printf("[PRESET] Júpiter e Plutão posicionados. Distância inicial: "
	       "%.2f (Sim Units)\n",
	       offset_dist);
}

/* Backward compatibility dummies if needed, but we replaced the main loop */
struct ri_body ri_preset_sun(struct ri_vec3 pos)
{
	/* Não usado pelo main loop, mas mantido para compatibilidade */
	struct ri_planet_desc d = ri_sun_get_desc();
	struct ri_body b = ri_body_create_from_desc(&d, pos);
	b.state.mass = RI_KG_TO_SIM(b.state.mass);
	b.state.radius = RI_RADIUS_TO_SIM(b.state.radius);
	return b;
}

struct ri_body ri_preset_earth(struct ri_vec3 sun_pos)
{
	struct ri_planet_desc d = ri_earth_get_desc();
	/* Shim: assuming static sun */
	return create_body_from_module(d, sun_pos, (struct ri_vec3){ 0, 0, 0 },
				       RI_SIM_MASS_SUN, RI_ENTITY_INVALID,
				       NULL);
}

struct ri_body ri_preset_moon(struct ri_vec3 earth_pos,
				struct ri_vec3 earth_vel)
{
	/* Not used in main preset anymore, but keeping for compatibility/API completeness */
	(void)earth_vel;
	/* Still need a central mass for detailed orbital calc if using create_body_from_module, 
       but here we just take Earth Sim Mass approx or rely on module defaults */
	struct ri_planet_desc d = ri_moon_get_desc();
	/* Assuming Earth Mass approx 5.97e24 */
	return create_body_from_module(d, earth_pos, earth_vel, 5.972e24,
				       RI_ENTITY_INVALID, NULL);
}
