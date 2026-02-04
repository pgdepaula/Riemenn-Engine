/**
 * @file geodesic.c
 * @brief Implementação do integrador de geodésicas RK4
 *
 * "O universo não calcula trajetórias. Ele simplesmente É.
 * Nós que precisamos de matemática pra entender."
 *
 * Este arquivo implementa o integrador Runge-Kutta de 4ª ordem
 * para a equação de geodésica em espaço-tempo curvo.
 */

#define _GNU_SOURCE /* Para M_PI */

#include "geodesic.h"
#include <math.h>
#include <string.h>

/* ============================================================================
 * HELPERS INTERNOS
 * ============================================================================
 */

/**
 * Calcula Christoffel para Kerr via diferença finita
 * (wrapper interno que usa a função de métrica)
 */
static int compute_christoffel_kerr(const struct bhs_kerr *bh,
				    struct bhs_vec4 pos,
				    struct bhs_christoffel *out)
{
	double h = 1e-5; /* Passo para diferença finita */
	return bhs_christoffel_compute(bhs_kerr_metric_func, pos, (void *)bh, h,
				       out);
}

/**
 * Deriva da posição (simplesmente a velocidade)
 */
static struct bhs_vec4 dpos_dlambda(struct bhs_vec4 vel)
{
	return vel;
}

/**
 * Deriva da velocidade (aceleração geodésica)
 */
static struct bhs_vec4 dvel_dlambda(const struct bhs_kerr *bh,
				    struct bhs_vec4 pos, struct bhs_vec4 vel)
{
	struct bhs_christoffel chris;
	if (compute_christoffel_kerr(bh, pos, &chris) != 0) {
		/* Erro numérico - retorna zero (vai dar ruim, mas não crasheia) */
		return bhs_vec4_zero();
	}
	return bhs_geodesic_accel(&chris, vel);
}

/* ============================================================================
 * INICIALIZAÇÃO
 * ============================================================================
 */

void bhs_geodesic_init(struct bhs_geodesic *geo, struct bhs_vec4 pos,
		       struct bhs_vec4 vel, enum bhs_geodesic_type type)
{
	memset(geo, 0, sizeof(*geo));
	geo->pos = pos;
	geo->vel = vel;
	geo->type = type;
	geo->status = BHS_GEO_PROPAGATING;
	geo->affine_param = 0.0;
	geo->step_count = 0;
}

void bhs_geodesic_init_photon(struct bhs_geodesic *geo, struct bhs_vec4 pos,
			      struct bhs_vec3 direction,
			      const struct bhs_kerr *bh)
{
	/*
   * Para fótons, precisamos construir uma 4-velocidade nula.
   *
   * g_μν k^μ k^ν = 0
   *
   * g_tt (k^t)² + 2 g_tφ k^t k^φ + g_rr (k^r)² + g_θθ (k^θ)² + g_φφ (k^φ)² = 0
   *
   * Para simplificar, começamos com k^φ = 0 (sem momento angular inicial)
   * e resolvemos para k^t dado k^r, k^θ.
   *
   * g_tt (k^t)² = -(g_rr (k^r)² + g_θθ (k^θ)²)
   * k^t = √[-(g_rr (k^r)² + g_θθ (k^θ)²) / g_tt]
   */

	double r = pos.x;
	double theta = pos.y;

	struct bhs_metric g;
	bhs_kerr_metric(bh, r, theta, &g);

	/* Componentes espaciais iniciais (direção normalizada) */
	struct bhs_vec3 dir_norm = bhs_vec3_normalize(direction);

	/* Convertemos direção 3D (cartesiana local) para coordenadas esféricas */
	/* Por simplicidade, assumimos que direction já está em (dr, dθ, dφ) */
	double kr = dir_norm.x;
	double ktheta = dir_norm.y;
	double kphi = dir_norm.z;

	/* Componente temporal para geodésica nula */
	double spatial_norm = g.g[1][1] * kr * kr +
			      g.g[2][2] * ktheta * ktheta +
			      g.g[3][3] * kphi * kphi +
			      2.0 * g.g[0][3] * kphi; /* Termo g_tφ */

	double kt;
	if (g.g[0][0] < 0.0 && -spatial_norm / g.g[0][0] >= 0.0) {
		kt = sqrt(-spatial_norm / g.g[0][0]);
	} else {
		/* Fallback - pode acontecer perto do horizonte */
		kt = 1.0;
	}

	struct bhs_vec4 vel = bhs_vec4_make(kt, kr, ktheta, kphi);
	bhs_geodesic_init(geo, pos, vel, BHS_GEODESIC_NULL);
}

/* ============================================================================
 * INTEGRAÇÃO RK4
 * ============================================================================
 */

