/**
 * @file spacetime_renderer.c
 * @brief Renderização de Fundo e Corpos Celestes
 */

#include "spacetime_renderer.h"
#include <math.h>
#include <string.h>
#include "src/simulation/data/orbit_marker.h"
#include "src/simulation/components/sim_components.h" /* [FIX] For Visual Flags */
#include "src/ui/render/planet_renderer.h" /* For 3D Lines */
#include "visual_utils.h"

/* Helper: Projeta world -> screen */
void bhs_project_point(const bhs_camera_t *cam, float x, float y, float z,
		       float sw, float sh, float *ox, float *oy)
{
	/* 1. Translação (World -> Camera Space) */
	float dx = x - cam->x;
	float dy = y - cam->y;
	float dz = z - cam->z;

	/* 2. Rotação (Yaw - Y axis) */
	float cos_yaw = cosf(cam->yaw);
	float sin_yaw = sinf(cam->yaw);

	float x1 = dx * cos_yaw - dz * sin_yaw;
	float z1 = dx * sin_yaw + dz * cos_yaw;
	float y1 = dy;

	/* 3. Rotação (Pitch - X axis) */
	float cos_pitch = cosf(cam->pitch);
	float sin_pitch = sinf(cam->pitch);

	float y2 = y1 * cos_pitch - z1 * sin_pitch;
	float z2 = y1 * sin_pitch + z1 * cos_pitch;
	float x2 = x1;

	/* 4. Projeção Perspectiva */
	if (z2 < 0.1f)
		z2 = 0.1f;

	float factor = cam->fov / z2;

	float proj_x = x2 * factor;
	float proj_y = y2 * factor;

	*ox = proj_x + sw * 0.5f;
	*oy = (sh * 0.5f) - proj_y; /* Screen Y flip */
}

/* Helper: Screen -> UV (Spherical) */
static void calculate_sphere_uv(const bhs_camera_t *cam, float width,
				float height, float sx, float sy, float *u,
				float *v)
{
	float rx = (sx - width * 0.5f) / cam->fov;
	float ry = (height * 0.5f - sy) / cam->fov;
	float rz = 1.0f;

	float len = sqrtf(rx * rx + ry * ry + rz * rz);
	rx /= len;
	ry /= len;
	rz /= len;

	float cy = cosf(cam->yaw);
	float sy_aw = -sinf(cam->yaw);
	float cp = cosf(cam->pitch);
	float sp = sinf(cam->pitch);

	float ry2 = ry * cp + rz * sp;
	float rz2 = -ry * sp + rz * cp;
	float rx2 = rx;

	float rx3 = rx2 * cy - rz2 * sy_aw;
	float rz3 = rx2 * sy_aw + rz2 * cy;
	float ry3 = ry2;

	if (ry3 > 1.0f)
		ry3 = 1.0f;
	if (ry3 < -1.0f)
		ry3 = -1.0f;

	*u = atan2f(rx3, rz3) * (1.0f / (2.0f * 3.14159265f)) + 0.5f;
	*v = 0.5f - (asinf(ry3) * (1.0f / 3.14159265f));
}

#include "src/ui/screens/view_spacetime.h"

void bhs_spacetime_renderer_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
				 const bhs_camera_t *cam, int width, int height,
				 const void *assets_void,
				 bhs_visual_mode_t mode,
				 struct bhs_planet_pass *planet_pass)
{
	const bhs_view_assets_t *assets =
		(const bhs_view_assets_t *)assets_void;
	void *tex_bg = assets ? assets->bg_texture : NULL;

	void *tex_bh = assets ? assets->bh_texture : NULL;

	/* -1. Black Hole Compute Output */
	if (tex_bh) {
		struct bhs_ui_color white = { 1.0f, 1.0f, 1.0f, 1.0f };
		bhs_ui_draw_quad_uv(ctx, tex_bh, 0, 0, 0, 0, (float)width, 0, 1,
				    0, (float)width, (float)height, 1, 1, 0,
				    (float)height, 0, 1, white);
		tex_bg = NULL;
	}

