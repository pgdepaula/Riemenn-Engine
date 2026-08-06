#ifndef RI_LIB_LOADER_IMAGE_H
#define RI_LIB_LOADER_IMAGE_H

#include <stdint.h>

/**
 * @struct ri_image
 * @brief Raw image data container (RAM)
 */
typedef struct ri_image {
	int width;
	int height;
	int channels;  /* 4 = RGBA */
	uint8_t *data; /* Raw pixels. Must be freed. */
} ri_image_t;

/**
 * @brief Load an image from disk into RAM.
 * Ensures 4 channels (RGBA) for GPU compatibility.
 * Returns {0} on failure.
 */
ri_image_t ri_image_load(const char *path);

/**
 * @brief Free image memory.
 */
void ri_image_free(ri_image_t img);

/**
 * @brief Generate a 3D sphere impostor texture.
 * Returns RGBA image. User must free.
 */
ri_image_t ri_image_gen_sphere(int size);

struct ri_planet_desc; /* Forward declare */

/**
 * @brief Generate a planet surface texture using its procedural definition.
 * Iterates over UV space (Equirectangular projection) and calls get_surface_color.
 * 
 * @param desc Pointer to the planet description dealing with the procedural logic.
 * @param width Texture width (e.g. 1024)
 * @param height Texture height (e.g. 512)
 */
ri_image_t ri_image_gen_planet_texture(const struct ri_planet_desc *desc,
					 int width, int height);

/**
 * @brief Downsample image by 4x using box filter (Software Anti-Aliasing).
 * Creates a new image with 1/4 width and 1/4 height.
 * The original image is NOT freed.
 * 
 * @param src Source image (RGBA)
 * @return ri_image_t Downsampled image
 */
ri_image_t ri_image_downsample_4x(ri_image_t src);

#endif /* RI_LIB_LOADER_IMAGE_H */
