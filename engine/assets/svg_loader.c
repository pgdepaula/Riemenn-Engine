/**
 * @file svg_loader.c
 * @brief Implementação SVG (Parser XML + Rasterizer Scanline)
 *
 * "XML é como violência. Se não está resolvendo, você não está usando o suficiente."
 * - Mentira, XML é horrível. Por isso esse parser é minimalista e brutal.
 */

#include "svg_loader.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/*                            ESTRUTURAS INTERNAS                            */
/* ========================================================================= */

#define BHS_PI 3.14159265358979323846f

/* Estruturas agora estão em svg_loader.h */

/* ========================================================================= */
/*                              PARSER HELPERS                               */
/* ========================================================================= */

/* Pula whitespace */
static const char *skip_ws(const char *s)
{
	while (*s && isspace(*s))
		s++;
	return s;
}

/* Pula delimitadores de path SVG (vírgula ou espaço) */
static const char *skip_sep(const char *s)
{
	while (*s && (isspace(*s) || *s == ','))
		s++;
	return s;
}

/* Parse float simples ignoring locale (always expects dot) */
static float parse_float(const char **s)
{
	const char *ptr = skip_sep(*s);

	float sign = 1.0f;
	if (*ptr == '-') {
		sign = -1.0f;
		ptr++;
	} else if (*ptr == '+') {
		ptr++;
	}

	float val = 0.0f;
	while (*ptr >= '0' && *ptr <= '9') {
		val = val * 10.0f + (*ptr - '0');
		ptr++;
	}

	if (*ptr == '.') {
		ptr++;
		float frac = 0.1f;
		while (*ptr >= '0' && *ptr <= '9') {
			val += (*ptr - '0') * frac;
			frac *= 0.1f;
			ptr++;
		}
	}

	/* Expoente (e-05 etc) */
	if (*ptr == 'e' || *ptr == 'E') {
		ptr++;
		int exp_sign = 1;
		if (*ptr == '-') {
			exp_sign = -1;
			ptr++;
		} else if (*ptr == '+') {
			ptr++;
		}

		int exp_val = 0;
		while (*ptr >= '0' && *ptr <= '9') {
			exp_val = exp_val * 10 + (*ptr - '0');
			ptr++;
		}
		val = val * powf(10.0f, exp_sign * exp_val);
	}

	/* Pula 'px' etc - REMOVIDO pois quebra comandos compactados como '10l' */
	/* if (*ptr == 'e'...) handle exponent above */
	/* No SVG path data, units are forbidden. In attributes, we stop parsing number and leave suffix. */

	*s = ptr;
	return val * sign;
}

/* Parse cor hex (#RRGGBB) */
static uint32_t parse_color(const char *str)
{
	if (!str)
		return 0;
	while (isspace(*str))
		str++;
	if (*str == '#')
		str++;

	unsigned int r = 0, g = 0, b = 0;
	if (sscanf(str, "%02x%02x%02x", &r, &g, &b) == 3) {
		/* Retorna ABGR (Little Endian para ser RGBA na memoria) */
		return 0xFF000000 | (b << 16) | (g << 8) | r;
	}
	return 0; /* Default preto transparente? Ou erro? */
}

/* ========================================================================= */
/*                           GEOMETRY BUILDER                                */
/* ========================================================================= */

static bhs_shape_t *alloc_shape(void)
{
	bhs_shape_t *shape = calloc(1, sizeof(bhs_shape_t));
	shape->fill_color =
		0xFFFFFFFF; /* Branco opaco default (melhor para UI) */
	shape->has_fill = 1;
	shape->stroke_width = 1.0f;
	shape->opacity = 1.0f;
	return shape;
}

static void free_paths(bhs_path_t *p)
{
	while (p) {
		bhs_path_t *next = p->next;
		free(p->pts);
		free(p);
		p = next;
	}
}

static void free_shapes(bhs_shape_t *s)
{
	while (s) {
		bhs_shape_t *next = s->next;
		free_paths(s->paths);
		free(s);
		s = next;
	}
}

/* Adiciona ponto a um path */
static void add_point(bhs_path_t *path, float x, float y)
{
	path->pts = realloc(path->pts, (path->n_pts + 1) * sizeof(bhs_vec2_t));
	path->pts[path->n_pts].x = x;
	path->pts[path->n_pts].y = y;
	path->n_pts++;
}

