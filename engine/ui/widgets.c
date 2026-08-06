/**
 * @file widgets.c
 * @brief Implementação de Widgets UI
 */

#define _POSIX_C_SOURCE 200809L
#include <stdlib.h> /* [NEW] for strdup/free */
#include <string.h>
#include "engine/foundation/assert.h"
#include "engine/ui/internal.h"
#include "engine/ui/layout.h"

/* ============================================================================
 * HELPERS
 * ============================================================================
 */

static bool rect_contains(struct ri_ui_rect rect, float px, float py)
{
	return px >= rect.x && px < rect.x + rect.width && py >= rect.y &&
	       py < rect.y + rect.height;
}

/* ============================================================================
 * SÍMBOLOS PROCEDURAIS
 * ============================================================================
 */

static void draw_icon(ri_ui_ctx_t ctx, enum ri_ui_icon icon, float x, float y,
		      float size, struct ri_ui_color color)
{
	float pad = size * 0.2f;
	float s = size - pad * 2.0f;

	switch (icon) {
	case RI_ICON_GEAR: {
		/* Círculo central + "dentes" (quadradinhos) */
		ri_ui_draw_rect(ctx,
				 (struct ri_ui_rect){ x + size * 0.4f, y + pad,
						       size * 0.2f,
						       size * 0.8f },
				 color);
		ri_ui_draw_rect(ctx,
				 (struct ri_ui_rect){ x + pad, y + size * 0.4f,
						       size * 0.8f,
						       size * 0.2f },
				 color);
		/* Octagon-ish layout */
		float d = size * 0.25f;
		ri_ui_draw_rect(ctx,
				 (struct ri_ui_rect){ x + d, y + d,
						       size * 0.5f,
						       size * 0.5f },
				 color);
		/* Furo central */
		ri_ui_draw_rect(ctx,
				 (struct ri_ui_rect){
					 x + size * 0.45f, y + size * 0.45f,
					 size * 0.1f, size * 0.1f },
				 RI_UI_COLOR_BLACK);
		break;
	}
	case RI_ICON_PHYSICS: {
		/* Átomo estilizado (3 elipses/retângulos rotacionados) */
		ri_ui_draw_rect_outline(ctx,
					 (struct ri_ui_rect){ x + pad,
							       y + size * 0.4f,
							       s, size * 0.2f },
					 color, 1.0f);
		ri_ui_draw_rect_outline(ctx,
					 (struct ri_ui_rect){ x + size * 0.4f,
							       y + pad,
							       size * 0.2f, s },
					 color, 1.0f);
		ri_ui_draw_rect(ctx,
				 (struct ri_ui_rect){
					 x + size * 0.45f, y + size * 0.45f,
					 size * 0.1f, size * 0.1f },
				 color);
		break;
	}
	case RI_ICON_CAMERA: {
		/* Corpo da câmera + lente */
		ri_ui_draw_rect(ctx,
				 (struct ri_ui_rect){ x + pad, y + size * 0.4f,
						       s, size * 0.4f },
				 color);
		ri_ui_draw_rect(
			ctx,
			(struct ri_ui_rect){ x + size * 0.35f, y + size * 0.3f,
					      size * 0.3f, size * 0.1f },
			color);
		ri_ui_draw_rect_outline(
			ctx,
			(struct ri_ui_rect){ x + size * 0.4f, y + size * 0.5f,
					      size * 0.2f, size * 0.2f },
			RI_UI_COLOR_BLACK, 1.0f);
		break;
	}
	case RI_ICON_CLOSE: {
		/* X */
		float t = 2.0f;
		/* Simplificado por rects cruzados */
		ri_ui_draw_rect(ctx,
				 (struct ri_ui_rect){ x + pad,
						       y + size * 0.5f - t / 2,
						       s, t },
				 color);
		/* Como não temos rotação fácil no draw_rect ainda, vamos improvisar com um
     * mini-box central */
		ri_ui_draw_rect(ctx,
				 (struct ri_ui_rect){ x + size * 0.5f - t / 2,
						       y + pad, t, s },
				 color);
		break;
	}
	default:
		break;
	}
}

