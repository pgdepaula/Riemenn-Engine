#ifndef RI_UX_UI_VIEW_SPACETIME_H
#define RI_UX_UI_VIEW_SPACETIME_H

#include "engine/scene/scene.h"
#include "engine/rhi/rhi.h"
#include "engine/ui/ui.h"
#include "engine/render/camera.h"

/* Render Modes */
typedef enum {
	RI_VISUAL_MODE_SCIENTIFIC = 0,
	RI_VISUAL_MODE_DIDACTIC = 1,
	RI_VISUAL_MODE_CINEMATIC = 2
} ri_visual_mode_t;

/* === View === */

/* Proxy to ri_camera_init */
void ri_camera_init_view(ri_camera_t *cam);

/* Proxy to ri_camera_controller_update */
void ri_camera_update_view(ri_camera_t *cam, ri_ui_ctx_t ctx, double dt);

struct ri_planet_pass; /* Forward Declaration */

struct ri_planet_tex_entry {
	char name[32];
	void *tex;
};

/* Asset container to avoid void* hacks */
typedef struct ri_view_assets {
	void *bg_texture;
	void *sphere_texture;
	void *bh_texture; /* Black Hole Compute Result */

	/* Procedural Cache */
	const struct ri_planet_tex_entry *tex_cache;
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
	const struct ri_orbit_marker_system *orbit_markers;
	/* [NEW] Detailed visual control */
	bool show_planet_markers; /* Purple */
	bool show_moon_markers;	  /* Green */

	/* [NEW] Interpolation Alpha (accumulator) */
	double sim_alpha;

	/* [NEW] Strongest Attractor Index (for isolation context) */
	int attractor_index; /* -1 if none */
} ri_view_assets_t;

/* Proxy to renderer */
/* Proxy to renderer */
void ri_view_spacetime_draw(ri_ui_ctx_t ctx, ri_scene_t scene,
			     const ri_camera_t *cam, int width, int height,
			     const ri_view_assets_t *assets,
			     ri_visual_mode_t mode,
			     struct ri_planet_pass *planet_pass);

#endif /* RI_UX_UI_VIEW_SPACETIME_H */
