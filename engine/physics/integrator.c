/**
 * @file integrator.c
 * @brief Implementação dos Integradores Numéricos
 *
 * "RK4: Quatro avaliações pra fazer o que Euler faz mal com uma."
 */

#include "engine/physics/integrator.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* 
 * Softening para evitar singularidade
 * Reduzido para permitir órbitas lunares/LEO precisas.
 * 100 km = 1e5 m.
 */
#define SOFTENING_DIST (1.0e5)
#define SOFTENING_SQ (SOFTENING_DIST * SOFTENING_DIST)

/*
 * Limiar de massa para considerar correções relativísticas.
 * Em SI, GM do Sol ≈ 1.32e20 m³/s². Só aplicar 1PN para objetos
 * comparáveis a estrelas de nêutrons ou buracos negros.
 * Para o Sol normal, 1PN é desprezível e pode causar instabilidade.
 */
#define RELATIVISTIC_MASS_THRESHOLD 1.0e25 /* GM muito alto (BH) */

/*
 * Velocidade da luz em SI (m/s)
 */
#define C_SIM 299792458.0

/* ============================================================================
 * CÁLCULO DE ACELERAÇÕES (COM CORREÇÃO 1PN)
 * ============================================================================
 */

void bhs_compute_accelerations(const struct bhs_system_state *state,
			       struct bhs_vec3 acc[])
{
	int n = state->n_bodies;

	/* Zera acelerações usando Kahan accumulators */
	struct bhs_kahan_vec3 acc_kahan[BHS_MAX_BODIES];
	for (int i = 0; i < n; i++) {
		bhs_kahan_vec3_init(&acc_kahan[i]);
	}

	/* Gravidade N-body com simetria (N²/2) */
	for (int i = 0; i < n; i++) {
		if (!state->bodies[i].is_alive)
			continue;

		for (int j = i + 1; j < n; j++) {
			if (!state->bodies[j].is_alive)
				continue;

			const struct bhs_body_state_rk *bi = &state->bodies[i];
			const struct bhs_body_state_rk *bj = &state->bodies[j];

			/* Vetor distância */
			double dx = bj->pos.x - bi->pos.x;
			double dy = bj->pos.y - bi->pos.y;
			double dz = bj->pos.z - bi->pos.z;

			double dist_sq = dx * dx + dy * dy + dz * dz;

			/*
			 * Plummer softening: F = GM₁M₂ / (r² + ε²)^(3/2) * r
			 * Evita singularidade mantendo comportamento correto a longas distâncias
			 */
			double soft_sq = dist_sq + SOFTENING_SQ;
			double soft_dist = sqrt(soft_sq);
			double inv_dist3 =
				1.0 / (soft_dist * soft_dist * soft_dist);

			/*
			 * ================ GRAVIDADE NEWTONIANA ================
			 * F_ij = G * m_i * m_j / r³ * r_vec
			 * a_i = F_ij / m_i = G * m_j / r³ * r_vec = GM_j / r³ * r_vec
			 * a_j = -F_ij / m_j = -GM_i / r³ * r_vec
			 * 
			 * NOTA: Mesmo que bi seja fixo (Sol), bj (planetas) AINDA
			 * precisa sentir a gravidade do Sol! O is_fixed só impede
			 * que o corpo fixo se mova, não que ele exerça gravidade.
			 */

			/* Aceleração de i devido a j (só se i não é fixo) */
			if (!bi->is_fixed) {
				double factor_i = bj->gm * inv_dist3;
				struct bhs_vec3 a_i = { .x = factor_i * dx,
							.y = factor_i * dy,
							.z = factor_i * dz };
				bhs_kahan_vec3_add(&acc_kahan[i], a_i);

				/* Correção 1PN (só pra buracos negros muito massivos) */
				if (bj->gm > RELATIVISTIC_MASS_THRESHOLD) {
					struct bhs_vec3 rel_pos = { dx, dy,
								    dz };
					struct bhs_vec3 a_1pn =
						bhs_compute_1pn_correction(
							bj->gm, rel_pos,
							bi->vel, C_SIM);
					bhs_kahan_vec3_add(&acc_kahan[i],
							   a_1pn);
				}
			}

			/* Aceleração de j devido a i (só se j não é fixo) */
			/* ISSO É CRÍTICO: mesmo que bi (Sol) seja fixo, 
			 * bj (Terra) ainda precisa sentir a gravidade! */
			if (!bj->is_fixed) {
				double factor_j = bi->gm * inv_dist3;
				struct bhs_vec3 a_j = { .x = -factor_j * dx,
							.y = -factor_j * dy,
							.z = -factor_j * dz };
				bhs_kahan_vec3_add(&acc_kahan[j], a_j);

				/* Correção 1PN para j se i é muito massivo */
				if (bi->gm > RELATIVISTIC_MASS_THRESHOLD) {
					struct bhs_vec3 rel_pos = { -dx, -dy,
								    -dz };
					struct bhs_vec3 a_1pn =
						bhs_compute_1pn_correction(
							bi->gm, rel_pos,
							bj->vel, C_SIM);
					bhs_kahan_vec3_add(&acc_kahan[j],
							   a_1pn);
				}

				/* [NEW] Correção J2 (Oblateness) se body i for achatado */
				if (bi->j2 > 0.0 && bi->radius > 0.0) {
					struct bhs_vec3 rel_pos_j = {
						-dx, -dy, -dz
					}; /* Pos de j relativa a i */
					/* Assumindo que o corpo i está alinhado com Z (simplificação comum) 
					   ou transformando para frame local. Aqui vamos aplicar assumindo alinhamento Z para P.O.C.
					   Se o corpo estiver inclinado (axis_tilt), precisaríamos rotacionar rel_pos.
					   TODO: Implementar rotação baseada em bi->rot_axis se necessário. 
					   Por enquanto, J2 assume eixo Z alinhado com momento angular do sistema (eclíptica). 
					   Para planetas, isso é razoável se Z for "Norte do Sistema". */

					struct bhs_vec3 a_j2 =
						bhs_compute_j2_correction(
							bi->gm, bi->j2,
							bi->radius, rel_pos_j);
					bhs_kahan_vec3_add(&acc_kahan[j], a_j2);
				}
			}

			/* [NEW] Correção J2 inversa: Se J for achatado, I sente força?
			   Sim, mas geralmente desprezível se I for o Sol ou outro planeta distante.
			   Mas para sistemas binários, ambos sentem. */
			if (!bi->is_fixed && bj->j2 > 0.0 && bj->radius > 0.0) {
				struct bhs_vec3 rel_pos_i = {
					dx, dy, dz
				}; /* Pos de i relativa a j */
				struct bhs_vec3 a_j2_i =
					bhs_compute_j2_correction(
						bj->gm, bj->j2, bj->radius,
						rel_pos_i);
				bhs_kahan_vec3_add(&acc_kahan[i], a_j2_i);
			}
		}
	}