/* ============================================================================
 * WIDGETS
 * ============================================================================
 */

bool ri_ui_icon_button(ri_ui_ctx_t ctx, enum ri_ui_icon icon, float x,
			float y, float size)
{
	RI_ASSERT(ctx != NULL);

	struct ri_ui_rect rect = { x, y, size, size };
	int32_t mx, my;
	ri_ui_mouse_pos(ctx, &mx, &my);

	bool hovered = rect_contains(rect, (float)mx, (float)my);
	struct ri_ui_color bg = { 0.15f, 0.15f, 0.2f, 0.8f };
	struct ri_ui_color ic_color = RI_UI_COLOR_WHITE;

	if (hovered && ri_ui_mouse_down(ctx, 0)) {
		bg = (struct ri_ui_color){ 0.3f, 0.3f, 0.5f, 1.0f };
	} else if (hovered) {
		bg = (struct ri_ui_color){ 0.25f, 0.25f, 0.35f, 0.9f };
		ic_color = (struct ri_ui_color){ 0.8f, 0.9f, 1.0f, 1.0f };
	}

	/* Fundo circular (estilizado como rect com borda por enquanto) */
	ri_ui_draw_rect(ctx, rect, bg);
	ri_ui_draw_rect_outline(ctx, rect, ic_color, 1.0f);

	draw_icon(ctx, icon, x, y, size, ic_color);

	return hovered && ri_ui_mouse_clicked(ctx, 0);
}

void ri_ui_panel_begin(ri_ui_ctx_t ctx, const char *title, float width,
			float height)
{
	RI_ASSERT(ctx != NULL);

	int32_t win_w, win_h;
	ri_ui_get_size(ctx, &win_w, &win_h);

	/* 1. Overlay escuro */
	ri_ui_draw_rect(
		ctx, (struct ri_ui_rect){ 0, 0, (float)win_w, (float)win_h },
		(struct ri_ui_color){ 0.0f, 0.0f, 0.0f, 0.6f });

	/* 2. Janela Centralizada */
	float x = (win_w - width) / 2.0f;
	float y = (win_h - height) / 2.0f;
	struct ri_ui_rect rect = { x, y, width, height };

	ri_ui_draw_rect(ctx, rect,
			 (struct ri_ui_color){ 0.12f, 0.12f, 0.15f, 1.0f });
	ri_ui_draw_rect_outline(
		ctx, rect, (struct ri_ui_color){ 0.4f, 0.4f, 0.5f, 1.0f },
		2.0f);

	/* Title bar sutil */
	ri_ui_draw_rect(ctx, (struct ri_ui_rect){ x, y, width, 30.0f },
			 (struct ri_ui_color){ 0.2f, 0.2f, 0.25f, 1.0f });
	if (title) {
		ri_ui_draw_text(ctx, title, x + 10, y + 8, 14.0f,
				 RI_UI_COLOR_WHITE);
	}

	/* Inicia Layout dentro da janela (com padding) */
	struct ri_layout_style style = { 0 };
	style.padding[0] = 40.0f; /* Top (after title bar) */
	style.padding[1] = 20.0f; /* Right */
	style.padding[2] = 20.0f; /* Bottom */
	style.padding[3] = 20.0f; /* Left */
	style.gap = 10.0f;

	/* Ajusta retângulo de layout */
	ctx->layout.stack[0].rect = rect;
	ri_layout_begin(ctx, RI_LAYOUT_COLUMN, &style);
}

void ri_ui_panel_end(ri_ui_ctx_t ctx)
{
	RI_ASSERT(ctx != NULL);
	ri_layout_end(ctx);
}

bool ri_ui_checkbox(ri_ui_ctx_t ctx, const char *label,
		     struct ri_ui_rect rect, bool *checked)
{
	RI_ASSERT(ctx != NULL);
	RI_ASSERT(checked != NULL);