/* Cubic Bezier Tessellation (simples e recursiva ou adaptativa) */
static void tess_cubic(bhs_path_t *path, float x1, float y1, float x2, float y2,
		       float x3, float y3, float x4, float y4, int level)
{
	/* Distancia dos pontos de controle ao segmento base */
	float dx = x4 - x1;
	float dy = y4 - y1;
	float d2 = (x2 - x4) * dy - (y2 - y4) * dx;
	float d3 = (x3 - x4) * dy - (y3 - y4) * dx;
	d2 = fabsf(d2);
	d3 = fabsf(d3);

	if ((d2 + d3) * (d2 + d3) < 0.25f * (dx * dx + dy * dy) || level > 10) {
		add_point(path, x4, y4);
		return;
	}

	float x12 = (x1 + x2) * 0.5f;
	float y12 = (y1 + y2) * 0.5f;
	float x23 = (x2 + x3) * 0.5f;
	float y23 = (y2 + y3) * 0.5f;
	float x34 = (x3 + x4) * 0.5f;
	float y34 = (y3 + y4) * 0.5f;
	float x123 = (x12 + x23) * 0.5f;
	float y123 = (y12 + y23) * 0.5f;
	float x234 = (x23 + x34) * 0.5f;
	float y234 = (y23 + y34) * 0.5f;
	float x1234 = (x123 + x234) * 0.5f;
	float y1234 = (y123 + y234) * 0.5f;

	tess_cubic(path, x1, y1, x12, y12, x123, y123, x1234, y1234, level + 1);
	tess_cubic(path, x1234, y1234, x234, y234, x34, y34, x4, y4, level + 1);
}

/* Quadratic Bezier -> Cubic -> Tess */
static void tess_quad(bhs_path_t *path, float x1, float y1, float x2, float y2,
		      float x3, float y3)
{
	tess_cubic(path, x1, y1, x1 + 2.0f / 3.0f * (x2 - x1),
		   y1 + 2.0f / 3.0f * (y2 - y1), x3 + 2.0f / 3.0f * (x2 - x3),
		   y3 + 2.0f / 3.0f * (y2 - y3), x3, y3, 0);
}

