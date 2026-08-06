/**
 * @file disk.h
 * @brief Física do Disco de Acreção
 *
 * "O disco de acreção é onde a matéria dança sua última valsa
 * antes de cair no abismo eterno."
 *
 * Este módulo implementa modelos físicos para o disco de acreção:
 * - Modelo de Novikov-Thorne (disco fino relativístico)
 * - Perfil de temperatura
 * - Redshift gravitacional e Doppler
 * - Beaming relativístico
 *
 * Referências:
 * - Novikov & Thorne (1973) - Black Holes (Les Houches)
 * - Page & Thorne (1974) - Disk-Accretion onto a Black Hole
 */

#ifndef RI_ENGINE_DISK_DISK_H
#define RI_ENGINE_DISK_DISK_H

#include <stdbool.h>

#include "engine/math/spacetime/kerr.h"
#include "engine/math/vec4.h"

/* ============================================================================
 * CONSTANTES FÍSICAS (unidades CGS ou naturais conforme contexto)
 * ============================================================================
 */

/** Constante de Stefan-Boltzmann (erg cm⁻² s⁻¹ K⁻⁴) */
#define RI_SIGMA_SB 5.6704e-5

/** Constante de Planck / k_B para Wien (cm K) */
#define RI_WIEN_CONST 0.2898

/** Velocidade da luz (cm/s) - para conversões */
#define RI_C_CGS 2.998e10

/** Massa solar (g) */
#define RI_MSUN_CGS 1.989e33

/** Raio de Schwarzschild do Sol (cm) */
#define RI_RS_SUN_CGS 2.95e5

/* ============================================================================
 * PARÂMETROS DO DISCO
 * ============================================================================
 */

/**
 * struct ri_disk - Parâmetros do disco de acreção
 * @inner_radius: Raio interno (em unidades de M, geralmente ISCO)
 * @outer_radius: Raio externo
 * @mdot: Taxa de acreção adimensional (Ṁc²/L_Edd)
 * @inclination: Ângulo de inclinação do disco (rad, 0 = face-on)
 */
struct ri_disk {
	double inner_radius;
	double outer_radius;
	double mdot;
	double inclination;
};

/* ============================================================================
 * NOVIKOV-THORNE: PERFIL DE TEMPERATURA
 * ============================================================================
 */

/**
 * ri_disk_isco - Retorna raio do ISCO para o buraco negro
 */
double ri_disk_isco(const struct ri_kerr *bh);

/**
 * ri_disk_temperature - Temperatura efetiva no raio r
 * @bh: parâmetros do buraco negro
 * @disk: parâmetros do disco
 * @r: raio (em unidades de M)
 *
 * Usa o modelo de Novikov-Thorne para disco fino relativístico.
 * T(r) ∝ r^(-3/4) * f(r)
 * onde f(r) é o fator relativístico de eficiência.
 *
 * Retorna: temperatura em unidades normalizadas (0 a 1 para colormap)
 */
double ri_disk_temperature(const struct ri_kerr *bh,
			    const struct ri_disk *disk, double r);

/**
 * ri_disk_flux - Fluxo de energia no raio r (modelo NT)
 * @bh: parâmetros do buraco negro
 * @disk: parâmetros do disco
 * @r: raio (em unidades de M)
 *
 * F(r) ∝ Ṁ / (r³) * f_NT(r)
 *
 * O fator f_NT(r) inclui correções relativísticas.
 */
double ri_disk_flux(const struct ri_kerr *bh, const struct ri_disk *disk,
		     double r);

/* ============================================================================
 * REDSHIFT E DOPPLER
 * ============================================================================
 */

/**
 * ri_disk_redshift_total - Redshift total visto por observador distante
 * @bh: parâmetros do buraco negro
 * @r: raio no disco
 * @phi: ângulo azimutal no disco
 * @observer_inclination: inclinação do observador (0 = polo, π/2 = equador)
 *
 * Combina:
 * - Redshift gravitacional (g = √(-g_tt - 2ω g_tφ - ω² g_φφ))
 * - Efeito Doppler (velocidade orbital)
 * - Beaming relativístico
 *
 * Retorna: fator de redshift z onde λ_obs = λ_emit * (1 + z)
 *          z < 0: blueshift (se aproximando)
 *          z > 0: redshift (se afastando)
 */
double ri_disk_redshift_total(const struct ri_kerr *bh, double r, double phi,
			       double observer_inclination);

/**
 * ri_disk_doppler_factor - Fator Doppler para elemento do disco
 * @bh: parâmetros
 * @r: raio
 * @phi: ângulo azimutal (0 = longe do observador, π = em direção)
 * @inclination: inclinação do observador
 *
 * Retorna: g = ν_obs / ν_emit (beaming Doppler)
 *         g > 1: blueshift
 *         g < 1: redshift
 */
double ri_disk_doppler_factor(const struct ri_kerr *bh, double r, double phi,
			       double inclination);

/* ============================================================================
 * VELOCIDADE ORBITAL
 * ============================================================================
 */

/**
 * ri_disk_omega_kepler - Velocidade angular Kepleriana
 * @bh: parâmetros do buraco negro
 * @r: raio
 *
 * Para Kerr: Ω_K = M^(1/2) / (r^(3/2) + a M^(1/2))
 * O sinal + é para órbitas prograde.
 */
double ri_disk_omega_kepler(const struct ri_kerr *bh, double r);

/**
 * ri_disk_velocity_phi - Velocidade tangencial v^φ
 * @bh: parâmetros
 * @r: raio
 *
 * Retorna: v^φ = Ω_K - ω (velocidade relativa ao frame dragging)
 */
double ri_disk_velocity_phi(const struct ri_kerr *bh, double r);

/* ============================================================================
 * CORES E VISUALIZAÇÃO
 * ============================================================================
 */

/**
 * struct ri_color_rgb - Cor RGB normalizada [0, 1]
 */
struct ri_color_rgb {
	float r, g, b;
};

/**
 * ri_disk_color - Cor do disco no raio r com redshift
 * @bh: parâmetros do buraco negro
 * @disk: parâmetros do disco
 * @r: raio
 * @phi: ângulo azimutal
 * @inclination: inclinação do observador
 *
 * Combina:
 * - Temperatura local → cor de corpo negro
 * - Redshift Doppler → deslocamento de cor
 * - Beaming → brilho
 */
struct ri_color_rgb ri_disk_color(const struct ri_kerr *bh,
				    const struct ri_disk *disk, double r,
				    double phi, double inclination);

/**
 * ri_blackbody_color - Cor de corpo negro para temperatura
 * @temperature: temperatura normalizada [0, 1] onde 1 = máximo
 *
 * Mapeia temperatura para cor visível:
 * frio (0) → vermelho escuro
 * médio (0.5) → laranja/amarelo
 * quente (1) → branco-azulado
 */
struct ri_color_rgb ri_blackbody_color(double temperature);

/**
 * ri_color_apply_redshift - Aplica redshift à cor
 * @color: cor original
 * @z: fator de redshift (positivo = vermelho, negativo = azul)
 *
 * Simula deslocamento espectral simplificado.
 */
struct ri_color_rgb ri_color_apply_redshift(struct ri_color_rgb color,
					      double z);

#endif /* RI_ENGINE_DISK_DISK_H */
