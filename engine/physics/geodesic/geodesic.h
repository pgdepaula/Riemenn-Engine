/**
 * @file geodesic.h
 * @brief Integração de geodésicas em espaço-tempo curvo
 *
 * "Uma geodésica é o caminho mais curto entre dois pontos.
 * Em espaço-tempo curvo, 'curto' fica... complicado."
 *
 * Este módulo implementa:
 * - Integração numérica de geodésicas (RK4)
 * - Suporte a geodésicas nulas (fótons) e timelike (matéria)
 * - Detecção de cruzamento de horizonte
 * - Cache de trajetórias para performance
 */

#ifndef BHS_ENGINE_GEODESIC_GEODESIC_H
#define BHS_ENGINE_GEODESIC_GEODESIC_H

#include <stdbool.h>

#include "math/spacetime/kerr.h"
#include "math/tensor/tensor.h"
#include "math/vec4.h"

/* ============================================================================
 * TIPOS
 * ============================================================================
 */

/**
 * enum bhs_geodesic_type - Tipo de geodésica
 *
 * Determina a condição de normalização da 4-velocidade.
 */
enum bhs_geodesic_type {
	BHS_GEODESIC_NULL,     /* Fótons: g_μν u^μ u^ν = 0 */
	BHS_GEODESIC_TIMELIKE, /* Partículas massivas: g_μν u^μ u^ν = -1 */
};

/**
 * enum bhs_geodesic_status - Estado da geodésica
 */
enum bhs_geodesic_status {
	BHS_GEO_PROPAGATING, /* Ainda se propagando */
	BHS_GEO_ESCAPED,     /* Escapou para o infinito (r > r_max) */
	BHS_GEO_CAPTURED,    /* Capturada pelo horizonte (r < r+) */
	BHS_GEO_HIT_DISK,    /* Atingiu o disco de acreção */
	BHS_GEO_TIMEOUT,     /* Limite de passos atingido */
};

/**
 * struct bhs_geodesic - Estado de uma geodésica
 */
struct bhs_geodesic {
	struct bhs_vec4 pos;		 /* Posição 4D (t, r, θ, φ) */
	struct bhs_vec4 vel;		 /* 4-velocidade dx^μ/dλ */
	enum bhs_geodesic_type type;	 /* Tipo (null ou timelike) */
	enum bhs_geodesic_status status; /* Status atual */
	double affine_param;		 /* Parâmetro afim acumulado */
	int step_count;			 /* Número de passos dados */
};

/* ============================================================================
 * CONSTANTES
 * ============================================================================
 */

/** Tolerância para verificar normalização */
#define BHS_GEODESIC_NULL_TOL 1e-6

/** Passos máximos padrão */
#define BHS_GEODESIC_MAX_STEPS 10000

/** Raio de escape padrão */
#define BHS_GEODESIC_ESCAPE_RADIUS 100.0

/* ============================================================================
 * INICIALIZAÇÃO
 * ============================================================================
 */

/**
 * bhs_geodesic_init - Inicializa geodésica
 * @geo: [out] geodésica a inicializar
 * @pos: posição inicial (t, r, θ, φ)
 * @vel: 4-velocidade inicial (será normalizada se necessário)
 * @type: tipo da geodésica
 *
 * Para geodésicas nulas, a 4-velocidade será ajustada para garantir
 * g_μν u^μ u^ν = 0.
 */
void bhs_geodesic_init(struct bhs_geodesic *geo, struct bhs_vec4 pos,
		       struct bhs_vec4 vel, enum bhs_geodesic_type type);

/**
 * bhs_geodesic_init_photon - Inicializa geodésica de fóton
 * @geo: [out] geodésica
 * @pos: posição inicial
 * @direction: direção 3D normalizada (será convertida para 4-velocidade nula)
 * @bh: parâmetros do buraco negro (para calcular componente temporal)
 */
void bhs_geodesic_init_photon(struct bhs_geodesic *geo, struct bhs_vec4 pos,
			      struct bhs_vec3 direction,
			      const struct bhs_kerr *bh);

/* ============================================================================
 * INTEGRAÇÃO RK4
 * ============================================================================
 */