/* Parse "d" attribute */
static void parse_path_d(bhs_shape_t *shape, const char *d)
{
	bhs_path_t *path = NULL;
	float cur_x = 0, cur_y = 0;
	float start_x = 0, start_y = 0;
	float c1x, c1y, c2x, c2y;
	float last_c2x = 0, last_c2y = 0; /* Para S/s e T/t reflection */
	char cmd = 0;
	char last_cmd = 0;

	while (*d) {
		d = skip_ws(d);
		if (!*d)
			break;

		if (isalpha(*d)) {
			last_cmd = cmd;
			cmd = *d++;
		}
		/* Se nao for letra, repete o comando anterior (implicito) 
		   Para M vira L */

		/* Reset control point se comando anterior não foi Curva */
		if (last_cmd != 'C' && last_cmd != 'c' && last_cmd != 'S' &&
		    last_cmd != 's' && last_cmd != 'Q' && last_cmd != 'q' &&
		    last_cmd != 'T' && last_cmd != 't') {
			last_c2x = cur_x;
			last_c2y = cur_y;
		}

		switch (cmd) {
		case 'M': /* Move Absolute */
			cur_x = parse_float(&d);
			cur_y = parse_float(&d);

			/* Novo subpath */
			{
				bhs_path_t *new_p =
					calloc(1, sizeof(bhs_path_t));
				new_p->next = shape->paths;
				shape->paths = new_p;
				path = new_p;
				add_point(path, cur_x, cur_y);
				start_x = cur_x;
				start_y = cur_y;
			}
			cmd = 'L'; /* Subsequent coords are lines */
			break;

		case 'm': /* Move Relative */
			cur_x += parse_float(&d);
			cur_y += parse_float(&d);
			{
				bhs_path_t *new_p =
					calloc(1, sizeof(bhs_path_t));
				new_p->next = shape->paths;
				shape->paths = new_p;
				path = new_p;
				add_point(path, cur_x, cur_y);
				start_x = cur_x;
				start_y = cur_y;
			}
			cmd = 'l';
			break;

		case 'L': /* Line To Abs */
			cur_x = parse_float(&d);
			cur_y = parse_float(&d);
			if (path)
				add_point(path, cur_x, cur_y);
			break;
		case 'l': /* Line To Rel */
			cur_x += parse_float(&d);
			cur_y += parse_float(&d);
			if (path)
				add_point(path, cur_x, cur_y);
			break;

		case 'H':
			cur_x = parse_float(&d);
			if (path)
				add_point(path, cur_x, cur_y);
			break;
		case 'h':
			cur_x += parse_float(&d);
			if (path)
				add_point(path, cur_x, cur_y);
			break;
		case 'V':
			cur_y = parse_float(&d);
			if (path)
				add_point(path, cur_x, cur_y);
			break;
		case 'v':
			cur_y += parse_float(&d);
			if (path)
				add_point(path, cur_x, cur_y);
			break;

		case 'C': /* Cubic Bezier Absolute */
			c1x = parse_float(&d);
			c1y = parse_float(&d);
			c2x = parse_float(&d);
			c2y = parse_float(&d);
			{
				float end_x = parse_float(&d);
				float end_y = parse_float(&d);
				if (path)
					tess_cubic(path, cur_x, cur_y, c1x, c1y,
						   c2x, c2y, end_x, end_y, 0);
				cur_x = end_x;
				cur_y = end_y;
				last_c2x = c2x;
				last_c2y = c2y;
			}
			break;

		case 'c': /* Cubic Bezier Relative */
			c1x = cur_x + parse_float(&d);
			c1y = cur_y + parse_float(&d);
			c2x = cur_x + parse_float(&d);
			c2y = cur_y + parse_float(&d);
			{
				float end_x = cur_x + parse_float(&d);
				float end_y = cur_y + parse_float(&d);
				if (path)
					tess_cubic(path, cur_x, cur_y, c1x, c1y,
						   c2x, c2y, end_x, end_y, 0);
				cur_x = end_x;
				cur_y = end_y;
				last_c2x = c2x;
				last_c2y = c2y;
			}
			break;

		case 'S': /* Smooth Cubic Absolute */
		case 's': /* Smooth Cubic Relative */
			/* Reflexão do último control point (c2) em relação ao ponto atual. */
			{
				float next_c2x = parse_float(&d);
				float next_c2y = parse_float(&d);
				float end_x = parse_float(&d);
				float end_y = parse_float(&d);

				if (cmd == 's') {
					next_c2x += cur_x;
					next_c2y += cur_y;
					end_x += cur_x;
					end_y += cur_y;
				}

				/* C1 = reflexao de last_c2 em relacao a cur */
				/* cur + (cur - last_c2) = 2*cur - last_c2 */
				float inf_c1x = 2.0f * cur_x - last_c2x;
				float inf_c1y = 2.0f * cur_y - last_c2y;

				if (path)
					tess_cubic(path, cur_x, cur_y, inf_c1x,
						   inf_c1y, next_c2x, next_c2y,
						   end_x, end_y, 0);
				cur_x = end_x;
				cur_y = end_y;
				last_c2x = next_c2x;
				last_c2y = next_c2y;
			}
			break;

		case 'Q': /* Quad Bezier Abs */
			c1x = parse_float(&d);
			c1y = parse_float(&d);
			{
				float end_x = parse_float(&d);
				float end_y = parse_float(&d);
				if (path)
					tess_quad(path, cur_x, cur_y, c1x, c1y,
						  end_x, end_y);
				cur_x = end_x;
				cur_y = end_y;
				last_c2x = c1x;
				last_c2y = c1y; /* Quad control point */
			}
			break;

		case 'q': /* Quad Bezier Rel */
			c1x = cur_x + parse_float(&d);
			c1y = cur_y + parse_float(&d);
			{
				float end_x = cur_x + parse_float(&d);
				float end_y = cur_y + parse_float(&d);
				if (path)
					tess_quad(path, cur_x, cur_y, c1x, c1y,
						  end_x, end_y);
				cur_x = end_x;
				cur_y = end_y;
				last_c2x = c1x;
				last_c2y = c1y;
			}
			break;

		case 'T': /* Smooth Quad Abs */
		case 't': /* Smooth Quad Rel */
		{
			float end_x = parse_float(&d);
			float end_y = parse_float(&d);
			if (cmd == 't') {
				end_x += cur_x;
				end_y += cur_y;
			}

			/* C1 inferido */
			float inf_c1x = 2.0f * cur_x - last_c2x;
			float inf_c1y = 2.0f * cur_y - last_c2y;

			if (path)
				tess_quad(path, cur_x, cur_y, inf_c1x, inf_c1y,
					  end_x, end_y);
			cur_x = end_x;
			cur_y = end_y;
			last_c2x = inf_c1x;
			last_c2y =
				inf_c1y; /* Para T subsequente, o novo ctrl point é o inferido */
		} break;

		case 'Z':
		case 'z':
			if (path) {
				path->closed = 1;
				add_point(path, start_x, start_y);
				cur_x = start_x;
				cur_y = start_y;
			}
			break;

		default:
			/* Consome floats orfaos para evitar loop infinito se parse falhar */
			parse_float(&d);
			break;
		}
	}
	if (shape->paths) {
		// printf("[SVG] Shape parsed: %d points in first path, fill=%08x\n", shape->paths->n_pts, shape->fill_color);
	}
}