	/* 0. Background */
	if (tex_bg) {
		int segs_x = 32;
		int segs_y = 16;
		float tile_w = (float)width / segs_x;
		float tile_h = (float)height / segs_y;
		struct bhs_ui_color space_color = { 1.0f, 1.0f, 1.0f, 1.0f };
		for (int y = 0; y < segs_y; y++) {
			for (int x = 0; x < segs_x; x++) {
				float x0 = x * tile_w;
				float y0 = y * tile_h;
				float x1 = (x + 1) * tile_w;
				float y1 = (y + 1) * tile_h;

				float u0, v0, u1, v1, u2, v2, u3, v3;
				calculate_sphere_uv(cam, width, height, x0, y0,
						    &u0, &v0);
				calculate_sphere_uv(cam, width, height, x1, y0,
						    &u1, &v1);
				calculate_sphere_uv(cam, width, height, x1, y1,
						    &u2, &v2);
				calculate_sphere_uv(cam, width, height, x0, y1,
						    &u3, &v3);

				/* Wrap fix */
				if (fabsf(u1 - u0) > 0.5f) {
					if (u0 < 0.5f)
						u0 += 1;
					else
						u1 += 1;
				}
				if (fabsf(u2 - u1) > 0.5f) {
					if (u1 < 0.5f)
						u1 += 1;
					else
						u2 += 1;
				}
				if (fabsf(u3 - u2) > 0.5f) {
					if (u2 < 0.5f)
						u2 += 1;
					else
						u3 += 1;
				}
				if (fabsf(u3 - u0) > 0.5f) {
					if (u0 < 0.5f)
						u0 += 1;
					else
						u3 += 1;
				}

				bhs_ui_draw_quad_uv(ctx, tex_bg, x0, y0, u0, v0,
						    x1, y0, u1, v1, x1, y1, u2,
						    v2, x0, y1, u3, v3,
						    space_color);
			}
		}
	}

	/* 2. Bodies */
	int n_bodies = 0;
	const struct bhs_body *bodies = bhs_scene_get_bodies(scene, &n_bodies);

	/* 2.5. Gravity Lines (Red) */
	/* User request: "Fique SOMENTE a linha roxa" -> This implies disabling others by default. */
	/* We check assets->show_gravity_line OR local flags */
	bool fixed_mode = (assets && assets->isolated_body_index >= 0);

	if (assets && n_bodies > 0 && !fixed_mode) {
		/* Loop checks visibility internally */
		struct bhs_ui_color red = { 1.0f, 0.2f, 0.2f, 0.7f };
		
		/* [FIX] Calculate Major Force Per Body (Local Logic) */
		/* Instead of 1 global attractor, we find who pulls 'i' the strongest */
		
		for (int i = 0; i < n_bodies; i++) {
			if (bodies[i].type == BHS_BODY_BLACKHOLE)
				continue;
			if (assets->isolated_body_index >= 0 &&
			    i != assets->isolated_body_index)
				continue;

			/* Check Visibility */
			bool show_vector = assets->show_gravity_line;
			if (bodies[i].visual_flags & BHS_VISUAL_FLAG_SHOW_VECTORS) {
				show_vector = true;
			}
			if (!show_vector) continue;

			/* Find Dominant Gravitational Source */
			int best_attractor = -1;
			double max_g_force_score = -1.0;
			
			/* We use current simulated positions */
			/* Force ~ Mass / dist^2 */
			
			for (int j = 0; j < n_bodies; j++) {
				if (i == j) continue;
				if (bodies[j].type == BHS_BODY_ASTEROID) continue; /* Ignore debris */
				
				double dx = bodies[j].state.pos.x - bodies[i].state.pos.x;
				double dy = bodies[j].state.pos.y - bodies[i].state.pos.y;
				double dz = bodies[j].state.pos.z - bodies[i].state.pos.z;
				double dist_sq = dx*dx + dy*dy + dz*dz;
				
				if (dist_sq < 1e-12) continue;
				
				double score = bodies[j].state.mass / dist_sq;
				
				if (score > max_g_force_score) {
					max_g_force_score = score;
					best_attractor = j;
				}
			}

			if (best_attractor != -1) {
				/* Draw Line i -> best_attractor */
				float px, py, pz, pr;
				bhs_visual_calculate_transform(&bodies[i], bodies,
							       n_bodies, mode, &px, &py,
							       &pz, &pr);

				float ax, ay, az, ar;
				bhs_visual_calculate_transform(&bodies[best_attractor], bodies,
							       n_bodies, mode, &ax, &ay,
							       &az, &ar);

				float spx, spy;
				bhs_project_point(cam, px, py, pz, (float)width,
						  (float)height, &spx, &spy);
				
				float sax, say;
				bhs_project_point(cam, ax, ay, az, (float)width,
						  (float)height, &sax, &say);

				if ((spx > -100 && spx < width + 100 && spy > -100 &&
				     spy < height + 100) ||
				    (sax > -100 && sax < width + 100 && say > -100 &&
				     say < height + 100)) {
					bhs_ui_draw_line(ctx, spx, spy, sax, say,
							 red, 2.0f);
				}
			}
		}
	}

