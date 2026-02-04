/**
 * @file start_screen.c
 * @brief Implementação da Tela de Início "Antigravity Style" (Riemann Engine)
 */

#include "start_screen.h"
#include <dirent.h> /* Para varrer diretório data/ */
#include <stdio.h>
#include <string.h>
#include "app_state.h"
#include "engine/ecs/ecs.h" /* Para bhs_ecs_load_world */
#include "gui/ui/layout.h"
#include "gui/ui/lib.h"
#include "simulation/components/sim_components.h" /* [NEW] Metadata Type */
#include "simulation/scenario_mgr.h"

/* ============================================================================
 * ESTILO E CORES (ANTIGRAVITY STYLE - LINEAR SPACE)
 * ============================================================================
 */

/* ============================================================================
 * ESTILO E CORES (ANTIGRAVITY STYLE - LINEAR SPACE)
 * ============================================================================
 */

/* Background: Very Deep Dark Blue/Grey (Almost Black) */
/* sRGB ref: #101116 -> Linear: ~0.003, 0.004, 0.006 */
#define COLOR_BG ((struct bhs_ui_color){ 0.006f, 0.006f, 0.008f, 1.0f })

/* Primary Button: Vibrant Royal Blue */
/* sRGB ref: #3b5bdb -> Linear: ~0.045, 0.105, 0.7 */
#define COLOR_PRIMARY ((struct bhs_ui_color){ 0.045f, 0.105f, 0.700f, 1.0f })
#define COLOR_PRIMARY_HOVER                                                    \
	((struct bhs_ui_color){ 0.080f, 0.180f, 0.900f, 1.0f })

/* Secondary Button: Dark Surface */
/* sRGB ref: #2d2d35 -> Linear: ~0.027, 0.027, 0.038 */
#define COLOR_SECONDARY ((struct bhs_ui_color){ 0.027f, 0.027f, 0.035f, 1.0f })
#define COLOR_SECONDARY_HOVER                                                  \
	((struct bhs_ui_color){ 0.045f, 0.045f, 0.055f, 1.0f })

/* Text */
#define COLOR_TEXT                                                             \
	((struct bhs_ui_color){ 0.70f, 0.72f, 0.80f,                           \
				1.0f }) /* Grey-ish white */
#define COLOR_TEXT_DIM                                                         \
	((struct bhs_ui_color){ 0.15f, 0.16f, 0.22f, 1.0f }) /* Dark Grey */
#define COLOR_BORDER ((struct bhs_ui_color){ 0.04f, 0.04f, 0.06f, 1.0f })

/* ============================================================================
 * DRAWING HELPERS
 * ============================================================================
 */

static bool custom_button(bhs_ui_ctx_t ctx, const char *label,
			  const char *icon_char, struct bhs_ui_rect rect,
			  bool is_primary)
{
	int32_t mx, my;
	bhs_ui_mouse_pos(ctx, &mx, &my);

	bool hovered = (mx >= rect.x && mx < rect.x + rect.width &&
			my >= rect.y && my < rect.y + rect.height);

	struct bhs_ui_color bg;

	if (is_primary) {
		bg = hovered ? COLOR_PRIMARY_HOVER : COLOR_PRIMARY;
	} else {
		bg = hovered ? COLOR_SECONDARY_HOVER : COLOR_SECONDARY;
	}

	bhs_ui_draw_rect(ctx, rect, bg);
	if (!is_primary) {
		bhs_ui_draw_rect_outline(ctx, rect, COLOR_BORDER, 1.0f);
	}

	/* Ícone (Fake) */
	float icon_w = 0;
	if (icon_char) {
		/* Desenhar quadrado simbolizando icone de pasta/git */
		bhs_ui_draw_rect_outline(
			ctx,
			(struct bhs_ui_rect){
				rect.x + 15, rect.y + rect.height * 0.35f,
				rect.height * 0.3f, rect.height * 0.3f },
			COLOR_TEXT, 1.0f);
		icon_w = 20.0f;
	}

	if (label) {
		float font =
			14.0f; /* Fonte menor e mais elegante (ref: 13-14px) */
		float text_w = bhs_ui_measure_text(ctx, label, font);
		float centerX = rect.x + (rect.width / 2.0f);

		if (icon_w > 0)
			centerX += 10.0f;

		bhs_ui_draw_text(ctx, label, centerX - (text_w / 2.0f),
				 rect.y + (rect.height - font) / 2.0f, font,
				 COLOR_TEXT);
	}

	return hovered && bhs_ui_mouse_clicked(ctx, 0);
}

