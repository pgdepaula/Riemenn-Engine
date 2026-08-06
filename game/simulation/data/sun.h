#ifndef RI_SIMULATION_DATA_SUN_H
#define RI_SIMULATION_DATA_SUN_H

#include "planet.h"

struct ri_sun_desc {
    const char *name;
    double mass;
    double radius;
    double luminosity;
    double surface_temp;
    double axis_tilt;
    double rotation_period;
    double temperature;
    double age;
    double metallicity;
    char spectral_type[8];
    int stage;
    struct ri_vec3 color;
    struct ri_vec3 base_color;
};

#endif /* RI_SIMULATION_DATA_SUN_H */