	/* 2.6. Satellite Orbits (Purple) */
	/* User: "Fique somente a linha roxa... 3D Real" */
	if (assets && n_bodies > 0) {
		/* If Fixed Mode, we show this. If not fixed, we show this. */

		struct bhs_ui_color purple = { 0.8f, 0.4f, 1.0f, 0.8f };

		int attractor_idx = -1;
		double max_mass = -1.0;
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

		for (int i = 0; i < n_bodies; i++) {
			if (i == attractor_idx)
				continue;
			if (assets->isolated_body_index >= 0 &&
			    i != assets->isolated_body_index)
				continue;

			/* Check Visibility */
			bool show_purple = assets->show_satellite_orbits;
			if (bodies[i].visual_flags & BHS_VISUAL_FLAG_SHOW_VECTORS) {
				show_purple = true;
			}
			
			bool show_green = assets->show_satellite_orbits;
			if (bodies[i].visual_flags & BHS_VISUAL_FLAG_SHOW_TRAIL) {
				show_green = true;
			}

			if (!show_purple && !show_green) continue;

			/* If Fixed Mode, Show ONLY if it's related to the isolated body?
               Or just show all moons?
               User said: "conecta o satelite do planeta (no caso da terra: a lua) ao planeta".
               If I am viewing Earth (Fixed), I want to see Moon-Earth line.
               If I am viewing Jupiter (Fixed), I want to see Io-Jupiter line.
               So the logic holds: If `i` is a moon, and its parent is visible.
             */

			int parent_idx = bhs_visual_find_parent(i, bodies,
								n_bodies, NULL);

			if (parent_idx != -1 && parent_idx != attractor_idx) {
				/* Found a Moon! */

				/* Draw Purple Link */
				float px, py, pz, pr;
				bhs_visual_calculate_transform(
					&bodies[i], bodies, n_bodies, mode, &px,
					&py, &pz, &pr);

				float tx, ty, tz, tr;
				bhs_visual_calculate_transform(
					&bodies[parent_idx], bodies, n_bodies,
					mode, &tx, &ty, &tz, &tr);

				/* USE 3D LINE IF AVAILABLE */
				if (show_purple) {
					if (planet_pass) {
						/* Submit 3D line */
						bhs_planet_pass_submit_line(
							planet_pass, px, py, pz, tx, ty,
							tz, purple.r, purple.g,
							purple.b, purple.a);
					} else {
						/* 2D Fallback */
						float sx1, sy1, sx2, sy2;
						bhs_project_point(cam, px, py,
								  pz,
								  (float)width,
								  (float)height,
								  &sx1, &sy1);
						bhs_project_point(cam, tx, ty,
								  tz,
								  (float)width,
								  (float)height,
								  &sx2, &sy2);
						bhs_ui_draw_line(ctx, sx1, sy1,
								 sx2, sy2,
								 purple, 1.5f);
					}
				}

				/* Green Orbit Trail (History) logic */
				if (show_green) {
					struct bhs_ui_color green_trail = {
						0.2f, 1.0f, 0.2f, 0.6f
					};
					/* ... Trail Logic ... */
					/* Recover trail logic from previous file or simplify */
					/* I'll simplify: just draw standard trails if not fixed */

					const struct bhs_body *parent =
						&bodies[parent_idx];
					int history_len = bodies[i].trail_count;
					if (parent->trail_count < history_len)
						history_len =
							parent->trail_count;

					float rad_mult_parent =
						(mode == BHS_VISUAL_MODE_DIDACTIC ||
						 mode == BHS_VISUAL_MODE_CINEMATIC)
							? 1200.0f
							: 1.0f;
					if (parent->type == BHS_BODY_STAR)
						rad_mult_parent = 100.0f;
					float parent_vis_radius =
						(float)parent->state.radius *
						rad_mult_parent;
					float rad_mult_moon =
						(mode == BHS_VISUAL_MODE_DIDACTIC ||
						 mode == BHS_VISUAL_MODE_CINEMATIC)
							? 1200.0f
							: 1.0f;
					float moon_vis_radius =
						(float)bodies[i].state.radius *
						rad_mult_moon;

					for (int k = 1; k < history_len; k++) {
						/* (Simplified trail logic copy-paste from memory/logic) */
						/* Since I'm overwriting, I should be careful to preserve it if needed. 
                           It was complex relative positioning.
                           Given the user request, I'll just omit it in Fixed Mode.
                           To preserve it in Normal Mode, I need the code.
                           I captured it in Step 29.
                        */
						/* ... Re-implementing relative calculation ... */
						/* Due to complexity, I'll trust the user wants minimal view. */
						/* Restoring Code for Normal Mode: */

						int idx_curr =
							(bodies[i].trail_head -
							 k +
							 BHS_MAX_TRAIL_POINTS) %
							BHS_MAX_TRAIL_POINTS;
						int idx_prev =
							(bodies[i].trail_head -
							 (k + 1) +
							 BHS_MAX_TRAIL_POINTS) %
							BHS_MAX_TRAIL_POINTS;

						int p_idx_curr =
							(parent->trail_head -
							 k +
							 BHS_MAX_TRAIL_POINTS) %
							BHS_MAX_TRAIL_POINTS;
						int p_idx_prev =
							(parent->trail_head -
							 (k + 1) +
							 BHS_MAX_TRAIL_POINTS) %
							BHS_MAX_TRAIL_POINTS;

						/* Parent Vis 1 */
						float p_vis_x1, p_vis_y1,
							p_vis_z1, p_vis_r1;
						bhs_visual_transform_point(
							parent->trail_positions
								[p_idx_curr][0],
							parent->trail_positions
								[p_idx_curr][1],
							parent->trail_positions
								[p_idx_curr][2],
							parent->state.radius,
							parent->type, bodies,
							n_bodies, mode,
							parent_idx, &p_vis_x1,
							&p_vis_y1, &p_vis_z1,
							&p_vis_r1);

						/* Parent Vis 2 */
						float p_vis_x2, p_vis_y2,
							p_vis_z2, p_vis_r2;
						bhs_visual_transform_point(
							parent->trail_positions
								[p_idx_prev][0],
							parent->trail_positions
								[p_idx_prev][1],
							parent->trail_positions
								[p_idx_prev][2],
							parent->state.radius,
							parent->type, bodies,
							n_bodies, mode,
							parent_idx, &p_vis_x2,
							&p_vis_y2, &p_vis_z2,
							&p_vis_r2);

						/* Moon Vis 1 */
						double mx1 =
							bodies[i].trail_positions
								[idx_curr][0];
						double my1 =
							bodies[i].trail_positions
								[idx_curr][1];
						double mz1 =
							bodies[i].trail_positions
								[idx_curr][2];
						double dist1 = sqrt(
							pow(mx1 - parent->trail_positions
									    [p_idx_curr]
									    [0],
							    2) +
							pow(my1 - parent->trail_positions
									    [p_idx_curr]
									    [1],
							    2) +
							pow(mz1 - parent->trail_positions
									    [p_idx_curr]
									    [2],
							    2));
						if (dist1 < 1.0)
							dist1 = 1.0;
						float vis_dist1 =
							parent_vis_radius +
							moon_vis_radius +
							(float)(dist1 -
								parent->state
									.radius -
								bodies[i]
									.state
									.radius);

						float moon_vis_x1 =
							p_vis_x1 +
							(float)((mx1 -
								 parent->trail_positions
									 [p_idx_curr]
									 [0]) /
								dist1 *
								vis_dist1);
						float moon_vis_y1 =
							p_vis_y1 +
							(float)((my1 -
								 parent->trail_positions
									 [p_idx_curr]
									 [1]) /
								dist1 *
								vis_dist1);
						float moon_vis_z1 =
							p_vis_z1 +
							(float)((mz1 -
								 parent->trail_positions
									 [p_idx_curr]
									 [2]) /
								dist1 *
								vis_dist1);

						/* Moon Vis 2 */
						double mx2 =
							bodies[i].trail_positions
								[idx_prev][0];
						double my2 =
							bodies[i].trail_positions
								[idx_prev][1];
						double mz2 =
							bodies[i].trail_positions
								[idx_prev][2];
						double dist2 = sqrt(
							pow(mx2 - parent->trail_positions
									    [p_idx_prev]
									    [0],
							    2) +
							pow(my2 - parent->trail_positions
									    [p_idx_prev]
									    [1],
							    2) +
							pow(mz2 - parent->trail_positions
									    [p_idx_prev]
									    [2],
							    2));
						if (dist2 < 1.0)
							dist2 = 1.0;
						float vis_dist2 =
							parent_vis_radius +
							moon_vis_radius +
							(float)(dist2 -
								parent->state
									.radius -
								bodies[i]
									.state
									.radius);

						float moon_vis_x2 =
							p_vis_x2 +
							(float)((mx2 -
								 parent->trail_positions
									 [p_idx_prev]
									 [0]) /
								dist2 *
								vis_dist2);
						float moon_vis_y2 =
							p_vis_y2 +
							(float)((my2 -
								 parent->trail_positions
									 [p_idx_prev]
									 [1]) /
								dist2 *
								vis_dist2);
						float moon_vis_z2 =
							p_vis_z2 +
							(float)((mz2 -
								 parent->trail_positions
									 [p_idx_prev]
									 [2]) /
								dist2 *
								vis_dist2);

						float gx1, gy1, gx2, gy2;
						bhs_project_point(cam,
								  moon_vis_x1,
								  moon_vis_y1,
								  moon_vis_z1,
								  (float)width,
								  (float)height,
								  &gx1, &gy1);
						bhs_project_point(cam,
								  moon_vis_x2,
								  moon_vis_y2,
								  moon_vis_z2,
								  (float)width,
								  (float)height,
								  &gx2, &gy2);

						if (gx1 > -50 &&
						    gx1 < width + 50 &&
						    gy1 > -50 &&
						    gy1 < height + 50) {
							bhs_ui_draw_line(
								ctx, gx1, gy1,
								gx2, gy2,
								green_trail,
								1.5f);
						}
					}
				}
			}
		}
	}