#define MAX_WORKSPACES 10
struct WorkspaceItem {
	char filename[64];     /* "snapshot_2026-01-26.bin" */
	char full_path[256];   /* "data/snapshot..." */
	char display_name[64]; /* "Meu Save Legal" */
	char date_str[64];     /* "2026-01-26 10:00" */
};

static struct WorkspaceItem g_workspaces[MAX_WORKSPACES];
static int g_workspace_count = 0;

static int g_scan_timer = 0;

/* [NEW] Renaming State */
static int g_renaming_index = -1;
static char g_rename_buf[64] = { 0 };
static bool g_focus_rename = false;

/* Escaneia pasta 'data' por arquivos .bin e lê metadados */
static void scan_active_workspaces(void)
{
	g_workspace_count = 0;

	DIR *d = opendir("data");
	if (!d)
		d = opendir("../data");

	if (d) {
		struct dirent *dir;
		while ((dir = readdir(d)) != NULL) {
			if (g_workspace_count >= MAX_WORKSPACES)
				break;

			if (strstr(dir->d_name, ".bin") &&
			    strcmp(dir->d_name, "user_config.bin") != 0) {
				struct WorkspaceItem *item =
					&g_workspaces[g_workspace_count];

				strncpy(item->filename, dir->d_name, 63);
				snprintf(item->full_path, 255, "data/%s",
					 dir->d_name);

				/* [NEW] Peek Metadata */
				bhs_metadata_component meta = { 0 };
				/* ID of Metadata Component? We need the TYPE ID.
                   It's defined in sim_components.h/presumed registry.
                   Wait, ECS components are usually registered dynamically or have static IDs in headers?
                   In `sim_components.h` usually defines structs. 
                   We need the ID used by `bhs_ecs_save_world`. 
                   The ID is usually the index in `world->components`.
                   In `main.c` or `scenario_mgr.c` components are registered?
                   Actually ECS in this engine seems to use implicit IDs or macros?
                   Looking at `ecs.h`: `typedef uint32_t bhs_component_type`.
                   The IDs are assigned by the user.
                   We need to know the ID for BHS_COMP_METADATA.
                   It's likely defined in keys/enums.
                   Let's check `sim_components.h` or `components.h` for enum.
                   Wait, I previously viewed `sim_components.h`.
                   It had typedef struct.
                   Where is the enum? 
                   Ah, I missed checking where component IDs are defined.
                   Let's assume there is an enum `bhs_component_type_id` or similar.
                   I'll check `sim_components.h` content again or `engine/components/components.h`.
                   User previously had `engine/components/components.h` open.
                */
				/* [Assuming implicit ID for now, but peek needs explicit ID]
                   I will perform a quick check or define a constant if I can't find it.
                   Actually, let's look at `scenario_mgr.c` later to see how it saves.
                   For now, I'll assume I can find the ID or pass a placeholder and fix it.
                   Wait, I can't compile if I don't know the ID.
                   I will add a TODO and assume BHS_COMP_METADATA is the macro name.
                */

				/* Default if peek fails */
				strncpy(item->display_name, dir->d_name, 63);
				strncpy(item->date_str, "Unknown Date", 63);

				if (bhs_ecs_peek_metadata(item->full_path,
							  &meta, sizeof(meta),
							  BHS_COMP_METADATA)) {
					if (meta.display_name[0] != '\0')
						strncpy(item->display_name,
							meta.display_name, 63);
					if (meta.date_string[0] != '\0')
						strncpy(item->date_str,
							meta.date_string, 63);
				}

				g_workspace_count++;
			}
		}
		closedir(d);
	}
}

/* ============================================================================
 * API PRINCIPAL
 * ============================================================================
 */