	/* Extrai resultados dos acumuladores */
	for (int i = 0; i < n; i++) {
		acc[i] = bhs_kahan_vec3_get(&acc_kahan[i]);
	}
}

/* ============================================================================
 * CORREÇÃO RELATIVÍSTICA 1PN (POST-NEWTONIAN)
 * ============================================================================
 *
 * Baseado na formulação de Einstein-Infeld-Hoffmann (EIH).
 * Produz precessão do periélio observada em Mercúrio (~43 arcsec/século).
 *
 * Referência: MTW Gravitation, eq. 39.41
 */

struct bhs_vec3 bhs_compute_1pn_correction(double gm_central,
					   struct bhs_vec3 pos,
					   struct bhs_vec3 vel, double c)
{
	double c2 = c * c;

	/* Distância e versores */
	double r2 = pos.x * pos.x + pos.y * pos.y + pos.z * pos.z;
	double r = sqrt(r2);

	if (r < 1e-10) {
		return (struct bhs_vec3){ 0, 0, 0 };
	}

	double inv_r = 1.0 / r;

	/* Versor radial */
	struct bhs_vec3 r_hat = { .x = pos.x * inv_r,
				  .y = pos.y * inv_r,
				  .z = pos.z * inv_r };

	/* Velocidade ao quadrado */
	double v2 = vel.x * vel.x + vel.y * vel.y + vel.z * vel.z;

	/* Velocidade radial (v · r̂) */
	double v_r = vel.x * r_hat.x + vel.y * r_hat.y + vel.z * r_hat.z;

	/*
	 * Correção 1PN simplificada (Schwarzschild):
	 * a_1PN = (GM/r²c²) * [(4GM/r - v²) * r̂ + 4v_r * v]
	 *
	 * Esta é a aproximação para campo fraco e velocidades baixas.
	 */
	double gm_over_r = gm_central * inv_r;
	double coeff = gm_central / (r2 * c2);

	/* Termo radial: (4GM/r - v²) */
	double radial_term = 4.0 * gm_over_r - v2;

	/* Termo tangencial: 4 * v_r */
	double tangential_term = 4.0 * v_r;

	struct bhs_vec3 a_1pn = {
		.x = coeff * (radial_term * r_hat.x + tangential_term * vel.x),
		.y = coeff * (radial_term * r_hat.y + tangential_term * vel.y),
		.z = coeff * (radial_term * r_hat.z + tangential_term * vel.z)
	};

	return a_1pn;
}