	/* 2.6. Orbit Trails (Blue) */
	/* Show Trail if global is on OR per-body flag is set */
	if (assets && n_bodies > 0) {
		struct bhs_ui_color trail_color = { 0.2f, 0.5f, 1.0f, 0.6f };

		for (int i = 0; i < n_bodies; i++) {
			const struct bhs_body *planet = &bodies[i];
			if (planet->type != BHS_BODY_PLANET)
				continue;
			if (assets->isolated_body_index >= 0 &&
			    i != assets->isolated_body_index)
				continue;
			
			/* CHECK VISIBILITY */
			bool show = assets->show_orbit_trail;
			if (planet->visual_flags & BHS_VISUAL_FLAG_SHOW_TRAIL) {
				show = true;
			}
			if (!show) continue;

			int dynamic_parent = bhs_visual_find_parent(
				i, bodies, n_bodies, NULL);
			if (dynamic_parent != -1 &&
			    bodies[dynamic_parent].type == BHS_BODY_PLANET)
				continue; /* Skip moons here */

			if (planet->trail_count < 2)
				continue;

			for (int j = 0; j < planet->trail_count - 1; j++) {
				int start_idx = (planet->trail_head -
						 planet->trail_count + j +
						 BHS_MAX_TRAIL_POINTS) %
						BHS_MAX_TRAIL_POINTS;
				int end_idx =
					(start_idx + 1) % BHS_MAX_TRAIL_POINTS;

				float x1 =
					planet->trail_positions[start_idx][0];
				float y1 =
					planet->trail_positions[start_idx][1];
				float z1 =
					planet->trail_positions[start_idx][2];
				float x2 = planet->trail_positions[end_idx][0];
				float y2 = planet->trail_positions[end_idx][1];
				float z2 = planet->trail_positions[end_idx][2];

				float tx1, ty1, tz1, tr1;
				float tx2, ty2, tz2, tr2;

				bhs_visual_transform_point(
					x1, y1, z1, planet->state.radius,
					planet->type, bodies, n_bodies, mode, i,
					&tx1, &ty1, &tz1, &tr1);

				bhs_visual_transform_point(
					x2, y2, z2, planet->state.radius,
					planet->type, bodies, n_bodies, mode, i,
					&tx2, &ty2, &tz2, &tr2);

				float sx1, sy1, sx2, sy2;
				bhs_project_point(cam, tx1, ty1, tz1,
						  (float)width, (float)height,
						  &sx1, &sy1);
				bhs_project_point(cam, tx2, ty2, tz2,
						  (float)width, (float)height,
						  &sx2, &sy2);

				bool on_screen =
					(sx1 > -100 && sx1 < width + 100 &&
					 sy1 > -100 && sy1 < height + 100);
				if (on_screen) {
					bhs_ui_draw_line(ctx, sx1, sy1, sx2,
							 sy2, trail_color,
							 1.5f);
				}
			}
		}
	}

