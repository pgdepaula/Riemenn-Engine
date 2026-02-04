/**
 * @file units.h
 * @brief Sistema de Unidades Unificado para Simulação Gravitacional
 *
 * ============================================================================
 * FILOSOFIA: PROPORÇÕES REAIS, VALORES MANEJÁVEIS
 * ============================================================================
 *
 * Este arquivo define o sistema de unidades usado em toda a simulação.
 * A regra fundamental é:
 *
 *   "PRESERVE RAZÕES, NÃO VALORES ABSOLUTOS"
 *
 * A física orbital depende de proporções de massa e distância, não de
 * valores absolutos. Usando G = 1 (unidades naturais), podemos escalar
 * tudo de forma consistente.
 *
 * ============================================================================
 * UNIDADES NATURAIS (G = c = 1)
 * ============================================================================
 *
 * Em relatividade geral, é comum usar G = c = 1. Isso simplifica as equações:
 *
 *   F = m₁m₂/r²     (sem G explícito)
 *   rₛ = 2M         (raio de Schwarzschild = 2 × massa)
 *   v = √(M/r)      (velocidade orbital circular)
 *
 * ============================================================================
 * ESCALAS DEFINIDAS
 * ============================================================================
 *
 * DISTÂNCIA:
 *   1 AU (Unidade Astronômica) = 50 unidades de simulação
 *   Escala: 1 unidade ≈ 3 milhões de km
 *
 * MASSA:
 *   Massa Solar (M☉) = 20 unidades de simulação
 *   Escala: 1 unidade ≈ 10²⁹ kg
 *
 * RAIO:
 *   Raio Solar (R☉) = 3 unidades de simulação
 *   Escala: 1 unidade ≈ 230,000 km
 *
 * TEMPO:
 *   Segundos reais (não escalado)
 *   Nota: Com G=1 e massas/distâncias escaladas, os períodos orbitais
 *   ficam proporcionalmente corretos mas não em segundos SI.
 *
 * ============================================================================
 * PROPORÇÕES PRESERVADAS
 * ============================================================================
 *
 * As proporções físicas reais são mantidas:
 *
 * MASSA (relativo ao Sol):
 *   Sol     : 1.000
 *   Júpiter : 0.000955  (1/1047)
 *   Saturno : 0.000286  (1/3498)
 *   Terra   : 0.000003  (1/333000)
 *
 * RAIO (relativo ao Sol):
 *   Sol     : 1.000
 *   Júpiter : 0.100     (R☉/10)
 *   Saturno : 0.084
 *   Terra   : 0.0092    (R☉/109)
 *
 * DISTÂNCIA (relativo a 1 AU = Terra-Sol):
 *   Mercúrio : 0.387 AU
 *   Vênus    : 0.723 AU
 *   Terra    : 1.000 AU
 *   Marte    : 1.524 AU
 *   Júpiter  : 5.203 AU
 *   Saturno  : 9.537 AU
 *
 * ============================================================================
 */

#ifndef BHS_UNITS_H
#define BHS_UNITS_H

/*
 * ============================================================================
 * CONSTANTES FÍSICAS FUNDAMENTAIS (SI)
 * ============================================================================
 */

/** Constante gravitacional (SI: m³/(kg·s²)) */
#define BHS_G_SI 6.67430e-11

/** Velocidade da luz (SI: m/s) */
#define BHS_C_SI 299792458.0

/** Unidade Astronômica (SI: m) */
#define BHS_AU_SI 1.495978707e11

/** Massa Solar (SI: kg) */
#define BHS_MASS_SUN_SI 1.98847e30

/** Raio Solar (SI: m) */
#define BHS_RADIUS_SUN_SI 6.9634e8

/*
 * ============================================================================
 * ESCALAS DE CONVERSÃO (SI → SIMULAÇÃO)
 * ============================================================================
 *
 * Estas escalas definem como converter valores SI para unidades de simulação.
 *
 * IMPORTANTE: Todas as escalas são derivadas de duas escolhas arbitrárias:
 *   1. 1 AU = 50 unidades de distância
 *   2. M☉ = 20 unidades de massa
 *
 * O raio é escalado independentemente para visualização:
 *   3. R☉ = 3 unidades de raio
 */

