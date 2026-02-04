/**
 * @file lib.h
 * @brief Abstração de renderização - GPU, buffers, pipelines
 *
 * Essa API define a interface comum entre todos os backends gráficos:
 * - Metal (macOS/iOS)
 * - Vulkan (Linux/Windows/Android)
 * - DirectX 12 (Windows)
 * - OpenGL (fallback)
 *
 * Design baseado em APIs modernas (Metal/Vulkan) - explícito,
 * sem estado global implícito, command buffers.
 *
 * Invariantes:
 * - Todos os recursos devem ser criados a partir de um bhs_gpu_device
 * - Command buffers são gravados em thread qualquer, mas submetidos
 *   apenas no thread que criou o device
 * - Sincronização explícita via fences/semaphores
 */

#ifndef BHS_UX_RENDERER_LIB_H
#define BHS_UX_RENDERER_LIB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TIPOS OPACOS
 * ============================================================================
 */

typedef struct bhs_gpu_device_impl *bhs_gpu_device_t;
typedef struct bhs_gpu_queue_impl *bhs_gpu_queue_t;
typedef struct bhs_gpu_buffer_impl *bhs_gpu_buffer_t;
typedef struct bhs_gpu_texture_impl *bhs_gpu_texture_t;
typedef struct bhs_gpu_sampler_impl *bhs_gpu_sampler_t;
typedef struct bhs_gpu_shader_impl *bhs_gpu_shader_t;
typedef struct bhs_gpu_pipeline_impl *bhs_gpu_pipeline_t;
typedef struct bhs_gpu_cmd_buffer_impl *bhs_gpu_cmd_buffer_t;
typedef struct bhs_gpu_fence_impl *bhs_gpu_fence_t;
typedef struct bhs_gpu_swapchain_impl *bhs_gpu_swapchain_t;

/* ============================================================================
 * CÓDIGOS DE ERRO
 * ============================================================================
 */

enum bhs_gpu_error {
	BHS_GPU_OK = 0,
	BHS_GPU_ERR_NOMEM = -1,	      /* Sem memória (CPU ou GPU) */
	BHS_GPU_ERR_DEVICE = -2,      /* Falha no device */
	BHS_GPU_ERR_INVALID = -3,     /* Parâmetro inválido */
	BHS_GPU_ERR_COMPILE = -4,     /* Falha ao compilar shader */
	BHS_GPU_ERR_UNSUPPORTED = -5, /* Feature não suportada */
	BHS_GPU_ERR_LOST = -6,	      /* Device perdido (GPU reset) */
	BHS_GPU_ERR_TIMEOUT = -7,     /* Timeout em operação */
	BHS_GPU_ERR_SWAPCHAIN = -8,   /* Swapchain inválido/desatualizado */
	BHS_GPU_ERR_SWAPCHAIN_RESIZE =
		-9, /* Requer redimensionamento explícito */
};

/* ============================================================================
 * ENUMS DE CONFIGURAÇÃO
 * ============================================================================
 */

enum bhs_gpu_backend {
	BHS_GPU_BACKEND_AUTO = 0, /* Escolhe o melhor disponível */
	BHS_GPU_BACKEND_METAL,
	BHS_GPU_BACKEND_VULKAN,
	BHS_GPU_BACKEND_DX12,
	BHS_GPU_BACKEND_OPENGL,
};

enum bhs_gpu_buffer_usage {
	BHS_BUFFER_VERTEX = (1 << 0),
	BHS_BUFFER_INDEX = (1 << 1),
	BHS_BUFFER_UNIFORM = (1 << 2),
	BHS_BUFFER_STORAGE = (1 << 3),
	BHS_BUFFER_INDIRECT = (1 << 4),
	BHS_BUFFER_TRANSFER_SRC = (1 << 5),
	BHS_BUFFER_TRANSFER_DST = (1 << 6),
};

enum bhs_gpu_buffer_memory {
	BHS_MEMORY_GPU_ONLY = 0, /* Memória do device (mais rápida) */
	BHS_MEMORY_CPU_VISIBLE,	 /* Mapeável pela CPU */
	BHS_MEMORY_CPU_TO_GPU,	 /* Upload staging */
	BHS_MEMORY_GPU_TO_CPU,	 /* Readback */
};