/* ========================================================================= */
/*                              XML PARSER MINI                              */
/* ========================================================================= */

/* Encontra atributo em uma tag: attr="valor" 
 * Agora mais robusto: procura apenas até o fim da tag '>' 
 */
static void parse_attr(const char *tag_start, const char *attr, char *out_val,
		       int max_len)
{
	out_val[0] = 0;
	const char *tag_end = strchr(tag_start, '>');
	if (!tag_end)
		return;

	const char *p = tag_start;
	while (p < tag_end) {
		/* Procura a palavra exata do atributo seguid de '=' ou space */
		if (strncmp(p, attr, strlen(attr)) == 0) {
			const char *check = p + strlen(attr);
			while (p < tag_end && isspace(*check))
				check++;
			if (p < tag_end && *check == '=') {
				p = check + 1;
				goto found;
			}
		}
		p++;
	}
	return;

found:
	while (p < tag_end && isspace(*p))
		p++;

	char quote = 0;
	if (p < tag_end && (*p == '"' || *p == '\''))
		quote = *p++;

	int i = 0;
	while (p < tag_end && i < max_len - 1) {
		if (quote) {
			if (*p == quote)
				break;
		} else {
			if (isspace(*p) || *p == '>')
				break;
		}
		out_val[i++] = *p++;
	}
	out_val[i] = 0;
}

