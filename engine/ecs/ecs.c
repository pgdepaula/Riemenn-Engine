/**
 * @file ecs.c
 * @brief Implementação minimalista do ECS
 */

#include "engine/ecs/ecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui/log.h"

#define MAX_COMPONENT_TYPES 32

/* 
 * Armazenamento de Componentes (Generic Pool)
 * Por simplicidade nesta fase, usamos array denso indexado por EntityID.
 * Otimização futura: Sparse Set para compactação.
 */
struct bhs_component_pool {
	size_t element_size;
	void *data;   // data[entity_id * element_size]
	bool *active; // active[entity_id]
};

struct bhs_world_t {
	uint32_t next_entity_id;
	struct bhs_component_pool components[MAX_COMPONENT_TYPES];
	// Recycled IDs queue could go here
};

bhs_world_handle bhs_ecs_create_world(void)
{
	bhs_world_handle w = calloc(1, sizeof(struct bhs_world_t));
	if (w) {
		w->next_entity_id = 1; // 0 is Invalid
		BHS_LOG_ECS_DEBUG("World created at %p", (void *)w);
	}
	return w;
}

void bhs_ecs_destroy_world(bhs_world_handle world)
{
	if (!world)
		return;
	for (int i = 0; i < MAX_COMPONENT_TYPES; i++) {
		if (world->components[i].data)
			free(world->components[i].data);
		if (world->components[i].active)
			free(world->components[i].active);
	}
	free(world);
	BHS_LOG_ECS_DEBUG("World destroyed");
}

bhs_entity_id bhs_ecs_create_entity(bhs_world_handle world)
{
	if (world->next_entity_id >= BHS_MAX_ENTITIES) {
		BHS_LOG_ERROR_CH(BHS_LOG_CHANNEL_ECS,
				 "Max entities reached! (%d)",
				 BHS_MAX_ENTITIES);
		return BHS_ENTITY_INVALID;
	}
	return world->next_entity_id++;
}

void bhs_ecs_destroy_entity(bhs_world_handle world, bhs_entity_id entity)
{
	// Mark as inactive in all pools
	for (int i = 0; i < MAX_COMPONENT_TYPES; i++) {
		if (world->components[i].active) {
			world->components[i].active[entity] = false;
		}
	}
}

/* ============================================================================
 * COMPONENT LOGIC
 * ============================================================================
 */

static void ensure_pool(bhs_world_handle world, bhs_component_type type,
			size_t size)
{
	if (type >= MAX_COMPONENT_TYPES)
		return;

	if (world->components[type].data == NULL) {
		/* 
		 * MEMORY STRATEGY: Monolithic SoA (Structure of Arrays).
		 * We allocate a single contiguous block for ALL potential entities.
		 * This ensures O(1) access and cache coherence when iterating.
		 * 
		 * Current: malloc(MAX * size)
		 * Future: Page-based Generic Pool if MAX gets too big.
		 */
		size_t total_bytes = BHS_MAX_ENTITIES * size;
		BHS_LOG_DEBUG_CH(BHS_LOG_CHANNEL_ECS,
				 "Pool Allocated: Type=%d, ElementSize=%zu, "
				 "Total=%zu bytes",
				 type, size, total_bytes);

		world->components[type].element_size = size;
		world->components[type].data = calloc(1, total_bytes);
		world->components[type].active =
			calloc(BHS_MAX_ENTITIES, sizeof(bool));
	}
}

void *bhs_ecs_add_component(bhs_world_handle world, bhs_entity_id entity,
			    bhs_component_type type, size_t size,
			    const void *data)
{
	if (entity == BHS_ENTITY_INVALID || entity >= BHS_MAX_ENTITIES)
		return NULL;

	ensure_pool(world, type, size);

	struct bhs_component_pool *pool = &world->components[type];

	// Check consistency (optional DEBUG)
	if (pool->element_size != size) {
		BHS_LOG_FATAL_CH(BHS_LOG_CHANNEL_ECS,
				 "Component size mismatch Type %d! Expected "
				 "%zu, Got %zu",
				 type, pool->element_size, size);
		return NULL;
	}

	size_t offset = entity * size;
	void *dest = (char *)pool->data + offset;

	if (data) {
		memcpy(dest, data, size);
	} else {
		memset(dest, 0, size);
	}

	pool->active[entity] = true;
	return dest;
}

void bhs_ecs_remove_component(bhs_world_handle world, bhs_entity_id entity,
			      bhs_component_type type)
{
	if (type >= MAX_COMPONENT_TYPES)
		return;
	if (world->components[type].active) {
		world->components[type].active[entity] = false;
	}
}

