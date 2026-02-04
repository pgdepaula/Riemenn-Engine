#ifndef BHS_CMD_UI_HUD_H
#define BHS_CMD_UI_HUD_H

#include "gui/ui/lib.h"

#include "engine/scene/scene.h"
#include "src/simulation/data/planet.h"	   // Added as per instruction
#include "src/ui/screens/view_spacetime.h" /* Enum bhs_visual_mode_t */

typedef struct {
	bool show_fps;
	bool vsync_enabled;
	float time_scale_val;  /* [NEW] Slider 0..1 (Logarithmic) */
	int active_menu_index; /* -1 = none, 0=Config, 1=Add, 2=Visual */
	int add_menu_category; /* -1 = Root, 0=Planets, 1=Suns, 2=Moons, 3=BlackHoles */

	/* Visualization Mode */
	bhs_visual_mode_t visual_mode;
	bool top_down_view;	    /* [NEW] */
	bool show_gravity_line;	    /* [NEW] Show line to strongest attractor */
	bool show_orbit_trail;	    /* [NEW] Show blue orbit path */
	bool show_satellite_orbits; /* [NEW] Show Green/Purple for moons */
	bool show_planet_markers;   /* [NEW] Purple Dots (Orbit around Sun) */
	bool show_moon_markers; /* [NEW] Green Dots (Orbit around Planets) */
	bool fixed_planet_cam;	/* [NEW] Câmera fixa planeta-sol */

	/* Selection State */
	int selected_body_index;	     /* -1 = none */
	bool req_refresh_selected_body; /* [NEW] Sync */
	struct bhs_body selected_body_cache; /* Para exibir info sem lock */
	bool isolate_view; /* [NEW] Mostrar apenas o planeta selecionado */
	int selected_marker_index; /* [NEW] -1 = none, seleciona um marco roxo */

	/* Requests to Main Loop */
	bool req_delete_body;
	int req_add_body_type; /* -1 = none */
	const struct bhs_planet_registry_entry
		*req_add_registry_entry; /* NULL = none */

	/* Data for Object Inspector (Calculated in app_state) */
	char attractor_name[64];
	double attractor_dist;

	/* [NEW] Tempo simulado em segundos desde J2000.0 (passado pelo app_state) */
	double sim_time_seconds;
	float current_fps; /* [NEW] FPS calculated by app */

	/* [NEW] Acesso ao sistema de marcadores para exibição de detalhes */
	const struct bhs_orbit_marker_system *orbit_markers_ptr;
	float orbit_history_scroll; /* [NEW] Estado de rolagem do histórico */

	/* [NEW] REFS Panel State */
	bool refs_collapsed;

	/* [NEW] Time Control State */
	bool is_paused;	       /* Display only */
	bool req_toggle_pause; /* Command to App */

	/* Persistence Requests */
	bool req_save_snapshot;
	bool req_reload_workspace;

	/* Debug / Profiling */
	int draw_calls;
	/* [NEW] System Requests */
	bool req_update_vsync;

	/* [NEW] Navigation Request */
	bool req_exit_to_menu;
	bool show_exit_confirmation; /* [NEW] Ask before quit */

	/* [NEW] Save Modal State */
	bool show_save_modal;
	char save_input_buf[64];
	char save_default_title[64]; /* "Meu 2026-..." */
	int current_scenario;	     /* [NEW] For placeholder generation */
	bool input_focused;	     /* [NEW] Is the text box active? */

	/* [NEW] Camera State Restoration for Fixed Cam */
	bhs_camera_t saved_camera_state;
	bool has_saved_camera;

	/* [NEW] Icon Texture Handles */
	void *tex_icon_camera;
	void *tex_icon_swap;
	void *tex_icon_trash;

	/* [NEW] Warning Visual Clutter */
	bool show_visual_warning;
	bool visual_warning_accepted; /* [NEW] Session-based suppression */
	bool pending_toggle_val;      /* Value attempting to set */
	int pending_toggle_id;	      /* ID: 0=Grav, 1=Trail, 2=Sat, 3=PMark, 4=MMark */

	/* [NEW] ECS Visual Request */
	bool req_toggle_visual_bit;
	uint32_t req_visual_mask;
	bhs_entity_id req_visual_entity;
} bhs_hud_state_t;

void bhs_hud_init(bhs_hud_state_t *state);
void bhs_hud_draw(bhs_ui_ctx_t ctx, bhs_hud_state_t *state, int window_w,
		  int window_h);

/* Retorna true se o mouse estiver sobre algum elemento da HUD */
bool bhs_hud_is_mouse_over(bhs_ui_ctx_t ctx, const bhs_hud_state_t *state,
			   int mx, int my, int win_w, int win_h);

#endif