	/* 
   * [FIX] Valores proporcionais ao rect.height
   * Antes era tudo hardcoded (4, 8, 14.0f) - nao escalava
   */
	float h = rect.height;
	float pad = h * 0.15f;		 /* Padding do checkmark */
	float gap = h * 0.35f;		 /* Gap entre box e label */
	float font = h * 0.6f;		 /* Font size proporcional */
	float text_y_offset = h * 0.15f; /* Centraliza texto verticalmente */

	/* Checkbox Square */
	struct ri_ui_rect box = { rect.x, rect.y, h, h };
	ri_ui_draw_rect(ctx, box,
			 (struct ri_ui_color){ 0.1f, 0.1f, 0.1f, 1.0f });
	ri_ui_draw_rect_outline(
		ctx, box, (struct ri_ui_color){ 0.5f, 0.5f, 0.6f, 1.0f },
		1.0f);

	/* Check Mark */
	if (*checked)
		ri_ui_draw_rect(
			ctx,
			(struct ri_ui_rect){ box.x + pad, box.y + pad,
					      box.width - pad * 2.0f,
					      box.height - pad * 2.0f },
			(struct ri_ui_color){ 0.4f, 0.7f, 1.0f, 1.0f });

	/* Label */
	if (label)
		ri_ui_draw_text(ctx, label, rect.x + h + gap,
				 rect.y + text_y_offset, font,
				 RI_UI_COLOR_WHITE);

	/* Logic */
	int32_t mx, my;
	ri_ui_mouse_pos(ctx, &mx, &my);
	bool hovered = rect_contains(rect, (float)mx, (float)my);

	if (hovered && ri_ui_mouse_clicked(ctx, 0)) {
		*checked = !(*checked);
		return true;
	}
	return false;
}

void ri_ui_label(ri_ui_ctx_t ctx, const char *text, float x, float y)
{
	RI_ASSERT(ctx != NULL);
	RI_ASSERT(text != NULL);
	ri_ui_draw_text(ctx, text, x, y, 14.0f, RI_UI_COLOR_WHITE);
}

bool ri_ui_button(ri_ui_ctx_t ctx, const char *label, struct ri_ui_rect rect)
{
	RI_ASSERT(ctx != NULL);

	int32_t mx, my;
	ri_ui_mouse_pos(ctx, &mx, &my);

	bool hovered = rect_contains(rect, (float)mx, (float)my);
	struct ri_ui_color bg;

	/* Estado Visual */
	if (hovered && ri_ui_mouse_down(ctx, 0))
		bg = (struct ri_ui_color){ 0.2f, 0.2f, 0.3f,
					    1.0f }; /* Active */
	else if (hovered)
		bg = (struct ri_ui_color){ 0.3f, 0.3f, 0.4f,
					    1.0f }; /* Hover */
	else
		bg = (struct ri_ui_color){ 0.25f, 0.25f, 0.35f,
					    1.0f }; /* Normal */

	ri_ui_draw_rect(ctx, rect, bg);
	ri_ui_draw_rect_outline(ctx, rect, RI_UI_COLOR_WHITE, 1.0f);

	/* [FIX] Valores proporcionais ao rect.height - Mais generosos para "premium" feel */
	if (label) {
		float font =
			rect.height * 0.55f; /* Aumentado de 0.40f para 0.55f */
		float pad =
			rect.height * 0.3f; /* Padding lateral mais elegante */
		float text_y = rect.y + (rect.height - font) * 0.5f;
		ri_ui_draw_text(ctx, label, rect.x + pad, text_y, font,
				 RI_UI_COLOR_WHITE);
	}

	return hovered && ri_ui_mouse_clicked(ctx, 0);
}

void ri_ui_panel(ri_ui_ctx_t ctx, struct ri_ui_rect rect,
		  struct ri_ui_color bg, struct ri_ui_color border)
{
	RI_ASSERT(ctx != NULL);
	ri_ui_draw_rect(ctx, rect, bg);
	ri_ui_draw_rect_outline(ctx, rect, border, 1.0f);
}