/* ============================================================================
 * CORREÇÃO J2 (OBLATENESS)
 * ============================================================================
 *
 * Corrige para o achatamento de planetas/estrelas (não são esferas perfeitas).
 * A Terra tem J2 = 1.08263e-3, causando precessão nodal em satélites.
 *
 * Fórmula (em coordenadas cartesianas):
 *   a_x = -3/2 * J2 * (GM/r²) * (R_eq/r)² * x/r * (5z²/r² - 1)
 *   a_y = -3/2 * J2 * (GM/r²) * (R_eq/r)² * y/r * (5z²/r² - 1)
 *   a_z = -3/2 * J2 * (GM/r²) * (R_eq/r)² * z/r * (5z²/r² - 3)
 */

struct bhs_vec3 bhs_compute_j2_correction(double gm_central, double j2,
					  double r_eq, struct bhs_vec3 pos)
{
	double x = pos.x, y = pos.y, z = pos.z;
	double r2 = x * x + y * y + z * z;
	double r = sqrt(r2);

	if (r < 1e-10 || j2 == 0) {
		return (struct bhs_vec3){ 0, 0, 0 };
	}

	double r5 = r2 * r2 * r;
	double z2 = z * z;
	double r_eq2 = r_eq * r_eq;

	/* Coeficiente comum: -3/2 * J2 * GM * R_eq² / r⁵ */
	double coeff = -1.5 * j2 * gm_central * r_eq2 / r5;

	/* Termos dependentes de z²/r² */
	double xy_factor = 5.0 * z2 / r2 - 1.0;
	double z_factor = 5.0 * z2 / r2 - 3.0;

	struct bhs_vec3 a_j2 = { .x = coeff * x * xy_factor,
				 .y = coeff * y * xy_factor,
				 .z = coeff * z * z_factor };

	return a_j2;
}

/* ============================================================================
 * TORQUE DE MARÉ (TIDAL TORQUE) & SIMULAÇÃO 6-DOF
 * ============================================================================
 *
 * Simula o torque que tende a sincronizar a rotação com a órbita.
 * T = -k * (M_primario^2 / r^6) * (w_spin - w_orbital)
 * 
 * k é um coeficiente de eficiência de maré. Para visualização, 
 * usamos um valor alto para ver o efeito em tempo útil.
 */

#define TIDAL_K 1.0e-5 /* Coeficiente fictício para acelerar locking */

void bhs_compute_torques(const struct bhs_system_state *state,
			 struct bhs_vec3 torques[])
{
	int n = state->n_bodies;

	/* Zera torques */
	for (int i = 0; i < n; i++) {
		torques[i] = (struct bhs_vec3){ 0, 0, 0 };
	}

	for (int i = 0; i < n; i++) {
		if (state->bodies[i].is_fixed || !state->bodies[i].is_alive)
			continue;

		for (int j = 0; j < n; j++) {
			if (i == j)
				continue;
			if (!state->bodies[j].is_alive)
				continue;

			/* Considerar apenas maré de corpos massivos (para otimizar) */
			/* Mas para purismo, todo corpo exerce maré. */
			/* Filtro: A massa do outro deve ser significativa */
			if (state->bodies[j].mass < state->bodies[i].mass * 0.1)
				continue;

			const struct bhs_body_state_rk *bi = &state->bodies[i];
			const struct bhs_body_state_rk *bj = &state->bodies[j];

			double dx = bj->pos.x - bi->pos.x;
			double dy = bj->pos.y - bi->pos.y;
			double dz = bj->pos.z - bi->pos.z;
			double r2 = dx * dx + dy * dy + dz * dz;
			double r = sqrt(r2);

			if (r < 1e-10)
				continue;

			/* Velocidade orbital relativa (w_orbital) */
			/* W_orb = (r x v_rel) / r^2 */
			/* v_rel = vj - vi */
			double dvx = bj->vel.x - bi->vel.x;
			double dvy = bj->vel.y - bi->vel.y;
			double dvz = bj->vel.z - bi->vel.z;

			struct bhs_vec3 cross = { .x = dy * dvz - dz * dvy,
						  .y = dz * dvx - dx * dvz,
						  .z = dx * dvy - dy * dvx };
			struct bhs_vec3 w_orb = { .x = cross.x / r2,
						  .y = cross.y / r2,
						  .z = cross.z / r2 };

			/* Diferença de velocidade angular (Spin - Orbit) */
			struct bhs_vec3 dw = { .x = bi->rot_vel.x - w_orb.x,
					       .y = bi->rot_vel.y - w_orb.y,
					       .z = bi->rot_vel.z - w_orb.z };

			/* Torque ~ - Dw / r^6 */
			/* Usando fator amplificado para simulação visual e GM^2 */
			/* Nota: Formula real complicada dependendo de Q factor e k2 Love number.
			   Modelo simplificado proporcional a diferença de vel angular. */

			double r6 = r2 * r2 * r2;
			double factor = TIDAL_K * (bj->gm * bj->gm) / r6;

			/* Limiter para evitar instabilidade numérica */
			if (factor > 1.0)
				factor = 1.0;

			torques[i].x -= factor * dw.x;
			torques[i].y -= factor * dw.y;
			torques[i].z -= factor * dw.z;
		}
	}
}

