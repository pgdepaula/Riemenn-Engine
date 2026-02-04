/**
 * @file repro_issue.c
 * @brief Reproduction test for SVG rendering issues (Holes & Curves)
 */

#include "assets/svg_loader.h"
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	printf("Running SVG Repro Test...\n");
	
	/* Load the camera icon which has a hole in the middle */
	bhs_svg_t *svg = bhs_svg_load("src/assets/icons/camera.svg");
	if (!svg) {
		fprintf(stderr, "FAIL: Could not load svg\n");
		return 1;
	}
	
	/* Rasterize at 1.0 scale */
	bhs_image_t img = bhs_svg_rasterize(svg, 1.0f);
	
	/* Check pixel at (32, 20) - this is inside the hole (Radius 18) but outside the inner circle (Radius 10)
	   Center is (32, 34). dist(32, 20) = 14. 10 < 14 < 18.
	   In correct rendering, this should be transparent (the gap).
	   In broken rendering (no holes), this was filled by the 2nd subpath of the main path. */
	
	int x = 32;
	int y = 20;
	
	if (x >= img.width || y >= img.height) {
        printf("FAIL: Image too small (%dx%d)\n", img.width, img.height);
        return 1;
    }

	uint8_t *p = &img.data[(y * img.width + x) * 4];
	printf("Pixel at (%d, %d): R=%d G=%d B=%d A=%d\n", x, y, p[0], p[1], p[2], p[3]);
	
	int passed_camera = 0;
	if (p[3] == 0) {
		printf("Camera Hole Test: PASS (Pixel is transparent)\n");
		passed_camera = 1;
	} else if (p[3] == 255) {
		printf("Camera Hole Test: FAIL (Pixel is opaque)\n");
		passed_camera = 0;
	} else {
        printf("Camera Hole Test: WARN (Pixel alpha %d)\n", p[3]);
        passed_camera = 0;
    }
	
	bhs_image_free(img);
	bhs_svg_free(svg);
	
	/* TEST 2: Parser Command Consumption Bug */
	/* "M10 10h10" should mean Move(10,10), LineRelX(10) -> (20,10).
	   If bug exists, 'h' is eaten by parse_float(10), next char is '1'.
	   It becomes Move(10,10), Implicit LineAbs(10, 0) -> (10,0). */
	printf("\nRunning Parser Consumption Test...\n");
	const char *svg_str = "<svg width='40' height='40'><path d='M10 10h10' stroke='white' stroke-width='2' fill='none'/></svg>";
	bhs_svg_t *svg2 = bhs_svg_parse(svg_str);
    int passed_parser = 0;
	if (!svg2) {
		printf("Parser Test: FAIL (Parse error)\n");
	} else {
        /* Rasterize */
        bhs_image_t img2 = bhs_svg_rasterize(svg2, 1.0f);
        
        /* Check pixel at (15, 10). Should be white if line exists from 10,10 to 20,10. */
        /* Note: stroke is white. */
        /* Wait, our rasterizer IGNORES strokes currently in the previous code? 
           Check svg_loader.c: "if (!s->has_fill) continue;"
           Ah, I need to use FILL for this test or update rasterizer to support stroke. 
           The icons use FILL. `trash.svg` uses fill.
           So let's use a filled rect using path: M10 10 h 10 v 10 h -10 z. 
           If h is eaten, it fails. */
           
        bhs_svg_free(svg2);
        
        /* Use filled shape for test to be safe with current rasterizer limits */
        const char *svg_str_fill = "<svg width='40' height='40'><path d='M10 10h10v10h-10z' fill='white'/></svg>";
        svg2 = bhs_svg_parse(svg_str_fill);
        img2 = bhs_svg_rasterize(svg2, 1.0f);
        
        int tx = 15, ty = 15; /* Center of 10,10 20,20 rect */
        uint8_t *p2 = &img2.data[(ty * img2.width + tx) * 4];
        printf("Pixel at (%d, %d): R=%d A=%d\n", tx, ty, p2[0], p2[3]);
        
        if (p2[3] == 255) {
             printf("Parser Test: PASS (Rect filled)\n");
             passed_parser = 1;
        } else {
             printf("Parser Test: FAIL (Rect missing or malformed)\n");
             passed_parser = 0;
        }
        bhs_image_free(img2);
        bhs_svg_free(svg2);
    }

	return (passed_camera && passed_parser) ? 0 : 1;
}