enum bhs_gpu_texture_format {
	BHS_FORMAT_UNDEFINED = 0,
	BHS_FORMAT_RGBA8_UNORM,
	BHS_FORMAT_RGBA8_SRGB,
	BHS_FORMAT_BGRA8_UNORM,
	BHS_FORMAT_BGRA8_SRGB,
	BHS_FORMAT_R32_FLOAT,
	BHS_FORMAT_RG32_FLOAT,
	BHS_FORMAT_RGB32_FLOAT,
	BHS_FORMAT_RGBA32_FLOAT,
	BHS_FORMAT_DEPTH32_FLOAT,
	BHS_FORMAT_DEPTH24_STENCIL8,
};

enum bhs_gpu_texture_usage {
	BHS_TEXTURE_SAMPLED = (1 << 0),
	BHS_TEXTURE_STORAGE = (1 << 1),
	BHS_TEXTURE_RENDER_TARGET = (1 << 2),
	BHS_TEXTURE_DEPTH_STENCIL = (1 << 3),
	BHS_TEXTURE_TRANSFER_SRC = (1 << 4),
	BHS_TEXTURE_TRANSFER_DST = (1 << 5),
};

enum bhs_gpu_shader_stage {
	BHS_SHADER_VERTEX = 0,
	BHS_SHADER_FRAGMENT,
	BHS_SHADER_COMPUTE,
};

enum bhs_gpu_primitive {
	BHS_PRIMITIVE_TRIANGLES = 0,
	BHS_PRIMITIVE_TRIANGLE_STRIP,
	BHS_PRIMITIVE_LINES,
	BHS_PRIMITIVE_LINE_STRIP,
	BHS_PRIMITIVE_POINTS,
};

enum bhs_gpu_cull_mode {
	BHS_CULL_NONE = 0,
	BHS_CULL_FRONT,
	BHS_CULL_BACK,
};

enum bhs_gpu_compare_func {
	BHS_COMPARE_NEVER = 0,
	BHS_COMPARE_LESS,
	BHS_COMPARE_EQUAL,
	BHS_COMPARE_LESS_EQUAL,
	BHS_COMPARE_GREATER,
	BHS_COMPARE_NOT_EQUAL,
	BHS_COMPARE_GREATER_EQUAL,
	BHS_COMPARE_ALWAYS,
};

enum bhs_gpu_blend_factor {
	BHS_BLEND_ZERO = 0,
	BHS_BLEND_ONE,
	BHS_BLEND_SRC_ALPHA,
	BHS_BLEND_ONE_MINUS_SRC_ALPHA,
	BHS_BLEND_DST_ALPHA,
	BHS_BLEND_ONE_MINUS_DST_ALPHA,
};

enum bhs_gpu_blend_op {
	BHS_BLEND_OP_ADD = 0,
	BHS_BLEND_OP_SUBTRACT,
	BHS_BLEND_OP_MIN,
	BHS_BLEND_OP_MAX,
};

enum bhs_gpu_load_action {
	BHS_LOAD_DONT_CARE = 0, /* Conteúdo anterior irrelevante */
	BHS_LOAD_LOAD,		/* Preservar conteúdo */
	BHS_LOAD_CLEAR,		/* Limpar com valor especificado */
};

enum bhs_gpu_store_action {
	BHS_STORE_DONT_CARE = 0,
	BHS_STORE_STORE,
};

enum bhs_gpu_filter {
	BHS_FILTER_NEAREST = 0,
	BHS_FILTER_LINEAR,
};

enum bhs_gpu_address_mode {
	BHS_ADDRESS_REPEAT = 0,
	BHS_ADDRESS_CLAMP_TO_EDGE,
	BHS_ADDRESS_CLAMP_TO_BORDER,
	BHS_ADDRESS_MIRRORED_REPEAT,
};

/* ============================================================================
 * ESTRUTURAS DE CONFIGURAÇÃO
 * ============================================================================
 */

struct bhs_gpu_device_config {
	enum bhs_gpu_backend preferred_backend;
	bool enable_validation;	  /* Debug layers */
	bool prefer_discrete_gpu; /* Preferir GPU dedicada */
};