/* ============================================================================
 * RK4 CLÁSSICO
 * ============================================================================
 * 
 * Algoritmo:
 *   k1 = f(t, y)
 *   k2 = f(t + dt/2, y + dt/2 * k1)
 *   k3 = f(t + dt/2, y + dt/2 * k2)
 *   k4 = f(t + dt, y + dt * k3)
 *   y_new = y + dt/6 * (k1 + 2*k2 + 2*k3 + k4)
 *
 * Para nosso sistema:
 *   y = (pos, vel)
 *   f(y) = (vel, acc(pos))
 */

void bhs_integrator_rk4(struct bhs_system_state *state, double dt)
{
	int n = state->n_bodies;
	if (n == 0)
		return;

	/* Arrays temporários para k1, k2, k3, k4 */
	struct bhs_vec3 k1_pos[BHS_MAX_BODIES], k1_vel[BHS_MAX_BODIES];
	struct bhs_vec3 k2_pos[BHS_MAX_BODIES], k2_vel[BHS_MAX_BODIES];
	struct bhs_vec3 k3_pos[BHS_MAX_BODIES], k3_vel[BHS_MAX_BODIES];
	struct bhs_vec3 k4_pos[BHS_MAX_BODIES], k4_vel[BHS_MAX_BODIES];

	/* Estado temporário para avaliações intermediárias */
	struct bhs_system_state temp_state;
	memcpy(&temp_state, state, sizeof(temp_state));

	/* ===== k1 = f(t, y) ===== */
	struct bhs_vec3 acc[BHS_MAX_BODIES];
	bhs_compute_accelerations(state, acc);

	for (int i = 0; i < n; i++) {
		k1_pos[i] = state->bodies[i].vel;
		k1_vel[i] = acc[i];
	}

	/* ===== k2 = f(t + dt/2, y + dt/2 * k1) ===== */
	for (int i = 0; i < n; i++) {
		temp_state.bodies[i].pos.x =
			state->bodies[i].pos.x + 0.5 * dt * k1_pos[i].x;
		temp_state.bodies[i].pos.y =
			state->bodies[i].pos.y + 0.5 * dt * k1_pos[i].y;
		temp_state.bodies[i].pos.z =
			state->bodies[i].pos.z + 0.5 * dt * k1_pos[i].z;
		temp_state.bodies[i].vel.x =
			state->bodies[i].vel.x + 0.5 * dt * k1_vel[i].x;
		temp_state.bodies[i].vel.y =
			state->bodies[i].vel.y + 0.5 * dt * k1_vel[i].y;
		temp_state.bodies[i].vel.z =
			state->bodies[i].vel.z + 0.5 * dt * k1_vel[i].z;
	}
	bhs_compute_accelerations(&temp_state, acc);

	for (int i = 0; i < n; i++) {
		k2_pos[i] = temp_state.bodies[i].vel;
		k2_vel[i] = acc[i];
	}

	/* ===== k3 = f(t + dt/2, y + dt/2 * k2) ===== */
	for (int i = 0; i < n; i++) {
		temp_state.bodies[i].pos.x =
			state->bodies[i].pos.x + 0.5 * dt * k2_pos[i].x;
		temp_state.bodies[i].pos.y =
			state->bodies[i].pos.y + 0.5 * dt * k2_pos[i].y;
		temp_state.bodies[i].pos.z =
			state->bodies[i].pos.z + 0.5 * dt * k2_pos[i].z;
		temp_state.bodies[i].vel.x =
			state->bodies[i].vel.x + 0.5 * dt * k2_vel[i].x;
		temp_state.bodies[i].vel.y =
			state->bodies[i].vel.y + 0.5 * dt * k2_vel[i].y;
		temp_state.bodies[i].vel.z =
			state->bodies[i].vel.z + 0.5 * dt * k2_vel[i].z;
	}
	bhs_compute_accelerations(&temp_state, acc);

	for (int i = 0; i < n; i++) {
		k3_pos[i] = temp_state.bodies[i].vel;
		k3_vel[i] = acc[i];
	}

	/* ===== k4 = f(t + dt, y + dt * k3) ===== */
	for (int i = 0; i < n; i++) {
		temp_state.bodies[i].pos.x =
			state->bodies[i].pos.x + dt * k3_pos[i].x;
		temp_state.bodies[i].pos.y =
			state->bodies[i].pos.y + dt * k3_pos[i].y;
		temp_state.bodies[i].pos.z =
			state->bodies[i].pos.z + dt * k3_pos[i].z;
		temp_state.bodies[i].vel.x =
			state->bodies[i].vel.x + dt * k3_vel[i].x;
		temp_state.bodies[i].vel.y =
			state->bodies[i].vel.y + dt * k3_vel[i].y;
		temp_state.bodies[i].vel.z =
			state->bodies[i].vel.z + dt * k3_vel[i].z;
	}
	bhs_compute_accelerations(&temp_state, acc);

	for (int i = 0; i < n; i++) {
		k4_pos[i] = temp_state.bodies[i].vel;
		k4_vel[i] = acc[i];
	}

	/* ===== Atualiza estado: y = y + dt/6 * (k1 + 2*k2 + 2*k3 + k4) ===== */
	/* ===== Atualiza estado: y = y + dt/6 * (k1 + 2*k2 + 2*k3 + k4) ===== */
	double dt6 = dt / 6.0;

	/* [NEW] Torque Integration */
	/* Para rotação, usamos Euler simples ou integrador separado? 
	   Para manter consistência, deveriamos fazer RK4 para rotação também.
	   Simplificação: Usar Euler para rotação (já que torque muda lentamente) 
	   OU chamar bhs_compute_torques em cada etapa K.
	   
	   Vou optar por Euler simples para rotação para economizar 4xN^2 checks de torque,
	   pois maré é força fraca de variacao lenta comparada a orbita.
	*/

	struct bhs_vec3 torques[BHS_MAX_BODIES];
	bhs_compute_torques(state, torques);

	for (int i = 0; i < n; i++) {
		if (state->bodies[i].is_fixed)
			continue;

		state->bodies[i].pos.x += dt6 * (k1_pos[i].x + 2 * k2_pos[i].x +
						 2 * k3_pos[i].x + k4_pos[i].x);
		state->bodies[i].pos.y += dt6 * (k1_pos[i].y + 2 * k2_pos[i].y +
						 2 * k3_pos[i].y + k4_pos[i].y);
		state->bodies[i].pos.z += dt6 * (k1_pos[i].z + 2 * k2_pos[i].z +
						 2 * k3_pos[i].z + k4_pos[i].z);

		state->bodies[i].vel.x += dt6 * (k1_vel[i].x + 2 * k2_vel[i].x +
						 2 * k3_vel[i].x + k4_vel[i].x);
		state->bodies[i].vel.y += dt6 * (k1_vel[i].y + 2 * k2_vel[i].y +
						 2 * k3_vel[i].y + k4_vel[i].y);
		state->bodies[i].vel.z += dt6 * (k1_vel[i].z + 2 * k2_vel[i].z +
						 2 * k3_vel[i].z + k4_vel[i].z);

		/* [NEW] Update Rotation */
		/* w_new = w + (torque/I) * dt */
		if (state->bodies[i].inertia > 0.0) {
			double inv_I = 1.0 / state->bodies[i].inertia;
			state->bodies[i].rot_vel.x += torques[i].x * inv_I * dt;
			state->bodies[i].rot_vel.y += torques[i].y * inv_I * dt;
			state->bodies[i].rot_vel.z += torques[i].z * inv_I * dt;

			/* Update Rotation Angle (Scalar magnitude integration for rendering) */
			/* Assuming rotation roughly around Y local or main axis */
			/* Magnitude of w */

			/* Sign? Dot product with Up Vector? Simplification: Just add magnitude */
			/* state has no "angle" field in RK struct, only in ECS. 
			   Wait, checking integrator.h struct... it doesn't have angle. 
			   The ECS update needs to reconstruct angle from w * dt. 
			   So we just update W here. Angle update happens in Physics System. */
		}
	}

	state->time += dt;
}

