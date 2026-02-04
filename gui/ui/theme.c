/**
 * @file theme.c
 * @brief Implementação de Temas
 */

#include "gui/ui/theme.h"

static const struct bhs_theme default_theme = {
    .colors =
        {
            .background = {0.11f, 0.11f, 0.13f, 1.0f}, /* #1c1c21ish */
            .surface = {0.15f, 0.15f, 0.18f, 1.0f},
            .primary = {0.4f, 0.2f, 0.8f, 1.0f}, /* Roxo Cósmico */
            .secondary = {0.2f, 0.8f, 0.8f,
                          1.0f}, /* Ciano Horizonte de Eventos */
            .text = {0.9f, 0.9f, 0.9f, 1.0f},
            .text_dim = {0.6f, 0.6f, 0.6f, 1.0f},
            .border = {0.3f, 0.3f, 0.35f, 1.0f},
            .error = {0.9f, 0.3f, 0.3f, 1.0f},
        },
    .border_radius = 4.0f,
    .border_width = 1.0f,
    .font_size_base = 14.0f,
};

const struct bhs_theme *bhs_theme_get_default(void)
{
	return &default_theme;
}
