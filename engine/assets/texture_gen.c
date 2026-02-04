/**
 * @file texture_gen.c
 * @brief Gerador Procedural de Texturas Planetárias
 *
 * Conecta o asset system à definição lógica do planeta (src/simulation/data).
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "image_loader.h"
#include "math/mat4.h" /* Para math basics se precisar */
#include "src/simulation/data/planet.h"

#ifndef PI
#define PI 3.14159265359f
#endif

bhs_image_t bhs_image_gen_planet_texture(const struct bhs_planet_desc *desc,
					 int width, int height)
{
	bhs_image_t img = { 0 };

	if (!desc || width <= 0 || height <= 0)
		return img;

	img.width = width;
	img.height = height;
	img.channels = 4;
	img.data = malloc(width * height * 4);

	if (!img.data)
		return img;

	/* 
	 * Equirectangular Projection (Plate Carrée)
	 * U [0, 1] -> Longitude [-PI, PI]
	 * V [0, 1] -> Latitude [PI/2, -PI/2] (Top to Bottom)
	 */

	for (int y = 0; y < height; y++) {
		/* Normalized V (0 top, 1 bottom) */
		float v = (float)y / (float)(height - 1);

		/* Latitude: +PI/2 (North) -> -PI/2 (South) */
		float lat = (1.0f - v) * PI - (PI * 0.5f);

		float cos_lat = cosf(lat);
		float sin_lat = sinf(lat);

		for (int x = 0; x < width; x++) {
			/* Normalized U (0 left, 1 right) */
			float u = (float)x / (float)(width - 1);

			/* Longitude: -PI -> +PI */
			float lon = u * 2.0f * PI - PI;

			/* Spherical to Cartesian (Radius = 1) */
			/* x = r sin(lat) cos(lon) -- wait, math convention varies. */
			/* Let's align with computer graphics (Y up) or Z up? */
			/* Planet.h likely expects Z-up or geometric center. */
			/* Ponto na esfera unitária "local space" */
			struct bhs_vec3 p_local;

			/* Standard physics (ISO):
			   x = r * cos(lat) * cos(lon)
			   y = r * cos(lat) * sin(lon)
			   z = r * sin(lat)
			*/
			p_local.x = cos_lat * cosf(lon);
			p_local.y = cos_lat * sinf(lon);
			p_local.z = sin_lat;

			/* Call the planet's generator */
			struct bhs_vec3 color = { 0 };

			if (desc->get_surface_color) {
				color = desc->get_surface_color(p_local);
			} else {
				/* Fallback to base color */
				color = desc->base_color;
			}

			/* Write to buffer (RGBA8) */
			uint8_t *pixel = &img.data[(y * width + x) * 4];

			/* Clamp 0..1 -> 0..255 */
			pixel[0] = (uint8_t)(fminf(color.x, 1.0f) * 255.0f);
			pixel[1] = (uint8_t)(fminf(color.y, 1.0f) * 255.0f);
			pixel[2] = (uint8_t)(fminf(color.z, 1.0f) * 255.0f);
			pixel[3] = 255; /* Opaque */
		}
	}

	return img;
}
