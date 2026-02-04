#include "hud.h"
#include <math.h> /* [NEW] for powf */
#include <stdio.h>
#include <time.h>			/* [NEW] For dynamic date */
#include "engine/assets/image_loader.h" /* [NEW] */
#include "engine/assets/svg_loader.h"	/* [NEW] */
#include "math/units.h"			/* [NEW] Para bhs_sim_time_to_date */
#include "src/simulation/data/orbit_marker.h" /* [NEW] */
#include "src/simulation/data/planet.h"
#include "src/simulation/scenario_mgr.h" /* [NEW] For scenario_get_name */
#include "src/simulation/components/sim_components.h" /* [FIX] For visual flags */
#include "system/config.h"		 /* [NEW] */

/* ... Helper to save config ... */
static void save_hud_config(const bhs_hud_state_t *state)
{
	bhs_user_config_t cfg;
	bhs_config_defaults(&cfg); /* Init defaults/magic */

	cfg.vsync_enabled = state->vsync_enabled;
	cfg.show_fps = state->show_fps;
	cfg.time_scale_val = state->time_scale_val;

	cfg.visual_mode = state->visual_mode;
	cfg.top_down_view = state->top_down_view;
	cfg.show_gravity_line = state->show_gravity_line;
	cfg.show_orbit_trail = state->show_orbit_trail;
	cfg.show_satellite_orbits = state->show_satellite_orbits;
	cfg.show_planet_markers = state->show_planet_markers;
	cfg.show_moon_markers = state->show_moon_markers;

	bhs_config_save(&cfg, "data/user_config.bin");
}

static const char *MENU_ITEMS[] = { "Config", "Add", "View" };
static const int MENU_COUNT = 3;
static const int MENU_SYSTEM = 99; /* Special index for System Menu */

static bool is_inside(int mx, int my, float x, float y, float w, float h);

/* Layout helper struct to ensure consistency between Draw and Input */
typedef struct {
	float ui_scale;
	float top_bar_height;
	float padding_x;
	float font_size_logo;
	float font_size_tab;
	float logo_width;
	float tab_start_x;
	/* [FIX] Adicionando dimensões do info panel pra consistência */
	float info_panel_w;
	float info_panel_h;
	float info_panel_margin;
} bhs_ui_layout_t;

static bhs_ui_layout_t get_ui_layout(bhs_ui_ctx_t ctx, int win_w, int win_h)
{
	bhs_ui_layout_t l;
	/* 
     * [FIX] Base Scale: considera AMBAS dimensões
     * Usa a menor razão pra não distorcer em janelas estreitas/largas
     * (tipo quando tu mete a janela em metade da tela no Hyprland)
     */
	float scale_h = (float)win_h / 1080.0f;
	float scale_w = (float)win_w / 1920.0f;
	l.ui_scale = (scale_h < scale_w) ? scale_h : scale_w;
	if (l.ui_scale < 0.5f)
		l.ui_scale = 0.5f; /* Mínimo mais baixo pra janelas pequenas */
	if (l.ui_scale > 2.0f)
		l.ui_scale = 2.0f;

	/* Metrics */
	l.top_bar_height = 45.0f * l.ui_scale;
	l.padding_x = 15.0f * l.ui_scale;
	l.font_size_logo = 16.0f * l.ui_scale;
	l.font_size_tab = 14.0f * l.ui_scale;

	/* robust colunas tipo CSS */
	/* Coluna do Logo: Largura medida dinamicamente + padding */
	const char *logo_text = "RiemannEngine";
	float logo_text_w =
		bhs_ui_measure_text(ctx, logo_text, l.font_size_logo);
	l.logo_width = logo_text_w + (10.0f * l.ui_scale);

	/* Layout: [Pad] [Logo] [Pad] [Line] [Pad] [Tabs...] */
	l.tab_start_x = l.padding_x + l.logo_width + (15.0f * l.ui_scale) +
			(1.0f * l.ui_scale) + (15.0f * l.ui_scale);

	/* [FIX] Info Panel - valores centralizados aqui pra draw e input usarem o mesmo */
	/* [FIX] Info Panel - valores centralizados aqui pra draw e input usarem o mesmo */
	l.info_panel_w = 280.0f * l.ui_scale;
	l.info_panel_h = 760.0f * l.ui_scale; /* Adjusted height (was 850, too big) */
	l.info_panel_margin = 20.0f * l.ui_scale;

	return l;
}

void bhs_hud_init(bhs_hud_state_t *state)
{
	if (state) {
		state->show_fps = true;
		state->vsync_enabled = true;
		state->time_scale_val =
			0.28f; /* Default ~1 day/min (Formula: 0.1 * 3650^val) */
		state->active_menu_index = -1;
		state->add_menu_category = -1;
		state->selected_body_index = -1;
		state->req_refresh_selected_body = false; /* [NEW] Sync */
		state->req_delete_body = false;
		state->req_add_body_type = -1;
		state->req_add_registry_entry = NULL;
		state->visual_mode = BHS_VISUAL_MODE_DIDACTIC;
		state->top_down_view = false;
		state->show_gravity_line = false;
		state->show_orbit_trail = false;
		state->show_satellite_orbits = false;
		state->show_planet_markers = false; /* Default OFF (User Req) */
		state->show_moon_markers = false;   /* Default OFF (User Req) */
		state->isolate_view = false;
		state->show_visual_warning = false;
		state->show_save_modal = false; /* [FIX] Ensure init */
		state->visual_warning_accepted = false; /* [NEW] Start as false */
		state->req_toggle_visual_bit = false;
		state->selected_marker_index = -1;
		state->selected_marker_index = -1;
		state->orbit_history_scroll = 0.0f;
		state->refs_collapsed = false; /* Start open */
		state->is_paused = false;
		state->req_toggle_pause = false;
	} /* End Info Panel */
}

static void *load_icon_tex(bhs_ui_ctx_t ctx, const char *path)
{
	bhs_svg_t *svg = bhs_svg_load(path);
	if (!svg) {
		/* Try src/ prefix if running from root without install */
		char buf[256];
		snprintf(buf, 256, "src/%s", path);
		svg = bhs_svg_load(buf);
		if (!svg)
			return NULL;
	}
	/* Render high-res enough for UI (Anti-aliasing via supersampling) */
	bhs_image_t img_large = bhs_svg_rasterize_fit(svg, 256, 256);
	bhs_svg_free(svg);

	if (!img_large.data)
		return NULL;

	/* Software AA: Downsample 4x (256 -> 64) */
	bhs_image_t img_final = bhs_image_downsample_4x(img_large);
	bhs_image_free(img_large);

	if (!img_final.data)
		return NULL;

	void *tex = bhs_ui_create_texture_from_rgba(
		ctx, img_final.width, img_final.height, img_final.data);
	bhs_image_free(img_final);
	return tex;
}

