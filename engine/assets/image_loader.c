/**
 * @file image_loader.c
 * @brief Carregador de Imagens PNG (Implementação Pure C Custom)
 *
 * "Se não roda em hardware de 2010, você está fazendo errado."
 *
 * Implementação limpa de um decodificador PNG + Inflate.
 * Nada de stb_image. Aqui nós construímos nossos próprios bits.
 */

#include "image_loader.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================= */
/*                                CONSTANTES                                 */
/* ========================================================================= */

#define PNG_SIG_SIZE 8
static const uint8_t PNG_SIG[PNG_SIG_SIZE] = {
	137, 80, 78, 71, 13, 10, 26, 10
};

/* Tipos de Chunk (Big Endian - porque endianness é a raiz de todo mal) */
#define CHUNK_IHDR 0x49484452
#define CHUNK_PLTE 0x504C5445
#define CHUNK_IDAT 0x49444154
#define CHUNK_IEND 0x49454E44

/* Comprimento máximo para tabelas Huffman (RFC 1951 - leitura de cabeceira pra quem tem insônia) */
#define MAX_BITS 15
#define MAX_LIT 288
#define MAX_DIST 32
#define CODE_LEN_CODES 19

/* ========================================================================= */
/*                            ESTRUTURAS INTERNAS                            */
/* ========================================================================= */

struct png_ihdr {
	uint32_t width;
	uint32_t height;
	uint8_t bit_depth;
	uint8_t color_type;
	uint8_t compression;
	uint8_t filter;
	uint8_t interlace;
};

struct bit_reader {
	const uint8_t *data;
	size_t size;
	size_t pos;
	uint32_t bit_buf;
	int bit_count;
};

struct huffman_table {
	uint16_t count[MAX_BITS + 1];
	uint16_t symbol[MAX_LIT]; /* Símbolos ordenados pra facilitar a vida */
};

/* ========================================================================= */
/*                              HELPERS UTILITÁRIOS                          */
/* ========================================================================= */

static uint32_t read_be32(const uint8_t *p)
{
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	       ((uint32_t)p[2] << 8) | p[3];
}

static uint32_t read_be32_stream(const uint8_t **stream)
{
	uint32_t v = read_be32(*stream);
	*stream += 4;
	return v;
}

/* ========================================================================= */
/*                                DEFLATE / INFLATE                          */
/* ========================================================================= */

/* Lê bits (LSB primeiro para Deflate, porque alguém achou que seria engraçado) */
static uint32_t bits_peak(struct bit_reader *br, int n)
{
	while (br->bit_count < n) {
		if (br->pos >= br->size)
			return 0; /* EOF inesperado (ou padding, ou o arquivo acabou mesmo) */
		br->bit_buf |= (uint32_t)br->data[br->pos++] << br->bit_count;
		br->bit_count += 8;
	}
	return br->bit_buf & ((1 << n) - 1);
}

static void bits_consume(struct bit_reader *br, int n)
{
	br->bit_buf >>= n;
	br->bit_count -= n;
}

static uint32_t bits_read(struct bit_reader *br, int n)
{
	uint32_t val = bits_peak(br, n);
	bits_consume(br, n);
	return val;
}

/* Tabela de lookup para decode rápido (porque O(n) é para os fracos) */
struct huff_lut {
	uint16_t counts[MAX_BITS + 1];
	uint16_t offsets[MAX_BITS + 1];
	uint16_t
		symbols[MAX_LIT]; /* Símbolos ordenados por len, depois valor */
};

static void build_lut(struct huff_lut *lut, const uint8_t *lens, int n)
{
	memset(lut->counts, 0, sizeof(lut->counts));
	for (int i = 0; i < n; i++)
		lut->counts[lens[i]]++;
	lut->counts[0] = 0;

	lut->offsets[1] = 0;
	for (int i = 2; i <= MAX_BITS; i++) {
		lut->offsets[i] = lut->offsets[i - 1] + lut->counts[i - 1];
	}

	for (int i = 0; i < n; i++) {
		int l = lens[i];
		if (l != 0) {
			lut->symbols[lut->offsets[l]++] = i;
		}
	}

	/* Recalcula offsets para base (sim, de novo, pare de reclamar) */
	lut->offsets[1] = 0;
	for (int i = 2; i <= MAX_BITS; i++) {
		lut->offsets[i] = lut->offsets[i - 1] + lut->counts[i - 1];
	}
}