/* ============================================================================
 * LEAPFROG / STÖRMER-VERLET (SIMPLÉTICO)
 * ============================================================================
 *
 * O Leapfrog é um integrador SIMPLÉTICO - preserva a estrutura hamiltoniana
 * do sistema e, portanto, conserva energia por tempo indefinido (exceto
 * erros de arredondamento).
 *
 * Algoritmo (Kick-Drift-Kick variant):
 *   1. v(t + dt/2) = v(t) + a(t) * dt/2        [KICK]
 *   2. x(t + dt) = x(t) + v(t + dt/2) * dt     [DRIFT]
 *   3. Calcula a(t + dt)
 *   4. v(t + dt) = v(t + dt/2) + a(t + dt) * dt/2  [KICK]
 *
 * Vantagens:
 *   - Conserva energia (erro limitado, não acumula)
 *   - Reversível no tempo
 *   - O(dt²) - segunda ordem mas MUITO estável
 *
 * Usado em: GADGET, REBOUND, GROMACS, e maioria dos códigos N-body profissionais.
 *
 * Referência: Hockney & Eastwood (1988), "Computer Simulation Using Particles"
 */

void bhs_integrator_leapfrog(struct bhs_system_state *state, double dt)
{
	int n = state->n_bodies;
	if (n == 0)
		return;

	struct bhs_vec3 acc[BHS_MAX_BODIES];

	/* ===== KICK 1: v(t + dt/2) = v(t) + a(t) * dt/2 ===== */
	bhs_compute_accelerations(state, acc);

	double half_dt = 0.5 * dt;
	for (int i = 0; i < n; i++) {
		if (state->bodies[i].is_fixed || !state->bodies[i].is_alive)
			continue;

		state->bodies[i].vel.x += acc[i].x * half_dt;
		state->bodies[i].vel.y += acc[i].y * half_dt;
		state->bodies[i].vel.z += acc[i].z * half_dt;
	}

	/* ===== DRIFT: x(t + dt) = x(t) + v(t + dt/2) * dt ===== */
	for (int i = 0; i < n; i++) {
		if (state->bodies[i].is_fixed || !state->bodies[i].is_alive)
			continue;

		state->bodies[i].pos.x += state->bodies[i].vel.x * dt;
		state->bodies[i].pos.y += state->bodies[i].vel.y * dt;
		state->bodies[i].pos.z += state->bodies[i].vel.z * dt;
	}

	/* ===== KICK 2: v(t + dt) = v(t + dt/2) + a(t + dt) * dt/2 ===== */
	bhs_compute_accelerations(state, acc);

	/* [NEW] Torque Calculation (Once per step for Leapfrog too) */
	struct bhs_vec3 torques[BHS_MAX_BODIES];
	bhs_compute_torques(state, torques);

	for (int i = 0; i < n; i++) {
		if (state->bodies[i].is_fixed || !state->bodies[i].is_alive)
			continue;

		state->bodies[i].vel.x += acc[i].x * half_dt;
		state->bodies[i].vel.y += acc[i].y * half_dt;
		state->bodies[i].vel.z += acc[i].z * half_dt;

		/* Update Rotation (Symplectic-ish? Just Euler Kick) */
		if (state->bodies[i].inertia > 0.0) {
			double inv_I = 1.0 / state->bodies[i].inertia;
			state->bodies[i].rot_vel.x += torques[i].x * inv_I * dt;
			state->bodies[i].rot_vel.y += torques[i].y * inv_I * dt;
			state->bodies[i].rot_vel.z += torques[i].z * inv_I * dt;
		}

		// Debug Mercury (Index 1) acceleration
		if (i == 1) {
			static int debug_counter = 0;
			if (debug_counter++ % 60 == 0) {
				// ...
			}
		}
	}

	state->time += dt;
}

