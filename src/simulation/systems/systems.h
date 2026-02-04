#ifndef SIMULATION_SYSTEMS_H
#define SIMULATION_SYSTEMS_H

#include "engine/ecs/ecs.h"

/* Unified Physics System (uses Integrator.c) */
void physics_system_update(bhs_world_handle world, double dt);

#include "simulation/systems/celestial_system.h"

#endif