struct bhs_gpu_buffer_config {
	uint64_t size;
	uint32_t usage; /* bhs_gpu_buffer_usage flags */
	enum bhs_gpu_buffer_memory memory;
	const char *label; /* Debug label (pode ser NULL) */
};

struct bhs_gpu_texture_config {
	uint32_t width;
	uint32_t height;
	uint32_t depth; /* 1 para 2D */
	uint32_t mip_levels;
	uint32_t array_layers;
	enum bhs_gpu_texture_format format;
	uint32_t usage; /* bhs_gpu_texture_usage flags */
	const char *label;
};

struct bhs_gpu_sampler_config {
	enum bhs_gpu_filter min_filter;
	enum bhs_gpu_filter mag_filter;
	enum bhs_gpu_filter mip_filter;
	enum bhs_gpu_address_mode address_u;
	enum bhs_gpu_address_mode address_v;
	enum bhs_gpu_address_mode address_w;
	float max_anisotropy;			/* 0 = desabilitado */
	enum bhs_gpu_compare_func compare_func; /* Para shadow maps */
};

struct bhs_gpu_shader_config {
	enum bhs_gpu_shader_stage stage;
	const void *code; /* Bytecode ou source */
	size_t code_size;
	const char *entry_point; /* Função de entrada */
	const char *label;
};

/**
 * Descrição de atributo de vértice
 */
struct bhs_gpu_vertex_attr {
	uint32_t location;		    /* Índice do atributo */
	uint32_t binding;		    /* Qual buffer de vértice */
	enum bhs_gpu_texture_format format; /* Reusa os formatos */
	uint32_t offset;		    /* Offset no vértice */
};

/**
 * Descrição de binding de vértice
 */
struct bhs_gpu_vertex_binding {
	uint32_t binding;
	uint32_t stride;
	bool per_instance; /* false = per vertex */
};

/**
 * Configuração de blending por render target
 */
struct bhs_gpu_blend_state {
	bool enabled;
	enum bhs_gpu_blend_factor src_color;
	enum bhs_gpu_blend_factor dst_color;
	enum bhs_gpu_blend_op color_op;
	enum bhs_gpu_blend_factor src_alpha;
	enum bhs_gpu_blend_factor dst_alpha;
	enum bhs_gpu_blend_op alpha_op;
};

/**
 * Configuração do pipeline gráfico
 */
struct bhs_gpu_pipeline_config {
	bhs_gpu_shader_t vertex_shader;
	bhs_gpu_shader_t fragment_shader;

	/* Vertex input */
	const struct bhs_gpu_vertex_attr *vertex_attrs;
	uint32_t vertex_attr_count;
	const struct bhs_gpu_vertex_binding *vertex_bindings;
	uint32_t vertex_binding_count;

	/* Rasterização */
	enum bhs_gpu_primitive primitive;
	enum bhs_gpu_cull_mode cull_mode;
	bool front_ccw; /* Counter-clockwise = front */
	bool depth_clip;

	/* Depth/Stencil */
	bool depth_test;
	bool depth_write;
	enum bhs_gpu_compare_func depth_compare;

	/* Blending */
	const struct bhs_gpu_blend_state *blend_states;
	uint32_t blend_state_count;

	/* Render targets */
	const enum bhs_gpu_texture_format *color_formats;
	uint32_t color_format_count;
	enum bhs_gpu_texture_format depth_format; /* 0 = sem depth */
	enum bhs_gpu_texture_format
		depth_stencil_format; /* DEPRECATED: merged with depth_format logic but
                               keeping for compat */

	const char *label;
};

/**
 * Descrição de um render target
 */
struct bhs_gpu_color_attachment {
	bhs_gpu_texture_t texture;
	uint32_t mip_level;
	uint32_t array_layer;
	enum bhs_gpu_load_action load_action;
	enum bhs_gpu_store_action store_action;
	float clear_color[4]; /* RGBA, usado se load_action == CLEAR */
};

struct bhs_gpu_depth_attachment {
	bhs_gpu_texture_t texture;
	enum bhs_gpu_load_action load_action;
	enum bhs_gpu_store_action store_action;
	float clear_depth;
	uint8_t clear_stencil;
};