/**
 * bhs_geodesic_step_rk4 - Um passo RK4 da geodésica
 * @geo: geodésica a integrar
 * @bh: parâmetros do buraco negro
 * @dlambda: tamanho do passo no parâmetro afim
 *
 * Integra a equação da geodésica:
 * d²x^μ/dλ² = -Γ^μ_αβ (dx^α/dλ)(dx^β/dλ)
 *
 * Retorna:
 *   0 em sucesso
 *  -1 em erro numérico
 */
int bhs_geodesic_step_rk4(struct bhs_geodesic *geo, const struct bhs_kerr *bh,
			  double dlambda);

/**
 * bhs_geodesic_step_adaptive - Passo adaptativo
 * @geo: geodésica
 * @bh: parâmetros
 * @dlambda: tamanho do passo inicial (pode ser modificado)
 * @tolerance: erro relativo tolerado
 *
 * Ajusta automaticamente dlambda para manter precisão.
 */
int bhs_geodesic_step_adaptive(struct bhs_geodesic *geo,
			       const struct bhs_kerr *bh, double *dlambda,
			       double tolerance);

/* ============================================================================
 * PROPAGAÇÃO COMPLETA
 * ============================================================================
 */

/**
 * struct bhs_geodesic_config - Configuração de propagação
 */
struct bhs_geodesic_config {
	double dlambda;		    /* Tamanho do passo */
	int max_steps;		    /* Máximo de passos (0 = default) */
	double escape_radius;	    /* Raio de escape (0 = default) */
	double disk_inner;	    /* Raio interno do disco */
	double disk_outer;	    /* Raio externo do disco */
	double disk_half_thickness; /* Meia-espessura do disco (em z) */
};

/**
 * bhs_geodesic_propagate - Propaga geodésica até critério de parada
 * @geo: geodésica (modificada in-place)
 * @bh: parâmetros do buraco negro
 * @config: configuração de propagação
 *
 * Retorna: status final (BHS_GEO_ESCAPED, BHS_GEO_CAPTURED, etc.)
 */
enum bhs_geodesic_status
bhs_geodesic_propagate(struct bhs_geodesic *geo, const struct bhs_kerr *bh,
		       const struct bhs_geodesic_config *config);

/* ============================================================================
 * VERIFICAÇÕES
 * ============================================================================
 */

/**
 * bhs_geodesic_is_inside_horizon - Verifica se está dentro do horizonte
 */
bool bhs_geodesic_is_inside_horizon(const struct bhs_geodesic *geo,
				    const struct bhs_kerr *bh);

/**
 * bhs_geodesic_is_in_disk - Verifica se está no disco de acreção
 */
bool bhs_geodesic_is_in_disk(const struct bhs_geodesic *geo, double inner,
			     double outer, double half_thickness);

/**
 * bhs_geodesic_norm2 - Norma² da 4-velocidade (para debug)
 *
 * Deve ser ≈ 0 para nulo, ≈ -1 para timelike.
 */
double bhs_geodesic_norm2(const struct bhs_geodesic *geo,
			  const struct bhs_kerr *bh);

/* ============================================================================
 * UTILIDADES
 * ============================================================================
 */

/**
 * bhs_geodesic_ray_from_camera - Cria raio a partir de parâmetros de câmera
 * @geo: [out] geodésica inicializada
 * @cam_pos: posição da câmera em coordenadas cartesianas
 * @cam_dir: direção para onde a câmera aponta
 * @cam_up: vetor "para cima" da câmera
 * @u, v: coordenadas do pixel normalizadas [-1, 1]
 * @fov: campo de visão (field of view)
 * @bh: parâmetros do buraco negro
 *
 * Converte um pixel da imagem em uma geodésica nula saindo da câmera.
 */
void bhs_geodesic_ray_from_camera(struct bhs_geodesic *geo,
				  struct bhs_vec3 cam_pos,
				  struct bhs_vec3 cam_dir,
				  struct bhs_vec3 cam_up, double u, double v,
				  double fov, const struct bhs_kerr *bh);

#endif /* BHS_ENGINE_GEODESIC_GEODESIC_H */
