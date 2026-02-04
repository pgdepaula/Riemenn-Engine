/**
 * @file test_svg_manual.c
 * @brief Teste manual para o loader SVG
 */

#include "assets/svg_loader.h"
#include <stdio.h>
#include <stdlib.h>

void save_ppm(const char *path, bhs_image_t *img)
{
	FILE *f = fopen(path, "wb");
	if (!f) return;
	
	fprintf(f, "P3\n%d %d\n255\n", img->width, img->height);
	
	for (int i = 0; i < img->width * img->height; i++) {
		uint8_t *p = &img->data[i * 4];
		fprintf(f, "%d %d %d ", p[0], p[1], p[2]);
	}
	
	fclose(f);
	printf("Salvo: %s\n", path);
}

int main(int argc, char **argv)
{
	printf("Iniciando teste SVG...\n");
	
	const char *path = (argc > 1) ? argv[1] : "engine/test_sample.svg";
	bhs_svg_t *svg = bhs_svg_load(path);
	
	if (!svg) {
		fprintf(stderr, "FALHA: Nao carregou SVG '%s'\n", path);
		return 1;
	}
	
	printf("SVG Carregado com sucesso!\n");
	
	/* Rasteriza em tamanho original */
	bhs_image_t img = bhs_svg_rasterize(svg, 1.0f);
	if (img.data) {
		save_ppm("output_1x.ppm", &img);
		bhs_image_free(img);
	} else {
		fprintf(stderr, "FALHA: Rasterizacao retornou nulo\n");
	}

	/* Rasteriza 2x */
	bhs_image_t img2 = bhs_svg_rasterize(svg, 2.0f);
	if (img2.data) {
		save_ppm("output_2x.ppm", &img2);
		bhs_image_free(img2);
	}

	bhs_svg_free(svg);
	printf("Teste concluido.\n");
	return 0;
}
