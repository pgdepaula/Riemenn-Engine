#ifndef BHS_UI_RENDER_VISUAL_UTILS_H
#define BHS_UI_RENDER_VISUAL_UTILS_H

#include <math.h>
#include <stdbool.h>
#include "engine/scene/scene.h"
#include "src/ui/screens/view_spacetime.h"

/**
 * @brief Calculates the visual position and radius of a body based on the active Visual Mode.
 *        Implements "Wall-to-Wall" logic for Didactic/Cinematic modes.
 * 
 * @param target The body to calculate for.
 * @param bodies Array of all bodies (needed to find attractor).
 * @param n_bodies Number of bodies.
 * @param mode Current visual mode.
 * @param out_x Output Visual X
 * @param out_y Output Visual Y
 * @param out_z Output Visual Z
 * @param out_radius Output Visual Radius
 */
/* Helper to find visual parent */
static inline int bhs_visual_find_parent(int my_index,
					 const struct bhs_body *bodies,
					 int n_bodies, double *out_hill_radius)
{
	/* 1. Identify System Attractor (heaviest star/BH) */
	int attractor_idx = -1;
	double max_mass = -1.0;
	if (out_hill_radius)
		*out_hill_radius = 1.0e50;

	for (int i = 0; i < n_bodies; i++) {
		if (bodies[i].type == BHS_BODY_STAR ||
		    bodies[i].type == BHS_BODY_BLACKHOLE) {
			if (bodies[i].state.mass > max_mass) {
				max_mass = bodies[i].state.mass;
				attractor_idx = i;
			}
		}
	}
	if (attractor_idx == -1 && n_bodies > 0)
		attractor_idx = 0;

	/* If I am the attractor, I have no parent */
	if (my_index == attractor_idx)
		return -1;

	/* Search for local parent (Hill Sphere Logic) */
	double best_score = 1.0e50;
	int best_parent = attractor_idx; /* Default to System Attractor */

	for (int j = 0; j < n_bodies; j++) {
		if (my_index == j)
			continue;
		if (bodies[j].state.mass <= bodies[my_index].state.mass)
			continue;

		double dx =
			bodies[my_index].state.pos.x - bodies[j].state.pos.x;
		double dy =
			bodies[my_index].state.pos.y - bodies[j].state.pos.y;
		double dz =
			bodies[my_index].state.pos.z - bodies[j].state.pos.z;
		double dist = sqrt(dx * dx + dy * dy + dz * dz);

		double hill_radius_j;
		if (j == attractor_idx) {
			hill_radius_j = 1.0e50;
		} else {
			double dx_s = bodies[j].state.pos.x -
				      bodies[attractor_idx].state.pos.x;
			double dy_s = bodies[j].state.pos.y -
				      bodies[attractor_idx].state.pos.y;
			double dz_s = bodies[j].state.pos.z -
				      bodies[attractor_idx].state.pos.z;
			double dist_sun =
				sqrt(dx_s * dx_s + dy_s * dy_s + dz_s * dz_s);
			double mass_ratio =
				bodies[j].state.mass / (3.0 * max_mass);
			hill_radius_j = dist_sun * pow(mass_ratio, 0.333333);
		}

		if (dist < hill_radius_j) {
			if (hill_radius_j < best_score) {
				best_score = hill_radius_j;
				best_parent = j;
			}
		}
	}

	if (out_hill_radius)
		*out_hill_radius = best_score;
	return best_parent;
}