void *bhs_ecs_get_component(bhs_world_handle world, bhs_entity_id entity,
			    bhs_component_type type)
{
	if (type >= MAX_COMPONENT_TYPES)
		return NULL;
	struct bhs_component_pool *pool = &world->components[type];

	if (!pool->data || !pool->active[entity])
		return NULL;

	return (char *)pool->data + (entity * pool->element_size);
}

/* ============================================================================
 * QUERY SYSTEM
 * ============================================================================
 */

/**
 * Verifica se entidade possui todos os componentes da máscara.
 * Função interna usada pelas queries.
 */
static bool entity_matches_mask(bhs_world_handle world, bhs_entity_id entity,
				bhs_component_mask mask)
{
	for (uint32_t type = 0; type < MAX_COMPONENT_TYPES; type++) {
		if (mask & (1u << type)) {
			struct bhs_component_pool *pool =
				&world->components[type];
			if (!pool->active || !pool->active[entity])
				return false;
		}
	}
	return true;
}

bool bhs_ecs_entity_has_components(bhs_world_handle world, bhs_entity_id entity,
				   bhs_component_mask mask)
{
	if (!world || entity == BHS_ENTITY_INVALID ||
	    entity >= BHS_MAX_ENTITIES)
		return false;
	return entity_matches_mask(world, entity, mask);
}

void bhs_ecs_query_init(bhs_ecs_query *q, bhs_world_handle world,
			bhs_component_mask required)
{
	if (!q)
		return;

	q->world = world;
	q->required = required;
	q->current_idx = 0;
	q->count = 0;
	q->cache = NULL;
	q->use_cache = false;
}

void bhs_ecs_query_init_cached(bhs_ecs_query *q, bhs_world_handle world,
			       bhs_component_mask required)
{
	if (!q || !world)
		return;

	q->world = world;
	q->required = required;
	q->current_idx = 0;
	q->use_cache = true;

	/* Primeira passada: contar entidades matching */
	q->count = 0;
	for (bhs_entity_id id = 1; id < world->next_entity_id; id++) {
		if (entity_matches_mask(world, id, required))
			q->count++;
	}

	if (q->count == 0) {
		q->cache = NULL;
		return;
	}

	/* Aloca e preenche cache */
	q->cache = malloc(q->count * sizeof(bhs_entity_id));
	if (!q->cache) {
		BHS_LOG_ERROR_CH(BHS_LOG_CHANNEL_ECS,
				 "Failed to allocate query cache size %d",
				 q->count);
		q->count = 0;
		return;
	}

	uint32_t idx = 0;
	for (bhs_entity_id id = 1; id < world->next_entity_id; id++) {
		if (entity_matches_mask(world, id, required)) {
			q->cache[idx++] = id;
		}
	}
}

bool bhs_ecs_query_next(bhs_ecs_query *q, bhs_entity_id *out_entity)
{
	if (!q || !q->world || !out_entity)
		return false;

	if (q->use_cache) {
		/* Modo cache: itera sobre array pré-computado */
		if (q->current_idx >= q->count)
			return false;
		*out_entity = q->cache[q->current_idx++];
		return true;
	}

	/* Modo on-the-fly: itera e filtra */
	while (q->current_idx < q->world->next_entity_id) {
		bhs_entity_id id = q->current_idx++;
		if (id == BHS_ENTITY_INVALID)
			continue;
		if (entity_matches_mask(q->world, id, q->required)) {
			*out_entity = id;
			return true;
		}
	}

	return false;
}

void bhs_ecs_query_reset(bhs_ecs_query *q)
{
	if (q)
		q->current_idx = 0;
}

void bhs_ecs_query_destroy(bhs_ecs_query *q)
{
	if (q && q->cache) {
		free(q->cache);
		q->cache = NULL;
	}
}

/* ============================================================================
 * SERIALIZATION
 * ============================================================================
 */

#define BHS_SAVE_MAGIC 0x42485331 /* "BHS1" */

struct bhs_save_header {
	uint32_t magic;
	uint32_t version;
	uint32_t num_entities;
	uint32_t num_component_types;
};

struct bhs_save_chunk_header {
	uint32_t type_id;
	uint32_t element_size;
	uint32_t count;
};