static int huff_decode_lut(struct bit_reader *br, const struct huff_lut *lut)
{
	int code = 0;
	int first = 0;

	for (int k = 1; k <= MAX_BITS; k++) {
		code = (code << 1) | bits_read(br, 1);
		int count = lut->counts[k];
		if (code < first + count) {
			return lut->symbols[lut->offsets[k] + (code - first)];
		}
		first += count;
		first <<= 1;
	}
	return -1;
}

/* Valores base e bits extras para Length e Distance (Deflate spec 3.2.5 - decore isso pro teste) */
static const uint16_t LEN_BASE[] = { 3,	  4,   5,   6,	 7,  8,	 9,  10,
				     11,  13,  15,  17,	 19, 23, 27, 31,
				     35,  43,  51,  59,	 67, 83, 99, 115,
				     131, 163, 195, 227, 258 };
static const uint8_t LEN_EXTRA[] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
				     1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
				     4, 4, 4, 4, 5, 5, 5, 5, 0 };

static const uint16_t DIST_BASE[] = { 1,    2,	  3,	4,     5,     7,
				      9,    13,	  17,	25,    33,    49,
				      65,   97,	  129,	193,   257,   385,
				      513,  769,  1025, 1537,  2049,  3073,
				      4097, 6145, 8193, 12289, 16385, 24577 };