void bhs_hud_draw(bhs_ui_ctx_t ctx, bhs_hud_state_t *state, int window_w,
		  int window_h)
{
	(void)window_h;

	/* Lazy Load Icons */
	if (!state->tex_icon_camera) {
		state->tex_icon_camera =
			load_icon_tex(ctx, "assets/icons/camera.svg");
		state->tex_icon_swap =
			load_icon_tex(ctx, "assets/icons/swap.svg");
		state->tex_icon_trash =
			load_icon_tex(ctx, "assets/icons/trash.svg");
	}

	/* --- LAYOUT CALCULATION --- */
	bhs_ui_layout_t layout = get_ui_layout(ctx, window_w, window_h);
	float ui_scale = layout.ui_scale; /* Keep for local usage if needed */

	/* [FIX] Block input for background if modal is active */
	bool modal_active = state->show_save_modal || state->show_visual_warning;
	bhs_ui_set_input_blocked(ctx, modal_active);

	struct bhs_ui_color theme_bg = { 0.05f, 0.05f, 0.05f, 0.95f };
	struct bhs_ui_color theme_border = { 0.0f, 0.8f, 1.0f, 0.3f };
	struct bhs_ui_color theme_text_normal = { 0.7f, 0.7f, 0.7f, 1.0f };
	struct bhs_ui_color theme_text_active = { 1.0f, 1.0f, 1.0f, 1.0f };
	struct bhs_ui_color theme_highlight = { 0.0f, 0.8f, 1.0f, 1.0f };

	/* 1. Header Bar Background */
	struct bhs_ui_rect bar_rect = { .x = 0,
					.y = 0,
					.width = (float)window_w,
					.height = layout.top_bar_height };
	bhs_ui_draw_rect(ctx, bar_rect, theme_bg);
	bhs_ui_draw_line(ctx, 0, layout.top_bar_height, (float)window_w,
			 layout.top_bar_height, theme_border, 1.0f * ui_scale);

	/* 2. Branding / Logo (Fixed Column) */
	float x_cursor = layout.padding_x;
	const char *logo_text = "RiemannEngine";
	/* Vertically Center: (BarH - FontH) / 2 */
	float logo_y = (layout.top_bar_height - layout.font_size_logo) * 0.5f;

	/* Make Logo Clickable */
	struct bhs_ui_rect logo_rect = { x_cursor, 0, layout.logo_width,
					 layout.top_bar_height };

	/* Input Update for Logo */
	int mx_logo, my_logo;
	bhs_ui_mouse_pos(ctx, &mx_logo, &my_logo);
	bool logo_hovered =
		is_inside(mx_logo, my_logo, logo_rect.x, logo_rect.y,
			  logo_rect.width, logo_rect.height);

	if (logo_hovered) {
		/* Subtle highlight */
		bhs_ui_draw_rect(
			ctx, logo_rect,
			(struct bhs_ui_color){ 1.0f, 1.0f, 1.0f, 0.05f });
		if (bhs_ui_mouse_clicked(ctx, 0)) {
			state->active_menu_index =
				(state->active_menu_index == MENU_SYSTEM)
					? -1
					: MENU_SYSTEM;
		}
	}

	bhs_ui_draw_text(
		ctx, logo_text, x_cursor, logo_y, layout.font_size_logo,
		(state->active_menu_index == MENU_SYSTEM)
			? theme_highlight
			: (struct bhs_ui_color){ 1.0f, 1.0f, 1.0f, 1.0f });

	/* Move cursor to end of logo column */
	x_cursor += layout.logo_width;

	/* Divider */
	float div_x = x_cursor + (15.0f * ui_scale);
	float div_h = 20.0f * ui_scale;
	float div_y = (layout.top_bar_height - div_h) * 0.5f;
	bhs_ui_draw_line(ctx, div_x, div_y, div_x, div_y + div_h,
			 (struct bhs_ui_color){ 0.4f, 0.4f, 0.4f, 0.8f },
			 1.0f * ui_scale);

	/* 3. Navigation Tabs */
	/* Start exactly where the layout says, robustly */
	x_cursor = layout.tab_start_x;

	/* 3. Navigation Tabs */
	/* Start exactly where the layout says, robustly */
	x_cursor = layout.tab_start_x;

	for (int i = 0; i < MENU_COUNT; i++) {
		bool is_active = (state->active_menu_index == i);

		/* Tab Width: Text + Padding (CSS: padding: 0 20px) */
		float item_padding = 20.0f * ui_scale;
		float text_w = bhs_ui_measure_text(ctx, MENU_ITEMS[i],
						   layout.font_size_tab);
		float width = text_w + (item_padding * 2.0f);

		struct bhs_ui_rect item_rect = { x_cursor, 0, width,
						 layout.top_bar_height };

		/* Input Update */
		int mx, my;
		bhs_ui_mouse_pos(ctx, &mx, &my);
		bool hovered = is_inside(mx, my, item_rect.x, item_rect.y,
					 item_rect.width, item_rect.height);

		if (hovered) {
			bhs_ui_draw_rect(ctx, item_rect,
					 (struct bhs_ui_color){ 1.0f, 1.0f,
								1.0f, 0.05f });
			if (bhs_ui_mouse_clicked(ctx, 0)) {
				state->active_menu_index =
					(state->active_menu_index == i) ? -1
									: i;
			}
		}

		/* Render Text Centered */
		float text_y =
			(layout.top_bar_height - layout.font_size_tab) * 0.5f;
		float text_x = x_cursor + item_padding;

		bhs_ui_draw_text(ctx, MENU_ITEMS[i], text_x, text_y,
				 layout.font_size_tab,
				 is_active ? theme_text_active
					   : (hovered ? theme_text_active
						      : theme_text_normal));

		/* Active Indicator */
		if (is_active) {
			float line_y =
				layout.top_bar_height - (2.0f * ui_scale);
			bhs_ui_draw_line(
				ctx, x_cursor + (5.0f * ui_scale), line_y,
				x_cursor + width - (5.0f * ui_scale), line_y,
				theme_highlight, 2.0f * ui_scale);
		}

		x_cursor += width;
	}

	/* [NEW] EXIT BUTTON (Right aligned before Telemetry?) or just next to tabs? */
	/* Let's put it next to tabs for now as a distinct action */
	/* [REMOVED] EXIT BUTTON (Movido para o menu do logo) */
	x_cursor += (20.0f * ui_scale); /* Apenas espaço */

	/* 4. Telemetry (Top Right) */
	/* 4. Speed & FPS Display (Top Right) */
	{
		/* --- 1. SIMULATION SPEED (Always Visible) --- */
		float days_per_min =
			0.1f * powf(3650.0f, state->time_scale_val);

		char speed_text[64];
		if (days_per_min < 1.0f) {
			snprintf(speed_text, 64, "Speed: %.1f d/min",
				 days_per_min);
		} else {
			snprintf(speed_text, 64, "Speed: %.0f d/min",
				 days_per_min);
		}

		float margin_right = 20.0f * layout.ui_scale;
		float font_sz = layout.font_size_tab;
		float speed_w = bhs_ui_measure_text(ctx, speed_text, font_sz);
		float y_pos = 17.0f * layout.ui_scale;

		/* Draw Speed (Cyan-ish) */
		bhs_ui_draw_text(
			ctx, speed_text,
			(float)window_w - speed_w - margin_right, y_pos,
			font_sz,
			(struct bhs_ui_color){ 0.0f, 0.9f, 0.9f, 1.0f });

		/* --- 2. FPS COUNTER (Conditional) --- */
		if (state->show_fps) {
			char fps_text[32];
			snprintf(fps_text, 32, "FPS: %.0f", state->current_fps);

			float fps_w =
				bhs_ui_measure_text(ctx, fps_text, font_sz);
			/* Left of Speed */
			float x_fps = (float)window_w - speed_w - margin_right -
				      fps_w - (15.0f * layout.ui_scale);

			/* Draw FPS (Green/Yellow/Red) */
			struct bhs_ui_color fps_col = BHS_UI_COLOR_GREEN;
			if (state->current_fps < 30.0f)
				fps_col =
					(struct bhs_ui_color){ 1.0f, 0.5f, 0.0f,
							       1.0f }; // Orange
			if (state->current_fps < 15.0f)
				fps_col = BHS_UI_COLOR_RED;

			bhs_ui_draw_text(ctx, fps_text, x_fps, y_pos, font_sz,
					 fps_col);

			/* Separator */
			bhs_ui_draw_text(ctx, "|",
					 x_fps + fps_w +
						 (5.0f * layout.ui_scale),
					 y_pos, font_sz, BHS_UI_COLOR_GRAY);
		}
	}

	/* 3. Dropdown Menu */
	if (state->active_menu_index != -1) {
		/* Use pre-calculated consistent start position */
		float dropdown_x = layout.tab_start_x;

		/* Calculate offset to active item */
		/* [FIX] Check bounds: if MENU_SYSTEM (99), skip this loop, as System Menu is positioned differently */
		if (state->active_menu_index < MENU_COUNT) {
			for (int j = 0; j < state->active_menu_index; ++j) {
				/* Same width calc as Draw */
				float text_w = bhs_ui_measure_text(
					ctx, MENU_ITEMS[j],
					layout.font_size_tab);
				float w = text_w + (40.0f * layout.ui_scale);
				dropdown_x += w;
			}
		}

		/* Calculate Dynamic Height */
		int count = 0;
		if (state->active_menu_index == 1) {
			if (state->add_menu_category == -1) {
				count = 4; /* Categorias principais */
			} else {
				/* Conta itens da categoria */
				const struct bhs_planet_registry_entry *e =
					bhs_planet_registry_get_head();
				while (e) {
					count++;
					e = e->next;
				}
			}
		} else if (state->active_menu_index == 2) {
			count = 6; /* 3 Modes + 3 Toggles */
		} else if (state->active_menu_index == MENU_SYSTEM) {
			count = 10; /* More items: Workspace(2), Diag(2), Exit(1) + Headers + Separators */
		} else {
			count = 12; /* Config items expanded */
		}

		float row_h = 32.0f * layout.ui_scale; /* Aumentado */
		float panel_h = (50.0f * layout.ui_scale) + (count * row_h);
		if (panel_h < 150.0f * layout.ui_scale)
			panel_h = 150.0f * layout.ui_scale;

		struct bhs_ui_rect panel_rect;

		if (state->active_menu_index == MENU_SYSTEM) {
			/* System Menu aligns with Logo */
			panel_rect = (struct bhs_ui_rect){
				layout.padding_x, layout.top_bar_height,
				200.0f * layout.ui_scale, panel_h
			};
		} else {
			/* Tab Menus align with tabs */
			panel_rect = (struct bhs_ui_rect){
				dropdown_x, layout.top_bar_height,
				200.0f * layout.ui_scale, panel_h
			};
		}

		/* Modern Menu Bg */
		bhs_ui_panel(ctx, panel_rect,
			     (struct bhs_ui_color){ 0.1f, 0.1f, 0.1f, 0.98f },
			     theme_border); /* Border */

		float y = layout.top_bar_height + (20.0f * ui_scale);
		float item_pad = 12.0f * ui_scale; /* Mais robusto */
		float item_w = panel_rect.width - (item_pad * 2.0f);
		float item_h = 28.0f * ui_scale; /* Botões maiores */
		float font_header = 15.0f * ui_scale;
		float font_label = 13.0f * ui_scale;
		float row_spacing = 34.0f * ui_scale; /* Mais respiro */

		/* INDEX 0: CONFIG */
		if (state->active_menu_index == 0) {
			bhs_ui_draw_text(ctx, "SETTINGS",
					 panel_rect.x + item_pad, y,
					 font_header, BHS_UI_COLOR_GRAY);
			y += 30.0f * ui_scale;

			/* --- SECTION: DISPLAY --- */
			bhs_ui_draw_text(ctx, "Display",
					 panel_rect.x + item_pad, y, font_label,
					 (struct bhs_ui_color){ 0.5f, 0.5f,
								0.6f, 1.0f });
			y += 18.0f * ui_scale;

			struct bhs_ui_rect item_rect = {
				panel_rect.x + item_pad, y, item_w, item_h
			};

			/* VSYNC */
			bool vsync_prev = state->vsync_enabled;
			bhs_ui_checkbox(ctx, "Enable VSync", item_rect,
					&state->vsync_enabled);
			if (state->vsync_enabled != vsync_prev) {
				state->req_update_vsync = true;
				save_hud_config(state); /* [NEW] Autosave */
			}
			y += row_spacing;
			item_rect.y = y;

			/* FPS Counter */
			bool fps_prev = state->show_fps;
			bhs_ui_checkbox(ctx, "Show FPS Overlay", item_rect,
					&state->show_fps);
			if (state->show_fps != fps_prev)
				save_hud_config(state);

			y += row_spacing + 5.0f * ui_scale;

			bhs_ui_draw_line(
				ctx, panel_rect.x + item_pad,
				y - (row_spacing * 0.5f) + 5.0f,
				panel_rect.x + panel_rect.width - item_pad,
				y - (row_spacing * 0.5f) + 5.0f,
				(struct bhs_ui_color){ 0.3f, 0.3f, 0.3f, 0.5f },
				1.0f);

			/* Speed Control Removed from Config (Moved to View) */
			/* Keeping Separator for aesthetics if needed, or removing section if empty. */
			/* User removed "Time Flow" completely from here. */
			y += row_spacing;

			/* Separator */
			bhs_ui_draw_line(
				ctx, panel_rect.x + item_pad,
				y - (row_spacing * 0.5f),
				panel_rect.x + panel_rect.width - item_pad,
				y - (row_spacing * 0.5f),
				(struct bhs_ui_color){ 0.3f, 0.3f, 0.3f, 0.5f },
				1.0f);

			/* --- SECTION: INTERFACE --- */
			bhs_ui_draw_text(ctx, "Interface",
					 panel_rect.x + item_pad, y, font_label,
					 (struct bhs_ui_color){ 0.5f, 0.5f,
								0.6f, 1.0f });
			y += 18.0f * ui_scale;

			/* Placeholders for FOV/Scale */
			bhs_ui_draw_text(ctx, "UI Scale (Auto)",
					 panel_rect.x + item_pad, y,
					 13.0f * ui_scale,
					 (struct bhs_ui_color){ 0.6f, 0.6f,
								0.6f, 1.0f });
			y += row_spacing * 0.8f;
		}
		/* ... inside logic ... */
		/* INDEX 1: ADD */
		else if (state->active_menu_index == 1) {
			bhs_ui_draw_text(ctx, "Inject Body",
					 panel_rect.x + item_pad, y,
					 font_header, BHS_UI_COLOR_GRAY);
			y += 25.0f * ui_scale;

			/* Root Menu: Categories */
			if (state->add_menu_category == -1) {
				const char *categories[] = { "Planets >",
							     "Suns >",
							     "Moons >",
							     "Black Holes >" };
				for (int i = 0; i < 4; i++) {
					struct bhs_ui_rect btn_rect = {
						panel_rect.x + item_pad, y,
						item_w, item_h
					};
					if (bhs_ui_button(ctx, categories[i],
							  btn_rect)) {
						state->add_menu_category = i;
					}
					y += row_spacing;
				}
			}
			/* Sub Menu: Items */
			else {
				/* Back Button */
				struct bhs_ui_rect back_rect = {
					panel_rect.x + item_pad, y, item_w,
					item_h
				};
				if (bhs_ui_button(ctx, "< Back", back_rect)) {
					state->add_menu_category = -1;
				}
				y += row_spacing;

				/* Iterate Registry and Filter */
				const struct bhs_planet_registry_entry *entry =
					bhs_planet_registry_get_head();
				while (entry) {
					/* Filter by Category */
					bool show = false;
					if (entry->getter) {
						struct bhs_planet_desc d =
							entry->getter();
						switch (state->add_menu_category) {
						case 0: /* Planets */
							show = (d.type ==
									BHS_PLANET_TERRESTRIAL ||
								d.type ==
									BHS_PLANET_GAS_GIANT ||
								d.type ==
									BHS_PLANET_ICE_GIANT ||
								d.type ==
									BHS_PLANET_DWARF);
							break;
						case 1: /* Suns */
							show = (d.type ==
								BHS_STAR_MAIN_SEQ);
							break;
						case 2: /* Moons */
							/* No explicit moon type in detailed enum yet, assuming none match or explicit if added */
							show = false;
							break;
						case 3: /* BlackHoles */
							show = (d.type ==
								BHS_BLACK_HOLE);
							break;
						}
					}

					if (show) {
						struct bhs_ui_rect btn_rect = {
							panel_rect.x + item_pad,
							y, item_w, item_h
						};
						if (bhs_ui_button(ctx,
								  entry->name,
								  btn_rect)) {
							state->req_add_registry_entry =
								entry;
							state->req_add_body_type =
								BHS_BODY_PLANET; /* Simplified for main loop */
							state->active_menu_index =
								-1; /* Close menu */
							state->add_menu_category =
								-1; /* Reset */
						}
						y += row_spacing;
					}
					entry = entry->next;
				}
			}
		}
		/* INDEX 2: VIEW (SCALES) */
		else if (state->active_menu_index == 2) {
			bhs_ui_draw_text(ctx, "Visual Scale",
					 panel_rect.x + item_pad, y,
					 font_header, BHS_UI_COLOR_GRAY);
			y += 25.0f * ui_scale;

			const char *modes[] = { "Scientific (Real)",
						"Didactic (Teaching)",
						"Cinematic (Epic)" };
			bhs_visual_mode_t vals[] = {
				BHS_VISUAL_MODE_SCIENTIFIC,
				BHS_VISUAL_MODE_DIDACTIC,
				BHS_VISUAL_MODE_CINEMATIC
			};

			for (int k = 0; k < 3; k++) {
				bool selected = (state->visual_mode == vals[k]);
				struct bhs_ui_rect btn_rect = {
					panel_rect.x + item_pad, y, item_w,
					item_h
				};

				/* Highlight selected */
				if (selected) {
					bhs_ui_draw_rect(ctx, btn_rect,
							 (struct bhs_ui_color){
								 0.0f, 0.4f,
								 0.5f, 0.5f });
				}

				if (bhs_ui_button(ctx, modes[k], btn_rect)) {
					state->visual_mode = vals[k];
					/* Auto-close? No, let user switch and see.*/
					save_hud_config(state);
				}
				y += row_spacing;
			}

			/* Top Down Camera Toggle */
			y += 5.0f * ui_scale;
			struct bhs_ui_rect td_rect = { panel_rect.x + item_pad,
						       y, item_w, item_h };
			bool td_prev = state->top_down_view;
			bhs_ui_checkbox(ctx, "Top Down Camera", td_rect,
					&state->top_down_view);
			if (state->top_down_view != td_prev)
				save_hud_config(state);

			y += row_spacing;

			/* Helper for Toggle with Warning */
#undef DO_TOGGLE
#define DO_TOGGLE(label, field_ptr, id_val)                                    \
	do {                                                                   \
		struct bhs_ui_rect t_rect = { panel_rect.x + item_pad, y,      \
					      item_w, item_h };                \
		bool cur = *(field_ptr);                                       \
		bool next = cur;                                               \
		bhs_ui_checkbox(ctx, label, t_rect, &next);                    \
		if (next != cur) {                                             \
			if (next && !state->visual_warning_accepted) { /* [FIX] Check accepted flag */ \
				state->show_visual_warning = true;             \
				state->is_paused = true; /* Pause Sim */       \
				state->req_toggle_pause = true;                \
				state->pending_toggle_val = true;              \
				state->pending_toggle_id = id_val;             \
				state->active_menu_index = -1; /* Close Menu */\
			} else { /* Disabling or Already Accepted? Apply immediately */ \
				*(field_ptr) = next;                           \
				save_hud_config(state);                        \
			}                                                      \
		}                                                              \
		y += row_spacing;                                              \
	} while (0)

			/* Gravity Line */
			DO_TOGGLE("Gravity Line (Att)", &state->show_gravity_line, 0);
			/* Orbit Trail */
			DO_TOGGLE("Orbit Trails (All)", &state->show_orbit_trail, 1);
			/* Satellite Orbits */
			DO_TOGGLE("Satellite Orbits", &state->show_satellite_orbits, 2);
			/* Planet Markers */
			DO_TOGGLE("Planet Markers (P)", &state->show_planet_markers, 3);
			/* Moon Markers */
			DO_TOGGLE("Moon Markers (G)", &state->show_moon_markers, 4);

#undef DO_TOGGLE

			y += 10.0f * ui_scale;
			bhs_ui_draw_line(
				ctx, panel_rect.x + item_pad,
				y - (row_spacing * 0.5f) + 5.0f,
				panel_rect.x + panel_rect.width - item_pad,
				y - (row_spacing * 0.5f) + 5.0f,
				(struct bhs_ui_color){ 0.3f, 0.3f, 0.3f, 0.5f },
				1.0f);

			/* Moved Speed Control Here */
			float days_per_min =
				0.1f * powf(3650.0f, state->time_scale_val);
			char time_label[64];
			snprintf(time_label, 64, "Speed: %.0f d/min", days_per_min);
			bhs_ui_draw_text(ctx, time_label, panel_rect.x + item_pad, y,
					 13.0f * ui_scale, BHS_UI_COLOR_WHITE);
			y += 15.0f * ui_scale;
			struct bhs_ui_rect slider_rect = { panel_rect.x + item_pad, y,
							   item_w,
							   12.0f * ui_scale };
			bhs_ui_slider(ctx, slider_rect, &state->time_scale_val);
			y += row_spacing;

			y += row_spacing;

			/* Descrição do modo atual */
			y += 5.0f * ui_scale;
			bhs_ui_draw_line(ctx, panel_rect.x + item_pad, y,
					 panel_rect.x + panel_rect.width -
						 item_pad,
					 y, BHS_UI_COLOR_GRAY, 1.0f);
			y += 10.0f * ui_scale;

			const char *desc = "";
			switch (state->visual_mode) {
			case BHS_VISUAL_MODE_SCIENTIFIC:
				desc = "True Physics.\nPlanets are "
				       "dots.\nSpace is empty.";
				break;
			case BHS_VISUAL_MODE_DIDACTIC:
				desc = "Balanced.\nVisible orbits.\nReadable "
				       "sizes.";
				break;
			case BHS_VISUAL_MODE_CINEMATIC:
				desc = "Hollywood.\nMassive planets.\nClose "
				       "Stars.\nNot physics.";
				break;
			}
			bhs_ui_draw_text(ctx, desc, panel_rect.x + item_pad, y,
					 font_label, BHS_UI_COLOR_GRAY);
		}

		/* INDEX MENU_SYSTEM: SYSTEM OPTIONS */
		/* INDEX MENU_SYSTEM: SYSTEM OPTIONS */
		else if (state->active_menu_index == MENU_SYSTEM) {
			bhs_ui_draw_text(ctx, "SYSTEM CONTROL",
					 panel_rect.x + item_pad, y,
					 font_header, BHS_UI_COLOR_GRAY);
			y += 30.0f * ui_scale;

			/* --- GROUP 1: STATE MANAGEMENT --- */
			bhs_ui_draw_text(ctx, "Workspace",
					 panel_rect.x + item_pad, y, font_label,
					 (struct bhs_ui_color){ 0.5f, 0.5f,
								0.6f, 1.0f });
			y += 18.0f * ui_scale;

			/* Save Snapshot */
			struct bhs_ui_rect save_rect = {
				panel_rect.x + item_pad, y, item_w, item_h
			};
			if (bhs_ui_button(ctx, "Save Snapshot", save_rect)) {
				state->show_save_modal = true;
				state->req_toggle_pause = true; /* Auto-pause */
				state->save_input_buf[0] =
					'\0';		       /* Reset input */
				state->active_menu_index = -1; /* Close menu */
			}
			y += row_spacing;

			/* Reload Scenario */
			struct bhs_ui_rect reset_rect = {
				panel_rect.x + item_pad, y, item_w, item_h
			};
			if (bhs_ui_button(ctx, "Reload Workspace",
					  reset_rect)) {
				state->req_reload_workspace = true;
				state->active_menu_index = -1;
			}
			y += row_spacing + 5.0f * ui_scale;

			/* Separator */
			bhs_ui_draw_line(
				ctx, panel_rect.x + item_pad,
				y - (row_spacing * 0.5f) + 5.0f,
				panel_rect.x + panel_rect.width - item_pad,
				y - (row_spacing * 0.5f) + 5.0f,
				(struct bhs_ui_color){ 0.3f, 0.3f, 0.3f, 0.5f },
				1.0f);

			/* --- GROUP 2: TOOLS & DIAGNOSTICS --- */
			bhs_ui_draw_text(ctx, "Diagnostics",
					 panel_rect.x + item_pad, y, font_label,
					 (struct bhs_ui_color){ 0.5f, 0.5f,
								0.6f, 1.0f });
			y += 18.0f * ui_scale;

			struct bhs_ui_rect export_rect = {
				panel_rect.x + item_pad, y, item_w, item_h
			};
			/* Draw as disabled (grayed out) manually if button tool doesn't support it, 
               but standard button works. We'll just print "Coming Soon". */
			if (bhs_ui_button(ctx, "Export Metrics", export_rect)) {
				printf("[HUD] Export (Planned Feature)\n");
			}
			y += row_spacing;

			struct bhs_ui_rect bug_rect = { panel_rect.x + item_pad,
							y, item_w, item_h };
			if (bhs_ui_button(ctx, "Report Bug", bug_rect)) {
				printf("[HUD] Bug Report clicked\n");
			}
			y += row_spacing + 10.0f * ui_scale;

			/* Separator (Reddish before Exit) */
			bhs_ui_draw_line(
				ctx, panel_rect.x + item_pad,
				y - (row_spacing * 0.5f),
				panel_rect.x + panel_rect.width - item_pad,
				y - (row_spacing * 0.5f),
				(struct bhs_ui_color){ 0.5f, 0.2f, 0.2f, 0.5f },
				1.0f);

			/* --- GROUP 3: EXIT --- */
			struct bhs_ui_rect exit_rect = {
				panel_rect.x + item_pad, y, item_w, item_h
			};

			if (state->show_exit_confirmation) {
				/* Confirmed State: "Really Exit?" and "Cancel" */
				/* Split width for two buttons */
				float sub_w =
					(item_w - 10.0f * ui_scale) * 0.5f;
				struct bhs_ui_rect confirm_rect = {
					panel_rect.x + item_pad, y, sub_w,
					item_h
				};
				struct bhs_ui_rect cancel_rect = {
					panel_rect.x + item_pad + sub_w +
						10.0f * ui_scale,
					y, sub_w, item_h
				};

				/* Custom Draw for Red Button */
				bhs_ui_draw_rect(
					ctx, confirm_rect,
					(struct bhs_ui_color){ 0.6f, 0.1f, 0.1f,
							       1.0f });
				if (is_inside(0, 0, 0, 0, 0, 0)) {
				} /* Dummy to suppress unused warning if logic differs */
				int mx, my;
				bhs_ui_mouse_pos(ctx, &mx, &my);

				/* Confirm Logic */
				bool hover_conf = is_inside(
					mx, my, confirm_rect.x, confirm_rect.y,
					confirm_rect.width,
					confirm_rect.height);
				if (hover_conf) {
					bhs_ui_draw_rect(ctx, confirm_rect,
							 (struct bhs_ui_color){
								 0.8f, 0.2f,
								 0.2f, 1.0f });
					if (bhs_ui_mouse_clicked(ctx, 0)) {
						state->req_exit_to_menu = true;
					}
				}
				float tw_c = bhs_ui_measure_text(ctx, "CONFIRM",
								 font_label);
				bhs_ui_draw_text(
					ctx, "CONFIRM",
					confirm_rect.x + (sub_w - tw_c) * 0.5f,
					confirm_rect.y + 5.0f * ui_scale,
					font_label, BHS_UI_COLOR_WHITE);

				/* Cancel Logic */
				if (bhs_ui_button(ctx, "Cancel", cancel_rect)) {
					state->show_exit_confirmation = false;
				}

				/* Show prompt text below */
				y += row_spacing;
				bhs_ui_draw_text(
					ctx, "Unsaved changes will be lost!",
					panel_rect.x + item_pad, y,
					font_label * 0.9f,
					(struct bhs_ui_color){ 0.8f, 0.4f, 0.4f,
							       1.0f });

			} else {
				/* Normal State */
				if (bhs_ui_button(ctx, "EXIT APP", exit_rect)) {
					state->show_exit_confirmation = true;
				}
			}
		}
	}

	/* 3b. Persistent REFS Panel (Redesigned) */
	{
		/* Dimensions & layout config */
		float p_width = 260.0f * ui_scale;
		float p_height = 135.0f * ui_scale;
		float margin = 20.0f * ui_scale;
		float btn_sz = 24.0f * ui_scale;

		/* Collapsed state */
		if (state->refs_collapsed) {
			p_width = btn_sz + 8.0f;
			p_height = btn_sz + 8.0f;
		}

		/* Position: Bottom Right */
		float px = (float)window_w - p_width - margin;
		float py = (float)window_h - p_height - margin;

		struct bhs_ui_rect p_rect = { px, py, p_width, p_height };

		/* --- Draw Panel --- */
		if (!state->refs_collapsed) {
			/* Background: Deep scifi dark, high transparency */
			bhs_ui_panel(ctx, p_rect,
				     (struct bhs_ui_color){ 0.02f, 0.04f, 0.08f,
							    0.85f },
				     (struct bhs_ui_color){
					     0.0f, 0.0f, 0.0f,
					     0.0f }); /* Transparent border */

			/* Accent Line (Left Border) */
			bhs_ui_draw_line(ctx, px, py, px, py + p_height,
					 theme_highlight, 3.0f * ui_scale);
		}

		/* --- Toggle Button (Collapse) --- */
		/* Position: Top Right of panel */
		float bx, by;
		if (state->refs_collapsed) {
			bx = (float)window_w - btn_sz - margin;
			by = (float)window_h - btn_sz - margin;
		} else {
			bx = px + p_width - btn_sz - 5.0f;
			by = py + 5.0f;
		}

		struct bhs_ui_rect btn_rect = { bx, by, btn_sz, btn_sz };
		/* Minimalist Toggle Icon */
		if (bhs_ui_button(ctx, state->refs_collapsed ? "<" : ">",
				  btn_rect)) {
			state->refs_collapsed = !state->refs_collapsed;
		}

		/* --- Content --- */
		if (!state->refs_collapsed) {
			float x = px + 20.0f * ui_scale;
			float y = py + 15.0f * ui_scale;

			/* 1. Header */
			bhs_ui_draw_text(ctx, "TEMPORAL FRAME", x, y,
					 11.0f * ui_scale,
					 (struct bhs_ui_color){ 0.0f, 0.8f,
								1.0f, 0.8f });
			y += 20.0f * ui_scale;

			/* 2. Epoch/Date Block */
			int yr, mo, dy, hr, mi, sc;
			bhs_sim_time_to_date(state->sim_time_seconds, &yr, &mo,
					     &dy, &hr, &mi, &sc);

			/* DATE (Large) */
			char date_buf[64];
			snprintf(date_buf, sizeof(date_buf), "%04d-%02d-%02d",
				 yr, mo, dy);
			bhs_ui_draw_text(ctx, date_buf, x, y, 26.0f * ui_scale,
					 (struct bhs_ui_color){ 1.0f, 1.0f,
								1.0f, 1.0f });
			y += 30.0f * ui_scale;

			/* TIME (Monospace-ish) + UTC */
			char time_buf[64];
			snprintf(time_buf, sizeof(time_buf),
				 "%02d:%02d:%02d UTC", hr, mi, sc);
			bhs_ui_draw_text(ctx, time_buf, x, y, 18.0f * ui_scale,
					 (struct bhs_ui_color){ 0.8f, 0.9f,
								0.9f, 0.7f });
			y += 35.0f * ui_scale;

			/* 3. Controls & Info Footer */
			/* Play/Pause Button */
			float play_btn_w = 80.0f * ui_scale;
			float play_btn_h = 24.0f * ui_scale;
			struct bhs_ui_rect play_rect = { x, y, play_btn_w,
							 play_btn_h };

			const char *pp_label =
				state->is_paused ? ">  PLAY" : "||  PAUSE";
			/* Custom styled button logic would be better, but standard button works for now */
			if (bhs_ui_button(ctx, pp_label, play_rect)) {
				state->req_toggle_pause = true;
			}

			/* Elapsed Time (Right aligned relative to button or panel?) */
			/* Let's put elapsed time next to the button */
			float elapsed_x = x + play_btn_w + 15.0f * ui_scale;
			double days_elapsed = state->sim_time_seconds / 86400.0;
			double years_elapsed = days_elapsed / 365.25;
			char elapsed_buf[64];
			if (fabs(years_elapsed) >= 1.0) {
				snprintf(elapsed_buf, sizeof(elapsed_buf),
					 "%.2f yrs", years_elapsed);
			} else {
				snprintf(elapsed_buf, sizeof(elapsed_buf),
					 "%.1f days", days_elapsed);
			}
			/* Vertically align text with button center */
			bhs_ui_draw_text(ctx, elapsed_buf, elapsed_x,
					 y + 4.0f * ui_scale, 14.0f * ui_scale,
					 (struct bhs_ui_color){ 0.5f, 0.5f,
								0.6f, 1.0f });
		}
	}

	/* 4. Info Panel (Expanded & Polished) */
	if (state->selected_body_index != -1 ||
	    state->selected_marker_index != -1) {
		/* [FIX] Refresh Body Data for Checkboxes */
		if (state->selected_body_index != -1) {
			state->req_refresh_selected_body = true;
		}
		/* [FIX] Usando valores do layout pra consistência com hit test */
		struct bhs_ui_rect info_rect = {
			(float)window_w - layout.info_panel_w -
				layout.info_panel_margin,
			layout.top_bar_height + 10.0f * ui_scale,
			layout.info_panel_w, layout.info_panel_h
		};
		/* Sleek Dark Panel */
		bhs_ui_panel(
			ctx, info_rect,
			(struct bhs_ui_color){ 0.05f, 0.05f, 0.05f, 0.95f },
			(struct bhs_ui_color){ 0.2f, 0.2f, 0.2f, 1.0f });

		float pad = 15.0f * ui_scale; /* Padding interno */
		float y = info_rect.y + pad;
		float x = info_rect.x + pad;
		float w = info_rect.width - (pad * 2.0f);

		/* Font sizes escalados - Mais legíveis */
		float font_title = 16.0f * ui_scale;
		float font_name = 18.0f * ui_scale;
		float font_prop = 15.0f * ui_scale;
		float font_section = 14.0f * ui_scale;
		float prop_offset = 90.0f * ui_scale; /* Offset ajustado */
		float line_h = 22.0f * ui_scale;      /* Mais respiro */

		/* Macro para desenhar propriedades (reutilizável) */
		char buf[128];
#undef DRAW_PROP
#define DRAW_PROP(label, val_fmt, ...)                                         \
	do {                                                                   \
		bhs_ui_draw_text(                                              \
			ctx, label, x, y, font_prop,                           \
			(struct bhs_ui_color){ 0.6f, 0.6f, 0.6f, 1.0f });      \
		snprintf(buf, sizeof(buf), val_fmt, __VA_ARGS__);              \
		bhs_ui_draw_text(ctx, buf, x + prop_offset, y, font_prop,      \
				 BHS_UI_COLOR_WHITE);                          \
		y += line_h;                                                   \
	} while (0)

		/* CASO A: Marcador de Órbita Selecionado */
		if (state->selected_marker_index != -1 &&
		    state->orbit_markers_ptr) {
			const struct bhs_orbit_marker *m =
				&state->orbit_markers_ptr->markers
					 [state->selected_marker_index];

			/* Header */
			bhs_ui_draw_text(ctx, "ORBIT EVENT", x, y, font_title,
					 (struct bhs_ui_color){ 0.6f, 0.2f,
								0.8f, 1.0f });
			y += 20.0f * ui_scale;
			bhs_ui_draw_line(
				ctx, x, y, x + w, y,
				(struct bhs_ui_color){ 0.6f, 0.2f, 0.8f, 0.3f },
				1.0f * ui_scale);
			y += 15.0f * ui_scale;

			/* Nome (Planeta que completou) */
			bhs_ui_draw_text(ctx, m->planet_name, x, y, font_name,
					 BHS_UI_COLOR_WHITE);
			y += 25.0f * ui_scale;

			/* Detalhes do Marco */
			DRAW_PROP("Orbit #:", "%d", m->orbit_number);

			int yr, mo, dy, hr, mi, sc;
			bhs_sim_time_to_date(m->timestamp_seconds, &yr, &mo,
					     &dy, &hr, &mi, &sc);
			DRAW_PROP("Date:", "%04d-%02d-%02d", yr, mo, dy);
			DRAW_PROP("Time:", "%02d:%02d:%02d UTC", hr, mi, sc);

			y += 10.0f * ui_scale;
			bhs_ui_draw_text(ctx, "MEASUREMENTS", x, y,
					 font_section,
					 (struct bhs_ui_color){ 0.6f, 0.2f,
								0.8f, 0.8f });
			y += line_h;

			DRAW_PROP("Period:", "%.3f days",
				  m->orbital_period_measured / 86400.0);
			DRAW_PROP("Pos X:", "%.2e m", m->position.x);
			DRAW_PROP("Pos Z:", "%.2e m", m->position.z);

			/* Seção de HISTÓRICO */
			y += 20.0f * ui_scale;
			bhs_ui_draw_line(
				ctx, x, y, x + w, y,
				(struct bhs_ui_color){ 0.3f, 0.3f, 0.3f, 0.8f },
				1.0f * ui_scale);
			y += 10.0f * ui_scale;
			bhs_ui_draw_text(ctx, "ORBIT HISTORY", x, y,
					 font_section,
					 (struct bhs_ui_color){ 0.6f, 0.2f,
								0.8f, 0.8f });
			y += line_h + 5.0f * ui_scale;

			/* Área rolável */
			float history_y_start = y;
			float history_h_max = info_rect.y + info_rect.height -
					      y - 10.0f * ui_scale;

			/* Lógica de Scroll do mouse */
			int mx, my;
			bhs_ui_mouse_pos(ctx, &mx, &my);
			if (is_inside(mx, my, info_rect.x, history_y_start,
				      info_rect.width, history_h_max)) {
				float scroll = bhs_ui_mouse_scroll(ctx);
				if (fabsf(scroll) > 0.1f) {
					state->orbit_history_scroll +=
						scroll * 20.0f * ui_scale;
				}
			}

			/* Iterar todos os marcadores do mesmo planeta */
			int count_found = 0;
			if (state->orbit_markers_ptr) {
				for (int i = 0;
				     i < state->orbit_markers_ptr->marker_count;
				     i++) {
					const struct bhs_orbit_marker *hist =
						&state->orbit_markers_ptr
							 ->markers[i];
					if (!hist->active ||
					    hist->planet_index !=
						    m->planet_index)
						continue;

					/* Calcula Y com scroll */
					float entry_y =
						y + state->orbit_history_scroll;

					/* Clipping simples */
					if (entry_y >= history_y_start &&
					    entry_y <
						    history_y_start +
							    history_h_max -
							    15.0f * ui_scale) {
						int hyr, hmo, hdy, hhr, hmi,
							hsc;
						bhs_sim_time_to_date(
							hist->timestamp_seconds,
							&hyr, &hmo, &hdy, &hhr,
							&hmi, &hsc);

						char entry_buf[128];
						snprintf(
							entry_buf,
							sizeof(entry_buf),
							"#%d | %04d-%02d-%02d "
							"| %.1f d",
							hist->orbit_number, hyr,
							hmo, hdy,
							hist->orbital_period_measured /
								86400.0);

						bhs_ui_draw_text(
							ctx, entry_buf, x,
							entry_y,
							font_section * 0.9f,
							(hist->orbit_number ==
							 m->orbit_number)
								? BHS_UI_COLOR_WHITE
								: (struct
								   bhs_ui_color){
									  0.6f,
									  0.6f,
									  0.61f,
									  1.0f });
					}

					y += 15.0f *
					     ui_scale; /* Mantém o y "virtual" crescendo */
					count_found++;
				}
			}

			/* Clamping do scroll */
			float total_content_h = count_found * 15.0f * ui_scale;
			float min_scroll = -(total_content_h - history_h_max);
			if (min_scroll > 0)
				min_scroll = 0;
			if (state->orbit_history_scroll < min_scroll)
				state->orbit_history_scroll = min_scroll;
			if (state->orbit_history_scroll > 0)
				state->orbit_history_scroll = 0;

			return; /* Sai aqui se for marcador */
		}

		/* CASO B: Corpo Celeste Selecionado (Código original migrado para dentro do if) */
		const struct bhs_body *b = &state->selected_body_cache;

		/* Header */
		bhs_ui_draw_text(
			ctx, "OBJECT INSPECTOR", x, y, font_title,
			(struct bhs_ui_color){ 0.0f, 0.8f, 1.0f, 1.0f });
		y += 20.0f * ui_scale;

		/* High-tech Separator Line */
		bhs_ui_draw_line(
			ctx, x, y, x + w, y,
			(struct bhs_ui_color){ 0.0f, 0.8f, 1.0f, 0.3f },
			1.0f * ui_scale);
		y += 15.0f * ui_scale;

		/* [NEW] Quick Actions Row (Camera, Swap, Delete) */
		{
			float btn_gap = 12.0f * ui_scale;
			float btn_w = (w - (btn_gap * 2.0f)) / 3.0f;
			float btn_h = 48.0f * ui_scale; /* "Bem Grandes" */
			float icon_pad = 8.0f * ui_scale;

			/* 1. Camera Button */
			struct bhs_ui_rect rect_cam = { x, y, btn_w, btn_h };
			struct bhs_ui_color cam_bg =
				state->fixed_planet_cam
					? (struct bhs_ui_color){ 0.0f, 0.5f,
								 0.6f, 1.0f }
					: /* Active: Teal */
					(struct bhs_ui_color){
						0.15f, 0.15f, 0.2f, 1.0f
					}; /* Inactive: Dark */

			bhs_ui_draw_rect(ctx, rect_cam, cam_bg);
			bhs_ui_draw_rect_outline(
				ctx, rect_cam,
				(struct bhs_ui_color){ 0.4f, 0.4f, 0.5f, 0.5f },
				1.0f);

			if (bhs_ui_button(ctx, "", rect_cam)) {
				state->fixed_planet_cam =
					!state->fixed_planet_cam;
			}
			if (state->tex_icon_camera) {
				bhs_ui_draw_texture(
					ctx, state->tex_icon_camera,
					rect_cam.x + icon_pad,
					rect_cam.y + icon_pad,
					rect_cam.width - 2 * icon_pad,
					rect_cam.height - 2 * icon_pad,
					BHS_UI_COLOR_WHITE);
			} else {
				bhs_ui_draw_text(ctx, "[O]",
						 rect_cam.x + 10.0f * ui_scale,
						 rect_cam.y + 5.0f * ui_scale,
						 14.0f * ui_scale,
						 BHS_UI_COLOR_WHITE);
			}

			/* 2. Swap Button */
			struct bhs_ui_rect rect_swp = { x + btn_w + btn_gap, y,
							btn_w, btn_h };
			bhs_ui_draw_rect(ctx, rect_swp,
					 (struct bhs_ui_color){ 0.2f, 0.2f,
								0.25f, 1.0f });
			if (bhs_ui_button(ctx, "", rect_swp)) {
				/* Placeholder */
			}
			if (state->tex_icon_swap) {
				bhs_ui_draw_texture(
					ctx, state->tex_icon_swap,
					rect_swp.x + icon_pad,
					rect_swp.y + icon_pad,
					rect_swp.width - 2 * icon_pad,
					rect_swp.height - 2 * icon_pad,
					BHS_UI_COLOR_WHITE);
			} else {
				bhs_ui_draw_text(ctx, "<->",
						 rect_swp.x + 10.0f * ui_scale,
						 rect_swp.y + 5.0f * ui_scale,
						 14.0f * ui_scale,
						 BHS_UI_COLOR_WHITE);
			}

			/* 3. Delete Button */
			struct bhs_ui_rect rect_del = {
				x + (btn_w + btn_gap) * 2.0f, y, btn_w, btn_h
			};
			/* Red tint for delete */
			bhs_ui_draw_rect(ctx, rect_del,
					 (struct bhs_ui_color){ 0.3f, 0.1f,
								0.1f, 1.0f });

			if (bhs_ui_button(ctx, "", rect_del)) {
				state->req_delete_body = true;
			}
			if (state->tex_icon_trash) {
				bhs_ui_draw_texture(
					ctx, state->tex_icon_trash,
					rect_del.x + icon_pad,
					rect_del.y + icon_pad,
					rect_del.width - 2 * icon_pad,
					rect_del.height - 2 * icon_pad,
					BHS_UI_COLOR_WHITE);
			} else {
				bhs_ui_draw_text(
					ctx, "X", rect_del.x + 10.0f * ui_scale,
					rect_del.y + 5.0f * ui_scale,
					14.0f * ui_scale, BHS_UI_COLOR_WHITE);
			}

			y += btn_h + 15.0f * ui_scale;
		}

		/* --- UNIVERSAL DATA --- */
		const char *type_str = "Unknown";
		if (b->name[0] != '\0')
			type_str = b->name;

		/* Name (Large) */
		bhs_ui_draw_text(ctx, type_str, x, y, font_name,
				 BHS_UI_COLOR_WHITE);
		y += 25.0f * ui_scale;

/* Helper to draw property row - agora usando variaveis escaladas */
#undef DRAW_PROP
#define DRAW_PROP(label, val_fmt, ...)                                         \
	do {                                                                   \
		bhs_ui_draw_text(                                              \
			ctx, label, x, y, font_prop,                           \
			(struct bhs_ui_color){ 0.6f, 0.6f, 0.6f, 1.0f });      \
		snprintf(buf, sizeof(buf), val_fmt, __VA_ARGS__);              \
		bhs_ui_draw_text(ctx, buf, x + prop_offset, y, font_prop,      \
				 BHS_UI_COLOR_WHITE);                          \
		y += line_h;                                                   \
	} while (0)

		// Basic Physics
		DRAW_PROP("Mass:", "%.2e kg", b->state.mass);
		DRAW_PROP("Radius:", "%.2e m", b->state.radius);
		DRAW_PROP("Pos:", "(%.1f, %.1f)", b->state.pos.x,
			  b->state.pos.z);
		DRAW_PROP("Vel:", "(%.2f, %.2f)", b->state.vel.x,
			  b->state.vel.z);
		y += 10.0f * ui_scale;

		/* Separator */
		bhs_ui_draw_line(
			ctx, x, y, x + w, y,
			(struct bhs_ui_color){ 0.3f, 0.3f, 0.3f, 1.0f },
			1.0f * ui_scale);
		y += 10.0f * ui_scale;

		/* --- MOTION DYNAMICS --- */
		bhs_ui_draw_text(
			ctx, "MOTION", x, y, font_section,
			(struct bhs_ui_color){ 0.0f, 0.8f, 1.0f, 0.8f });
		y += line_h;

		double speed_ms = sqrt(b->state.vel.x * b->state.vel.x +
				       b->state.vel.z * b->state.vel.z);
		if (speed_ms > 1000.0) {
			DRAW_PROP("Orb. Spd:", "%.2f km/s", speed_ms / 1000.0);
		} else {
			DRAW_PROP("Orb. Spd:", "%.2f m/s", speed_ms);
		}

		/* Escape Velocity sqrt(2GM/r) relative to ITSELF? No, relative to parent...
           We don't know parent R here easily without query. Skip escape vel for now. */

		y += 10.0f * ui_scale;

		/* --- ROTATION --- */
		if (b->type == BHS_BODY_PLANET || b->type == BHS_BODY_STAR) {
			bhs_ui_draw_text(ctx, "ROTATION", x, y, font_section,
					 (struct bhs_ui_color){ 0.0f, 0.8f,
								1.0f, 0.8f });
			y += line_h;

			/* Period Calc: T = 2pi / omega */
			double period_s = 0.0;
			if (fabs(b->state.rot_speed) > 1e-9) {
				period_s = (2.0 * 3.14159265359) /
					   fabs(b->state.rot_speed);
				/* Check sign for retrograde */
				bool retro =
					(b->state.rot_speed < 0 &&
					 b->prop.planet.rotation_period < 0);
				/* If period is negative in prop, it's retro. */
				if (b->type == BHS_BODY_PLANET &&
				    b->prop.planet.rotation_period < 0)
					retro = true;

				if (period_s > 86400.0) {
					DRAW_PROP("Period:", "%.2f d %s",
						  period_s / 86400.0,
						  retro ? "(R)" : "");
				} else {
					DRAW_PROP("Period:", "%.2f h %s",
						  period_s / 3600.0,
						  retro ? "(R)" : "");
				}
			} else {
				DRAW_PROP("Period:", "Locked/Static%s", "");
			}

			/* Axis Tilt from Prop (converted to deg) */
			double tilt_deg = 0.0;
			if (b->type == BHS_BODY_PLANET)
				tilt_deg = b->prop.planet.axis_tilt *
					   (180.0 / 3.14159265359);
			// else if (b->type == BHS_BODY_STAR) tilt_deg = b->prop.star.axis_tilt... (struct doesnt have it explicitly yet in Display/Prop union?)
			// Only planet prop has explicit tilt field in `bhs_planet_data`.

			DRAW_PROP("Tilt:", "%.2f deg", tilt_deg);

			/* Calc Linear Velocity at Equator: v = omega * r */
			double v_eq =
				fabs(b->state.rot_speed) * b->state.radius;
			DRAW_PROP("Eq. Vel:", "%.1f m/s", v_eq);

			y += 10.0f * ui_scale;
		}

		bhs_ui_draw_text(
			ctx, "PROPERTIES", x, y, font_section,
			(struct bhs_ui_color){ 0.0f, 0.8f, 1.0f, 0.8f });
		y += line_h;

		/* --- TYPE SPECIFIC --- */
		if (b->type == BHS_BODY_PLANET) {
			DRAW_PROP("Density:", "%.0f kg/m3",
				  b->prop.planet.density);
			DRAW_PROP("Temp:", "%.1f K",
				  b->prop.planet.temperature);
			DRAW_PROP("Press:", "%.2f atm",
				  b->prop.planet.surface_pressure);
			DRAW_PROP("Atmos:", "%s",
				  b->prop.planet.has_atmosphere ? "Yes" : "No");
			DRAW_PROP("Comp:", "%s", b->prop.planet.composition);
		} else if (b->type == BHS_BODY_STAR) {
			DRAW_PROP("Lum:", "%.2e W", b->prop.star.luminosity);
			DRAW_PROP("Teff:", "%.0f K",
				  b->prop.star.temp_effective);
			DRAW_PROP("Class:", "%s", b->prop.star.spectral_type);
			DRAW_PROP("Age:", "%.1e yr", b->prop.star.age);
		} else if (b->type == BHS_BODY_BLACKHOLE) {
			DRAW_PROP("Spin:", "%.2f", b->prop.bh.spin_factor);
			DRAW_PROP("Horizon:", "%.2f",
				  b->prop.bh.event_horizon_r);
		}

		/* Strongest Attractor Display */
		if (state->attractor_name[0] != '\0') {
			y += 10.0f * ui_scale;
			bhs_ui_draw_line(
				ctx, x, y, x + w, y,
				(struct bhs_ui_color){ 0.3f, 0.3f, 0.3f, 1.0f },
				1.0f * ui_scale);
			y += 10.0f * ui_scale;

			bhs_ui_draw_text(ctx, "MAJOR FORCE", x, y, font_section,
					 (struct bhs_ui_color){ 0.0f, 0.8f,
								1.0f, 0.8f });
			y += line_h;

			DRAW_PROP("Source:", "%s", state->attractor_name);

			double d_val = state->attractor_dist;
			/* Smart Units: AU > Mkm > km */
			if (d_val >=
			    149597870700.0 * 0.01) { /* > 0.01 AU (~1.5 Mkm) */
				DRAW_PROP("Dist:", "%.3f AU",
					  d_val / 149597870700.0);
			} else if (d_val >= 1.0e9) { /* > 1 Mkm */
				DRAW_PROP("Dist:", "%.2f Mkm", d_val / 1.0e9);
			} else {
				DRAW_PROP("Dist:", "%.0f km", d_val / 1000.0);
			}
		}

		y += 20.0f * ui_scale; /* Spacing before buttons */

		y += 20.0f * ui_scale; /* Spacing before buttons */

		/* [NEW] Checkbox para Isolamento Visual */
		/* (Delete button removed, Fixed camera toggle moved to top) */
		/* y += btn_h + 10.0f * ui_scale; <-- adjust spacing */

		/* [NEW] Local Visualization Toggles */
		/* [NEW] Local Visualization Toggles */
		y += 15.0f * ui_scale;

		/* Separator Line */
		bhs_ui_draw_line(
			ctx, x, y, x + w, y,
			(struct bhs_ui_color){ 0.0f, 0.8f, 1.0f, 0.5f },
			1.0f);
		y += 8.0f * ui_scale;

		bhs_ui_draw_text(
			ctx, "LOCAL VISUALS", x, y, font_section,
			(struct bhs_ui_color){ 0.0f, 0.8f, 1.0f, 0.9f });
		y += line_h;

		/* We need to know current flags. `bhs_body` (legacy) now has `visual_flags`. */
		/* But b is `state->selected_body_cache`, which is a COPY. */
		/* Render Checkboxes based on existing State */
		
		bool show_trail = (b->visual_flags & BHS_VISUAL_FLAG_SHOW_TRAIL);
		bool show_markers = (b->visual_flags & BHS_VISUAL_FLAG_SHOW_MARKERS);
		bool show_vectors = (b->visual_flags & BHS_VISUAL_FLAG_SHOW_VECTORS);

		/* Indentation for checkboxes */
		float indent = 15.0f * ui_scale;
		float chk_h = 24.0f * ui_scale;     /* Match "Isolar Visao" height */
		float chk_step = 28.0f * ui_scale;  /* Consistent spacing */
		
		struct bhs_ui_rect chk_rect = { x + indent, y, w - indent, chk_h };
		
		/* Trail */
		bool next_trail = show_trail;
		bhs_ui_checkbox(ctx, "Show Trail", chk_rect, &next_trail);
		if (next_trail != show_trail) {
			state->req_toggle_visual_bit = true;
			state->req_visual_entity = b->entity_id;
			state->req_visual_mask = BHS_VISUAL_FLAG_SHOW_TRAIL;
		}
		y += chk_step;

		/* Markers */
		chk_rect.y = y;
		bool next_mk = show_markers;
		bhs_ui_checkbox(ctx, "Show Markers", chk_rect, &next_mk);
		if (next_mk != show_markers) {
			state->req_toggle_visual_bit = true;
			state->req_visual_entity = b->entity_id;
			state->req_visual_mask = BHS_VISUAL_FLAG_SHOW_MARKERS;
		}
		y += chk_step;

		/* Vectors */
		chk_rect.y = y;
		bool next_vec = show_vectors;
		bhs_ui_checkbox(ctx, "Show Gravity/Vel", chk_rect, &next_vec);
		if (next_vec != show_vectors) {
			state->req_toggle_visual_bit = true;
			state->req_visual_entity = b->entity_id;
			state->req_visual_mask = BHS_VISUAL_FLAG_SHOW_VECTORS;
		}
		y += chk_step;

		/* [NEW] Checkbox para Isolamento Visual */
		/* Previous spacing was manual, now using standard step */
		/* y += 5.0f * ui_scale; */ /* Removed extra gap to keep them grouped or keep it? User said "well organized distances". 
		                           Isolar is a bit separate logically. Let's keep a small extra gap for grouping logic or remove for uniformity.
								   User said "seguirem o padrão", "bem organizadas". Uniformity is safest. */
		
		struct bhs_ui_rect iso_rect = { x + indent, y, w - indent, chk_h };
		bhs_ui_checkbox(ctx, "Isolar Visao", iso_rect,
				&state->isolate_view);
	}

	/* 4. FPS Counter (Overlay - independent of menu) */
	/* FPS Counter moved to Top Bar */

	/* ============================================================================
	 * 5. SAVE MODAL (OVERLAY)
	 * ============================================================================
	 */
	if (state->show_save_modal) {
		/* Enable input for the modal */
		bhs_ui_set_input_blocked(ctx, false);

		/* Dim Background */
		bhs_ui_draw_rect(
			ctx,
			(struct bhs_ui_rect){ 0, 0, (float)window_w,
					      (float)window_h },
			(struct bhs_ui_color){ 0.0f, 0.0f, 0.0f, 0.6f });

		/* Modal Window */
		float modal_w = 400.0f * layout.ui_scale;
		float modal_h = 220.0f * layout.ui_scale;
		float mx = ((float)window_w - modal_w) * 0.5f;
		float my = ((float)window_h - modal_h) * 0.5f;

		bhs_ui_panel(ctx,
			     (struct bhs_ui_rect){ mx, my, modal_w, modal_h },
			     (struct bhs_ui_color){ 0.1f, 0.12f, 0.15f, 1.0f },
			     (struct bhs_ui_color){ 0.0f, 0.8f, 1.0f, 0.5f });

		float pad = 20.0f * layout.ui_scale;
		float y = my + pad;

		/* Title */
		bhs_ui_draw_text(
			ctx, "SAVE SNAPSHOT", mx + pad, y,
			16.0f * layout.ui_scale,
			(struct bhs_ui_color){ 0.0f, 0.8f, 1.0f, 1.0f });
		y += 40.0f * layout.ui_scale;

		/* Input Label */
		struct bhs_ui_rect input_rect = { mx + pad, y,
						  modal_w - (pad * 2),
						  35.0f * layout.ui_scale };



		/* Placeholder Logic (Visual Only) */
		if (state->save_input_buf[0] == '\0' && !state->input_focused) {
			static char placeholder[128];
			time_t t = time(NULL);
			struct tm tm = *localtime(&t);
			const char *scen_name = scenario_get_name(
				(enum scenario_type)state->current_scenario);
			snprintf(placeholder, 128, "Meu %04d-%02d-%02d %s",
				 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
				 scen_name);

			/* Draw Placeholder manually */
			bhs_ui_draw_rect(ctx, input_rect,
					 (struct bhs_ui_color){ 0.05f, 0.05f,
								0.08f, 1.0f });
			bhs_ui_draw_rect_outline(
				ctx, input_rect,
				(struct bhs_ui_color){ 0.3f, 0.3f, 0.4f, 1.0f },
				1.0f);
			bhs_ui_draw_text(ctx, placeholder, input_rect.x + 10.0f,
					 input_rect.y + 8.0f,
					 14.0f * layout.ui_scale,
					 (struct bhs_ui_color){ 0.5f, 0.5f,
								0.5f, 1.0f });

			/* Handle click on placeholder to activate */
			int32_t mx_in, my_in;
			bhs_ui_mouse_pos(ctx, &mx_in, &my_in);
			if (bhs_ui_mouse_clicked(ctx, 0) &&
			    is_inside(mx_in, my_in, input_rect.x, input_rect.y,
				      input_rect.width, input_rect.height)) {
				state->input_focused = true;
			}
		} else {
			/* Use actual Widget */
			bhs_ui_text_field(ctx, input_rect,
					  state->save_input_buf, 63,
					  &state->input_focused);
		}

		y += 50.0f * layout.ui_scale;

		/* Buttons */
		float btn_w = 100.0f * layout.ui_scale;
		float btn_h = 30.0f * layout.ui_scale;

		/* Save Button */
		if (bhs_ui_button(ctx, "SAVE",
				  (struct bhs_ui_rect){ mx + pad, y, btn_w,
							btn_h })) {
			state->req_save_snapshot =
				true; /* Trigger save logic in input_layer */
			state->show_save_modal = false;
			state->input_focused = false;
		}

		/* Cancel Button */
		if (bhs_ui_button(
			    ctx, "CANCEL",
			    (struct bhs_ui_rect){ mx + modal_w - pad - btn_w, y,
						  btn_w, btn_h })) {
			state->show_save_modal = false;
			state->req_toggle_pause = true;	 /* Unpause */
			state->save_input_buf[0] = '\0'; /* Clear */
			state->input_focused = false;
		}
	}

	/* 5.5 WARNING MODAL */
	if (state->show_visual_warning) {
		/* Enable input for the modal */
		bhs_ui_set_input_blocked(ctx, false);

		/* Dim Background */
		bhs_ui_draw_rect(
			ctx,
			(struct bhs_ui_rect){ 0, 0, (float)window_w,
					      (float)window_h },
			(struct bhs_ui_color){ 0.0f, 0.0f, 0.0f, 0.8f });

		float mw = 500.0f * layout.ui_scale;
		float mh = 250.0f * layout.ui_scale;
		float warn_mx = ((float)window_w - mw) * 0.5f;
		float warn_my = ((float)window_h - mh) * 0.5f;

		bhs_ui_panel(ctx, (struct bhs_ui_rect){ warn_mx, warn_my, mw, mh },
			     (struct bhs_ui_color){ 0.15f, 0.1f, 0.1f, 1.0f },
			     (struct bhs_ui_color){ 1.0f, 0.5f, 0.0f, 0.8f });

		float p = 30.0f * layout.ui_scale;
		float ty = warn_my + p;
		
		bhs_ui_draw_text(ctx, "WARNING: VISUAL CLUTTER", warn_mx + p, ty, 20.0f * layout.ui_scale, (struct bhs_ui_color){1.0f, 0.6f, 0.0f, 1.0f});
		ty += 40.0f * layout.ui_scale;
		
		const char *msg1 = "Ativar todos os rastros ou marcadores pode";
		const char *msg2 = "causar sujeira visual severa ou perda de";
		const char *msg3 = "performance. Deseja continuar?";
		
		bhs_ui_draw_text(ctx, msg1, warn_mx + p, ty, 14.0f * layout.ui_scale, BHS_UI_COLOR_WHITE);
		ty += 20.0f * layout.ui_scale;
		bhs_ui_draw_text(ctx, msg2, warn_mx + p, ty, 14.0f * layout.ui_scale, BHS_UI_COLOR_WHITE);
		ty += 20.0f * layout.ui_scale;
		bhs_ui_draw_text(ctx, msg3, warn_mx + p, ty, 14.0f * layout.ui_scale, BHS_UI_COLOR_WHITE);
		ty += 50.0f * layout.ui_scale;

		float btn_w = 120.0f * layout.ui_scale;
		struct bhs_ui_rect r_yes = { warn_mx + p, ty, btn_w, 30.0f * layout.ui_scale };
		struct bhs_ui_rect r_no = { warn_mx + p + btn_w + 20.0f, ty, btn_w, 30.0f * layout.ui_scale };

		if (bhs_ui_button(ctx, "ATIVAR", r_yes)) {
			/* Apply Pending */
			state->show_visual_warning = false;
			state->visual_warning_accepted = true; /* [FIX] Remember choice */
			
			/* [FIX] Unpause if paused */
			state->req_toggle_pause = true; 

			switch(state->pending_toggle_id) {
				case 0: state->show_gravity_line = true; break;
				case 1: state->show_orbit_trail = true; break;
				case 2: state->show_satellite_orbits = true; break;
				case 3: state->show_planet_markers = true; break;
				case 4: state->show_moon_markers = true; break;
			}
			save_hud_config(state);
		}
		if (bhs_ui_button(ctx, "CANCELAR", r_no)) {
			state->show_visual_warning = false;
			/* [FIX] Just unpause, don't apply */
			state->req_toggle_pause = true; 
		}
	}
	
	/* Cleanup: Restore input state */
	bhs_ui_set_input_blocked(ctx, false);
}

