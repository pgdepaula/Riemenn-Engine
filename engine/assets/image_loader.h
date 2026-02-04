#ifndef BHS_LIB_LOADER_IMAGE_H
#define BHS_LIB_LOADER_IMAGE_H

#include <stdint.h>

/**
 * @struct bhs_image
 * @brief Raw image data container (RAM)
 */
typedef struct bhs_image {
	int width;
	int height;
	int channels;  /* 4 = RGBA */
	uint8_t *data; /* Raw pixels. Must be freed. */
} bhs_image_t;

/**
 * @brief Load an image from disk into RAM.
 * Ensures 4 channels (RGBA) for GPU compatibility.
 * Returns {0} on failure.
 */
bhs_image_t bhs_image_load(const char *path);

/**
 * @brief Free image memory.
 */
void bhs_image_free(bhs_image_t img);

/**
 * @brief Generate a 3D sphere impostor texture.
 * Returns RGBA image. User must free.
 */
bhs_image_t bhs_image_gen_sphere(int size);

struct bhs_planet_desc; /* Forward declare */

/**
 * @brief Generate a planet surface texture using its procedural definition.
 * Iterates over UV space (Equirectangular projection) and calls get_surface_color.
 * 
 * @param desc Pointer to the planet description dealing with the procedural logic.
 * @param width Texture width (e.g. 1024)
 * @param height Texture height (e.g. 512)
 */
bhs_image_t bhs_image_gen_planet_texture(const struct bhs_planet_desc *desc,
					 int width, int height);

/**
 * @brief Downsample image by 4x using box filter (Software Anti-Aliasing).
 * Creates a new image with 1/4 width and 1/4 height.
 * The original image is NOT freed.
 * 
 * @param src Source image (RGBA)
 * @return bhs_image_t Downsampled image
 */
bhs_image_t bhs_image_downsample_4x(bhs_image_t src);

#endif /* BHS_LIB_LOADER_IMAGE_H */