static const uint8_t DIST_EXTRA[] = { 0, 0, 0,	0,  1,	1,  2,	2,  3,	3,
				      4, 4, 5,	5,  6,	6,  7,	7,  8,	8,
				      9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

static const uint8_t CLEN_ORDER[] = { 16, 17, 18, 0, 8,	 7, 9,	6, 10, 5,
				      11, 4,  12, 3, 13, 2, 14, 1, 15 };

/* O processador Deflate principal - Onde a mágica (e os bugs) acontece */
static int inflate_block(struct bit_reader *br, uint8_t *out, size_t *out_pos,
			 size_t out_cap)
{
	for (;;) {
		int bfinal = bits_read(br, 1);
		int btype = bits_read(br, 2);

		if (btype == 0) { /* Sem compressão */
			/* Alinha com byte boundary */
			bits_consume(br, br->bit_count % 8);

			uint16_t len = bits_read(br, 16);
			uint16_t nlen = bits_read(br, 16);

			if ((len ^ 0xFFFF) != nlen) {
				fprintf(stderr, "[PNG] Block uncompressed com "
						"len invalido\n");
				return -1;
			}

			for (int i = 0; i < len; i++) {
				if (*out_pos >= out_cap)
					return -1;
				out[(*out_pos)++] = bits_read(br, 8);
			}

		} else if (btype == 1 || btype == 2) { /* Fixo ou Dinâmico */
			struct huff_lut lit_lut = { 0 };
			struct huff_lut dist_lut = { 0 };

			if (btype == 1) {
				/* Fixed Huffman codes (Preguiça do compressor) */
				uint8_t lens[288];
				for (int i = 0; i < 144; i++)
					lens[i] = 8;
				for (int i = 144; i < 256; i++)
					lens[i] = 9;
				for (int i = 256; i < 280; i++)
					lens[i] = 7;
				for (int i = 280; i < 288; i++)
					lens[i] = 8;
				build_lut(&lit_lut, lens, 288);

				uint8_t dlens[32];
				for (int i = 0; i < 32; i++)
					dlens[i] = 5;
				build_lut(&dist_lut, dlens, 32);

			} else {
				/* Dynamic Huffman (Agora o bicho pega) */
				int hlit = bits_read(br, 5) + 257;
				int hdist = bits_read(br, 5) + 1;
				int hclen = bits_read(br, 4) + 4;

				uint8_t clens[19] = { 0 };
				for (int i = 0; i < hclen; i++) {
					clens[CLEN_ORDER[i]] = bits_read(br, 3);
				}

				struct huff_lut cl_lut = { 0 };
				build_lut(&cl_lut, clens, 19);

				/* Decode lit/dist lengths */
				uint8_t all_lens[MAX_LIT + MAX_DIST];
				int n = 0;
				while (n < hlit + hdist) {
					int sym = huff_decode_lut(br, &cl_lut);
					if (sym < 16) {
						all_lens[n++] = sym;
					} else if (sym == 16) {
						int copy = bits_read(br, 2) + 3;
						uint8_t prev = all_lens[n - 1];
						while (copy--)
							all_lens[n++] = prev;
					} else if (sym == 17) {
						int copy = bits_read(br, 3) + 3;
						while (copy--)
							all_lens[n++] = 0;
					} else if (sym == 18) {
						int copy =
							bits_read(br, 7) + 11;
						while (copy--)
							all_lens[n++] = 0;
					} else {
						return -1; /* Bad symbol */
					}
				}

				build_lut(&lit_lut, all_lens, hlit);
				build_lut(&dist_lut, all_lens + hlit, hdist);
			}

			/* Decode data loop */
			for (;;) {
				int sym = huff_decode_lut(br, &lit_lut);
				if (sym < 0)
					return -1;

				if (sym < 256) {
					if (*out_pos >= out_cap)
						return -1;
					out[(*out_pos)++] = sym;
				} else if (sym == 256) {
					break; /* End of block */
				} else {
					/* Pointer/Length pair */
					sym -= 257;
					if (sym >= 29)
						return -1;
					int len = LEN_BASE[sym] +
						  bits_read(br, LEN_EXTRA[sym]);

					int dsym =
						huff_decode_lut(br, &dist_lut);
					if (dsym < 0)
						return -1;
					int dist =
						DIST_BASE[dsym] +
						bits_read(br, DIST_EXTRA[dsym]);

					if ((size_t)dist > *out_pos)
						return -1; /* Lookbehind invalid */

					for (int i = 0; i < len; i++) {
						if (*out_pos >= out_cap)
							return -1;
						out[*out_pos] =
							out[*out_pos - dist];
						(*out_pos)++;
					}
				}
			}

		} else {
			return -1; /* Reservado/Erro */
		}

		if (bfinal)
			break;
	}
	return 0;
}

/* ========================================================================= */
/*                                FILTAGEM PNG                               */
/* ========================================================================= */

static uint8_t paeth(uint8_t a, uint8_t b, uint8_t c)
{
	int p = (int)a + (int)b - (int)c;
	int pa = abs(p - (int)a);
	int pb = abs(p - (int)b);
	int pc = abs(p - (int)c);

	if (pa <= pb && pa <= pc)
		return a;
	if (pb <= pc)
		return b;
	return c;
}

static void unfilter_scanline(uint8_t *recon, const uint8_t *scanline,
			      const uint8_t *prev, int stride, int n_bytes,
			      int type)
{
	for (int i = 0; i < n_bytes; i++) {
		uint8_t x = scanline[i];
		uint8_t a = (i >= stride) ? recon[i - stride] : 0;
		uint8_t b = prev ? prev[i] : 0;
		uint8_t c = (prev && i >= stride) ? prev[i - stride] : 0;

		switch (type) {
		case 0:
			recon[i] = x;
			break; /* None */
		case 1:
			recon[i] = x + a;
			break; /* Sub */
		case 2:
			recon[i] = x + b;
			break; /* Up */
		case 3:
			recon[i] = x + (a + b) / 2;
			break; /* Average */
		case 4:
			recon[i] = x + paeth(a, b, c);
			break; /* Paeth */
		default:
			recon[i] = x;
			break;
		}
	}
}

/* ========================================================================= */
/*                                API PÚBLICA                                */
/* ========================================================================= */

bhs_image_t bhs_image_load(const char *path)
{
	bhs_image_t result = { 0 };
	FILE *f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "[PNG] Nao abriu arquivo: %s\n", path);
		return result;
	}

	/* Carregar arquivo para memoria (simplicidade) */
	fseek(f, 0, SEEK_END);
	size_t fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	uint8_t *raw = malloc(fsize);
	if (!raw || fread(raw, 1, fsize, f) != fsize) {
		fclose(f);
		free(raw);
		return result;
	}
	fclose(f);

	if (memcmp(raw, PNG_SIG, PNG_SIG_SIZE) != 0) {
		fprintf(stderr, "[PNG] Assinatura invalida: %s\n", path);
		free(raw);
		return result;
	}

	const uint8_t *cur = raw + PNG_SIG_SIZE;
	struct png_ihdr ihdr = { 0 };
	uint8_t *idat_buf = NULL;
	size_t idat_size = 0;

	/* Parse Chunks */
	while (cur < raw + fsize) {
		uint32_t len = read_be32_stream(&cur);
		uint32_t type = read_be32_stream(&cur);
		const uint8_t *data_ptr = cur;
		cur += len;
		read_be32_stream(&cur); /* Skip CRC */

		if (type == CHUNK_IHDR) {
			ihdr.width = read_be32(data_ptr);
			ihdr.height = read_be32(data_ptr + 4);
			ihdr.bit_depth = data_ptr[8];
			ihdr.color_type = data_ptr[9];
			ihdr.compression = data_ptr[10];
			ihdr.filter = data_ptr[11];
			ihdr.interlace = data_ptr[12];

			if (ihdr.bit_depth != 8 ||
			    (ihdr.color_type != 2 && ihdr.color_type != 6)) {
				fprintf(stderr, "[PNG] Suporte apenas para "
						"8-bit RGB/RGBA. Foi mal.\n");
				goto error;
			}
			if (ihdr.interlace != 0) {
				fprintf(stderr, "[PNG] Interlace nao suportado "
						"(quem usa isso?).\n");
				goto error;
			}

		} else if (type == CHUNK_IDAT) {
			/* Concatena IDATs */
			uint8_t *new_buf = realloc(idat_buf, idat_size + len);
			if (!new_buf)
				goto error;
			idat_buf = new_buf;
			memcpy(idat_buf + idat_size, data_ptr, len);
			idat_size += len;

		} else if (type == CHUNK_IEND) {
			break;
		}
	}

	/* Descomprimir IDAT */
	/* ZLIB header check (RFC 1950) */
	if (idat_buf[0] != 0x78) { /* Deflate default 32k wnd */
		fprintf(stderr, "[PNG] Zlib header estranho, ignorando: %02x\n",
			idat_buf[0]);
	}

	int channels = (ihdr.color_type == 6) ? 4 : 3;
	int stride = ihdr.width * channels;
	/* +1 byte de filter type por linha */
	size_t raw_scanline_size = stride + 1;
	size_t uncompressed_size = raw_scanline_size * ihdr.height;

	uint8_t *inflated = malloc(uncompressed_size);
	if (!inflated)
		goto error;

	struct bit_reader br = {
		.data = idat_buf + 2, /* Pula header ZLIB (2 bytes) */
		.size = idat_size - 2 -
			4, /* Exclui header e Adler32 (4 bytes) */
		.bit_count = 0,
		.bit_buf = 0
	};

	size_t out_pos = 0;
	if (inflate_block(&br, inflated, &out_pos, uncompressed_size) != 0) {
		fprintf(stderr, "[PNG] Falha no inflate.\n");
		free(inflated);
		goto error;
	}

	if (out_pos != uncompressed_size) {
		fprintf(stderr,
			"[PNG] Tamanho descomprimido mismatch: %zu vs %zu\n",
			out_pos, uncompressed_size);
		/* free(inflated); goto error; Warning apenas */
	}

	/* Unfilter & RGB->RGBA conversion */
	result.width = ihdr.width;
	result.height = ihdr.height;
	result.channels = 4; /* Engine pede sempre RGBA */
	result.data = malloc(result.width * result.height * 4);
	if (!result.data) {
		free(inflated);
		goto error;
	}

	uint8_t *scanline_prev = NULL;
	uint8_t *scanline_recon = malloc(stride);

	for (uint32_t y = 0; y < ihdr.height; y++) {
		uint8_t *src = inflated + y * raw_scanline_size;
		uint8_t type = *src++; /* Filter type */

		unfilter_scanline(scanline_recon, src, scanline_prev, channels,
				  stride, type);

		/* Copia para imagem final (convertendo para RGBA se necessario) */
		uint8_t *dst_row = result.data + y * result.width * 4;
		for (int x = 0; x < result.width; x++) {
			if (channels == 3) {
				dst_row[x * 4 + 0] = scanline_recon[x * 3 + 0];
				dst_row[x * 4 + 1] = scanline_recon[x * 3 + 1];
				dst_row[x * 4 + 2] = scanline_recon[x * 3 + 2];
				dst_row[x * 4 + 3] = 255;
			} else {
				dst_row[x * 4 + 0] = scanline_recon[x * 4 + 0];
				dst_row[x * 4 + 1] = scanline_recon[x * 4 + 1];
				dst_row[x * 4 + 2] = scanline_recon[x * 4 + 2];
				dst_row[x * 4 + 3] = scanline_recon[x * 4 + 3];
			}
		}

		/* Swap buffers para proxima linha */
		if (!scanline_prev)
			scanline_prev = malloc(stride);
		memcpy(scanline_prev, scanline_recon, stride);
	}

	free(scanline_recon);
	free(scanline_prev);
	free(inflated);
	free(idat_buf);
	free(raw);

	// printf("[PNG] Loaded pure C: %s\n", path);
	return result;

