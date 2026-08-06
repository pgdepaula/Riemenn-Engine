/**
 * @file planet_descs.c
 * @brief Implementação dos descritores astronômicos pré-definidos (Sun, Earth, Moon, Jupiter, etc.)
 */

#include "game/simulation/data/planet.h"
#include "game/simulation/data/sun.h"

struct ri_planet_desc ri_sun_get_desc(void) {
    struct ri_planet_desc d = { 0 };
    d.name = "Sun";
    d.type = RI_STAR_MAIN_SEQ;
    d.mass = 1.989e30;
    d.radius = 6.9634e8;
    d.semimajor_axis = 0.0;
    d.rotation_period = 2160000.0;
    d.mean_temperature = 5778.0;
    d.base_color = (struct ri_vec3){1.0f, 0.9f, 0.4f};
    return d;
}

struct ri_planet_desc ri_mercury_get_desc(void) {
    struct ri_planet_desc d = { 0 };
    d.name = "Mercury";
    d.type = RI_PLANET_TERRESTRIAL;
    d.mass = 3.3011e23;
    d.radius = 2.4397e6;
    d.semimajor_axis = 5.7909e10;
    d.eccentricity = 0.2056;
    d.orbital_period = 7600521.6;
    d.base_color = (struct ri_vec3){0.7f, 0.7f, 0.7f};
    return d;
}

struct ri_planet_desc ri_venus_get_desc(void) {
    struct ri_planet_desc d = { 0 };
    d.name = "Venus";
    d.type = RI_PLANET_TERRESTRIAL;
    d.mass = 4.8675e24;
    d.radius = 6.0518e6;
    d.semimajor_axis = 1.0821e11;
    d.eccentricity = 0.0067;
    d.orbital_period = 19414166.4;
    d.base_color = (struct ri_vec3){0.9f, 0.7f, 0.4f};
    return d;
}

struct ri_planet_desc ri_earth_get_desc(void) {
    struct ri_planet_desc d = { 0 };
    d.name = "Earth";
    d.type = RI_PLANET_TERRESTRIAL;
    d.mass = 5.9722e24;
    d.radius = 6.371e6;
    d.semimajor_axis = 1.496e11;
    d.eccentricity = 0.0167;
    d.orbital_period = 31558149.76;
    d.base_color = (struct ri_vec3){0.2f, 0.5f, 0.9f};
    return d;
}

struct ri_planet_desc ri_moon_get_desc(void) {
    struct ri_planet_desc d = { 0 };
    d.name = "Moon";
    d.type = RI_PLANET_TERRESTRIAL;
    d.mass = 7.342e22;
    d.radius = 1.7374e6;
    d.semimajor_axis = 3.844e8;
    d.eccentricity = 0.0549;
    d.orbital_period = 2360591.5;
    d.base_color = (struct ri_vec3){0.8f, 0.8f, 0.8f};
    return d;
}

struct ri_planet_desc ri_mars_get_desc(void) {
    struct ri_planet_desc d = { 0 };
    d.name = "Mars";
    d.type = RI_PLANET_TERRESTRIAL;
    d.mass = 6.4171e23;
    d.radius = 3.3895e6;
    d.semimajor_axis = 2.2792e11;
    d.eccentricity = 0.0934;
    d.orbital_period = 59355072.0;
    d.base_color = (struct ri_vec3){0.9f, 0.3f, 0.2f};
    return d;
}

struct ri_planet_desc ri_jupiter_get_desc(void) {
    struct ri_planet_desc d = { 0 };
    d.name = "Jupiter";
    d.type = RI_PLANET_GAS_GIANT;
    d.mass = 1.8982e27;
    d.radius = 6.9911e7;
    d.semimajor_axis = 7.7857e11;
    d.eccentricity = 0.0489;
    d.orbital_period = 374335776.0;
    d.base_color = (struct ri_vec3){0.8f, 0.6f, 0.4f};
    return d;
}

struct ri_planet_desc ri_saturn_get_desc(void) {
    struct ri_planet_desc d = { 0 };
    d.name = "Saturn";
    d.type = RI_PLANET_GAS_GIANT;
    d.mass = 5.6834e26;
    d.radius = 5.8232e7;
    d.semimajor_axis = 1.4335e12;
    d.eccentricity = 0.0565;
    d.orbital_period = 929596608.0;
    d.base_color = (struct ri_vec3){0.9f, 0.8f, 0.5f};
    return d;
}

struct ri_planet_desc ri_uranus_get_desc(void) {
    struct ri_planet_desc d = { 0 };
    d.name = "Uranus";
    d.type = RI_PLANET_ICE_GIANT;
    d.mass = 8.6810e25;
    d.radius = 2.5362e7;
    d.semimajor_axis = 2.8725e12;
    d.eccentricity = 0.0463;
    d.orbital_period = 2651486400.0;
    d.base_color = (struct ri_vec3){0.4f, 0.8f, 0.9f};
    return d;
}

struct ri_planet_desc ri_neptune_get_desc(void) {
    struct ri_planet_desc d = { 0 };
    d.name = "Neptune";
    d.type = RI_PLANET_ICE_GIANT;
    d.mass = 1.02413e26;
    d.radius = 2.4622e7;
    d.semimajor_axis = 4.4951e12;
    d.eccentricity = 0.0095;
    d.orbital_period = 5200418560.0;
    d.base_color = (struct ri_vec3){0.2f, 0.4f, 0.9f};
    return d;
}

struct ri_planet_desc ri_pluto_get_desc(void) {
    struct ri_planet_desc d = { 0 };
    d.name = "Pluto";
    d.type = RI_PLANET_DWARF;
    d.mass = 1.303e22;
    d.radius = 1.1883e6;
    d.semimajor_axis = 5.9064e12;
    d.eccentricity = 0.2488;
    d.orbital_period = 7824384000.0;
    d.base_color = (struct ri_vec3){0.6f, 0.5f, 0.4f};
    return d;
}
