/**
 * @file integrator.h
 * @brief Integradores Numéricos de Alta Precisão
 *
 * "A diferença entre um simulador de brinquedo e um científico
 *  está no integrador. Não corte caminho aqui."
 *
 * Implementa:
 * - RK4 clássico (4ª ordem)
 * - RKF45 adaptativo (Runge-Kutta-Fehlberg)
 * - Kahan summation para acumulação precisa
 */

#ifndef BHS_ENGINE_INTEGRATOR_H
#define BHS_ENGINE_INTEGRATOR_H

#include <stdbool.h>
#include "math/vec4.h"

/* ============================================================================
 * CONSTANTES FÍSICAS IAU 2015
 * ============================================================================
 * Usamos GM (produto gravitacional) ao invés de G*M porque GM é
 * medido diretamente com muito maior precisão.
 */

/* Constante Gravitacional (CODATA 2018) */
#define IAU_G 6.67430e-11 /* m³/(kg·s²) ± 0.00015 */

/* Velocidade da Luz (exato por definição SI) */
#define IAU_C 299792458.0 /* m/s */

/* Unidade Astronômica (exato por definição IAU 2012) */
#define IAU_AU 1.495978707e11 /* m */

/* Parâmetros Gravitacionais (GM) - mais precisos que G*M */
#define IAU_GM_SUN 1.32712440042e20  /* m³/s² */
#define IAU_GM_EARTH 3.986004418e14  /* m³/s² */
#define IAU_GM_MOON 4.9028695e12     /* m³/s² */
#define IAU_GM_JUPITER 1.26686534e17 /* m³/s² */
#define IAU_GM_SATURN 3.7931187e16   /* m³/s² */

/* Massas derivadas (M = GM/G) */
#define IAU_MASS_SUN (IAU_GM_SUN / IAU_G)
#define IAU_MASS_EARTH (IAU_GM_EARTH / IAU_G)
#define IAU_MASS_MOON (IAU_GM_MOON / IAU_G)

/* J2 (oblateness) - para correções futuras */
#define IAU_J2_EARTH 1.08263e-3
#define IAU_J2_SUN 2.0e-7

/*
 * G_SIM: Constante gravitacional para simulações visuais.
 * 
 * Em unidades SI reais, G = 6.67e-11 é muito pequeno para massas
 * da ordem de 10 kg. Para visualização, usamos G = 1.0 (unidades naturais)
 * onde v_orbital = sqrt(M_central / r).
 *
 * Para simulações científicas reais, use IAU_G com massas em kg.
 */
#define G_SIM 1.0

/* ============================================================================
 * KAHAN SUMMATION
 * ============================================================================
 * Evita cancelamento catastrófico em somas de floating point.
 * Essencial para simulações de longo prazo.
 */

struct bhs_kahan {
	double sum;
	double c; /* compensation */
};

static inline void bhs_kahan_init(struct bhs_kahan *k)
{
	k->sum = 0.0;
	k->c = 0.0;
}

static inline void bhs_kahan_add(struct bhs_kahan *k, double value)
{
	double y = value - k->c;
	double t = k->sum + y;
	k->c = (t - k->sum) - y;
	k->sum = t;
}

static inline double bhs_kahan_get(const struct bhs_kahan *k)
{
	return k->sum;
}

/* Versão vetorial */
struct bhs_kahan_vec3 {
	struct bhs_kahan x, y, z;
};

static inline void bhs_kahan_vec3_init(struct bhs_kahan_vec3 *k)
{
	bhs_kahan_init(&k->x);
	bhs_kahan_init(&k->y);
	bhs_kahan_init(&k->z);
}

static inline void bhs_kahan_vec3_add(struct bhs_kahan_vec3 *k,
				      struct bhs_vec3 v)
{
	bhs_kahan_add(&k->x, v.x);
	bhs_kahan_add(&k->y, v.y);
	bhs_kahan_add(&k->z, v.z);
}

static inline struct bhs_vec3 bhs_kahan_vec3_get(const struct bhs_kahan_vec3 *k)
{
	return (struct bhs_vec3){ .x = bhs_kahan_get(&k->x),
				  .y = bhs_kahan_get(&k->y),
				  .z = bhs_kahan_get(&k->z) };
}

/* ============================================================================
 * ESTADO DO SISTEMA
 * ============================================================================
 */

#define BHS_MAX_BODIES 128

/**
 * Estado de um corpo para integração
 */
struct bhs_body_state_rk {
	struct bhs_vec3 pos;
	struct bhs_vec3 vel;
	double mass;
	double gm;     /* GM = G * mass (pré-calculado) */
	double radius; /* Raio Equatorial (p/ J2) */
	double j2;     /* Termo J2 (achatamento) */

	/* [NEW] Rotational State (6-DOF) */
	struct bhs_vec3 rot_vel; /* Velocidade Angular (rad/s) */
	double inertia;		 /* Momento de Inércia (kg*m^2) */

	bool is_fixed;
	bool is_alive;
};

/**
 * Sistema completo para integração
 */
struct bhs_system_state {
	struct bhs_body_state_rk bodies[BHS_MAX_BODIES];
	int n_bodies;
	double time;
};

/**
 * Derivada do estado (acelerações)
 */
