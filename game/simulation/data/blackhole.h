#ifndef RI_SIMULATION_DATA_BLACKHOLE_H
#define RI_SIMULATION_DATA_BLACKHOLE_H

#include "engine/math/vec4.h"

struct ri_blackhole_desc {
    const char *name;
    double mass;
    double spin;              /* a parameter: 0..1 */
    double isco_radius;
    double photon_sphere_r;
    double event_horizon_r;
    double accretion_disk_inner;
    double accretion_disk_outer;
    double accretion_disk_mass;
    struct ri_vec3 position;
    struct ri_vec3 base_color;
};

#endif /* RI_SIMULATION_DATA_BLACKHOLE_H */