error:
	if (idat_buf)
		free(idat_buf);
	if (raw)
		free(raw);
	bhs_image_free(result);
	return result;
}

void bhs_image_free(bhs_image_t img)
{
	if (img.data)
		free(img.data);
}

bhs_image_t bhs_image_downsample_4x(bhs_image_t src)
{
	bhs_image_t dst = { 0 };
	if (!src.data || src.width % 4 != 0 || src.height % 4 != 0) {
		fprintf(stderr,
			"[IMAGE] Downsample 4x requer dimensoes multipas de 4. "
			"(Got %dx%d)\n",
			src.width, src.height);
		/* Fallback: copy or return empty? Return empty makes it fail load_icon and standard path might handle null?
		   Actually, better to return invalid so caller knows. */
		return dst;
	}

	dst.width = src.width / 4;
	dst.height = src.height / 4;
	dst.channels = 4;
	dst.data = calloc(dst.width * dst.height * 4, 1);
	if (!dst.data)
		return dst;

	for (int y = 0; y < dst.height; y++) {
		for (int x = 0; x < dst.width; x++) {
			uint32_t r = 0, g = 0, b = 0, a = 0;
			/* Box filter 4x4 sample */
			for (int dy = 0; dy < 4; dy++) {
				for (int dx = 0; dx < 4; dx++) {
					int sx = x * 4 + dx;
					int sy = y * 4 + dy;
					int idx = (sy * src.width + sx) * 4;

					r += src.data[idx + 0];
					g += src.data[idx + 1];
					b += src.data[idx + 2];
					a += src.data[idx + 3];
				}
			}

			int dst_idx = (y * dst.width + x) * 4;
			dst.data[dst_idx + 0] = (uint8_t)(r / 16);
			dst.data[dst_idx + 1] = (uint8_t)(g / 16);
			dst.data[dst_idx + 2] = (uint8_t)(b / 16);
			dst.data[dst_idx + 3] = (uint8_t)(a / 16);
		}
	}
	return dst;
}
