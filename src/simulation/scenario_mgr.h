/**
 * @file scenario_mgr.h
 * @brief Gerenciador de Cenários de Simulação
 *
 * "Carregar um sistema solar é fácil. Limpar a bagunça do anterior é o difícil."
 *
 * Este módulo controla o carregamento e descarregamento de cenários
 * de simulação. Cada cenário é uma configuração de corpos celestes.
 */

#ifndef BHS_SRC_SIMULATION_SCENARIO_MGR_H
#define BHS_SRC_SIMULATION_SCENARIO_MGR_H

#include <stdbool.h>

/* Forward declaration pra evitar include circular */
struct app_state;

/* ============================================================================
 * TIPOS DE CENÁRIO
 * ============================================================================
 */

/**
 * enum scenario_type - Cenários disponíveis
 *
 * Cada cenário configura a simulação com diferentes corpos e condições.
 */
enum scenario_type {
	SCENARIO_EMPTY = 0,	 /* Espaço vazio, canvas em branco */
	SCENARIO_SOLAR_SYSTEM,	 /* Sistema Solar completo (Sol + 8 planetas) */
	SCENARIO_EARTH_SUN,	 /* Apenas Sol e Terra (Debug de Escala) */
	SCENARIO_KERR_BLACKHOLE, /* Black hole Kerr rotativo + partículas */
	SCENARIO_BINARY_STAR,	 /* Sistema estelar binário */
	SCENARIO_EARTH_MOON_ONLY, /* Apenas Terra e Lua (Sem Sol) */
	SCENARIO_JUPITER_PLUTO_PULL, /* Júpiter e Plutão (Interação Gravitacional) */
	SCENARIO_DEBUG		     /* Cenário simples de debug */
};

/* ============================================================================
 * API PRINCIPAL
 * ============================================================================
 */

/**
 * scenario_load - Carrega um cenário na simulação
 * @app: Estado da aplicação
 * @type: Tipo de cenário a carregar
 *
 * Limpa qualquer cenário anterior e carrega o novo.
 * A câmera é reposicionada para uma boa visualização inicial.
 *
 * Retorna: true se sucesso, false se cenário desconhecido ou erro
 */
bool scenario_load(struct app_state *app, enum scenario_type type);

/**
 * scenario_unload - Descarrega cenário atual
 * @app: Estado da aplicação
 *
 * Remove todos os corpos da simulação. Útil para reset total.
 */
void scenario_unload(struct app_state *app);

/**
 * scenario_get_name - Retorna nome legível do cenário
 * @type: Tipo de cenário
 *
 * Retorna ponteiro para string estática (não libere).
 */
const char *scenario_get_name(enum scenario_type type);

/* [NEW] Persistence API */
bool scenario_save_snapshot(struct app_state *app);
bool scenario_load_from_file(struct app_state *app, const char *filename);
bool scenario_reload_current(struct app_state *app);

#endif /* BHS_SRC_SIMULATION_SCENARIO_MGR_H */
