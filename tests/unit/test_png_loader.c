#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "../../engine/assets/image_loader.h"

static const uint8_t PNG_1x1_RED[] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, /* Signature */
    
    /* IHDR: 13 bytes */
    0x00, 0x00, 0x00, 0x0D, 
    0x49, 0x48, 0x44, 0x52, 
    0x00, 0x00, 0x00, 0x01, /* w=1 */
    0x00, 0x00, 0x00, 0x01, /* h=1 */
    0x08, 0x06, 0x00, 0x00, 0x00,
    0x1F, 0x15, 0xC4, 0x89, /* CRC IHDR */

    /* IDAT: Store Block (No Compression) 
       Data: 00 FF 00 00 FF (Filter 0, R=255, G=0, B=0, A=255)
       
       Structure:
       ZLIB Header: 78 01
       Block: 01 (Final=1, Type=00)
       LEN: 05 00 (5 bytes)
       NLEN: FA FF (~5)
       Data: 00 FF 00 00 FF
       Adler32: 00 00 00 00 (Ignored)
    */
    0x00, 0x00, 0x00, 0x10, /* len=16 */
    0x49, 0x44, 0x41, 0x54, /* 'IDAT' */
    0x78, 0x01,             /* Zlib Header */
    0x01,                   /* Block Header */
    0x05, 0x00,             /* LEN */
    0xFA, 0xFF,             /* NLEN */
    0x00, 0xFF, 0x00, 0x00, 0xFF, /* DATA */
    0x00, 0x00, 0x00, 0x00, /* Adler */

    0x30, 0x22, 0x96, 0x3d, /* CRC IDAT (Calculado fake, ignorado) */

    /* IEND */
    0x00, 0x00, 0x00, 0x00,
    0x49, 0x45, 0x4E, 0x44,
    0xAE, 0x42, 0x60, 0x82
};

int main()
{
    printf("[TEST] Iniciando teste do PNG Loader Custom (Uncompressed IDAT)...\n");

    const char *tmp_path = "test_1x1_raw.png";
    FILE *f = fopen(tmp_path, "wb");
    if (!f) return 1;
    fwrite(PNG_1x1_RED, 1, sizeof(PNG_1x1_RED), f);
    fclose(f);

    bhs_image_t img = bhs_image_load(tmp_path);
    
    if (!img.data) {
        fprintf(stderr, "[FAIL] Imagem nula.\n");
        return 1;
    }
    if (img.width != 1 || img.height != 1) {
        fprintf(stderr, "[FAIL] Dimensao %dx%d\n", img.width, img.height);
        return 1;
    }

    uint8_t r = img.data[0];
    uint8_t g = img.data[1];
    uint8_t b = img.data[2];
    uint8_t a = img.data[3];

    printf("[INFO] Pixel: %02X %02X %02X %02X\n", r, g, b, a);

    if (r != 0xFF || g != 0x00 || b != 0x00 || a != 0xFF) {
        fprintf(stderr, "[FAIL] Cor errada!\n");
        return 1;
    }

    bhs_image_free(img);
    remove(tmp_path);
    printf("[PASS] Sucesso!\n");
    return 0;
}