/**
 * ESCALA DE DISTÂNCIA
 * 1 AU (1.496e11 m) → 50 unidades
 */
#define BHS_SCALE_DISTANCE (50.0 / BHS_AU_SI)

/**
 * ESCALA DE MASSA
 * M☉ (1.989e30 kg) → 20 unidades
 */
#define BHS_SCALE_MASS (20.0 / BHS_MASS_SUN_SI)

/**
 * ESCALA DE RAIO
 * R☉ (6.963e8 m) → 3 unidades
 *
 * NOTA: Esta escala é INDEPENDENTE da escala de distância.
 * Isso permite que corpos sejam visualmente maiores do que seriam
 * em escala real de distância (onde a Terra seria invisível).
 */
#define BHS_SCALE_RADIUS (3.0 / BHS_RADIUS_SUN_SI)

/*
 * ============================================================================
 * CONSTANTE GRAVITACIONAL DE SIMULAÇÃO
 * ============================================================================
 *
 * Usamos G = 1 (unidades naturais).
 *
 * Isso significa que as equações são:
 *   a = M/r²          (aceleração gravitacional)
 *   v = √(M/r)        (velocidade orbital circular)
 *   T = 2π√(r³/M)     (período orbital)
 *   rₛ = 2M           (raio de Schwarzschild)
 */
#define BHS_G_SIM 1.0

/*
 * ============================================================================
 * VALORES DE REFERÊNCIA (EM UNIDADES DE SIMULAÇÃO)
 * ============================================================================
 *
 * Estes são os valores que corpos celestes terão após conversão.
 */

/** Massa do Sol em unidades de simulação */
#define BHS_SIM_MASS_SUN 20.0

/** Raio do Sol em unidades de simulação */
#define BHS_SIM_RADIUS_SUN 3.0

/** 1 AU em unidades de simulação */
#define BHS_SIM_AU 50.0

/*
 * ============================================================================
 * PROPORÇÕES REAIS (ADIMENSIONAIS)
 * ============================================================================
 *
 * Estas proporções são constantes físicas reais e NÃO dependem da escala.
 * Use-as para verificar consistência.
 */

/** Razão massa Júpiter / Sol */
#define BHS_RATIO_MASS_JUPITER_SUN (1.0 / 1047.348)

/** Razão massa Terra / Sol */
#define BHS_RATIO_MASS_EARTH_SUN (1.0 / 332946.0)

/** Razão raio Júpiter / Sol */
#define BHS_RATIO_RADIUS_JUPITER_SUN 0.10045

/** Razão raio Terra / Sol */
#define BHS_RATIO_RADIUS_EARTH_SUN 0.00916

/*
 * ============================================================================
 * MACROS DE CONVERSÃO
 * ============================================================================
 */

/** Converte metros para unidades de simulação (distância) */
#define BHS_METERS_TO_SIM(m) ((m) * BHS_SCALE_DISTANCE)

/** Converte kg para unidades de simulação (massa) */
#define BHS_KG_TO_SIM(kg) ((kg) * BHS_SCALE_MASS)

/** Converte metros para unidades de simulação (raio) */
#define BHS_RADIUS_TO_SIM(m) ((m) * BHS_SCALE_RADIUS)

/** Converte AU para unidades de simulação */
#define BHS_AU_TO_SIM(au) ((au) * BHS_SIM_AU)

/*
 * ============================================================================
 * VELOCIDADE ORBITAL
 * ============================================================================
 *
 * Com G = 1, a velocidade orbital circular é:
 *   v = √(M/r)
 *
 * onde M e r estão em unidades de simulação.
 */
#include <math.h>

static inline double bhs_orbital_velocity(double central_mass_sim,
					  double radius_sim)
{
	if (radius_sim <= 0.0)
		return 0.0;
	return sqrt(central_mass_sim / radius_sim);
}

