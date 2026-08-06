/**
 * @file theme.h
 * @brief Sistema de Temas
 */

#ifndef RI_UX_LIB_THEME_H
#define RI_UX_LIB_THEME_H

#include "engine/ui/ui.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ri_theme_colors {
	struct ri_ui_color background;
	struct ri_ui_color surface;
	struct ri_ui_color primary;
	struct ri_ui_color secondary;
	struct ri_ui_color text;
	struct ri_ui_color text_dim;
	struct ri_ui_color border;
	struct ri_ui_color error;
};

struct ri_theme {
	struct ri_theme_colors colors;
	float border_radius;
	float border_width;
	float font_size_base;
};

/**
 * ri_theme_get_default - Retorna o tema padrão (Dark/Dracula-ish)
 */
const struct ri_theme *ri_theme_get_default(void);

#ifdef __cplusplus
}
#endif

#endif /* RI_UX_LIB_THEME_H */