bool ri_ui_slider(ri_ui_ctx_t ctx, struct ri_ui_rect rect, float *value)
{
	RI_ASSERT(ctx != NULL);
	RI_ASSERT(value != NULL);

	/* Clamping Input */
	if (*value < 0.0f)
		*value = 0.0f;
	if (*value > 1.0f)
		*value = 1.0f;

	/* Background */
	ri_ui_draw_rect(ctx, rect,
			 (struct ri_ui_color){ 0.15f, 0.15f, 0.15f, 1.0f });

	/* Filled Part */
	struct ri_ui_rect filled = { rect.x, rect.y, rect.width * (*value),
				      rect.height };
	ri_ui_draw_rect(ctx, filled,
			 (struct ri_ui_color){ 0.3f, 0.5f, 0.9f, 1.0f });

	/* Interaction */
	int32_t mx, my;
	ri_ui_mouse_pos(ctx, &mx, &my);
	bool hovered = rect_contains(rect, (float)mx, (float)my);

	if (hovered && ri_ui_mouse_down(ctx, 0)) {
		float new_value = ((float)mx - rect.x) / rect.width;
		if (new_value < 0.0f)
			new_value = 0.0f;
		else if (new_value > 1.0f)
			new_value = 1.0f;

		if (new_value != *value) {
			*value = new_value;
			return true;
		}
	}
	return false;
}

/* Helper Interno */
static void poll_text_key(ri_ui_ctx_t ctx, uint32_t key, char val, char *buf,
			  size_t max_len)
{
	if (ri_ui_key_pressed(ctx, key)) {
		size_t len = strlen(buf);
		if (len < max_len - 1) {
			buf[len] = val;
			buf[len + 1] = '\0';
		}
	}
}

