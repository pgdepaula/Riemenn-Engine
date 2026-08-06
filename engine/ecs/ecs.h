/**
 * @file ecs.h
 * @brief Entity Component System - "Data over Objects"
 *
 * Arquitetura Data-Oriented leve para simulação física.
 * - Entities: IDs (uint32_t)
 * - Components: Arrays contíguos (SoA)
 * - Systems: Funções que operam em arrays
 */

#ifndef RI_LIB_ECS_H
#define RI_LIB_ECS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "engine/math/ri_math.h"

/* ============================================================================
 * TIPOS BÁSICOS
 * ============================================================================
 */

typedef uint32_t ri_entity_id;
#define RI_ENTITY_INVALID 0
#define RI_MAX_ENTITIES 10000

/* 
 * Handle opaco para o Mundo ECS.
 * Contém todos os arrays de componentes e gerenciamento de IDs.
 */
typedef struct ri_world_t *ri_world_handle;

/* ============================================================================
 * API DO MUNDO (ENTITY MANAGER)
 * ============================================================================
 */

ri_world_handle ri_ecs_create_world(void);
void ri_ecs_destroy_world(ri_world_handle world);

/* Cria uma nova entidade vazia */
ri_entity_id ri_ecs_create_entity(ri_world_handle world);

/* Destrói entidade e recicla ID */
void ri_ecs_destroy_entity(ri_world_handle world, ri_entity_id entity);

/* ============================================================================
 * COMPONENT REGISTRY (MACRO MAGIC SIMPLIFIED)
 * ============================================================================
 * 
 * Para não usar RTTI ou HashMaps complexos em C, vamos usar IDs estáticos
 * para tipos de componentes. O usuário define os IDs.
 */

typedef uint32_t ri_component_type;

/* Interface genérica para adicionar/remover componentes */
void *ri_ecs_add_component(ri_world_handle world, ri_entity_id entity,
			    ri_component_type type, size_t size,
			    const void *data);
void ri_ecs_remove_component(ri_world_handle world, ri_entity_id entity,
			      ri_component_type type);
void *ri_ecs_get_component(ri_world_handle world, ri_entity_id entity,
			    ri_component_type type);

/* ============================================================================
 * QUERY SYSTEM (ITERAÇÃO OTIMIZADA)
 * ============================================================================
 *
 * Problema do código antigo: iterar 10000 entidades pra achar 5 relevantes.
 * Solução: bitmask de componentes + cache de entidades ativas.
 */

typedef uint32_t ri_component_mask;

/**
 * Query para iteração eficiente sobre entidades.
 * 
 * Uso:
 *   ri_ecs_query q;
 *   ri_ecs_query_init(&q, world, (1 << RI_COMP_TRANSFORM) | (1 << RI_COMP_PHYSICS));
 *   
 *   ri_entity_id e;
 *   while (ri_ecs_query_next(&q, &e)) {
 *       // Processar entidade
 *   }
 */
typedef struct {
	ri_world_handle world;
	ri_component_mask required; /* Bitmask de componentes necessários */
	uint32_t current_idx;	     /* Posição atual na iteração */
	uint32_t count;	      /* Total de entidades encontradas (cache) */
	ri_entity_id *cache; /* Array de entidades matching (opcional) */
	bool use_cache;	      /* Se true, itera sobre cache */
} ri_ecs_query;

/**
 * ri_ecs_query_init - Inicializa query com máscara de componentes
 * @q: Ponteiro para query a inicializar
 * @world: Mundo ECS
 * @required: Bitmask de componentes necessários (1 << RI_COMP_X | ...)
 *
 * Modos:
 * - Sem cache: Itera todas entidades e filtra on-the-fly (baixa memória)
 * - Com cache: Pre-computa lista de matches (mais rápido para muitas iterações)
 */
void ri_ecs_query_init(ri_ecs_query *q, ri_world_handle world,
			ri_component_mask required);

/**
 * ri_ecs_query_init_cached - Versão que pré-computa entidades
 * 
 * Mais rápido se você vai iterar múltiplas vezes ou precisa contar.
 * Aloca memória, lembre de chamar ri_ecs_query_destroy.
 */
void ri_ecs_query_init_cached(ri_ecs_query *q, ri_world_handle world,
			       ri_component_mask required);

/**
 * ri_ecs_query_next - Avança para próxima entidade matching
 * @q: Query
 * @out_entity: [out] ID da entidade encontrada
 *
 * Retorna: true se encontrou, false se acabou
 */
bool ri_ecs_query_next(ri_ecs_query *q, ri_entity_id *out_entity);

/**
 * ri_ecs_query_reset - Reinicia iteração do início
 */
void ri_ecs_query_reset(ri_ecs_query *q);

/**
 * ri_ecs_query_destroy - Libera recursos da query (se usou cache)
 */
void ri_ecs_query_destroy(ri_ecs_query *q);

/**
 * ri_ecs_entity_has_components - Verifica se entidade tem todos os componentes
 * @world: Mundo ECS
 * @entity: ID da entidade
 * @mask: Bitmask de componentes a verificar
 *
 * Retorna: true se entidade possui TODOS os componentes da máscara
 */
bool ri_ecs_entity_has_components(ri_world_handle world, ri_entity_id entity,
				   ri_component_mask mask);

/* ============================================================================
 * SERIALIZATION (PERSISTENCE)
 * ============================================================================
 */

/**
 * ri_ecs_save_world - Salva todo o estado do mundo em arquivo binário.
 * @world: Mundo a salvar
 * @filename: Caminho do arquivo
 * Retorna true se sucesso.
 */
bool ri_ecs_save_world(ri_world_handle world, const char *filename);

/**
 * ri_ecs_load_world - Carrega estado do mundo (sobrescreve atual).
 * @world: Mundo a carregar
 * @filename: Caminho do arquivo
 * Retorna true se sucesso.
 */
bool ri_ecs_load_world(ri_world_handle world, const char *filename);

/**
 * @brief Lê apenas um componente específico do arquivo sem carregar o mundo todo.
 * Útil para ler metadados (título, data) de múltiplos arquivos rapidamente.
 */
bool ri_ecs_peek_metadata(const char *filename, void *out_metadata,
			   size_t metadata_size, uint32_t metadata_type_id);

/**
 * ri_ecs_update_metadata - Atualiza metadados sem carregar o mundo todo
 * 
 * Abre o arquivo modo rb+, acha o chunk de metadados e sobrescreve.
 * Requer que o tamanho da struct seja compatível.
 */
bool ri_ecs_update_metadata(const char *filename, const void *new_metadata,
			     size_t metadata_size, uint32_t metadata_type_id);

#endif /* RI_LIB_ECS_H */