int bhs_geodesic_step_rk4(struct bhs_geodesic *geo, const struct bhs_kerr *bh,
			  double dlambda)
{
	/*
   * Runge-Kutta 4ª ordem para sistema de 8 ODEs:
   * dpos/dλ = vel
   * dvel/dλ = -Γ^μ_αβ vel^α vel^β
   *
   * k1 = f(y_n)
   * k2 = f(y_n + h/2 * k1)
   * k3 = f(y_n + h/2 * k2)
   * k4 = f(y_n + h * k3)
   * y_{n+1} = y_n + h/6 * (k1 + 2*k2 + 2*k3 + k4)
   */

	struct bhs_vec4 pos = geo->pos;
	struct bhs_vec4 vel = geo->vel;
	double h = dlambda;

	/* k1 */
	struct bhs_vec4 k1_pos = dpos_dlambda(vel);
	struct bhs_vec4 k1_vel = dvel_dlambda(bh, pos, vel);

	/* k2 */
	struct bhs_vec4 pos2 =
		bhs_vec4_add(pos, bhs_vec4_scale(k1_pos, 0.5 * h));
	struct bhs_vec4 vel2 =
		bhs_vec4_add(vel, bhs_vec4_scale(k1_vel, 0.5 * h));
	struct bhs_vec4 k2_pos = dpos_dlambda(vel2);
	struct bhs_vec4 k2_vel = dvel_dlambda(bh, pos2, vel2);

	/* k3 */
	struct bhs_vec4 pos3 =
		bhs_vec4_add(pos, bhs_vec4_scale(k2_pos, 0.5 * h));
	struct bhs_vec4 vel3 =
		bhs_vec4_add(vel, bhs_vec4_scale(k2_vel, 0.5 * h));
	struct bhs_vec4 k3_pos = dpos_dlambda(vel3);
	struct bhs_vec4 k3_vel = dvel_dlambda(bh, pos3, vel3);

	/* k4 */
	struct bhs_vec4 pos4 = bhs_vec4_add(pos, bhs_vec4_scale(k3_pos, h));
	struct bhs_vec4 vel4 = bhs_vec4_add(vel, bhs_vec4_scale(k3_vel, h));
	struct bhs_vec4 k4_pos = dpos_dlambda(vel4);
	struct bhs_vec4 k4_vel = dvel_dlambda(bh, pos4, vel4);

	/* Combina: y_{n+1} = y_n + h/6 * (k1 + 2*k2 + 2*k3 + k4) */
	struct bhs_vec4 delta_pos = bhs_vec4_scale(
		bhs_vec4_add(bhs_vec4_add(k1_pos, bhs_vec4_scale(k2_pos, 2.0)),
			     bhs_vec4_add(bhs_vec4_scale(k3_pos, 2.0), k4_pos)),
		h / 6.0);

	struct bhs_vec4 delta_vel = bhs_vec4_scale(
		bhs_vec4_add(bhs_vec4_add(k1_vel, bhs_vec4_scale(k2_vel, 2.0)),
			     bhs_vec4_add(bhs_vec4_scale(k3_vel, 2.0), k4_vel)),
		h / 6.0);

	geo->pos = bhs_vec4_add(pos, delta_pos);
	geo->vel = bhs_vec4_add(vel, delta_vel);
	geo->affine_param += dlambda;
	geo->step_count++;

	/* Wrap θ para [0, π] e φ para [-π, π] */
	if (geo->pos.y < 0.0) {
		geo->pos.y = -geo->pos.y;
		geo->pos.z += M_PI; /* φ inverte */
	}
	if (geo->pos.y > M_PI) {
		geo->pos.y = 2.0 * M_PI - geo->pos.y;
		geo->pos.z += M_PI;
	}

	/* Wrap φ para [-π, π] */
	while (geo->pos.z > M_PI)
		geo->pos.z -= 2.0 * M_PI;
	while (geo->pos.z < -M_PI)
		geo->pos.z += 2.0 * M_PI;

	return 0;
}

int bhs_geodesic_step_adaptive(struct bhs_geodesic *geo,
			       const struct bhs_kerr *bh, double *dlambda,
			       double tolerance)
{
	/*
   * Passo adaptativo usando estimativa de erro.
   * Compara resultado de um passo h vs dois passos h/2.
   */

	struct bhs_geodesic geo_full = *geo;
	struct bhs_geodesic geo_half = *geo;

	double h = *dlambda;

	/* Um passo grande */
	bhs_geodesic_step_rk4(&geo_full, bh, h);

	/* Dois passos pequenos */
	bhs_geodesic_step_rk4(&geo_half, bh, h / 2.0);
	bhs_geodesic_step_rk4(&geo_half, bh, h / 2.0);

	/* Estimativa de erro (diferença nas posições) */
	struct bhs_vec4 diff = bhs_vec4_sub(geo_full.pos, geo_half.pos);
	double error = sqrt(diff.t * diff.t + diff.x * diff.x +
			    diff.y * diff.y + diff.z * diff.z);

	/* Ajusta passo */
	double safety = 0.9;
	double factor = safety * pow(tolerance / (error + 1e-15), 0.2);
	factor = fmax(0.1, fmin(4.0, factor));
	*dlambda = h * factor;

	/* Usa resultado mais preciso (dois passos) */
	*geo = geo_half;

	return error > tolerance * 10.0 ? -1 : 0;
}

/* ============================================================================
 * PROPAGAÇÃO COMPLETA
 * ============================================================================
 */

