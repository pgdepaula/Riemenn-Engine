#ifndef RI_SIMULATION_DATA_ORBIT_MARKER_H
#define RI_SIMULATION_DATA_ORBIT_MARKER_H

#include <stdbool.h>
#include <stdint.h>
#include "engine/math/vec4.h"

#define RI_MAX_ORBIT_MARKERS 64

enum ri_orbit_marker_type {
    RI_MARKER_PERIAPSIS,
    RI_MARKER_APOAPSIS,
    RI_MARKER_NODE_ASCENDING,
    RI_MARKER_NODE_DESCENDING,
    RI_MARKER_CUSTOM
};

struct ri_orbit_marker {
    bool active;
    bool is_active;
    int planet_index;
    int parent_index;
    int orbit_number;
    enum ri_orbit_marker_type type;
    struct ri_vec3 pos;
    struct ri_vec3 position;
    double value;
    char label[32];
    char planet_name[32];
    double timestamp_seconds;
    double orbital_period_measured;
};

struct ri_orbit_marker_system {
    struct ri_orbit_marker markers[RI_MAX_ORBIT_MARKERS];
    int marker_count;
};

static inline void ri_orbit_markers_init(struct ri_orbit_marker_system *sys) {
    if (!sys) return;
    sys->marker_count = 0;
    for (int i = 0; i < RI_MAX_ORBIT_MARKERS; i++) {
        sys->markers[i].active = false;
        sys->markers[i].is_active = false;
    }
}

static inline void ri_orbit_markers_update(struct ri_orbit_marker_system *sys, const void *bodies, int count, double dt) {
    (void)sys;
    (void)bodies;
    (void)count;
    (void)dt;
}

static inline int ri_orbit_markers_get_at_screen(const struct ri_orbit_marker_system *sys, float mx, float my, const void *cam, int w, int h) {
    (void)sys; (void)mx; (void)my; (void)cam; (void)w; (void)h;
    return -1;
}

#endif /* RI_SIMULATION_DATA_ORBIT_MARKER_H */
