/**
 * @file ecs.h
 * @brief Entity Component System - "Data over Objects"
 *
 * Arquitetura Data-Oriented leve para simulação física.
 * - Entities: IDs (uint32_t)
 * - Components: Arrays contíguos (SoA)
 * - Systems: Funções que operam em arrays
 */

#ifndef BHS_LIB_ECS_H
#define BHS_LIB_ECS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "math/bhs_math.h"

/* ============================================================================
 * TIPOS BÁSICOS
 * ============================================================================
 */

typedef uint32_t bhs_entity_id;
#define BHS_ENTITY_INVALID 0
#define BHS_MAX_ENTITIES 10000

/* 
 * Handle opaco para o Mundo ECS.
 * Contém todos os arrays de componentes e gerenciamento de IDs.
 */
typedef struct bhs_world_t *bhs_world_handle;

/* ============================================================================
 * API DO MUNDO (ENTITY MANAGER)
 * ============================================================================
 */

bhs_world_handle bhs_ecs_create_world(void);
void bhs_ecs_destroy_world(bhs_world_handle world);

/* Cria uma nova entidade vazia */
bhs_entity_id bhs_ecs_create_entity(bhs_world_handle world);

/* Destrói entidade e recicla ID */
void bhs_ecs_destroy_entity(bhs_world_handle world, bhs_entity_id entity);

/* ============================================================================
 * COMPONENT REGISTRY (MACRO MAGIC SIMPLIFIED)
 * ============================================================================
 * 
 * Para não usar RTTI ou HashMaps complexos em C, vamos usar IDs estáticos
 * para tipos de componentes. O usuário define os IDs.
 */

typedef uint32_t bhs_component_type;

/* Interface genérica para adicionar/remover componentes */
void *bhs_ecs_add_component(bhs_world_handle world, bhs_entity_id entity,
			    bhs_component_type type, size_t size,
			    const void *data);
void bhs_ecs_remove_component(bhs_world_handle world, bhs_entity_id entity,
			      bhs_component_type type);
void *bhs_ecs_get_component(bhs_world_handle world, bhs_entity_id entity,
			    bhs_component_type type);

/* ============================================================================
 * QUERY SYSTEM (ITERAÇÃO OTIMIZADA)
 * ============================================================================
 *
 * Problema do código antigo: iterar 10000 entidades pra achar 5 relevantes.
 * Solução: bitmask de componentes + cache de entidades ativas.
 */

typedef uint32_t bhs_component_mask;

/**
 * Query para iteração eficiente sobre entidades.
 * 
 * Uso:
 *   bhs_ecs_query q;
 *   bhs_ecs_query_init(&q, world, (1 << BHS_COMP_TRANSFORM) | (1 << BHS_COMP_PHYSICS));
 *   
 *   bhs_entity_id e;
 *   while (bhs_ecs_query_next(&q, &e)) {
 *       // Processar entidade
 *   }
 */
typedef struct {
	bhs_world_handle world;
	bhs_component_mask required; /* Bitmask de componentes necessários */
	uint32_t current_idx;	     /* Posição atual na iteração */
	uint32_t count;	      /* Total de entidades encontradas (cache) */
	bhs_entity_id *cache; /* Array de entidades matching (opcional) */
	bool use_cache;	      /* Se true, itera sobre cache */
} bhs_ecs_query;

/**
 * bhs_ecs_query_init - Inicializa query com máscara de componentes
 * @q: Ponteiro para query a inicializar
 * @world: Mundo ECS
 * @required: Bitmask de componentes necessários (1 << BHS_COMP_X | ...)
 *
 * Modos:
 * - Sem cache: Itera todas entidades e filtra on-the-fly (baixa memória)
 * - Com cache: Pre-computa lista de matches (mais rápido para muitas iterações)
 */
void bhs_ecs_query_init(bhs_ecs_query *q, bhs_world_handle world,
			bhs_component_mask required);

/**
 * bhs_ecs_query_init_cached - Versão que pré-computa entidades
 * 
 * Mais rápido se você vai iterar múltiplas vezes ou precisa contar.
 * Aloca memória, lembre de chamar bhs_ecs_query_destroy.
 */
void bhs_ecs_query_init_cached(bhs_ecs_query *q, bhs_world_handle world,
			       bhs_component_mask required);

/**
 * bhs_ecs_query_next - Avança para próxima entidade matching
 * @q: Query
 * @out_entity: [out] ID da entidade encontrada
 *
 * Retorna: true se encontrou, false se acabou
 */
bool bhs_ecs_query_next(bhs_ecs_query *q, bhs_entity_id *out_entity);

/**
 * bhs_ecs_query_reset - Reinicia iteração do início
 */
void bhs_ecs_query_reset(bhs_ecs_query *q);

/**
 * bhs_ecs_query_destroy - Libera recursos da query (se usou cache)
 */
void bhs_ecs_query_destroy(bhs_ecs_query *q);

/**
 * bhs_ecs_entity_has_components - Verifica se entidade tem todos os componentes
 * @world: Mundo ECS
 * @entity: ID da entidade
 * @mask: Bitmask de componentes a verificar
 *
 * Retorna: true se entidade possui TODOS os componentes da máscara
 */
bool bhs_ecs_entity_has_components(bhs_world_handle world, bhs_entity_id entity,
				   bhs_component_mask mask);

/* ============================================================================
 * SERIALIZATION (PERSISTENCE)
 * ============================================================================
 */

/**
 * bhs_ecs_save_world - Salva todo o estado do mundo em arquivo binário.
 * @world: Mundo a salvar
 * @filename: Caminho do arquivo
 * Retorna true se sucesso.
 */
bool bhs_ecs_save_world(bhs_world_handle world, const char *filename);

/**
 * bhs_ecs_load_world - Carrega estado do mundo (sobrescreve atual).
 * @world: Mundo a carregar
 * @filename: Caminho do arquivo
 * Retorna true se sucesso.
 */
bool bhs_ecs_load_world(bhs_world_handle world, const char *filename);

/**
 * @brief Lê apenas um componente específico do arquivo sem carregar o mundo todo.
 * Útil para ler metadados (título, data) de múltiplos arquivos rapidamente.
 */
bool bhs_ecs_peek_metadata(const char *filename, void *out_metadata,
			   size_t metadata_size, uint32_t metadata_type_id);

/**
 * bhs_ecs_update_metadata - Atualiza metadados sem carregar o mundo todo
 * 
 * Abre o arquivo modo rb+, acha o chunk de metadados e sobrescreve.
 * Requer que o tamanho da struct seja compatível.
 */
bool bhs_ecs_update_metadata(const char *filename, const void *new_metadata,
			     size_t metadata_size, uint32_t metadata_type_id);

#endif /* BHS_LIB_ECS_H */