/* ============================================================================
 * YOSHIDA (4th Order Symplectic)
 * ============================================================================
 */
void bhs_integrator_yoshida(struct bhs_system_state *state, double dt)
{
	int n = state->n_bodies;
	if (n == 0)
		return;

	/* Coeficientes Yoshida (4th Order) */
	/* w1 = 1 / (2 - 2^(1/3)) */
	/* w0 = -2^(1/3) * w1 */

	/* Pre-calculated for speed */
	const double w1 = 1.351207191959657;
	const double w0 = -1.702414383919315;

	const double c1 = w1 / 2.0;
	const double c2 = (w0 + w1) / 2.0;
	const double c3 = c2;
	const double c4 = c1;

	const double d1 = w1;
	const double d2 = w0;
	const double d3 = w1;

	struct bhs_vec3 acc[BHS_MAX_BODIES];

	/* Step 1: x1 = x0 + c1 * v0 * dt */
	for (int i = 0; i < n; i++) {
		if (!state->bodies[i].is_alive || state->bodies[i].is_fixed)
			continue;
		state->bodies[i].pos.x += c1 * state->bodies[i].vel.x * dt;
		state->bodies[i].pos.y += c1 * state->bodies[i].vel.y * dt;
		state->bodies[i].pos.z += c1 * state->bodies[i].vel.z * dt;
	}

	/* Step 2: v1 = v0 + d1 * a(x1) * dt */
	bhs_compute_accelerations(state, acc);
	for (int i = 0; i < n; i++) {
		if (!state->bodies[i].is_alive || state->bodies[i].is_fixed)
			continue;
		state->bodies[i].vel.x += d1 * acc[i].x * dt;
		state->bodies[i].vel.y += d1 * acc[i].y * dt;
		state->bodies[i].vel.z += d1 * acc[i].z * dt;
	}

	/* Step 3: x2 = x1 + c2 * v1 * dt */
	for (int i = 0; i < n; i++) {
		if (!state->bodies[i].is_alive || state->bodies[i].is_fixed)
			continue;
		state->bodies[i].pos.x += c2 * state->bodies[i].vel.x * dt;
		state->bodies[i].pos.y += c2 * state->bodies[i].vel.y * dt;
		state->bodies[i].pos.z += c2 * state->bodies[i].vel.z * dt;
	}

	/* Step 4: v2 = v1 + d2 * a(x2) * dt */
	bhs_compute_accelerations(state, acc);
	for (int i = 0; i < n; i++) {
		if (!state->bodies[i].is_alive || state->bodies[i].is_fixed)
			continue;
		state->bodies[i].vel.x += d2 * acc[i].x * dt;
		state->bodies[i].vel.y += d2 * acc[i].y * dt;
		state->bodies[i].vel.z += d2 * acc[i].z * dt;
	}

	/* Step 5: x3 = x2 + c3 * v2 * dt */
	for (int i = 0; i < n; i++) {
		if (!state->bodies[i].is_alive || state->bodies[i].is_fixed)
			continue;
		state->bodies[i].pos.x += c3 * state->bodies[i].vel.x * dt;
		state->bodies[i].pos.y += c3 * state->bodies[i].vel.y * dt;
		state->bodies[i].pos.z += c3 * state->bodies[i].vel.z * dt;
	}

	/* Step 6: v3 = v2 + d3 * a(x3) * dt */
	bhs_compute_accelerations(state, acc);
	for (int i = 0; i < n; i++) {
		if (!state->bodies[i].is_alive || state->bodies[i].is_fixed)
			continue;
		state->bodies[i].vel.x += d3 * acc[i].x * dt;
		state->bodies[i].vel.y += d3 * acc[i].y * dt;
		state->bodies[i].vel.z += d3 * acc[i].z * dt;
	}

	/* Step 7: x4 = x3 + c4 * v3 * dt */
	for (int i = 0; i < n; i++) {
		if (!state->bodies[i].is_alive || state->bodies[i].is_fixed)
			continue;
		state->bodies[i].pos.x += c4 * state->bodies[i].vel.x * dt;
		state->bodies[i].pos.y += c4 * state->bodies[i].vel.y * dt;
		state->bodies[i].pos.z += c4 * state->bodies[i].vel.z * dt;
	}

	state->time += dt;
}