struct bhs_system_derivative {
	struct bhs_vec3 vel[BHS_MAX_BODIES]; /* dr/dt = v */
	struct bhs_vec3 acc[BHS_MAX_BODIES]; /* dv/dt = a */
};

/* ============================================================================
 * INTEGRADORES
 * ============================================================================
 */

/**
 * bhs_integrator_rk4 - Runge-Kutta 4ª ordem clássico
 * @state: Estado atual do sistema (modificado in-place)
 * @dt: Timestep
 *
 * RK4 tem erro local O(dt⁵) e erro global O(dt⁴).
 * Muito mais preciso que Euler para o mesmo dt.
 */
void bhs_integrator_rk4(struct bhs_system_state *state, double dt);

/**
 * bhs_integrator_leapfrog - Störmer-Verlet simplético
 * @state: Estado atual do sistema (modificado in-place)
 * @dt: Timestep
 *
 * Integrador SIMPLÉTICO - conserva energia por tempo indefinido.
 * Usado em GADGET, REBOUND, e maioria dos códigos N-body profissionais.
 *
 * Vantagens sobre RK4:
 *   - Energia não deriva (erro limitado)
 *   - Reversível no tempo
 *   - Mais estável para órbitas de longo prazo
 *
 * Desvantagem:
 *   - O(dt²) vs O(dt⁴) do RK4 por passo
 *   - Precisa de dt menor para mesma precisão LOCAL
 *
 * RECOMENDADO para simulações gravitacionais.
 */
void bhs_integrator_leapfrog(struct bhs_system_state *state, double dt);

/**
 * bhs_integrator_rkf45 - Runge-Kutta-Fehlberg adaptativo
 * @state: Estado atual do sistema
 * @dt: Timestep sugerido (será ajustado)
 * @tolerance: Erro máximo tolerado por passo
 * @dt_out: Timestep recomendado para próximo passo
 *
 * Retorna: Erro estimado do passo atual
 */
double bhs_integrator_rkf45(struct bhs_system_state *state, double dt,
			    double tolerance, double *dt_out);

/**
 * bhs_integrator_yoshida - Integrador Simplético de 4ª Ordem
 * @state: Estado atual
 * @dt: Timestep
 *
 * Integrador de alta precisão p/ longo prazo. 
 * Erro O(dt^4), conserva energia melhor que RK4.
 * Custo: 3 avaliações de força por passo (vs 1 do Leapfrog, 4 do RK4).
 */
void bhs_integrator_yoshida(struct bhs_system_state *state, double dt);

/**
 * bhs_compute_accelerations - Calcula acelerações gravitacionais
 * @state: Estado do sistema
 * @acc: Array de acelerações (output)
 *
 * Usa Kahan summation para precisão máxima.
 * Inclui softening para evitar singularidade.
 */
void bhs_compute_accelerations(const struct bhs_system_state *state,
			       struct bhs_vec3 acc[]);

/**
 * bhs_compute_1pn_correction - Calcula correção relativística 1PN
 * @gm_central: GM do corpo central (m³/s² ou unidades naturais)
 * @pos: Posição relativa ao corpo central
 * @vel: Velocidade do corpo
 * @c: Velocidade da luz (ou valor normalizado)
 *
 * Retorna: Aceleração adicional devido à relatividade geral (1ª ordem)
 *
 * Esta correção causa a precessão do periélio (ex: Mercúrio ~43"/século).
 * Fórmula: a_1PN = (GM/r²c²) * [4GM/r - v² + 4(v·r̂)²] * r̂ + 4(v·r̂)(GM/r²c²) * v̂
 */
struct bhs_vec3 bhs_compute_1pn_correction(double gm_central,
					   struct bhs_vec3 pos,
					   struct bhs_vec3 vel, double c);

/**
 * bhs_compute_j2_correction - Calcula perturbação J2 (Oblateness)
 * @gm_central: GM do corpo central
 * @j2: Coeficiente J2
 * @r_eq: Raio equatorial do corpo
 * @pos: Posição relativa
 */
struct bhs_vec3 bhs_compute_j2_correction(double gm_central, double j2,
					  double r_eq, struct bhs_vec3 pos);

/**
 * bhs_compute_torques - Calcula Torques (Maré + Outros)
 * @state: Estado do sistema
 * @torques: Array de torques (output)
 */
void bhs_compute_torques(const struct bhs_system_state *state,
			 struct bhs_vec3 torques[]);

/* ============================================================================
 * INVARIANTES (CONSERVAÇÃO)
 * ============================================================================
 */

struct bhs_invariants {
	double energy;			  /* Energia total (K + U) */
	struct bhs_vec3 momentum;	  /* Momento linear total */
	struct bhs_vec3 angular_momentum; /* Momento angular total */
};

/**
 * bhs_compute_invariants - Calcula quantidades conservadas
 */
void bhs_compute_invariants(const struct bhs_system_state *state,
			    struct bhs_invariants *inv);

/**
 * bhs_check_conservation - Verifica se invariantes estão conservados
 * @initial: Invariantes no tempo t=0
 * @current: Invariantes atuais
 * @tolerance: Desvio máximo permitido (fração)
 *
 * Retorna: true se dentro da tolerância
 */
bool bhs_check_conservation(const struct bhs_invariants *initial,
			    const struct bhs_invariants *current,
			    double tolerance);

#endif /* BHS_ENGINE_INTEGRATOR_H */