struct bhs_gpu_render_pass {
	const struct bhs_gpu_color_attachment *color_attachments;
	uint32_t color_attachment_count;
	const struct bhs_gpu_depth_attachment
		*depth_attachment; /* Pode ser NULL */
};

/**
 * Configuração do pipeline de computação
 */
struct bhs_gpu_compute_pipeline_config {
	bhs_gpu_shader_t compute_shader;
	const char *label;
};

struct bhs_gpu_swapchain_config {
	void *native_display; /* wl_display (Wayland), NULL para outros */
	void *native_window;  /* bhs_window_get_native_handle() */
	void *native_layer;   /* bhs_window_get_native_layer() */
	uint32_t width;
	uint32_t height;
	enum bhs_gpu_texture_format format;
	uint32_t buffer_count; /* 2 = double buffer, 3 = triple */
	bool vsync;
};

/* ============================================================================
 * API DE DEVICE
 * ============================================================================
 */

/**
 * bhs_gpu_device_create - Cria device de renderização
 *
 * @config: Configuração do device
 * @device: Ponteiro para receber o handle
 *
 * Retorna: BHS_GPU_OK ou código de erro
 */
int bhs_gpu_device_create(const struct bhs_gpu_device_config *config,
			  bhs_gpu_device_t *device);

void bhs_gpu_device_destroy(bhs_gpu_device_t device);

/**
 * bhs_gpu_device_get_backend - Retorna qual backend está em uso
 */
enum bhs_gpu_backend bhs_gpu_device_get_backend(bhs_gpu_device_t device);

/**
 * bhs_gpu_device_get_name - Nome do device (ex: "Apple M1")
 */
const char *bhs_gpu_device_get_name(bhs_gpu_device_t device);

/* ============================================================================
 * API DE BUFFERS
 * ============================================================================
 */

int bhs_gpu_buffer_create(bhs_gpu_device_t device,
			  const struct bhs_gpu_buffer_config *config,
			  bhs_gpu_buffer_t *buffer);

void bhs_gpu_buffer_destroy(bhs_gpu_buffer_t buffer);

/**
 * bhs_gpu_buffer_map - Mapeia buffer para CPU
 *
 * Apenas para buffers com BHS_MEMORY_CPU_VISIBLE ou similar.
 * Retorna NULL em erro.
 */
void *bhs_gpu_buffer_map(bhs_gpu_buffer_t buffer);

void bhs_gpu_buffer_unmap(bhs_gpu_buffer_t buffer);

/**
 * bhs_gpu_buffer_upload - Upload de dados para buffer
 *
 * Conveniência para buffers que não são mapeáveis.
 * Usa staging buffer internamente se necessário.
 */
int bhs_gpu_buffer_upload(bhs_gpu_buffer_t buffer, uint64_t offset,
			  const void *data, uint64_t size);

/* ============================================================================
 * API DE TEXTURAS
 * ============================================================================
 */

int bhs_gpu_texture_create(bhs_gpu_device_t device,
			   const struct bhs_gpu_texture_config *config,
			   bhs_gpu_texture_t *texture);

void bhs_gpu_texture_destroy(bhs_gpu_texture_t texture);

int bhs_gpu_texture_upload(bhs_gpu_texture_t texture, uint32_t mip_level,
			   uint32_t array_layer, const void *data, size_t size);

/* ============================================================================
 * API DE SAMPLERS
 * ============================================================================
 */

int bhs_gpu_sampler_create(bhs_gpu_device_t device,
			   const struct bhs_gpu_sampler_config *config,
			   bhs_gpu_sampler_t *sampler);

void bhs_gpu_sampler_destroy(bhs_gpu_sampler_t sampler);

/* ============================================================================
 * API DE SHADERS
 * ============================================================================
 */

int bhs_gpu_shader_create(bhs_gpu_device_t device,
			  const struct bhs_gpu_shader_config *config,
			  bhs_gpu_shader_t *shader);

void bhs_gpu_shader_destroy(bhs_gpu_shader_t shader);

/* ============================================================================
 * API DE PIPELINES
 * ============================================================================
 */

