/**
 * @file disk.c
 * @brief Implementação da física do disco de acreção
 *
 * "A matemática do disco de acreção é onde a relatividade geral
 * encontra a termodinâmica. É lindo. E complicado pra caralho."
 *
 * Baseado no modelo de Novikov-Thorne (1973) para discos finos.
 * Mandem a i.a me substituir filhos da puta
 */

#define _GNU_SOURCE
#include "disk.h"
#include <math.h>

/* ============================================================================
 * FUNÇÕES AUXILIARES INTERNAS
 * ============================================================================
 */

/**
 * Função auxiliar para o fator relativístico de Novikov-Thorne
 *
 * Esta é a função Q(r) que aparece no fluxo:
 * F = (3Ṁ / 8π) * (1/r³) * (1/(-E)) * Q(r)
 */
static double novikov_thorne_Q(const struct bhs_kerr *bh, double r)
{
	/*
   * Para disco em órbita circular no equador:
   *
   * Q(r) = ∫[r_isco to r] (E - Ω L) dL/dr' dr'
   *
   * onde E, L são energia e momento angular específicos.
   *
   * Aproximação simplificada para não explodir em complexidade:
   * Q(r) ≈ 1 - √(r_isco / r)
   *
   * Isso captura o comportamento qualitativo (zero no ISCO, crescente).
   */
	double r_isco = bhs_disk_isco(bh);

	if (r <= r_isco)
		return 0.0;

	return 1.0 - sqrt(r_isco / r);
}

/**
 * Energia específica para órbita circular em Kerr
 */
static double kerr_circular_energy(const struct bhs_kerr *bh, double r)
{
	/*
   * E = (1 - 2M/r + aM^(1/2)/(r^(3/2))) / √(1 - 3M/r + 2aM^(1/2)/(r^(3/2)))
   *
   * Simplificação: para r >> M, E → 1 (partícula no infinito)
   */
	double M = bh->M;
	double a = bh->a;
	double sqrtM = sqrt(M);
	double r32 = r * sqrt(r);

	double num = 1.0 - 2.0 * M / r + a * sqrtM / r32;
	double denom_sq = 1.0 - 3.0 * M / r + 2.0 * a * sqrtM / r32;

	if (denom_sq <= 0.0)
		return 0.0; /* Dentro do ISCO */

	return num / sqrt(denom_sq);
}

/* ============================================================================
 * ISCO E RAIOS
 * ============================================================================
 */

double bhs_disk_isco(const struct bhs_kerr *bh)
{
	/* Usa a função do módulo Kerr */
	return bhs_kerr_isco(bh, true); /* Prograde */
}

/* ============================================================================
 * TEMPERATURA E FLUXO
 * ============================================================================
 */

double bhs_disk_temperature(const struct bhs_kerr *bh,
			    const struct bhs_disk *disk, double r)
{
	/*
   * Temperatura efetiva do disco.
   *
   * T(r) = (F(r) / σ_SB)^(1/4)
   *
   * Normalizamos para retornar valor entre 0 e 1.
   */
	double r_isco = bhs_disk_isco(bh);

	if (r < r_isco || r > disk->outer_radius)
		return 0.0;

	/* Perfil T ∝ r^(-3/4) com fator relativístico */
	double Q = novikov_thorne_Q(bh, r);
	double base = pow(r_isco / r, 0.75);

	/* Normaliza para que temperatura no ISCO próximo seja ~1 */
	double temp_raw = base * pow(Q, 0.25);

	/* Clamp para [0, 1] */
	if (temp_raw > 1.0)
		temp_raw = 1.0;
	if (temp_raw < 0.0)
		temp_raw = 0.0;

	return temp_raw;
}

double bhs_disk_flux(const struct bhs_kerr *bh, const struct bhs_disk *disk,
		     double r)
{
	/*
   * Fluxo de energia (aproximação Novikov-Thorne):
   *
   * F(r) = (3Ṁ / 8πr³) * (1/(-E)) * Q(r)
   *
   * Normalizamos para valor adimensional.
   */
	double r_isco = bhs_disk_isco(bh);

	if (r < r_isco)
		return 0.0;

	double E = kerr_circular_energy(bh, r);
	double Q = novikov_thorne_Q(bh, r);

	if (E <= 0.0)
		return 0.0;

	double r3 = r * r * r;
	return (disk->mdot / r3) * (1.0 / E) * Q;
}

/* ============================================================================
 * VELOCIDADE ORBITAL
 * ============================================================================
 */

double bhs_disk_omega_kepler(const struct bhs_kerr *bh, double r)
{
	/*
   * Velocidade angular Kepleriana para Kerr (prograde):
   *
   * Ω_K = √(M) / (r^(3/2) + a√M)
   */
	double M = bh->M;
	double a = bh->a;
	double sqrtM = sqrt(M);
	double r32 = r * sqrt(r);

	return sqrtM / (r32 + a * sqrtM);
}

double bhs_disk_velocity_phi(const struct bhs_kerr *bh, double r)
{
	/*
   * Velocidade tangencial relativa ao ZAMO:
   * v^φ = r * (Ω_K - ω)
   *
   * onde ω é a velocidade angular do frame dragging.
   */
	double omega_k = bhs_disk_omega_kepler(bh, r);
	double omega_frame = bhs_kerr_omega_frame(bh, r, M_PI / 2.0);

	return r * (omega_k - omega_frame);
}

/* ============================================================================
 * REDSHIFT E DOPPLER
 * ============================================================================
 */

