#ifndef BHS_UX_UI_VIEW_SPACETIME_H
#define BHS_UX_UI_VIEW_SPACETIME_H

#include "engine/scene/scene.h"
#include "gui/rhi/rhi.h"
#include "gui/ui/lib.h"
#include "src/ui/camera/camera.h"

/* Render Modes */
typedef enum {
	BHS_VISUAL_MODE_SCIENTIFIC = 0,
	BHS_VISUAL_MODE_DIDACTIC = 1,
	BHS_VISUAL_MODE_CINEMATIC = 2
} bhs_visual_mode_t;

/* === View === */

/* Proxy to bhs_camera_init */
void bhs_camera_init_view(bhs_camera_t *cam);

/* Proxy to bhs_camera_controller_update */
void bhs_camera_update_view(bhs_camera_t *cam, bhs_ui_ctx_t ctx, double dt);

struct bhs_planet_pass; /* Forward Declaration */

struct bhs_planet_tex_entry {
	char name[32];
	void *tex;
};

/* Asset container to avoid void* hacks */
typedef struct bhs_view_assets {
	void *bg_texture;
	void *sphere_texture;
	void *bh_texture; /* Black Hole Compute Result */

	/* Procedural Cache */
	const struct bhs_planet_tex_entry *tex_cache;
	int tex_cache_count;

	/* 3D Renderer Status */
	bool render_3d_active;

	/* Gravity Line Visualization */
	bool show_gravity_line;
	int selected_body_index; /* -1 = no selection, show all lines */

	/* Orbit Trail Visualization */
	bool show_orbit_trail;

	/* Satellite Orbits Visualization */
	bool show_satellite_orbits;

	/* [NEW] Isolated View Mode */
	int isolated_body_index; /* -1 = sem isolamento, >= 0 = índice do corpo isolado */

	/* [NEW] Ponteiro para sistema de marcadores de órbita */
	const struct bhs_orbit_marker_system *orbit_markers;
	/* [NEW] Detailed visual control */
	bool show_planet_markers; /* Purple */
	bool show_moon_markers;	  /* Green */

	/* [NEW] Interpolation Alpha (accumulator) */
	double sim_alpha;

	/* [NEW] Strongest Attractor Index (for isolation context) */
	int attractor_index; /* -1 if none */
} bhs_view_assets_t;

/* Proxy to renderer */
/* Proxy to renderer */
void bhs_view_spacetime_draw(bhs_ui_ctx_t ctx, bhs_scene_t scene,
			     const bhs_camera_t *cam, int width, int height,
			     const bhs_view_assets_t *assets,
			     bhs_visual_mode_t mode,
			     struct bhs_planet_pass *planet_pass);

#endif /* BHS_UX_UI_VIEW_SPACETIME_H */
