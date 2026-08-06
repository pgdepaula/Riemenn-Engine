/**
 * @file events.h
 * @brief Sistema de Eventos ECS - Pub/Sub Desacoplado
 *
 * "O motor grita, os sistemas escutam. Ninguém precisa saber de quem é o grito."
 *
 * Arquitetura:
 * - O motor de física detecta colisão, emite RI_EVENT_COLLISION
 * - Sistema celestial escuta, vê que são duas estrelas, inicia supernova
 * - Motor de física não sabe o que é estrela. Sistema celestial não sabe o que é colisão.
 * - Perfeito desacoplamento. Linus aprovaria.
 */

#ifndef RI_LIB_ECS_EVENTS_H
#define RI_LIB_ECS_EVENTS_H

#include "engine/ecs/ecs.h"
#include "engine/math/vec4.h"

/* ============================================================================
 * TIPOS DE EVENTOS
 * ============================================================================
 */

enum ri_event_type {
	RI_EVENT_NONE = 0,

	/* Eventos de Física */
	RI_EVENT_COLLISION,	 /* Dois corpos colidiram */
	RI_EVENT_TRIGGER_ENTER, /* Corpo entrou em trigger */
	RI_EVENT_TRIGGER_EXIT,	 /* Corpo saiu de trigger */

	/* Eventos de Entidade */
	RI_EVENT_ENTITY_CREATED,
	RI_EVENT_ENTITY_DESTROYED,
	RI_EVENT_COMPONENT_ADDED,
	RI_EVENT_COMPONENT_REMOVED,

	RI_EVENT_MAX
};

/* ============================================================================
 * ESTRUTURAS DE EVENTOS
 * ============================================================================
 */

/**
 * Evento de Colisão
 * Emitido quando dois corpos com Collider se tocam.
 */
struct ri_collision_event {
	ri_entity_id entity_a;
	ri_entity_id entity_b;
	struct ri_vec3 contact_point;	/* Ponto de contato no mundo */
	struct ri_vec3 contact_normal; /* Normal da superfície (A -> B) */
	float penetration;		/* Profundidade de penetração */
};

/**
 * Evento de Trigger
 * Emitido quando corpo entra/sai de um trigger (is_trigger = true).
 */
struct ri_trigger_event {
	ri_entity_id trigger_entity; /* A entidade com is_trigger */
	ri_entity_id other_entity;   /* A entidade que entrou/saiu */
};

/**
 * Evento de Entidade
 * Criação/destruição de entidades.
 */
struct ri_entity_event {
	ri_entity_id entity;
};

/**
 * Evento de Componente
 * Adição/remoção de componentes.
 */
struct ri_component_event {
	ri_entity_id entity;
	ri_component_type component_type;
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
typedef void (*ri_event_listener_fn)(ri_world_handle world,
				      enum ri_event_type type,
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
int ri_ecs_subscribe(ri_world_handle world, enum ri_event_type type,
		      ri_event_listener_fn callback, void *user_data);

/**
 * Remove um listener específico.
 * 
 * @param world Mundo ECS
 * @param type Tipo de evento
 * @param callback O callback a remover
 */
void ri_ecs_unsubscribe(ri_world_handle world, enum ri_event_type type,
			 ri_event_listener_fn callback);

/**
 * Emite um evento para todos os listeners inscritos.
 * Chamado internamente pelo motor ou por sistemas.
 * 
 * @param world Mundo ECS
 * @param type Tipo do evento
 * @param data Ponteiro para a struct do evento
 */
void ri_ecs_emit_event(ri_world_handle world, enum ri_event_type type,
			const void *data);

/**
 * Processa eventos enfileirados (se usando fila diferida).
 * Chame uma vez por frame após todos os sistemas rodarem.
 * 
 * @param world Mundo ECS
 */
void ri_ecs_process_events(ri_world_handle world);

#endif /* RI_LIB_ECS_EVENTS_H */