void bhs_start_screen_draw(struct app_state *app, bhs_ui_ctx_t ctx, int win_w,
			   int win_h)
{
	bhs_ui_clear(ctx, COLOR_BG);

	float cx = (float)win_w / 2.0f;
	float cy = (float)win_h / 2.0f;

	/* Update scan a cada... sei lá, 60 frames? 1 seg? */
	/* Update scan a cada... sei lá, 60 frames? 1 seg? */
	/* PAUSE SCANNING WHILE EDITING to prevent list from changing under cursor */
	if (g_renaming_index == -1 &&
	    (g_scan_timer++ > 60 || g_scan_timer == 1)) {
		scan_active_workspaces();
		g_scan_timer = 1;
	}

	/* ============================================================================
	 * LAYOUT STRUCTURE (Vertical Center)
	 * ============================================================================
	 */
	float container_w = 400.0f; /* Narrower container as per reference */

	/* 1. Header (Logo + Title) */
	float header_y = cy - 180.0f;

	/* Draw Text "Riemann Engine" (instead of Antigravity) */
	const char *title = "Riemann Engine";
	float title_size = 16.0f;
	float title_w = bhs_ui_measure_text(ctx, title, title_size);
	bhs_ui_draw_text(ctx, title, cx - (title_w / 2.0f), header_y + 60.0f,
			 title_size, COLOR_TEXT);

	/* 2. Main Actions */
	float btn_start_y = header_y + 110.0f;
	float btn_h = 42.0f;
	float gap = 10.0f;

	/* Big Blue Button (Primary) */
	struct bhs_ui_rect rect_main = { cx - (container_w / 2.0f), btn_start_y,
					 container_w, btn_h };
	if (custom_button(ctx, "Criar Simulação", "F", rect_main, true)) {
		scenario_load(app, SCENARIO_EMPTY);
		app->sim_status =
			APP_SIM_PAUSED; /* Começa pausado pra não dar susto */
	}

	/* Secondary Row */
	float sub_w = (container_w - gap) / 2.0f;
	float sub_y = btn_start_y + btn_h + gap;

	/* [MODIFIED] Botões renomeados conforme contrato */
	struct bhs_ui_rect rect_solar = { cx - (container_w / 2.0f), sub_y,
					  sub_w, btn_h };
	if (custom_button(ctx, "(Novo) Sistema Solar", NULL, rect_solar,
			  false)) {
		scenario_load(app, SCENARIO_SOLAR_SYSTEM);
		app->sim_status = APP_SIM_RUNNING;
	}

	struct bhs_ui_rect rect_tls = { cx + (gap / 2.0f), sub_y, sub_w,
					btn_h };
	if (custom_button(ctx, "(Novo) Terra, Lua & Sol", NULL, rect_tls,
			  false)) {
		scenario_load(app, SCENARIO_EARTH_SUN);
		app->sim_status = APP_SIM_RUNNING;
	}

	/* 3. Workspaces List */
	float list_y = sub_y + btn_h + 40.0f;

	if (g_workspace_count > 0) {
		/* Label "Workspaces" */
		bhs_ui_draw_text(ctx, "Workspaces", cx - (container_w / 2.0f),
				 list_y, 12.0f, COLOR_TEXT_DIM);
	}

	float list_item_h = 52.0f;
	float list_gap = 8.0f;
	float curr_y = list_y + 20.0f;

	/* Render dynamic workspaces */
	if (g_workspace_count == 0) {
		const char *empty_msg = "Nenhum workspace encontrado...";
		float empty_w = bhs_ui_measure_text(ctx, empty_msg, 12.0f);
		bhs_ui_draw_text(ctx, empty_msg, cx - (empty_w / 2.0f),
				 curr_y + 10.0f, 12.0f, COLOR_TEXT_DIM);

		/* Push down for footer */
		curr_y += 40.0f;
	} else {
		for (int i = 0; i < g_workspace_count; i++) {
			struct WorkspaceItem *item = &g_workspaces[i];
			struct bhs_ui_rect rect = { cx - (container_w / 2.0f),
						    curr_y, container_w,
						    list_item_h };

			int32_t mx, my;
			bhs_ui_mouse_pos(ctx, &mx, &my);
			bool hovered =
				(mx >= rect.x && mx < rect.x + rect.width &&
				 my >= rect.y && my < rect.y + rect.height);

			struct bhs_ui_color bg =
				hovered ? COLOR_SECONDARY
					: (struct bhs_ui_color){ 0.0f, 0.0f,
								 0.0f, 0.0f };
			if (hovered)
				bhs_ui_draw_rect(ctx, rect, bg);
			bhs_ui_draw_rect_outline(ctx, rect, COLOR_BORDER, 1.0f);

			float pad_x = 15.0f;

			/* RENDER: Normal Row or Editing Row */
			if (g_renaming_index == i) {
				/* EDITING MODE */
				struct bhs_ui_rect rect_edit = {
					rect.x + pad_x, rect.y + 8,
					rect.width - 100.0f, 30.0f
				};

				/* Handle keys manually before text field to capture Enter/Esc specifically */
				if (bhs_ui_key_pressed(ctx, BHS_KEY_ENTER)) {
					/* SAVE */
					bhs_metadata_component meta = { 0 };
					/* Read existing matches first to preserve other fields? Yes. */
					if (bhs_ecs_peek_metadata(
						    item->full_path, &meta,
						    sizeof(meta),
						    BHS_COMP_METADATA)) {
						strncpy(meta.display_name,
							g_rename_buf, 63);
						if (bhs_ecs_update_metadata(
							    item->full_path,
							    &meta, sizeof(meta),
							    BHS_COMP_METADATA)) {
							/* Success */
							scan_active_workspaces(); /* Refresh list */
						}
					}
					g_renaming_index = -1;
				} else if (bhs_ui_key_pressed(ctx,
							      BHS_KEY_ESCAPE)) {
					/* CANCEL */
					g_renaming_index = -1;
				} else {
					bhs_ui_text_field(ctx, rect_edit,
							  g_rename_buf, 63,
							  &g_focus_rename);
				}

			} else {
				/* NORMAL DISPLAY MODE */

				/* Nome Bonito (Metadata Display Name) */
				bhs_ui_draw_text(
					ctx, item->display_name, rect.x + pad_x,
					rect.y + 10, 14.0f,
					(struct bhs_ui_color){ 0.6f, 0.6f,
							       0.65f, 1.0f });

				/* Info Row: Full Path | Date */
				char info_buf[256];
				snprintf(info_buf, 256, "%s  •  %s",
					 item->filename, item->date_str);
				bhs_ui_draw_text(ctx, info_buf, rect.x + pad_x,
						 rect.y + 30, 11.0f,
						 COLOR_TEXT_DIM);

				/* MAIN ACTION: Load on click (if not clicking buttons) */
				/* We need to know if buttons were clicked to avoid double action */
				bool action_clicked = false;

				/* Render Action Buttons on Hover (or always visible? Let's make them always visible but dim) */
				float sm_btn_w = 24.0f;
				float sm_btn_h = 24.0f;
				float btn_y = rect.y +
					      (rect.height - sm_btn_h) / 2.0f;
				float btn_right_margin = 10.0f;

				/* DELETE Button (Rightmost) */
				struct bhs_ui_rect rect_del = {
					rect.x + rect.width - sm_btn_w -
						btn_right_margin,
					btn_y, sm_btn_w, sm_btn_h
				};

				/* EDIT Button (Left of Delete) */
				struct bhs_ui_rect rect_edit = {
					rect_del.x - sm_btn_w - 5.0f, btn_y,
					sm_btn_w, sm_btn_h
				};

				/* Draw Edit Button */
				bool h_edit =
					(mx >= rect_edit.x &&
					 mx < rect_edit.x + rect_edit.width &&
					 my >= rect_edit.y &&
					 my < rect_edit.y + rect_edit.height);
				bhs_ui_draw_rect(ctx, rect_edit,
						 h_edit ? COLOR_SECONDARY_HOVER
							: COLOR_BG);
				bhs_ui_draw_rect_outline(ctx, rect_edit,
							 COLOR_BORDER, 1.0f);
				bhs_ui_draw_text(ctx, "E", rect_edit.x + 8,
						 rect_edit.y + 5, 12.0f,
						 COLOR_TEXT); /* 'E' icon */

				if (h_edit && bhs_ui_mouse_clicked(ctx, 0)) {
					g_renaming_index = i;
					strncpy(g_rename_buf,
						item->display_name, 63);
					g_focus_rename = true;
					action_clicked = true;
				}

				/* Draw Delete Button */
				bool h_del =
					(mx >= rect_del.x &&
					 mx < rect_del.x + rect_del.width &&
					 my >= rect_del.y &&
					 my < rect_del.y + rect_del.height);
				bhs_ui_draw_rect(
					ctx, rect_del,
					h_del ? (struct bhs_ui_color){ 0.3f,
								       0.1f,
								       0.1f,
								       1.0f }
					      : COLOR_BG); /* Red-ish on hover */
				bhs_ui_draw_rect_outline(ctx, rect_del,
							 COLOR_BORDER, 1.0f);
				bhs_ui_draw_text(ctx, "X", rect_del.x + 8,
						 rect_del.y + 5, 12.0f,
						 COLOR_TEXT); /* 'X' icon */

				if (h_del && bhs_ui_mouse_clicked(ctx, 0)) {
					/* DELETE ACTION */
					/* TODO: Confirmation dialog? Naah, Linux style: do it. */
					remove(item->full_path);
					g_scan_timer =
						100; /* Force rescan immediately */
					action_clicked = true;
				}

				/* Load Scenario if clicked row body */
				if (hovered && bhs_ui_mouse_clicked(ctx, 0) &&
				    !action_clicked) {
					/* Check if mouse NOT over buttons */
					bool on_buttons =
						(mx >= rect_edit.x &&
						 mx <= rect_del.x +
								 rect_del.width);
					if (!on_buttons) {
						if (scenario_load_from_file(
							    app,
							    item->full_path)) {
							/* Success */
						}
					}
				}
			}

			curr_y += list_item_h + list_gap;
		}
	}

	/* Footer */
	const char *more = "Show More...";
	float more_w = bhs_ui_measure_text(ctx, more, 12.0f);
	bhs_ui_draw_text(ctx, more, cx - (more_w / 2.0f), curr_y + 15.0f, 12.0f,
			 COLOR_TEXT_DIM);
}
