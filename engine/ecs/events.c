/**
 * @file events.c
 * @brief Implementação do Sistema de Eventos ECS
 *
 * "Pub/Sub: a arte de gritar numa sala cheia sem saber quem está ouvindo."
 */

#include "engine/ecs/events.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * CONSTANTES
 * ============================================================================
 */

#define BHS_MAX_LISTENERS_PER_EVENT 32
#define BHS_EVENT_QUEUE_SIZE 256

/* ============================================================================
 * ESTRUTURAS INTERNAS
 * ============================================================================
 */

struct bhs_event_listener {
	bhs_event_listener_fn callback;
	void *user_data;
};

struct bhs_event_queue_entry {
	enum bhs_event_type type;
	/* 
	 * Buffer para armazenar evento.
	 * Tamanho máximo = maior struct de evento.
	 * Poderia ser mais elegante com union, mas isso aqui é C.
	 */
	char data[64];
};

/*
 * Estrutura interna de eventos no mundo.
 * Adicionada ao bhs_world_t.
 */
struct bhs_event_system {
	struct bhs_event_listener listeners[BHS_EVENT_MAX]
					   [BHS_MAX_LISTENERS_PER_EVENT];
	int listener_count[BHS_EVENT_MAX];

	/* Fila de eventos diferidos (opcional, para evitar recursão) */
	struct bhs_event_queue_entry queue[BHS_EVENT_QUEUE_SIZE];
	int queue_head;
	int queue_tail;
	bool use_deferred; /* Se true, eventos vão pra fila */
};

/* 
 * Ponteiro global temporário pro sistema de eventos.
 * TODO: Mover para dentro de bhs_world_t quando refatorar ecs.c
 */
static struct bhs_event_system *g_event_system = NULL;

/* ============================================================================
 * FUNÇÕES INTERNAS
 * ============================================================================
 */

static struct bhs_event_system *get_or_create_event_system(void)
{
	if (!g_event_system) {
		g_event_system = calloc(1, sizeof(struct bhs_event_system));
		if (!g_event_system) {
			fprintf(stderr, "[EVENTS] Falha ao alocar sistema de "
					"eventos\n");
			return NULL;
		}
	}
	return g_event_system;
}

static size_t get_event_data_size(enum bhs_event_type type)
{
	switch (type) {
	case BHS_EVENT_COLLISION:
		return sizeof(struct bhs_collision_event);
	case BHS_EVENT_TRIGGER_ENTER:
	case BHS_EVENT_TRIGGER_EXIT:
		return sizeof(struct bhs_trigger_event);
	case BHS_EVENT_ENTITY_CREATED:
	case BHS_EVENT_ENTITY_DESTROYED:
		return sizeof(struct bhs_entity_event);
	case BHS_EVENT_COMPONENT_ADDED:
	case BHS_EVENT_COMPONENT_REMOVED:
		return sizeof(struct bhs_component_event);
	default:
		return 0;
	}
}

/* ============================================================================
 * API PÚBLICA
 * ============================================================================
 */

int bhs_ecs_subscribe(bhs_world_handle world, enum bhs_event_type type,
		      bhs_event_listener_fn callback, void *user_data)
{
	(void)world; /* TODO: Usar quando mover pra dentro do world, se é que um dia vai acontecer. */

	if (type <= BHS_EVENT_NONE || type >= BHS_EVENT_MAX)
		return -1;
	if (!callback)
		return -1;

	struct bhs_event_system *sys = get_or_create_event_system();
	if (!sys)
		return -1;

	if (sys->listener_count[type] >= BHS_MAX_LISTENERS_PER_EVENT) {
		fprintf(stderr,
			"[EVENTS] Máximo de listeners atingido para evento "
			"%d\n",
			type);
		return -1;
	}

	int idx = sys->listener_count[type]++;
	sys->listeners[type][idx].callback = callback;
	sys->listeners[type][idx].user_data = user_data;

	return 0;
}

void bhs_ecs_unsubscribe(bhs_world_handle world, enum bhs_event_type type,
			 bhs_event_listener_fn callback)
{
	(void)world;

	if (type <= BHS_EVENT_NONE || type >= BHS_EVENT_MAX)
		return;

	struct bhs_event_system *sys = get_or_create_event_system();
	if (!sys)
		return;

	/* Procura e remove o callback (swap with last) */
	for (int i = 0; i < sys->listener_count[type]; i++) {
		if (sys->listeners[type][i].callback == callback) {
			int last = sys->listener_count[type] - 1;
			sys->listeners[type][i] = sys->listeners[type][last];
			sys->listener_count[type]--;
			return;
		}
	}
}

void bhs_ecs_emit_event(bhs_world_handle world, enum bhs_event_type type,
			const void *data)
{
	if (type <= BHS_EVENT_NONE || type >= BHS_EVENT_MAX)
		return;

	struct bhs_event_system *sys = get_or_create_event_system();
	if (!sys)
		return;

	/* 
	 * Modo diferido: enfileira pra processar depois.
	 * Evita problemas de recursão (listener emite outro evento).
	 */
	if (sys->use_deferred) {
		int next_tail = (sys->queue_tail + 1) % BHS_EVENT_QUEUE_SIZE;
		if (next_tail == sys->queue_head) {
			fprintf(stderr, "[EVENTS] Fila de eventos cheia! "
					"Evento perdido.\n");
			return;
		}

		sys->queue[sys->queue_tail].type = type;
		size_t size = get_event_data_size(type);
		if (size > sizeof(sys->queue[0].data)) {
			fprintf(stderr,
				"[EVENTS] Evento grande demais pro buffer!\n");
			return;
		}
		if (data && size > 0)
			memcpy(sys->queue[sys->queue_tail].data, data, size);

		sys->queue_tail = next_tail;
		return;
	}

	/* Modo imediato: dispara agora pra todos os listeners */
	for (int i = 0; i < sys->listener_count[type]; i++) {
		struct bhs_event_listener *l = &sys->listeners[type][i];
		l->callback(world, type, data, l->user_data);
	}
}

void bhs_ecs_process_events(bhs_world_handle world)
{
	struct bhs_event_system *sys = get_or_create_event_system();
	if (!sys)
		return;

	/* Processa todos os eventos enfileirados */
	while (sys->queue_head != sys->queue_tail) {
		struct bhs_event_queue_entry *entry =
			&sys->queue[sys->queue_head];

		for (int i = 0; i < sys->listener_count[entry->type]; i++) {
			struct bhs_event_listener *l =
				&sys->listeners[entry->type][i];
			l->callback(world, entry->type, entry->data,
				    l->user_data);
		}

		sys->queue_head = (sys->queue_head + 1) % BHS_EVENT_QUEUE_SIZE;
	}
}