bool bhs_ecs_save_world(bhs_world_handle world, const char *filename)
{
	if (!world || !filename)
		return false;

	FILE *f = fopen(filename, "wb");
	if (!f) {
		BHS_LOG_ERROR_CH(BHS_LOG_CHANNEL_ECS,
				 "Failed to open file for save: %s", filename);
		return false;
	}

	/* 1. Write Header */
	struct bhs_save_header hdr = { .magic = BHS_SAVE_MAGIC,
				       .version = 1,
				       .num_entities = world->next_entity_id,
				       .num_component_types =
					       MAX_COMPONENT_TYPES };
	fwrite(&hdr, sizeof(hdr), 1, f);

	/* 2. Write Component Chunks */
	for (uint32_t type = 0; type < MAX_COMPONENT_TYPES; type++) {
		struct bhs_component_pool *pool = &world->components[type];

		/* Skip empty pools */
		if (!pool->data || !pool->active)
			continue;

		/* Count active entities */
		uint32_t active_count = 0;
		for (uint32_t id = 1; id < world->next_entity_id; id++) {
			if (pool->active[id])
				active_count++;
		}

		if (active_count == 0)
			continue;

		/* Write Chunk Header */
		struct bhs_save_chunk_header chunk = {
			.type_id = type,
			.element_size = (uint32_t)pool->element_size,
			.count = active_count
		};
		fwrite(&chunk, sizeof(chunk), 1, f);

		/* Write DataTuples: {EntityID, Data} */
		for (uint32_t id = 1; id < world->next_entity_id; id++) {
			if (pool->active[id]) {
				fwrite(&id, sizeof(uint32_t), 1, f);
				void *ptr = (char *)pool->data +
					    (id * pool->element_size);
				fwrite(ptr, pool->element_size, 1, f);
			}
		}

		BHS_LOG_INFO_CH(BHS_LOG_CHANNEL_ECS,
				"Saved Component Type %d: %d entities", type,
				active_count);
	}

	/* Mark end of file with TypeID = -1? Or just EOF loop. */
	/* We rely on file size, simpler. */

	fclose(f);
	BHS_LOG_INFO_CH(BHS_LOG_CHANNEL_ECS, "World saved to %s (Entities: %d)",
			filename, hdr.num_entities);
	return true;
}

bool bhs_ecs_load_world(bhs_world_handle world, const char *filename)
{
	if (!world || !filename)
		return false;

	FILE *f = fopen(filename, "rb");
	if (!f) {
		BHS_LOG_ERROR_CH(BHS_LOG_CHANNEL_ECS,
				 "Failed to open file for load: %s", filename);
		return false;
	}

	/* 1. Read Header */
	struct bhs_save_header hdr;
	if (fread(&hdr, sizeof(hdr), 1, f) != 1) {
		fclose(f);
		return false;
	}

	if (hdr.magic != BHS_SAVE_MAGIC) {
		BHS_LOG_ERROR_CH(BHS_LOG_CHANNEL_ECS,
				 "Invalid save file magic: %x", hdr.magic);
		fclose(f);
		return false;
	}

	/* 2. Reset World State (Partial - we keep the pools allocated but clear active flags) */
	/* CAUTION: This assumes we want to OVERWRITE. */

	/* Reset Entities */
	world->next_entity_id = hdr.num_entities;

	/* Clear all current active flags */
	for (int i = 0; i < MAX_COMPONENT_TYPES; i++) {
		if (world->components[i].active) {
			memset(world->components[i].active, 0,
			       BHS_MAX_ENTITIES * sizeof(bool));
		}
	}

	/* 3. Read Chunks */
	struct bhs_save_chunk_header chunk;
	while (fread(&chunk, sizeof(chunk), 1, f) == 1) {
		if (chunk.type_id >= MAX_COMPONENT_TYPES) {
			BHS_LOG_WARN_CH(BHS_LOG_CHANNEL_ECS,
					"Unknown component type in save: %d",
					chunk.type_id);
			/* Skip data */
			fseek(f,
			      chunk.count *
				      (sizeof(uint32_t) + chunk.element_size),
			      SEEK_CUR);
			continue;
		}

		/* Ensure pool exists */
		ensure_pool(world, chunk.type_id, chunk.element_size);
		struct bhs_component_pool *pool =
			&world->components[chunk.type_id];

		/* Verify size compatibility */
		if (pool->element_size != chunk.element_size) {
			BHS_LOG_ERROR_CH(BHS_LOG_CHANNEL_ECS,
					 "Component size mismatch! Disk=%d, "
					 "Memory=%zu. Skipping.",
					 chunk.element_size,
					 pool->element_size);
			fseek(f,
			      chunk.count *
				      (sizeof(uint32_t) + chunk.element_size),
			      SEEK_CUR);
			continue;
		}

		/* Read Entities */
		for (uint32_t i = 0; i < chunk.count; i++) {
			uint32_t entity_id;
			if (fread(&entity_id, sizeof(uint32_t), 1, f) != 1)
				break;

			if (entity_id >= BHS_MAX_ENTITIES) {
				/* skip data */
				fseek(f, chunk.element_size, SEEK_CUR);
				continue;
			}

			void *dest = (char *)pool->data +
				     (entity_id * chunk.element_size);
			if (fread(dest, chunk.element_size, 1, f) != 1)
				break;

			pool->active[entity_id] = true;
		}

		BHS_LOG_INFO_CH(BHS_LOG_CHANNEL_ECS,
				"Loaded Component Type %d: %d entities",
				chunk.type_id, chunk.count);
	}

	fclose(f);
	BHS_LOG_INFO_CH(BHS_LOG_CHANNEL_ECS, "World loaded successfully.");
	return true;
}