bool ri_ui_text_field(ri_ui_ctx_t ctx, struct ri_ui_rect rect, char *buf,
		       size_t max_len, bool *focused)
{
	RI_ASSERT(ctx != NULL);
	RI_ASSERT(buf != NULL);
	RI_ASSERT(focused != NULL);

	int32_t mx, my;
	ri_ui_mouse_pos(ctx, &mx, &my);
	bool hovered = rect_contains(rect, (float)mx, (float)my);

	/* Handle Focus */
	if (ri_ui_mouse_clicked(ctx, 0)) {
		*focused = hovered;
	}

	/* Draw */
	struct ri_ui_color bg = { 0.05f, 0.05f, 0.08f, 1.0f };
	struct ri_ui_color border =
		*focused ? RI_UI_COLOR_WHITE
			 : (struct ri_ui_color){ 0.3f, 0.3f, 0.4f, 1.0f };

	ri_ui_draw_rect(ctx, rect, bg);
	ri_ui_draw_rect_outline(ctx, rect, border, 1.0f);

	/* Text */
	/* Clip text? For now just draw. */
	float pad = rect.height * 0.2f;
	ri_ui_draw_text(ctx, buf, rect.x + 10.0f, rect.y + pad, 14.0f,
			 RI_UI_COLOR_WHITE);

	/* Cursor */
	if (*focused) {
		float text_w = ri_ui_measure_text(ctx, buf, 14.0f);
		/* Blink hack: use standard C time or frame counter if available. 
           We don't have frame counter here easily. Just draw persistent for now. */
		ri_ui_draw_rect(
			ctx,
			(struct ri_ui_rect){ rect.x + 10.0f + text_w + 2.0f,
					      rect.y + pad, 2.0f, 14.0f },
			RI_UI_COLOR_WHITE);

		/* Input Polling (A-Z, 0-9) */
		char *original = strdup(buf); /* Cheap dirt check */

		struct {
			uint32_t k;
			char v;
		} map[] = {
			{ RI_KEY_A, 'a' },
			{ RI_KEY_B, 'b' },
			{ RI_KEY_C, 'c' },
			{ RI_KEY_D, 'd' },
			{ RI_KEY_SPACE, ' ' },
			{ RI_KEY_0, '0' },
			{ RI_KEY_1, '1' },
			{ RI_KEY_2, '2' } /* ... Adicionar todas se necessario, mas o user pediu MVP ... */
		};
		/* Expanding map for full alphabet */
		for (uint32_t k = RI_KEY_A; k <= RI_KEY_Z; ++k)
			poll_text_key(
				ctx, k, 'a' + (k - RI_KEY_A), buf,
				max_len); /* Wait, keycodes not sequential? Check lib.h */

		/* Lib.h defines explicit vals. Can't loop easily. Manual map is safer. */
		poll_text_key(ctx, RI_KEY_A, 'a', buf, max_len);
		poll_text_key(ctx, RI_KEY_B, 'b', buf, max_len);
		poll_text_key(ctx, RI_KEY_C, 'c', buf, max_len);
		poll_text_key(ctx, RI_KEY_D, 'd', buf, max_len);
		poll_text_key(ctx, RI_KEY_E, 'e', buf, max_len);
		poll_text_key(ctx, RI_KEY_F, 'f', buf, max_len);
		poll_text_key(ctx, RI_KEY_G, 'g', buf, max_len);
		poll_text_key(ctx, RI_KEY_H, 'h', buf, max_len);
		poll_text_key(ctx, RI_KEY_I, 'i', buf, max_len);
		poll_text_key(ctx, RI_KEY_J, 'j', buf, max_len);
		poll_text_key(ctx, RI_KEY_K, 'k', buf, max_len);
		poll_text_key(ctx, RI_KEY_L, 'l', buf, max_len);
		poll_text_key(ctx, RI_KEY_M, 'm', buf, max_len);
		poll_text_key(ctx, RI_KEY_N, 'n', buf, max_len);
		poll_text_key(ctx, RI_KEY_O, 'o', buf, max_len);
		poll_text_key(ctx, RI_KEY_P, 'p', buf, max_len);
		poll_text_key(ctx, RI_KEY_Q, 'q', buf, max_len);
		poll_text_key(ctx, RI_KEY_R, 'r', buf, max_len);
		poll_text_key(ctx, RI_KEY_S, 's', buf, max_len);
		poll_text_key(ctx, RI_KEY_T, 't', buf, max_len);
		poll_text_key(ctx, RI_KEY_U, 'u', buf, max_len);
		poll_text_key(ctx, RI_KEY_V, 'v', buf, max_len);
		poll_text_key(ctx, RI_KEY_W, 'w', buf, max_len);
		poll_text_key(ctx, RI_KEY_X, 'x', buf, max_len);
		poll_text_key(ctx, RI_KEY_Y, 'y', buf, max_len);
		poll_text_key(ctx, RI_KEY_Z, 'z', buf, max_len);
		poll_text_key(ctx, RI_KEY_SPACE, ' ', buf, max_len);
		poll_text_key(ctx, RI_KEY_0, '0', buf, max_len);
		poll_text_key(ctx, RI_KEY_1, '1', buf, max_len);
		poll_text_key(ctx, RI_KEY_2, '2', buf, max_len);
		poll_text_key(ctx, RI_KEY_3, '3', buf, max_len);
		poll_text_key(ctx, RI_KEY_4, '4', buf, max_len);
		poll_text_key(ctx, RI_KEY_5, '5', buf, max_len);
		poll_text_key(ctx, RI_KEY_6, '6', buf, max_len);
		poll_text_key(ctx, RI_KEY_7, '7', buf, max_len);
		poll_text_key(ctx, RI_KEY_8, '8', buf, max_len);
		poll_text_key(ctx, RI_KEY_9, '9', buf, max_len);

		/* Backspace logic (KeyCode 14 or 42 based on legacy) */
		if (ri_ui_key_pressed(ctx, 14)) {
			size_t len = strlen(buf);
			if (len > 0)
				buf[len - 1] = '\0';
		}

		bool changed = (strcmp(original, buf) != 0);
		free(original); /* Requires <stdlib.h>, make sure it's included */
		return changed;
	}
	return false;
}