bhs_svg_t *bhs_svg_parse(const char *buffer)
{
	bhs_svg_t *svg = calloc(1, sizeof(bhs_svg_t));
	const char *p = buffer;

	/* Tenta achar tamanho do SVG */
	char val[128];
	if (strstr(p, "<svg")) {
		const char *svg_tag = strstr(p, "<svg");

		parse_attr(svg_tag, "width", val, sizeof(val));
		const char *vptr = val;
		svg->width = parse_float(&vptr);

		parse_attr(svg_tag, "height", val, sizeof(val));
		vptr = val;
		svg->height = parse_float(&vptr);

		/* ViewBox: "x y w h" */
		parse_attr(svg_tag, "viewBox", val, sizeof(val));
		if (val[0]) {
			const char *vptr2 = val;
			parse_float(&vptr2); /* skip x */
			parse_float(&vptr2); /* skip y */
			float vw = parse_float(&vptr2);
			float vh = parse_float(&vptr2);
			if (vw > 0 && vh > 0) {
				/* Sobrescreve width/height internas se viewBox existir */
				svg->width = vw;
				svg->height = vh;
			}
		}

		if (svg->width == 0)
			svg->width = 100;
		if (svg->height == 0)
			svg->height = 100;
	}

	while (*p) {
		const char *tag_start = strchr(p, '<');
		if (!tag_start)
			break;

		if (tag_start[1] == '/') { /* Fecha tag */
			p = strchr(tag_start, '>');
			if (p)
				p++;
			else
				break;
			continue;
		}

		/* Verifica tipo de tag */
		if (strncmp(tag_start, "<path", 5) == 0) {
			/* Parser de Path */
			bhs_shape_t *shape = alloc_shape();

			/* Extrai atributos */
			/* Parse 'd' - precisa alocar buffer grande pois pode ser enorme */
			const char *d_ptr = strstr(tag_start, "d=");
			if (d_ptr) {
				d_ptr += 2;
				char quote = *d_ptr;
				if (quote == '"' || quote == '\'') {
					d_ptr++;
					const char *end_d =
						strchr(d_ptr, quote);
					if (end_d) {
						size_t len = end_d - d_ptr;
						char *d_val = malloc(len + 1);
						memcpy(d_val, d_ptr, len);
						d_val[len] = 0;
						parse_path_d(shape, d_val);
						free(d_val);
					}
				}
			}

			/* Parse Fill */
			parse_attr(tag_start, "fill", val, sizeof(val));
			if (val[0]) {
				if (strcmp(val, "none") == 0)
					shape->has_fill = 0;
				else
					shape->fill_color = parse_color(val);
			}

			/* Parse Stroke */
			parse_attr(tag_start, "stroke", val, sizeof(val));
			if (val[0] && strcmp(val, "none") != 0) {
				shape->has_stroke = 1;
				shape->stroke_color = parse_color(val);
			}

			/* Add to list */
			shape->next = svg->shapes;
			svg->shapes = shape;
		} else if (strncmp(tag_start, "<rect", 5) == 0) {
			bhs_shape_t *shape = alloc_shape();
			float rx = 0, ry = 0, rw = 0, rh = 0;

			parse_attr(tag_start, "x", val, sizeof(val));
			if (val[0]) {
				const char *v = val;
				rx = parse_float(&v);
			}
			parse_attr(tag_start, "y", val, sizeof(val));
			if (val[0]) {
				const char *v = val;
				ry = parse_float(&v);
			}
			parse_attr(tag_start, "width", val, sizeof(val));
			if (val[0]) {
				const char *v = val;
				rw = parse_float(&v);
			}
			parse_attr(tag_start, "height", val, sizeof(val));
			if (val[0]) {
				const char *v = val;
				rh = parse_float(&v);
			}

			/* Converte rect para path */
			bhs_path_t *p_rect = calloc(1, sizeof(bhs_path_t));
			add_point(p_rect, rx, ry);
			add_point(p_rect, rx + rw, ry);
			add_point(p_rect, rx + rw, ry + rh);
			add_point(p_rect, rx, ry + rh);
			p_rect->closed = 1;
			add_point(p_rect, rx, ry);

			shape->paths = p_rect;

			/* Cores */
			parse_attr(tag_start, "fill", val, sizeof(val));
			if (val[0]) {
				if (strcmp(val, "none") == 0)
					shape->has_fill = 0;
				else
					shape->fill_color = parse_color(val);
			}

			shape->next = svg->shapes;
			svg->shapes = shape;
		} else if (strncmp(tag_start, "<circle", 7) == 0) {
			bhs_shape_t *shape = alloc_shape();
			float cx = 0, cy = 0, r = 0;

			parse_attr(tag_start, "cx", val, sizeof(val));
			if (val[0]) {
				const char *v = val;
				cx = parse_float(&v);
			}
			parse_attr(tag_start, "cy", val, sizeof(val));
			if (val[0]) {
				const char *v = val;
				cy = parse_float(&v);
			}
			parse_attr(tag_start, "r", val, sizeof(val));
			if (val[0]) {
				const char *v = val;
				r = parse_float(&v);
			}

			/* Converte círculo para path poligonal (32 segmentos) */
			bhs_path_t *p_circ = calloc(1, sizeof(bhs_path_t));
			int segs = 32;
			for (int i = 0; i <= segs; i++) {
				float ang =
					(float)i * 2.0f * BHS_PI / (float)segs;
				add_point(p_circ, cx + cosf(ang) * r,
					  cy + sinf(ang) * r);
			}
			p_circ->closed = 1;

			shape->paths = p_circ;

			/* Cores */
			parse_attr(tag_start, "fill", val, sizeof(val));
			if (val[0]) {
				if (strcmp(val, "none") == 0)
					shape->has_fill = 0;
				else
					shape->fill_color = parse_color(val);
			}

			shape->next = svg->shapes;
			svg->shapes = shape;
		}

		p = strchr(tag_start, '>');
		if (p)
			p++;
		else
			break;
	}

	// printf("[SVG] Parsed SVG: %dx%d, %p shapes\n", (int)svg->width, (int)svg->height, (void*)svg->shapes);
	// for(bhs_shape_t *s = svg->shapes; s; s=s->next) {
	//     printf("[SVG]   Shape: fill=%d(%08X) stroke=%d(%08X) paths=%p\n",
	//         s->has_fill, s->fill_color, s->has_stroke, s->stroke_color, (void*)s->paths);
	//     for(bhs_path_t *path = s->paths; path; path=path->next) {
	//         printf("[SVG]     Path: %d points\n", path->n_pts);
	//         if(path->n_pts > 0) printf("[SVG]       Start: %.1f, %.1f\n", path->pts[0].x, path->pts[0].y);
	//     }
	// }
	// fflush(stdout);

	return svg;
}