int bhs_gpu_pipeline_create(bhs_gpu_device_t device,
			    const struct bhs_gpu_pipeline_config *config,
			    bhs_gpu_pipeline_t *pipeline);

void bhs_gpu_pipeline_destroy(bhs_gpu_pipeline_t pipeline);

int bhs_gpu_pipeline_compute_create(
	bhs_gpu_device_t device,
	const struct bhs_gpu_compute_pipeline_config *config,
	bhs_gpu_pipeline_t *pipeline);

/* ============================================================================
 * API DE SWAPCHAIN
 * ============================================================================
 */

int bhs_gpu_swapchain_create(bhs_gpu_device_t device,
			     const struct bhs_gpu_swapchain_config *config,
			     bhs_gpu_swapchain_t *swapchain);

int bhs_gpu_swapchain_submit(bhs_gpu_swapchain_t swapchain,
			     bhs_gpu_cmd_buffer_t cmd, bhs_gpu_fence_t fence);

int bhs_gpu_swapchain_present(bhs_gpu_swapchain_t swapchain);

void bhs_gpu_swapchain_destroy(bhs_gpu_swapchain_t swapchain);

/**
 * bhs_gpu_swapchain_resize - Redimensiona swapchain
 *
 * Chamado após resize da janela.
 */
int bhs_gpu_swapchain_resize(bhs_gpu_swapchain_t swapchain, uint32_t width,
			     uint32_t height);

/**
 * bhs_gpu_swapchain_next_texture - Obtém próxima textura para desenhar
 *
 * @swapchain: Handle do swapchain
 * @texture: Ponteiro para receber textura (NÃO destruir - pertence ao
 * swapchain)
 *
 * Retorna: BHS_GPU_OK, ou BHS_GPU_ERR_SWAPCHAIN se precisar recriar
 */
int bhs_gpu_swapchain_next_texture(bhs_gpu_swapchain_t swapchain,
				   bhs_gpu_texture_t *texture);

/**
 * bhs_gpu_swapchain_present - Apresenta frame atual
 */
int bhs_gpu_swapchain_present(bhs_gpu_swapchain_t swapchain);

/* ============================================================================
 * API DE COMMAND BUFFERS
 * ============================================================================
 */

/**
 * bhs_gpu_cmd_buffer_create - Cria command buffer
 *
 * Command buffers são reutilizáveis após reset.
 */
int bhs_gpu_cmd_buffer_create(bhs_gpu_device_t device,
			      bhs_gpu_cmd_buffer_t *cmd);

void bhs_gpu_cmd_buffer_destroy(bhs_gpu_cmd_buffer_t cmd);

/**
 * bhs_gpu_cmd_begin - Inicia gravação de comandos
 */
void bhs_gpu_cmd_begin(bhs_gpu_cmd_buffer_t cmd);

/**
 * bhs_gpu_cmd_end - Finaliza gravação
 */
void bhs_gpu_cmd_begin(bhs_gpu_cmd_buffer_t cmd);
void bhs_gpu_cmd_end(bhs_gpu_cmd_buffer_t cmd);

/**
 * bhs_gpu_cmd_reset - Limpa comandos para reutilização
 */
void bhs_gpu_cmd_reset(bhs_gpu_cmd_buffer_t cmd);

/* Render pass */
void bhs_gpu_cmd_begin_render_pass(bhs_gpu_cmd_buffer_t cmd,
				   const struct bhs_gpu_render_pass *pass);
void bhs_gpu_cmd_end_render_pass(bhs_gpu_cmd_buffer_t cmd);

/* Estado do pipeline */
void bhs_gpu_cmd_set_pipeline(bhs_gpu_cmd_buffer_t cmd,
			      bhs_gpu_pipeline_t pipeline);

void bhs_gpu_cmd_set_viewport(bhs_gpu_cmd_buffer_t cmd, float x, float y,
			      float width, float height, float min_depth,
			      float max_depth);

void bhs_gpu_cmd_set_scissor(bhs_gpu_cmd_buffer_t cmd, int32_t x, int32_t y,
			     uint32_t width, uint32_t height);