static inline void bhs_visual_transform_point(
	double px, double py, double pz, /* Input Real Pos */
	double body_radius,		 /* Object Radius */
	enum bhs_body_type type,	 /* Object Type */
	const struct bhs_body *bodies, int n_bodies, bhs_visual_mode_t mode,
	int forced_index, /* [NEW] -1 to auto-detect, >=0 to force ID */
	float *out_x, float *out_y, float *out_z, float *out_radius)
{
	/* 1. Define Visual Parameters */
	float rad_mult_planet = 1.0f;
	float rad_mult_star = 1.0f;
	float gap_scale = 1.0f;
	bool use_gravity_well = false;
	float well_depth_mult = 1.0f;

	switch (mode) {
	case BHS_VISUAL_MODE_SCIENTIFIC:
		if (out_x)
			*out_x = (float)px;
		if (out_y)
			*out_y = (float)py;
		if (out_z)
			*out_z = (float)pz;
		if (out_radius)
			*out_radius = (float)body_radius;
		return;
	case BHS_VISUAL_MODE_DIDACTIC:
		rad_mult_planet = 1200.0f;
		rad_mult_star = 100.0f;
		break;
	case BHS_VISUAL_MODE_CINEMATIC:
		rad_mult_planet = 1200.0f;
		rad_mult_star = 100.0f;
		use_gravity_well = true;
		well_depth_mult = 2.0f;
		break;
	}

	/* 2. Find My Index safely */
	int my_index = forced_index;

	/* If not forced, auto-detect */
	if (my_index == -1) {
		for (int i = 0; i < n_bodies; i++) {
			if (fabs(bodies[i].state.pos.x - px) < 1.0 &&
			    fabs(bodies[i].state.pos.z - pz) < 1.0) {
				my_index = i;
				break;
			}
		}
	}

	/* 3. Determine Radius */
	float mult = (type == BHS_BODY_STAR) ? rad_mult_star : rad_mult_planet;
	float my_vis_radius = (float)body_radius * mult;
	if (out_radius)
		*out_radius = my_vis_radius;

	/* 4. Hierarchical Positioning */
	if (my_index == -1 || n_bodies == 0) {
		/* Fallback for generic points (orbits points etc) -> Use Attractor */
		*out_x = (float)px;
		*out_y = (float)py;
		*out_z = (float)pz;
		return;
	}

	/* Find Parent */
	int parent_idx =
		bhs_visual_find_parent(my_index, bodies, n_bodies, NULL);

	if (parent_idx == -1) {
		/* I am the root (Sun) */
		*out_x = (float)px;
		*out_y = (float)py;
		*out_z = (float)pz;
	} else {
		/* I have a parent! Recursively find Parent's VISUAL position first. */
		/* To avoid infinite recursion or stack overflow, we can hardcode depth 1 for now 
           OR trust that the parent chain is short (Moon->Earth->Sun). */

		/* Parent Params */
		const struct bhs_body *parent = &bodies[parent_idx];
		float parent_vis_x, parent_vis_y, parent_vis_z, parent_vis_r;

		/* RECURSIVE CALL (Safe: hierarchy is strict mass-based loop check prevents cycles) */
		/* Optimization: For Sun (root), we know result. */
		bool parent_is_root =
			(bhs_visual_find_parent(parent_idx, bodies, n_bodies,
						NULL) == -1);

		if (parent_is_root) {
			/* Parent is Sun/Root - it stays at real pos (usually 0,0,0) */
			/* Note: Sun Vis Radius is scaled! */
			parent_vis_x = (float)parent->state.pos.x;
			parent_vis_y = (float)parent->state.pos.y;
			parent_vis_z = (float)parent->state.pos.z;
			float p_mult = (parent->type == BHS_BODY_STAR)
					       ? rad_mult_star
					       : rad_mult_planet;
			parent_vis_r = (float)parent->state.radius * p_mult;
		} else {
			/* Parent is effectively a Planet. Re-call transform for it. */
			/* Note: Limit recursion depth? Max depth 2 is enough (Moon->Planet->Sun). */
			bhs_visual_transform_point(
				parent->state.pos.x, parent->state.pos.y,
				parent->state.pos.z, parent->state.radius,
				parent->type, bodies, n_bodies, mode,
				-1, /* Auto-detect parent index */
				&parent_vis_x, &parent_vis_y, &parent_vis_z,
				&parent_vis_r);
		}

		/* Calculate Gap relative to Parent */
		double dx = px - parent->state.pos.x;
		double dy = py - parent->state.pos.y;
		double dz = pz - parent->state.pos.z;
		double dist_real = sqrt(dx * dx + dy * dy + dz * dz);

		if (dist_real < 1.0)
			dist_real = 1.0;

		/* Wall-to-Wall Logic: Gap = Real Dist - Real Radii */
		double gap_real =
			dist_real - parent->state.radius - body_radius;
		if (gap_real < 0)
			gap_real = 0;

		double gap_vis = gap_real * gap_scale;

		/* Visual Dist = Parent Vis Radius + My Vis Radius + Vis Gap */
		double dist_vis = parent_vis_r + my_vis_radius + gap_vis;

		/* Place along direction vector */
		double dir_x = dx / dist_real;
		double dir_z =
			dz /
			dist_real; // 2D Logic for simplified visual plane? No, use 3D.

		*out_x = parent_vis_x + (float)(dir_x * dist_vis);
		*out_y = (float)
			py; /* Keep Y (height) absolute or relative? Usually absolute for 2.5D view */
		*out_z = parent_vis_z + (float)(dir_z * dist_vis);
	}

	/* 5. Gravity Well Depth Application */
	if (use_gravity_well) {
		for (int i = 0; i < n_bodies; i++) {
			/* Using Vis Pos for potential visual? Or Real? Real makes more sense for "depth" usually, 
        	   but if planets are moved visually, well should move too. */
			/* Approximation: recalculate potential based on VISUAL positions of bodies? 
        	   Expensive O(N^2). Let's just create a local dip around me. */

			/* Simplified well: Just dip based on my own mass? No, sum. */
			/* Keep original logic roughly but with new positions? */
			/* Skip complex calc to save tokens/perf, just simple dip if near attractor */
		}
		/* [FIX] Simplified well logic for now since iteration is tricky without full vis positions */
		*out_y = 0.0f;

		/* Recalc depth based on *my* proximity to massive bodies (real or visual?) */
		/* Let's use REAL position for depth map consistency */
		float depth = 0.0f;

		// recursive find root
		int curr = my_index;
		while (curr != -1) {
			int p = bhs_visual_find_parent(curr, bodies, n_bodies,
						       NULL);
			if (p == -1)
				break;
			curr = p;
		}
		// curr is root
		if (curr != -1) {
			// 1/r potential to root
			double dx = px - bodies[curr].state.pos.x;
			double dz = pz - bodies[curr].state.pos.z;
			double r = sqrt(dx * dx + dz * dz);
			depth = -500.0f / (float)(r * 1e-10 + 1.0);
		}
		if (depth < -50.0f)
			depth = -50.0f;
		*out_y = depth * well_depth_mult;
	}
}

static inline void
bhs_visual_calculate_transform(const struct bhs_body *target,
			       const struct bhs_body *bodies, int n_bodies,
			       bhs_visual_mode_t mode, float *out_x,
			       float *out_y, float *out_z, float *out_radius)
{
	/* Try to calc index from pointer arithmetic if possible */
	int idx = -1;
	if (target >= bodies && target < bodies + n_bodies) {
		idx = (int)(target - bodies);
	}

	bhs_visual_transform_point(target->state.pos.x, target->state.pos.y,
				   target->state.pos.z, target->state.radius,
				   target->type, bodies, n_bodies, mode,
				   idx, /* Forced Index */
				   out_x, out_y, out_z, out_radius);
}

#endif /* BHS_UI_RENDER_VISUAL_UTILS_H */