double bhs_disk_redshift_total(const struct bhs_kerr *bh, double r, double phi,
			       double observer_inclination)
{
	/*
   * Redshift total = gravitacional + Doppler
   *
   * 1 + z = (1 + z_grav) * (1 + z_doppler)
   */

	/* Redshift gravitacional usando fórmula de Schwarzschild */
	double M = bh->M;
	double rs = 2.0 * M;
	double f = 1.0 - rs / r;

	double z_grav;
	if (f > 0.01) {
		z_grav = 1.0 / sqrt(f) - 1.0;
	} else {
		z_grav = 100.0; /* Dentro/perto do horizonte */
	}

	/* Velocidade tangencial */
	double v_phi = bhs_disk_velocity_phi(bh, r);

	/* Projeção na linha de visão */
	double v_los = v_phi * sin(phi) * sin(observer_inclination);

	/* z_doppler: positivo se afastando */
	double z_doppler = v_los;

	/* Combinação */
	double z_total = (1.0 + z_grav) * (1.0 + z_doppler) - 1.0;

	return z_total;
}

double bhs_disk_doppler_factor(const struct bhs_kerr *bh, double r, double phi,
			       double inclination)
{
	/*
   * Fator Doppler relativístico:
   * g = 1 / [γ(1 - v·n̂)]
   *
   * onde γ = fator de Lorentz, v = velocidade, n̂ = direção do observador
   *
   * Para velocidades moderadas: g ≈ 1 - z
   */
	double z = bhs_disk_redshift_total(bh, r, phi, inclination);
	return 1.0 / (1.0 + z);
}

/* ============================================================================
 * CORES
 * ============================================================================
 */

struct bhs_color_rgb bhs_blackbody_color(double temperature)
{
	/*
   * Mapa de cores de corpo negro simplificado.
   *
   * Baseado em aproximação de Planckian locus:
   * T baixa → vermelho
   * T média → laranja/amarelo
   * T alta → branco azulado
   */
	struct bhs_color_rgb c;

	/* Clamp */
	if (temperature < 0.0)
		temperature = 0.0;
	if (temperature > 1.0)
		temperature = 1.0;

	if (temperature < 0.2) {
		/* Vermelho escuro */
		c.r = temperature * 5.0f * 0.5f;
		c.g = 0.0f;
		c.b = 0.0f;
	} else if (temperature < 0.4) {
		/* Vermelho → Laranja */
		float t = (temperature - 0.2f) * 5.0f;
		c.r = 0.5f + t * 0.5f;
		c.g = t * 0.4f;
		c.b = 0.0f;
	} else if (temperature < 0.6) {
		/* Laranja → Amarelo */
		float t = (temperature - 0.4f) * 5.0f;
		c.r = 1.0f;
		c.g = 0.4f + t * 0.4f;
		c.b = t * 0.1f;
	} else if (temperature < 0.8) {
		/* Amarelo → Branco */
		float t = (temperature - 0.6f) * 5.0f;
		c.r = 1.0f;
		c.g = 0.8f + t * 0.2f;
		c.b = 0.1f + t * 0.7f;
	} else {
		/* Branco → Branco azulado */
		float t = (temperature - 0.8f) * 5.0f;
		c.r = 1.0f - t * 0.1f;
		c.g = 1.0f;
		c.b = 0.8f + t * 0.2f;
	}

	return c;
}

struct bhs_color_rgb bhs_color_apply_redshift(struct bhs_color_rgb color,
					      double z)
{
	/*
   * Aplica redshift à cor de forma simplificada.
   *
   * z > 0: redshift (mais vermelho)
   * z < 0: blueshift (mais azul)
   *
   * Também ajusta brilho (redshift = dimmer, blueshift = brighter)
   */
	struct bhs_color_rgb result;

	/* Fator de brilho (beaming) */
	double g = 1.0 / (1.0 + z);
	double brightness =
		g * g * g * g; /* g^4 para intensidade bolométrica */

	/* Clamp brightness */
	if (brightness > 5.0)
		brightness = 5.0;
	if (brightness < 0.05)
		brightness = 0.05;

	/* Deslocamento de cor */
	double shift = -z * 0.3; /* Quanto deslocar no espectro */

	if (shift > 0) {
		/* Blueshift: mais azul */
		result.r = color.r * (1.0 - shift) * brightness;
		result.g = color.g * brightness;
		result.b = (color.b + shift * (1.0 - color.b)) * brightness;
	} else {
		/* Redshift: mais vermelho */
		shift = -shift;
		result.r = (color.r + shift * (1.0 - color.r)) * brightness;
		result.g = color.g * (1.0 - shift * 0.5) * brightness;
		result.b = color.b * (1.0 - shift) * brightness;
	}

	/* Clamp final */
	if (result.r > 1.0)
		result.r = 1.0;
	if (result.g > 1.0)
		result.g = 1.0;
	if (result.b > 1.0)
		result.b = 1.0;
	if (result.r < 0.0)
		result.r = 0.0;
	if (result.g < 0.0)
		result.g = 0.0;
	if (result.b < 0.0)
		result.b = 0.0;

	return result;
}

struct bhs_color_rgb bhs_disk_color(const struct bhs_kerr *bh,
				    const struct bhs_disk *disk, double r,
				    double phi, double inclination)
{
	/* Temperatura local */
	double temp = bhs_disk_temperature(bh, disk, r);

	if (temp < 0.001) {
		/* Fora do disco */
		return (struct bhs_color_rgb){ 0.0f, 0.0f, 0.0f };
	}

	/* Cor base do corpo negro */
	struct bhs_color_rgb base_color = bhs_blackbody_color(temp);

	/* Redshift total */
	double z = bhs_disk_redshift_total(bh, r, phi, inclination);

	/* Aplica redshift */
	return bhs_color_apply_redshift(base_color, z);
}
