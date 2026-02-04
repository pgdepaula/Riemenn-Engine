/**
 * @file image_gen.c
 * @brief Gerador Procedural de Texturas
 */

#include <math.h>
#include <stdlib.h>
#include "image_loader.h"

bhs_image_t bhs_image_gen_sphere(int size)
{
	bhs_image_t img = { 0 };
	img.width = size;
	img.height = size;
	img.channels = 4;
	img.data = malloc(size * size * 4);

	if (!img.data)
		return img;

	/* Lighting params */
	float light_x = -0.5f;
	float light_y = -0.5f;
	float light_z = 0.7071f; /* Normalize later if needed */
	float mag = sqrtf(light_x * light_x + light_y * light_y +
			  light_z * light_z);
	light_x /= mag;
	light_y /= mag;
	light_z /= mag;

	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			uint8_t *p = &img.data[(y * size + x) * 4];
			/* DEBUG: Solid WHITE Square to prove visibility */
			p[0] = 255;
			p[1] = 255;
			p[2] = 255;
			p[3] = 255;
		}
	}

	return img;
}