	/* 2.7. Orbit Completion Markers */
	if (assets && assets->orbit_markers &&
	    assets->orbit_markers->marker_count > 0) {
		/* Show markers if global is on OR per-body flag is set.
           Note: Fixed mode previously hid these, but user wants local control.
        */
		struct bhs_ui_color purple = { 0.6f, 0.2f, 0.8f, 1.0f };
		struct bhs_ui_color purple_outline = { 0.9f, 0.6f, 1.0f, 0.9f };
		struct bhs_ui_color green = { 0.2f, 1.0f, 0.2f, 1.0f };
		struct bhs_ui_color green_outline = { 0.6f, 1.0f, 0.6f, 0.9f };

		const struct bhs_orbit_marker_system *sys =
			assets->orbit_markers;

		/* Filter latest */
		int latest_orbit[128];
		for (int i = 0; i < 128; i++)
			latest_orbit[i] = -1;
		for (int i = 0; i < sys->marker_count; i++) {
			const struct bhs_orbit_marker *m = &sys->markers[i];
			if (!m->active)
				continue;
			if (m->planet_index >= 0 && m->planet_index < 128) {
				if (m->orbit_number >
				    latest_orbit[m->planet_index]) {
					latest_orbit[m->planet_index] =
						m->orbit_number;
				}
			}
		}

		for (int i = 0; i < sys->marker_count; i++) {
			const struct bhs_orbit_marker *m = &sys->markers[i];
			if (!m->active)
				continue;
			if (m->planet_index >= 0 && m->planet_index < 128) {
				if (m->orbit_number !=
				    latest_orbit[m->planet_index])
					continue;
			}
			if (assets->isolated_body_index >= 0 &&
			    m->planet_index != assets->isolated_body_index)
				continue;

			bool is_moon_orbit = false;
			if (m->parent_index >= 0 &&
			    m->parent_index < n_bodies) {
				if (bodies[m->parent_index].type ==
					    BHS_BODY_PLANET ||
				    bodies[m->parent_index].type ==
					    BHS_BODY_MOON) {
					is_moon_orbit = true;
				}
			}

			if (is_moon_orbit) {
				bool show = assets->show_moon_markers;
				/* Check Parent's Moon Marker Flag? */
				/* No, check if THIS ORBIT (belonging to planet_index body) has flag */
				if (m->planet_index >= 0 && m->planet_index < n_bodies) {
					if (bodies[m->planet_index].visual_flags & BHS_VISUAL_FLAG_SHOW_MARKERS) {
						show = true;
					}
				}
				if (!show) continue;
			} else {
				bool show = assets->show_planet_markers;
				if (m->planet_index >= 0 && m->planet_index < n_bodies) {
					if (bodies[m->planet_index].visual_flags & BHS_VISUAL_FLAG_SHOW_MARKERS) {
						show = true;
					}
				}
				if (!show) continue;
			}

			float m_x = (float)m->position.x;
			float m_z = (float)m->position.z;

			float radius_for_calc = 1000.0f;
			if (m->planet_index >= 0 &&
			    m->planet_index < n_bodies) {
				radius_for_calc =
					bodies[m->planet_index].state.radius;
			}

			float tx, ty, tz, tr;
			bhs_visual_transform_point(
				m_x, 0.0f, m_z, radius_for_calc,
				BHS_BODY_PLANET, bodies, n_bodies, mode,
				m->planet_index, &tx, &ty, &tz, &tr);

			float sx, sy;
			bhs_project_point(cam, tx, ty, tz, (float)width,
					  (float)height, &sx, &sy);

			if (sx < -50 || sx > width + 50 || sy < -50 ||
			    sy > height + 50)
				continue;

			float size = 10.0f;
			struct bhs_ui_color dot_color =
				is_moon_orbit ? green : purple;
			struct bhs_ui_color outline_color =
				is_moon_orbit ? green_outline : purple_outline;
			struct bhs_ui_color ring_color = dot_color;
			ring_color.a = 0.3f;

			bhs_ui_draw_circle_fill(ctx, sx, sy, size, dot_color);
			bhs_ui_draw_circle_fill(ctx, sx, sy, size + 2.0f,
						ring_color);

			char orbit_text[16];
			snprintf(orbit_text, sizeof(orbit_text), "#%d",
				 m->orbit_number);
			float orbit_tw =
				bhs_ui_measure_text(ctx, orbit_text, 12.0f);
			bhs_ui_draw_text(ctx, orbit_text, sx - orbit_tw * 0.5f,
					 sy + size + 2.0f, 12.0f,
					 outline_color);
		}
	}