/* ============================================================================
 * RKF45 (ADAPTATIVO)
 * ============================================================================
 * Runge-Kutta-Fehlberg usa ordem 4 e 5 para estimar erro e ajustar timestep.
 */

double bhs_integrator_rkf45(struct bhs_system_state *state, double dt,
			    double tolerance, double *dt_out)
{
	/* 
	 * Coeficientes de Fehlberg (Cash-Karp variant)
	 * Para simplificar, usamos RK4 + estimativa de erro básica
	 * Uma implementação completa teria os 6 estágios do RKF45
	 */

	int n = state->n_bodies;
	if (n == 0) {
		*dt_out = dt;
		return 0.0;
	}

	/* Salva estado original */
	struct bhs_system_state original;
	memcpy(&original, state, sizeof(original));

	/* Integra com dt inteiro */
	struct bhs_system_state state_full;
	memcpy(&state_full, &original, sizeof(state_full));
	bhs_integrator_rk4(&state_full, dt);

	/* Integra com dois passos de dt/2 */
	struct bhs_system_state state_half;
	memcpy(&state_half, &original, sizeof(state_half));
	bhs_integrator_rk4(&state_half, dt / 2.0);
	bhs_integrator_rk4(&state_half, dt / 2.0);

	/* Estima erro como diferença entre as duas integrações */
	double max_error = 0.0;
	for (int i = 0; i < n; i++) {
		if (original.bodies[i].is_fixed)
			continue;

		double dx =
			state_full.bodies[i].pos.x - state_half.bodies[i].pos.x;
		double dy =
			state_full.bodies[i].pos.y - state_half.bodies[i].pos.y;
		double dz =
			state_full.bodies[i].pos.z - state_half.bodies[i].pos.z;
		double err = sqrt(dx * dx + dy * dy + dz * dz);

		if (err > max_error)
			max_error = err;
	}

	/* Usa resultado mais preciso (dt/2 duplo) */
	memcpy(state, &state_half, sizeof(*state));

	/* Calcula novo dt baseado no erro */
	if (max_error > 0) {
		double safety = 0.9;
		double factor = safety * pow(tolerance / max_error, 0.2);
		factor = fmax(0.1, fmin(factor, 5.0)); /* Limita mudança */
		*dt_out = dt * factor;
	} else {
		*dt_out = dt * 2.0; /* Erro zero, pode aumentar */
	}

	/* Limites absolutos */
	*dt_out = fmax(*dt_out, 1e-6);
	*dt_out = fmin(*dt_out, 1.0);

	return max_error;
}

/* ============================================================================
 * INVARIANTES
 * ============================================================================
 */

void bhs_compute_invariants(const struct bhs_system_state *state,
			    struct bhs_invariants *inv)
{
	int n = state->n_bodies;