/* Helper: AABB Check */
static bool is_inside(int mx, int my, float x, float y, float w, float h)
{
	return (float)mx >= x && (float)mx <= x + w && (float)my >= y &&
	       (float)my <= y + h;
}

bool bhs_hud_is_mouse_over(bhs_ui_ctx_t ctx, const bhs_hud_state_t *state,
			   int mx, int my, int win_w, int win_h)
{
	bhs_ui_layout_t layout = get_ui_layout(ctx, win_w, win_h);

	/* 1. Top Bar */
	if (is_inside(mx, my, 0, 0, (float)win_w, layout.top_bar_height))
		return true;

	/* 2. Dropdown Menu */
	if (state->active_menu_index != -1) {
		float dropdown_x = layout.tab_start_x;

		/* Calculate offset to active item */
		if (state->active_menu_index < MENU_COUNT) {
			for (int j = 0; j < state->active_menu_index; ++j) {
				/* Same width calc as Draw */
				float text_w = bhs_ui_measure_text(
					ctx, MENU_ITEMS[j],
					layout.font_size_tab);
				float w = text_w + (40.0f * layout.ui_scale);
				dropdown_x += w;
			}
		}

		if (is_inside(mx, my, dropdown_x, layout.top_bar_height,
			      200.0f * layout.ui_scale,
			      600.0f * layout.ui_scale))
			return true;
	}

	/* 2.5 Modal Check (Top Priority) */
	if (state->show_save_modal || state->show_visual_warning) {
		return true; /* Block all input if modal is open */
	}

	/* 3. Info Panel */
	if (state->selected_body_index != -1) {
		float panel_x = (float)win_w - layout.info_panel_w -
				layout.info_panel_margin;
		float panel_y = layout.top_bar_height + 10.0f * layout.ui_scale;
		if (is_inside(mx, my, panel_x, panel_y, layout.info_panel_w,
			      layout.info_panel_h))
			return true;
	}

	/* 4. REFS Panel Hit Test */
	{
		float ui_scale = layout.ui_scale;
		float p_width = 240.0f * ui_scale;
		float p_height = 180.0f * ui_scale;
		float margin = 15.0f * ui_scale;
		float btn_sz = 24.0f * ui_scale;

		/* Check Button */
		float bx, by;
		if (state->refs_collapsed) {
			bx = (float)win_w - btn_sz - margin;
			by = (float)win_h - btn_sz - margin;
			if (is_inside(mx, my, bx, by, btn_sz, btn_sz))
				return true;
		} else {
			/* Check Panel */
			float px = (float)win_w - p_width - margin;
			float py = (float)win_h - p_height - margin;

			/* Button is top-right of panel */
			bx = px + p_width - btn_sz - 5.0f;
			by = py + 5.0f;

			/* If on button */
			if (is_inside(mx, my, bx, by, btn_sz, btn_sz))
				return true;

			if (is_inside(mx, my, px, py, p_width, p_height))
				return true;
		}
	}

	return false;
}