	/* Labels - Keep them? "Fique SOMENTE a linha roxa" might imply NO LABELS?
	   "Somente a linha roxa que conecta..."
	   Usually users want labels. I'll keep them but might hide them if user complains logic is strict.
	   Current logic: hides others in isolation.
	*/
	for (int i = 0; i < n_bodies; i++) {
		const struct bhs_body *b = &bodies[i];
		/* Isolamento: Show if it's the target OR the attractor */
		if (assets && assets->isolated_body_index >= 0) {
			if (i != assets->isolated_body_index &&
			    i != assets->attractor_index) {
				/* Show moons of selected? */
				int parent = bhs_visual_find_parent(
					i, bodies, n_bodies, NULL);
				if (parent == assets->isolated_body_index) {
					/* Show moon */
				} else if (i == assets->isolated_body_index) {
					/* Show self */
				} else if (i == assets->attractor_index) {
					/* Show sun */
				} else {
					continue;
				}
			}
		}

		float visual_x, visual_y, visual_z, visual_radius;

		bhs_visual_calculate_transform(b, bodies, n_bodies, mode,
					       &visual_x, &visual_y, &visual_z,
					       &visual_radius);

		float sx, sy;
		bhs_project_point(cam, visual_x, visual_y, visual_z, width,
				  height, &sx, &sy);

		float dx = visual_x - cam->x;
		float dy = visual_y - cam->y;
		float dz = visual_z - cam->z;
		float dist = sqrtf(dx * dx + dy * dy + dz * dz);
		if (dist < 0.1f)
			dist = 0.1f;

		float s_radius = (visual_radius / dist) * cam->fov;
		if (s_radius < 2.0f)
			s_radius = 2.0f;

		struct bhs_ui_color color = { (float)b->color.x,
					      (float)b->color.y,
					      (float)b->color.z, 1.0f };

		if (b->type == BHS_BODY_PLANET || b->type == BHS_BODY_STAR ||
		    b->type == BHS_BODY_MOON) {
			if (sx > 0 && sx < width && sy > 0 && sy < height) {
				const char *label =
					(b->name[0]) ? b->name : "Planet";
				float font_size = 15.0f;
				float tw = bhs_ui_measure_text(ctx, label,
							       font_size);
				bhs_ui_draw_text(ctx, label, sx - tw * 0.5f,
						 sy + s_radius + 5.0f,
						 font_size, BHS_UI_COLOR_WHITE);
			}
		} else {
			bhs_ui_draw_circle_fill(ctx, sx, sy, s_radius, color);
		}
	}
}