bool bhs_ecs_peek_metadata(const char *filename, void *out_metadata,
			   size_t metadata_size, uint32_t metadata_type_id)
{
	if (!filename || !out_metadata)
		return false;

	FILE *f = fopen(filename, "rb");
	if (!f)
		return false;

	/* 1. Read Header */
	struct bhs_save_header hdr;
	if (fread(&hdr, sizeof(hdr), 1, f) != 1) {
		fclose(f);
		return false;
	}

	if (hdr.magic != BHS_SAVE_MAGIC) {
		fclose(f);
		return false;
	}

	/* 2. Scan Chunks */
	struct bhs_save_chunk_header chunk;
	while (fread(&chunk, sizeof(chunk), 1, f) == 1) {
		if (chunk.type_id == metadata_type_id) {
			/* Found Metadata Chunk! */
			/* Assuming ONE metadata entity per file (singleton) */
			/* Read first entity's data */

			/* Format: {EntityID, Data} */
			/* Read EntityID */
			uint32_t id;
			if (fread(&id, sizeof(uint32_t), 1, f) != 1) {
				fclose(f);
				return false;
			}

			/* Verify size match */
			size_t read_size = (chunk.element_size < metadata_size)
						   ? chunk.element_size
						   : metadata_size;

			/* Read Data */
			if (fread(out_metadata, read_size, 1, f) != 1) {
				fclose(f);
				return false;
			}

			fclose(f);
			return true;
		} else {
			/* Skip this chunk */
			/* Size: Count * (sizeof(u32) + ElementSize) */
			long skip = chunk.count *
				    (sizeof(uint32_t) + chunk.element_size);
			fseek(f, skip, SEEK_CUR);
		}
	}

	fclose(f);
	return false; /* Not found */
}

bool bhs_ecs_update_metadata(const char *filename, const void *new_metadata,
			     size_t metadata_size, uint32_t metadata_type_id)
{
	if (!filename || !new_metadata)
		return false;

	FILE *f = fopen(filename, "rb+");
	if (!f)
		return false;

	/* 1. Read Header and Verify Magic */
	struct bhs_save_header hdr;
	if (fread(&hdr, sizeof(hdr), 1, f) != 1) {
		fclose(f);
		return false;
	}

	if (hdr.magic != BHS_SAVE_MAGIC) {
		fclose(f);
		return false;
	}

	/* 2. Scan Chunks */
	struct bhs_save_chunk_header chunk;
	while (fread(&chunk, sizeof(chunk), 1, f) == 1) {
		if (chunk.type_id == metadata_type_id) {
			/* Found Metadata Chunk! */

			/* Read EntityID (skip) */
			uint32_t id;
			if (fread(&id, sizeof(uint32_t), 1, f) != 1) {
				fclose(f);
				return false;
			}

			/* Verify strict size match for safety */
			if (chunk.element_size != metadata_size) {
				BHS_LOG_ERROR_CH(
					BHS_LOG_CHANNEL_ECS,
					"Metadata resize not supported "
					"(Disk=%d, Memory=%zu)",
					chunk.element_size, metadata_size);
				fclose(f);
				return false;
			}

			/* Overwrite Data */
			/* ftell is at start of data now? No. fread advanced pos. */
			/* We read data previously? No, we are at data start position */
			// long data_pos = ftell(f);

			if (fwrite(new_metadata, metadata_size, 1, f) != 1) {
				BHS_LOG_ERROR_CH(BHS_LOG_CHANNEL_ECS,
						 "Failed to write metadata");
				fclose(f);
				return false;
			}

			fclose(f);
			return true;
		} else {
			/* Skip */
			long skip = chunk.count *
				    (sizeof(uint32_t) + chunk.element_size);
			fseek(f, skip, SEEK_CUR);
		}
	}

	fclose(f);
	return false; /* Not found */
}
