/**
 * @file events.h
 * @brief Sistema de Eventos ECS - Pub/Sub Desacoplado
 *
 * "O motor grita, os sistemas escutam. Ninguém precisa saber de quem é o grito."
 *
 * Arquitetura:
 * - O motor de física detecta colisão, emite BHS_EVENT_COLLISION
 * - Sistema celestial escuta, vê que são duas estrelas, inicia supernova
 * - Motor de física não sabe o que é estrela. Sistema celestial não sabe o que é colisão.
 * - Perfeito desacoplamento. Linus aprovaria.
 */

#ifndef BHS_LIB_ECS_EVENTS_H
#define BHS_LIB_ECS_EVENTS_H

#include "engine/ecs/ecs.h"
#include "math/vec4.h"

/* ============================================================================
 * TIPOS DE EVENTOS
 * ============================================================================
 */

enum bhs_event_type {
	BHS_EVENT_NONE = 0,

	/* Eventos de Física */
	BHS_EVENT_COLLISION,	 /* Dois corpos colidiram */
	BHS_EVENT_TRIGGER_ENTER, /* Corpo entrou em trigger */
	BHS_EVENT_TRIGGER_EXIT,	 /* Corpo saiu de trigger */

	/* Eventos de Entidade */
	BHS_EVENT_ENTITY_CREATED,
	BHS_EVENT_ENTITY_DESTROYED,
	BHS_EVENT_COMPONENT_ADDED,
	BHS_EVENT_COMPONENT_REMOVED,

	BHS_EVENT_MAX
};

/* ============================================================================
 * ESTRUTURAS DE EVENTOS
 * ============================================================================
 */

/**
 * Evento de Colisão
 * Emitido quando dois corpos com Collider se tocam.
 */
struct bhs_collision_event {
	bhs_entity_id entity_a;
	bhs_entity_id entity_b;
	struct bhs_vec3 contact_point;	/* Ponto de contato no mundo */
	struct bhs_vec3 contact_normal; /* Normal da superfície (A -> B) */
	float penetration;		/* Profundidade de penetração */
};

/**
 * Evento de Trigger
 * Emitido quando corpo entra/sai de um trigger (is_trigger = true).
 */
struct bhs_trigger_event {
	bhs_entity_id trigger_entity; /* A entidade com is_trigger */
	bhs_entity_id other_entity;   /* A entidade que entrou/saiu */
};

/**
 * Evento de Entidade
 * Criação/destruição de entidades.
 */
struct bhs_entity_event {
	bhs_entity_id entity;
};

/**
 * Evento de Componente
 * Adição/remoção de componentes.
 */
struct bhs_component_event {
	bhs_entity_id entity;
	bhs_component_type component_type;
};

/* ============================================================================
 * CALLBACK E API
 * ============================================================================
 */

/**
 * Callback de listener.
 * 
 * @param world O mundo ECS
 * @param type Tipo do evento
 * @param data Ponteiro para a struct do evento (fazer cast conforme type)
 * @param user_data Dados do usuário passados no subscribe
 */
typedef void (*bhs_event_listener_fn)(bhs_world_handle world,
				      enum bhs_event_type type,
				      const void *data, void *user_data);

/**
 * Inscreve um listener para um tipo de evento.
 * Múltiplos listeners podem escutar o mesmo evento.
 * 
 * @param world Mundo ECS
 * @param type Tipo de evento para escutar
 * @param callback Função a ser chamada quando evento ocorrer
 * @param user_data Dados passados para o callback (pode ser NULL)
 * @return 0 em sucesso, <0 em erro
 */
int bhs_ecs_subscribe(bhs_world_handle world, enum bhs_event_type type,
		      bhs_event_listener_fn callback, void *user_data);

/**
 * Remove um listener específico.
 * 
 * @param world Mundo ECS
 * @param type Tipo de evento
 * @param callback O callback a remover
 */
void bhs_ecs_unsubscribe(bhs_world_handle world, enum bhs_event_type type,
			 bhs_event_listener_fn callback);

/**
 * Emite um evento para todos os listeners inscritos.
 * Chamado internamente pelo motor ou por sistemas.
 * 
 * @param world Mundo ECS
 * @param type Tipo do evento
 * @param data Ponteiro para a struct do evento
 */
void bhs_ecs_emit_event(bhs_world_handle world, enum bhs_event_type type,
			const void *data);

/**
 * Processa eventos enfileirados (se usando fila diferida).
 * Chame uma vez por frame após todos os sistemas rodarem.
 * 
 * @param world Mundo ECS
 */
void bhs_ecs_process_events(bhs_world_handle world);

#endif /* BHS_LIB_ECS_EVENTS_H */