enum bhs_geodesic_status
bhs_geodesic_propagate(struct bhs_geodesic *geo, const struct bhs_kerr *bh,
		       const struct bhs_geodesic_config *config)
{
	int max_steps = config->max_steps > 0 ? config->max_steps
					      : BHS_GEODESIC_MAX_STEPS;
	double escape_r = config->escape_radius > 0
				  ? config->escape_radius
				  : BHS_GEODESIC_ESCAPE_RADIUS;
	double r_horizon = bhs_kerr_horizon_outer(bh);

	for (int i = 0; i < max_steps; i++) {
		/* Verifica condições de parada */
		double r = geo->pos.x;

		/* Capturado pelo horizonte */
		if (r < r_horizon * 1.01) {
			geo->status = BHS_GEO_CAPTURED;
			return BHS_GEO_CAPTURED;
		}

		/* Escapou */
		if (r > escape_r) {
			geo->status = BHS_GEO_ESCAPED;
			return BHS_GEO_ESCAPED;
		}

		/* Atingiu o disco */
		if (config->disk_outer > 0 &&
		    bhs_geodesic_is_in_disk(geo, config->disk_inner,
					    config->disk_outer,
					    config->disk_half_thickness)) {
			geo->status = BHS_GEO_HIT_DISK;
			return BHS_GEO_HIT_DISK;
		}

		/* Próximo passo */
		bhs_geodesic_step_rk4(geo, bh, config->dlambda);
	}

	geo->status = BHS_GEO_TIMEOUT;
	return BHS_GEO_TIMEOUT;
}

/* ============================================================================
 * VERIFICAÇÕES
 * ============================================================================
 */

bool bhs_geodesic_is_inside_horizon(const struct bhs_geodesic *geo,
				    const struct bhs_kerr *bh)
{
	double r_horizon = bhs_kerr_horizon_outer(bh);
	return geo->pos.x < r_horizon;
}

bool bhs_geodesic_is_in_disk(const struct bhs_geodesic *geo, double inner,
			     double outer, double half_thickness)
{
	double r = geo->pos.x;
	double theta = geo->pos.y;

	/* Altura z = r * cos(θ) */
	double z = r * cos(theta);

	/* No disco se: inner < r < outer e |z| < half_thickness */
	return (r > inner && r < outer && fabs(z) < half_thickness);
}

double bhs_geodesic_norm2(const struct bhs_geodesic *geo,
			  const struct bhs_kerr *bh)
{
	struct bhs_metric g;
	bhs_kerr_metric(bh, geo->pos.x, geo->pos.y, &g);
	return bhs_metric_dot(&g, geo->vel, geo->vel);
}

/* ============================================================================
 * UTILIDADES
 * ============================================================================
 */

void bhs_geodesic_ray_from_camera(struct bhs_geodesic *geo,
				  struct bhs_vec3 cam_pos,
				  struct bhs_vec3 cam_dir,
				  struct bhs_vec3 cam_up, double u, double v,
				  double fov, const struct bhs_kerr *bh)
{
	/* Calcula base ortonormal da câmera */
	struct bhs_vec3 forward = bhs_vec3_normalize(cam_dir);
	struct bhs_vec3 right =
		bhs_vec3_normalize(bhs_vec3_cross(forward, cam_up));
	struct bhs_vec3 up = bhs_vec3_cross(right, forward);

	/* Direção do raio para pixel (u, v) */
	double tan_fov = tan(fov * 0.5);
	struct bhs_vec3 ray_dir = bhs_vec3_add(
		forward, bhs_vec3_add(bhs_vec3_scale(right, u * tan_fov),
				      bhs_vec3_scale(up, v * tan_fov)));
	ray_dir = bhs_vec3_normalize(ray_dir);

	/* Converte posição da câmera para esféricas */
	double r, theta, phi;
	bhs_vec3_to_spherical(cam_pos, &r, &theta, &phi);

	struct bhs_vec4 pos = bhs_vec4_make(0.0, r, theta, phi);

	/* Converte direção para esféricas (aproximação local) */
	/* Transformação Jacobiana cartesiana → esférica */
	double sin_theta = sin(theta);
	double cos_theta = cos(theta);
	double sin_phi = sin(phi);
	double cos_phi = cos(phi);

	/* ∂r/∂(x,y,z) */
	double dr = ray_dir.x * sin_theta * cos_phi +
		    ray_dir.y * sin_theta * sin_phi + ray_dir.z * cos_theta;

	/* ∂θ/∂(x,y,z) */
	double dtheta =
		(ray_dir.x * cos_theta * cos_phi +
		 ray_dir.y * cos_theta * sin_phi - ray_dir.z * sin_theta) /
		r;

	/* ∂φ/∂(x,y,z) */
	double dphi = (-ray_dir.x * sin_phi + ray_dir.y * cos_phi) /
		      (r * sin_theta + 1e-15);

	struct bhs_vec3 dir_spherical = bhs_vec3_make(dr, dtheta, dphi);

	bhs_geodesic_init_photon(geo, pos, dir_spherical, bh);
}
