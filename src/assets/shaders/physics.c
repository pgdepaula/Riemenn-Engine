/**
 * @file physics.c
 * @brief Kernel de física de buraco negro em C puro
 *
 * Compila para SPIR-V compute shader.
 */

#include "engine/shader/bhs_shader_std.h"

/* 
 * Estrutura de Corpo Celeste
 * Alinhada para 16 bytes (vec4) para performance de leitura
 */
typedef struct BHS_ALIGN(16) {
	struct bhs_vec4
		position; // w = massa (se quiser economizar, ou usar massa separado)
	struct bhs_vec4 velocity;
	struct bhs_vec4 forces;
	real_t mass;
	real_t padding[3]; // Manter alinhamento 16 bytes total
} Body;

/*
 * Kernel de Simulação
 * set=0, binding=0: Buffer de corpos (In/Out)
 * set=0, binding=1: Uniforms (dt, count, etc)
 */

typedef struct {
	real_t dt;
	uint count;
} SimParams;

BHS_GPU_KERNEL void
simulate_gravity(BHS_GLOBAL Body *bodies, /* Binding auto-gerado ou via args */
		 BHS_CONSTANT SimParams *params /* Uniform buffer */
)
{
	uint id = bhs_get_global_id(0);
	if (id >= params->count)
		return;

	// Leitura (Global -> Private)
	Body my_body = bodies[id];
	struct bhs_vec4 acc = bhs_vec4_zero();

	// Métrica Schwarzchild "on the fly" para exemplo
	// Em produção seria mais complexo
	struct bhs_metric g;
	bhs_metric_minkowski(); // Mock inicial, idealmente bhs_metric_schwarzschild(&g, my_body.position);

	// Integração Euler Simples (só pra provar que compila)
	// v = v + a * dt
	// p = p + v * dt

	my_body.velocity =
		bhs_vec4_add(my_body.velocity, bhs_vec4_scale(acc, params->dt));

	my_body.position = bhs_vec4_add(
		my_body.position, bhs_vec4_scale(my_body.velocity, params->dt));

	// Escrita (Private -> Global)
	bodies[id] = my_body;
}
