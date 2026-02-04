/**
 * @file theme.h
 * @brief Sistema de Temas
 */

#ifndef BHS_UX_LIB_THEME_H
#define BHS_UX_LIB_THEME_H

#include "gui/ui/lib.h"

#ifdef __cplusplus
extern "C" {
#endif

struct bhs_theme_colors {
	struct bhs_ui_color background;
	struct bhs_ui_color surface;
	struct bhs_ui_color primary;
	struct bhs_ui_color secondary;
	struct bhs_ui_color text;
	struct bhs_ui_color text_dim;
	struct bhs_ui_color border;
	struct bhs_ui_color error;
};

struct bhs_theme {
	struct bhs_theme_colors colors;
	float border_radius;
	float border_width;
	float font_size_base;
};

/**
 * bhs_theme_get_default - Retorna o tema padrão (Dark/Dracula-ish)
 */
const struct bhs_theme *bhs_theme_get_default(void);

#ifdef __cplusplus
}
#endif

#endif /* BHS_UX_LIB_THEME_H */