bhs_svg_t *bhs_svg_load(const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return NULL;

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *buf = malloc(size + 1);
	if (!buf) {
		fclose(f);
		return NULL;
	}

	fread(buf, 1, size, f);
	buf[size] = 0;
	fclose(f);

	bhs_svg_t *svg = bhs_svg_parse(buf);
	free(buf);
	return svg;
}

void bhs_svg_free(bhs_svg_t *svg)
{
	if (!svg)
		return;
	free_shapes(svg->shapes);
	free(svg);
}

/* ========================================================================= */
/*                              RASTERIZER SCANLINE                          */
/* ========================================================================= */

#define RAST_FIX_SHIFT 8
#define RAST_FIX_ONE (1 << RAST_FIX_SHIFT)

/* Edge table entry */
typedef struct {
	int y_max;
	int x;	   /* Fixed point */
	int dx_dy; /* Fixed point */
	struct edge *next;
} edge_t;

bhs_image_t bhs_svg_rasterize(const bhs_svg_t *svg, float scale)
{
	int w = (int)(svg->width * scale);
	int h = (int)(svg->height * scale);

	bhs_image_t img = { w, h, 4, calloc(w * h * 4, 1) };
	if (!img.data)
		return img;

	/* Para cada Shape */
	for (bhs_shape_t *s = svg->shapes; s; s = s->next) {
		if (!s->has_fill)
			continue; /* Ignore strokes for now in rasterizer simple version */
		/* Parse color */
		uint8_t r = s->fill_color & 0xFF;
		uint8_t g = (s->fill_color >> 8) & 0xFF;
		uint8_t b = (s->fill_color >> 16) & 0xFF;
		uint8_t a = 255; /* Default opacity 1.0 */

		/* Rasteriza scanline por scanline considerando TODOS os paths do shape juntos
		   Isso implementa implicitamente a regra Even-Odd para furos */
		for (int y = 0; y < h; y++) {
			float ly = (float)y / scale;

			/* Coleta intersecções de TODOS os paths deste shape */
			int cap = 64;
			float *nodes = malloc(cap * sizeof(float));
			int nodes_cnt = 0;

			for (bhs_path_t *p = s->paths; p; p = p->next) {
				if (p->n_pts < 3)
					continue;

				int j = p->n_pts - 1;
				for (int i = 0; i < p->n_pts; i++) {
					float y1 = p->pts[i].y;
					float y2 = p->pts[j].y;
					float x1 = p->pts[i].x;
					float x2 = p->pts[j].x;

					if ((y1 < ly && y2 >= ly) ||
					    (y2 < ly && y1 >= ly)) {
						if (nodes_cnt >= cap) {
							cap *= 2;
							float *new_nodes = realloc(
								nodes,
								cap * sizeof(float));
							if (!new_nodes)
								break;
							nodes = new_nodes;
						}
						nodes[nodes_cnt++] =
							x1 + (ly - y1) /
								     (y2 - y1) *
								     (x2 - x1);
					}
					j = i;
				}
			}

			if (nodes_cnt == 0) {
				free(nodes);
				continue;
			}

			/* Sort nodes */
			for (int i = 0; i < nodes_cnt - 1; i++) {
				for (int k = i + 1; k < nodes_cnt; k++) {
					if (nodes[i] > nodes[k]) {
						float tmp = nodes[i];
						nodes[i] = nodes[k];
						nodes[k] = tmp;
					}
				}
			}

			/* Fill spans */
			for (int i = 0; i < nodes_cnt; i += 2) {
				if (i + 1 >= nodes_cnt)
					break;
				int sx = (int)(nodes[i] * scale);
				int ex = (int)(nodes[i + 1] * scale);

				if (sx < 0)
					sx = 0;
				if (ex >= w)
					ex = w - 1;

				for (int x = sx; x < ex; x++) {
					uint8_t *pix =
						img.data + (y * w + x) * 4;

					/* Simple overwrite for now */
					pix[0] = r;
					pix[1] = g;
					pix[2] = b;
					pix[3] = a;
				}
			}
			free(nodes);
		}
	}

	return img;
}

bhs_image_t bhs_svg_rasterize_fit(const bhs_svg_t *svg, int width, int height)
{
	float sx = (float)width / svg->width;
	float sy = (float)height / svg->height;
	/* Scale uniforme (contain) */
	float scale = (sx < sy) ? sx : sy;

	/* Poderiamos centralizar mas por enquanto top-left */
	return bhs_svg_rasterize(svg, scale);
}
