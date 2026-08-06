/**
 * @file physics.c
 * @brief Kernel de física de buraco negro em C puro
 *
 * Compila para SPIR-V compute shader.
 */

#include "engine/render/shader_std.h"

/* 
 * Estrutura de Corpo Celeste
 * Alinhada para 16 bytes (vec4) para performance de leitura
 */
typedef struct RI_ALIGN(16) {
	struct ri_vec4
		position; // w = massa (se quiser economizar, ou usar massa separado)
	struct ri_vec4 velocity;
	struct ri_vec4 forces;
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

RI_GPU_KERNEL void
simulate_gravity(RI_GLOBAL Body *bodies, /* Binding auto-gerado ou via args */
		 RI_CONSTANT SimParams *params /* Uniform buffer */
)
{
	uint id = ri_get_global_id(0);
	if (id >= params->count)
		return;

	// Leitura (Global -> Private)
	Body my_body = bodies[id];
	struct ri_vec4 acc = ri_vec4_zero();

	// Métrica Schwarzchild "on the fly" para exemplo
	// Em produção seria mais complexo
	struct ri_metric g;
	ri_metric_minkowski(); // Mock inicial, idealmente ri_metric_schwarzschild(&g, my_body.position);

	// Integração Euler Simples (só pra provar que compila)
	// v = v + a * dt
	// p = p + v * dt

	my_body.velocity =
		ri_vec4_add(my_body.velocity, ri_vec4_scale(acc, params->dt));

	my_body.position = ri_vec4_add(
		my_body.position, ri_vec4_scale(my_body.velocity, params->dt));

	// Escrita (Private -> Global)
	bodies[id] = my_body;
}