/**
 * Período orbital (com G = 1):
 *   T = 2π√(r³/M)
 */
static inline double bhs_orbital_period(double central_mass_sim,
					double radius_sim)
{
	if (central_mass_sim <= 0.0 || radius_sim <= 0.0)
		return 0.0;
	return 2.0 * 3.14159265358979323846 *
	       sqrt(radius_sim * radius_sim * radius_sim / central_mass_sim);
}

/*
 * ============================================================================
 * ÉPOCA J2000.0 - REFERÊNCIA TEMPORAL ASTRONÔMICA
 * ============================================================================
 *
 * J2000.0 = 2000-01-01 12:00:00 UTC (meio-dia!)
 * É a época padrão usada em efemérides e cálculos astronômicos.
 *
 * A simulação usa segundos desde J2000.0 como referência temporal.
 */

/** Segundos por dia */
#define BHS_SECONDS_PER_DAY 86400.0

/** Segundos por ano (não-bissexto) */
#define BHS_SECONDS_PER_YEAR (365.0 * BHS_SECONDS_PER_DAY)

/** 
 * Epoch J2000.0 como Unix timestamp
 * 2000-01-01 12:00:00 UTC = 946728000 Unix time
 */
#define BHS_J2000_UNIX 946728000

/**
 * bhs_sim_time_to_date - Converte segundos desde J2000.0 para data legível
 * @sim_seconds: Segundos simulados desde J2000.0
 * @year: Ponteiro para ano (output)
 * @month: Ponteiro para mês 1-12 (output)
 * @day: Ponteiro para dia 1-31 (output)
 * @hour: Ponteiro para hora 0-23 (output)
 * @minute: Ponteiro para minuto 0-59 (output)
 * @second: Ponteiro para segundo 0-59 (output)
 *
 * Algoritmo simplificado que ignora segundos bissextos mas é preciso o suficiente.
 */
static inline void bhs_sim_time_to_date(double sim_seconds, int *year,
					int *month, int *day, int *hour,
					int *minute, int *second)
{
	/* Dias desde J2000.0 */
	double total_days = sim_seconds / BHS_SECONDS_PER_DAY;

	/* Fração do dia para hora/minuto/segundo */
	double frac_day = total_days - (long)total_days;
	if (frac_day < 0)
		frac_day += 1.0;

	double day_seconds = frac_day * BHS_SECONDS_PER_DAY;
	/* J2000 começa ao meio-dia (12:00), então somamos 12 horas */
	day_seconds += 12.0 * 3600.0;
	if (day_seconds >= BHS_SECONDS_PER_DAY) {
		day_seconds -= BHS_SECONDS_PER_DAY;
		total_days += 1.0;
	}

	*hour = (int)(day_seconds / 3600.0);
	*minute = (int)((day_seconds - *hour * 3600.0) / 60.0);
	*second = (int)(day_seconds - *hour * 3600.0 - *minute * 60.0);

	/* Ano, assumindo anos de 365.25 dias (média) */
	long days_since_2000 = (long)total_days;

	/* Começamos em 2000-01-01 */
	int y = 2000;
	int days_in_year;

	while (days_since_2000 > 0) {
		days_in_year = ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))
				       ? 366
				       : 365;
		if (days_since_2000 >= days_in_year) {
			days_since_2000 -= days_in_year;
			y++;
		} else {
			break;
		}
	}
	while (days_since_2000 < 0) {
		y--;
		days_in_year = ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))
				       ? 366
				       : 365;
		days_since_2000 += days_in_year;
	}

	*year = y;

	/* Mês e dia */
	int days_in_month[] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
	};
	if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
		days_in_month[1] = 29; /* Fevereiro em ano bissexto */
	}

	int m = 0;
	while (m < 12 && days_since_2000 >= days_in_month[m]) {
		days_since_2000 -= days_in_month[m];
		m++;
	}

	*month = m + 1;
	*day = (int)days_since_2000 + 1;
}

#endif /* BHS_UNITS_H */