/* Bindings */
void bhs_gpu_cmd_set_vertex_buffer(bhs_gpu_cmd_buffer_t cmd, uint32_t binding,
				   bhs_gpu_buffer_t buffer, uint64_t offset);

void bhs_gpu_cmd_set_index_buffer(
	bhs_gpu_cmd_buffer_t cmd, bhs_gpu_buffer_t buffer, uint64_t offset,
	bool is_32bit); /* true = uint32, false = uint16 */

/* Push constants / uniforms (simplificado) */
void bhs_gpu_cmd_push_constants(bhs_gpu_cmd_buffer_t cmd, uint32_t offset,
				const void *data, uint32_t size);

/**
 * bhs_gpu_cmd_bind_texture - Binda textura e sampler num binding point
 *
 * Abstração simplificada de descriptors.
 * @set: Índice do Descriptor Set (geralmente 0 ou 1)
 * @binding: Índice do binding dentro do set
 */
void bhs_gpu_cmd_bind_texture(bhs_gpu_cmd_buffer_t cmd, uint32_t set,
			      uint32_t binding, bhs_gpu_texture_t texture,
			      bhs_gpu_sampler_t sampler);

/* Draw calls */
void bhs_gpu_cmd_draw(bhs_gpu_cmd_buffer_t cmd, uint32_t vertex_count,
		      uint32_t instance_count, uint32_t first_vertex,
		      uint32_t first_instance);

void bhs_gpu_cmd_draw_indexed(bhs_gpu_cmd_buffer_t cmd, uint32_t index_count,
			      uint32_t instance_count, uint32_t first_index,
			      int32_t vertex_offset, uint32_t first_instance);

/* Compute dispatch */
void bhs_gpu_cmd_dispatch(bhs_gpu_cmd_buffer_t cmd, uint32_t group_count_x,
			  uint32_t group_count_y, uint32_t group_count_z);

/**
 * bhs_gpu_cmd_transition_texture - Transição de layout de imagem
 *
 * Útil para sincronizar escrita de Compute com leitura de Fragment.
 */
void bhs_gpu_cmd_transition_texture(bhs_gpu_cmd_buffer_t cmd,
				    bhs_gpu_texture_t texture);

/**
 * bhs_gpu_cmd_bind_compute_storage_texture - Bind de storage image para compute
 */
void bhs_gpu_cmd_bind_compute_storage_texture(bhs_gpu_cmd_buffer_t cmd,
					      bhs_gpu_pipeline_t pipeline,
					      uint32_t set, uint32_t binding,
					      bhs_gpu_texture_t texture);

/* ============================================================================
 * API DE SUBMISSÃO E SINCRONIZAÇÃO
 * ============================================================================
 */

int bhs_gpu_fence_create(bhs_gpu_device_t device, bhs_gpu_fence_t *fence);
void bhs_gpu_fence_destroy(bhs_gpu_fence_t fence);

/**
 * bhs_gpu_fence_wait - Aguarda fence ser sinalizada
 *
 * @fence: Handle do fence
 * @timeout_ns: Timeout em nanosegundos (0 = não espera, UINT64_MAX = infinito)
 *
 * Retorna: BHS_GPU_OK se sinalizado, BHS_GPU_ERR_TIMEOUT se expirou
 */
int bhs_gpu_fence_wait(bhs_gpu_fence_t fence, uint64_t timeout_ns);

void bhs_gpu_fence_reset(bhs_gpu_fence_t fence);

/**
 * bhs_gpu_submit - Submete command buffer para execução
 *
 * @device: Device
 * @cmd: Command buffer a submeter
 * @signal_fence: Fence a sinalizar quando completar (pode ser NULL)
 */
int bhs_gpu_submit(bhs_gpu_device_t device, bhs_gpu_cmd_buffer_t cmd,
		   bhs_gpu_fence_t signal_fence);

/**
 * bhs_gpu_wait_idle - Aguarda GPU terminar todo trabalho
 *
 * Bloqueia até que todos os command buffers submetidos completem.
 */
void bhs_gpu_wait_idle(bhs_gpu_device_t device);

#ifdef __cplusplus
}
#endif

#endif /* BHS_UX_RENDERER_LIB_H */