	/* Inicializa com Kahan */
	struct bhs_kahan E_kinetic, E_potential;
	struct bhs_kahan_vec3 momentum, angular_momentum;

	bhs_kahan_init(&E_kinetic);
	bhs_kahan_init(&E_potential);
	bhs_kahan_vec3_init(&momentum);
	bhs_kahan_vec3_init(&angular_momentum);

	/* Energia cinética e momento linear */
	for (int i = 0; i < n; i++) {
		const struct bhs_body_state_rk *b = &state->bodies[i];
		if (!b->is_alive)
			continue;

		double v2 = b->vel.x * b->vel.x + b->vel.y * b->vel.y +
			    b->vel.z * b->vel.z;

		/* K = 0.5 * m * v² */
		bhs_kahan_add(&E_kinetic, 0.5 * b->mass * v2);

		/* p = m * v */
		struct bhs_vec3 p = { .x = b->mass * b->vel.x,
				      .y = b->mass * b->vel.y,
				      .z = b->mass * b->vel.z };
		bhs_kahan_vec3_add(&momentum, p);

		/* L = r × p */
		struct bhs_vec3 L = { .x = b->pos.y * p.z - b->pos.z * p.y,
				      .y = b->pos.z * p.x - b->pos.x * p.z,
				      .z = b->pos.x * p.y - b->pos.y * p.x };
		bhs_kahan_vec3_add(&angular_momentum, L);
	}

	/* Energia potencial gravitacional */
	for (int i = 0; i < n; i++) {
		if (!state->bodies[i].is_alive)
			continue;

		for (int j = i + 1; j < n; j++) {
			if (!state->bodies[j].is_alive)
				continue;

			const struct bhs_body_state_rk *bi = &state->bodies[i];
			const struct bhs_body_state_rk *bj = &state->bodies[j];

			double dx = bj->pos.x - bi->pos.x;
			double dy = bj->pos.y - bi->pos.y;
			double dz = bj->pos.z - bi->pos.z;
			double r = sqrt(dx * dx + dy * dy + dz * dz +
					SOFTENING_SQ);

			/* U = -G * m1 * m2 / r = -gm1 * m2 / r */
			double U = -bi->gm * bj->mass / r;
			bhs_kahan_add(&E_potential, U);
		}
	}

	/* Extrai resultados */
	inv->energy = bhs_kahan_get(&E_kinetic) + bhs_kahan_get(&E_potential);
	inv->momentum = bhs_kahan_vec3_get(&momentum);
	inv->angular_momentum = bhs_kahan_vec3_get(&angular_momentum);
}

bool bhs_check_conservation(const struct bhs_invariants *initial,
			    const struct bhs_invariants *current,
			    double tolerance)
{
	/* Verifica energia */
	double dE = fabs(current->energy - initial->energy);
	double E_rel = (fabs(initial->energy) > 1e-20)
			       ? dE / fabs(initial->energy)
			       : dE;

	if (E_rel > tolerance) {
		fprintf(stderr,
			"[INTEGRATOR] AVISO: Energia driftou %.2e%% "
			"(tolerância: %.2e%%)\n",
			E_rel * 100, tolerance * 100);
		return false;
	}

	/* Verifica momento linear */
	double dp = sqrt(pow(current->momentum.x - initial->momentum.x, 2) +
			 pow(current->momentum.y - initial->momentum.y, 2) +
			 pow(current->momentum.z - initial->momentum.z, 2));
	double p_mag = sqrt(initial->momentum.x * initial->momentum.x +
			    initial->momentum.y * initial->momentum.y +
			    initial->momentum.z * initial->momentum.z);
	double p_rel = (p_mag > 1e-20) ? dp / p_mag : dp;

	if (p_rel > tolerance) {
		fprintf(stderr,
			"[INTEGRATOR] AVISO: Momento linear driftou %.2e%%\n",
			p_rel * 100);
		return false;
	}

	/* Verifica momento angular */
	double dL = sqrt(
		pow(current->angular_momentum.x - initial->angular_momentum.x,
		    2) +
		pow(current->angular_momentum.y - initial->angular_momentum.y,
		    2) +
		pow(current->angular_momentum.z - initial->angular_momentum.z,
		    2));
	double L_mag =
		sqrt(initial->angular_momentum.x * initial->angular_momentum.x +
		     initial->angular_momentum.y * initial->angular_momentum.y +
		     initial->angular_momentum.z * initial->angular_momentum.z);
	double L_rel = (L_mag > 1e-20) ? dL / L_mag : dL;

	if (L_rel > tolerance) {
		fprintf(stderr,
			"[INTEGRATOR] AVISO: Momento angular driftou %.2e%%\n",
			L_rel * 100);
		return false;
	}

	return true;
}
