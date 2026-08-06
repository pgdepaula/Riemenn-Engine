#ifndef RI_SIMULATION_DATA_PLANET_H
#define RI_SIMULATION_DATA_PLANET_H

#include <stdbool.h>
#include <stddef.h>
#include "engine/math/vec4.h"

enum ri_planet_type {
    RI_PLANET_TERRESTRIAL,
    RI_PLANET_GAS_GIANT,
    RI_PLANET_ICE_GIANT,
    RI_PLANET_DWARF,
    RI_STAR_MAIN_SEQ,
    RI_BLACK_HOLE
};

struct ri_planet_desc {
    const char *name;
    enum ri_planet_type type;
    double mass;             /* kg */
    double radius;           /* m */
    double semimajor_axis;   /* m */
    double eccentricity;
    double inclination;       /* deg */
    double long_asc_node;     /* deg */
    double long_perihelion;   /* deg */
    double mean_longitude;    /* deg */
    double rot_period;        /* s */
    double rotation_period;   /* s */
    double orbital_period;    /* s */
    double axis_tilt;         /* rad */
    double mean_temperature;
    double density;
    double albedo;
    bool has_atmosphere;
    double surface_pressure;
    double j2;
    struct ri_vec3 base_color;
    struct ri_vec3 (*get_surface_color)(struct ri_vec3 p);
};

struct ri_planet_registry_entry {
    const char *name;
    struct ri_planet_desc (*getter)(void);
    const struct ri_planet_registry_entry *next;
};

/* Presets getters */
struct ri_planet_desc ri_sun_get_desc(void);
struct ri_planet_desc ri_mercury_get_desc(void);
struct ri_planet_desc ri_venus_get_desc(void);
struct ri_planet_desc ri_earth_get_desc(void);
struct ri_planet_desc ri_moon_get_desc(void);
struct ri_planet_desc ri_mars_get_desc(void);
struct ri_planet_desc ri_jupiter_get_desc(void);
struct ri_planet_desc ri_saturn_get_desc(void);
struct ri_planet_desc ri_uranus_get_desc(void);
struct ri_planet_desc ri_neptune_get_desc(void);
struct ri_planet_desc ri_pluto_get_desc(void);

static inline const struct ri_planet_registry_entry *ri_planet_registry_get_head(void) {
    return NULL;
}

#endif /* RI_SIMULATION_DATA_PLANET_H */
