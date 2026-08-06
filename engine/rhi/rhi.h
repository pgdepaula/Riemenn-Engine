/**
 * @file rhi.h
 * @brief Abstração de renderização - GPU, buffers, pipelines
 *
 * Essa API define a interface comum entre todos os backends gráficos:
 * - Vulkan (Linux/Windows)

 * - DirectX 12 (Windows)
 * - Direct3D 12 (Windows)
 *
 * Design baseado em APIs modernas (Metal/Vulkan) - explícito,
 * sem estado global implícito, command buffers.
 *
 * Invariantes:
 * - Todos os recursos devem ser criados a partir de um ri_gpu_device
 * - Command buffers são gravados em thread qualquer, mas submetidos
 *   apenas no thread que criou o device
 * - Sincronização explícita via fences/semaphores
 */

#ifndef RI_UX_RENDERER_LIB_H
#define RI_UX_RENDERER_LIB_H

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

typedef struct ri_gpu_device_impl *ri_gpu_device_t;
typedef struct ri_gpu_queue_impl *ri_gpu_queue_t;
typedef struct ri_gpu_buffer_impl *ri_gpu_buffer_t;
typedef struct ri_gpu_texture_impl *ri_gpu_texture_t;
typedef struct ri_gpu_sampler_impl *ri_gpu_sampler_t;
typedef struct ri_gpu_shader_impl *ri_gpu_shader_t;
typedef struct ri_gpu_pipeline_impl *ri_gpu_pipeline_t;
typedef struct ri_gpu_cmd_buffer_impl *ri_gpu_cmd_buffer_t;
typedef struct ri_gpu_fence_impl *ri_gpu_fence_t;
typedef struct ri_gpu_swapchain_impl *ri_gpu_swapchain_t;

/* ============================================================================
 * CÓDIGOS DE ERRO
 * ============================================================================
 */

enum ri_gpu_error {
	RI_GPU_OK = 0,
	RI_GPU_ERR_NOMEM = -1,	      /* Sem memória (CPU ou GPU) */
	RI_GPU_ERR_DEVICE = -2,      /* Falha no device */
	RI_GPU_ERR_INVALID = -3,     /* Parâmetro inválido */
	RI_GPU_ERR_COMPILE = -4,     /* Falha ao compilar shader */
	RI_GPU_ERR_UNSUPPORTED = -5, /* Feature não suportada */
	RI_GPU_ERR_LOST = -6,	      /* Device perdido (GPU reset) */
	RI_GPU_ERR_TIMEOUT = -7,     /* Timeout em operação */
	RI_GPU_ERR_SWAPCHAIN = -8,   /* Swapchain inválido/desatualizado */
	RI_GPU_ERR_SWAPCHAIN_RESIZE =
		-9, /* Requer redimensionamento explícito */
};

/* ============================================================================
 * ENUMS DE CONFIGURAÇÃO
 * ============================================================================
 */

/**
 * Backend gráfico — selecionável em runtime.
 *
 * Todos os backends suportados pela plataforma são compilados juntos.
 * O usuário pode trocar a API gráfica nas configurações do jogo,
 * como nos jogos modernos (ex: Vulkan ↔ D3D12 no Windows).
 *
 * Plataforma  | Backends compilados
 * ------------|--------------------------------------------
 * Linux       | Vulkan (primário), OpenGL (fallback)
 * Windows     | D3D12 (primário), Vulkan, D3D11, OpenGL
 */
enum ri_gpu_backend {
	RI_GPU_BACKEND_AUTO   = 0, /* Escolhe o melhor disponível para a plataforma */
	RI_GPU_BACKEND_VULKAN,     /* Linux + Windows — API principal              */
	RI_GPU_BACKEND_DX12,       /* Windows — Direct3D 12                        */
	RI_GPU_BACKEND_DX11,       /* Windows — Direct3D 11 (compatibilidade)      */
	RI_GPU_BACKEND_OPENGL,     /* Linux + Windows — fallback legado            */
	RI_GPU_BACKEND_COUNT,      /* Sentinel — número total de backends          */
};

enum ri_gpu_buffer_usage {
	RI_BUFFER_VERTEX = (1 << 0),
	RI_BUFFER_INDEX = (1 << 1),
	RI_BUFFER_UNIFORM = (1 << 2),
	RI_BUFFER_STORAGE = (1 << 3),
	RI_BUFFER_INDIRECT = (1 << 4),
	RI_BUFFER_TRANSFER_SRC = (1 << 5),
	RI_BUFFER_TRANSFER_DST = (1 << 6),
};

enum ri_gpu_buffer_memory {
	RI_MEMORY_GPU_ONLY = 0, /* Memória do device (mais rápida) */
	RI_MEMORY_CPU_VISIBLE,	 /* Mapeável pela CPU */
	RI_MEMORY_CPU_TO_GPU,	 /* Upload staging */
	RI_MEMORY_GPU_TO_CPU,	 /* Readback */
};

enum ri_gpu_texture_format {
	RI_FORMAT_UNDEFINED = 0,
	RI_FORMAT_RGBA8_UNORM,
	RI_FORMAT_RGBA8_SRGB,
	RI_FORMAT_BGRA8_UNORM,
	RI_FORMAT_BGRA8_SRGB,
	RI_FORMAT_R32_FLOAT,
	RI_FORMAT_RG32_FLOAT,
	RI_FORMAT_RGB32_FLOAT,
	RI_FORMAT_RGBA32_FLOAT,
	RI_FORMAT_DEPTH32_FLOAT,
	RI_FORMAT_DEPTH24_STENCIL8,
};

enum ri_gpu_texture_usage {
	RI_TEXTURE_SAMPLED = (1 << 0),
	RI_TEXTURE_STORAGE = (1 << 1),
	RI_TEXTURE_RENDER_TARGET = (1 << 2),
	RI_TEXTURE_DEPTH_STENCIL = (1 << 3),
	RI_TEXTURE_TRANSFER_SRC = (1 << 4),
	RI_TEXTURE_TRANSFER_DST = (1 << 5),
};

enum ri_gpu_shader_stage {
	RI_SHADER_VERTEX = 0,
	RI_SHADER_FRAGMENT,
	RI_SHADER_COMPUTE,
};

enum ri_gpu_primitive {
	RI_PRIMITIVE_TRIANGLES = 0,
	RI_PRIMITIVE_TRIANGLE_STRIP,
	RI_PRIMITIVE_LINES,
	RI_PRIMITIVE_LINE_STRIP,
	RI_PRIMITIVE_POINTS,
};

enum ri_gpu_cull_mode {
	RI_CULL_NONE = 0,
	RI_CULL_FRONT,
	RI_CULL_BACK,
};

enum ri_gpu_compare_func {
	RI_COMPARE_NEVER = 0,
	RI_COMPARE_LESS,
	RI_COMPARE_EQUAL,
	RI_COMPARE_LESS_EQUAL,
	RI_COMPARE_GREATER,
	RI_COMPARE_NOT_EQUAL,
	RI_COMPARE_GREATER_EQUAL,
	RI_COMPARE_ALWAYS,
};

enum ri_gpu_blend_factor {
	RI_BLEND_ZERO = 0,
	RI_BLEND_ONE,
	RI_BLEND_SRC_ALPHA,
	RI_BLEND_ONE_MINUS_SRC_ALPHA,
	RI_BLEND_DST_ALPHA,
	RI_BLEND_ONE_MINUS_DST_ALPHA,
};

enum ri_gpu_blend_op {
	RI_BLEND_OP_ADD = 0,
	RI_BLEND_OP_SUBTRACT,
	RI_BLEND_OP_MIN,
	RI_BLEND_OP_MAX,
};

enum ri_gpu_load_action {
	RI_LOAD_DONT_CARE = 0, /* Conteúdo anterior irrelevante */
	RI_LOAD_LOAD,		/* Preservar conteúdo */
	RI_LOAD_CLEAR,		/* Limpar com valor especificado */
};

enum ri_gpu_store_action {
	RI_STORE_DONT_CARE = 0,
	RI_STORE_STORE,
};

enum ri_gpu_filter {
	RI_FILTER_NEAREST = 0,
	RI_FILTER_LINEAR,
};

enum ri_gpu_address_mode {
	RI_ADDRESS_REPEAT = 0,
	RI_ADDRESS_CLAMP_TO_EDGE,
	RI_ADDRESS_CLAMP_TO_BORDER,
	RI_ADDRESS_MIRRORED_REPEAT,
};

/* ============================================================================
 * ESTRUTURAS DE CONFIGURAÇÃO
 * ============================================================================
 */

struct ri_gpu_device_config {
	enum ri_gpu_backend preferred_backend;
	bool enable_validation;	  /* Debug layers */
	bool prefer_discrete_gpu; /* Preferir GPU dedicada */
};

struct ri_gpu_buffer_config {
	uint64_t size;
	uint32_t usage; /* ri_gpu_buffer_usage flags */
	enum ri_gpu_buffer_memory memory;
	const char *label; /* Debug label (pode ser NULL) */
};

struct ri_gpu_texture_config {
	uint32_t width;
	uint32_t height;
	uint32_t depth; /* 1 para 2D */
	uint32_t mip_levels;
	uint32_t array_layers;
	enum ri_gpu_texture_format format;
	uint32_t usage; /* ri_gpu_texture_usage flags */
	const char *label;
};

struct ri_gpu_sampler_config {
	enum ri_gpu_filter min_filter;
	enum ri_gpu_filter mag_filter;
	enum ri_gpu_filter mip_filter;
	enum ri_gpu_address_mode address_u;
	enum ri_gpu_address_mode address_v;
	enum ri_gpu_address_mode address_w;
	float max_anisotropy;			/* 0 = desabilitado */
	enum ri_gpu_compare_func compare_func; /* Para shadow maps */
};

struct ri_gpu_shader_config {
	enum ri_gpu_shader_stage stage;
	const void *code; /* Bytecode ou source */
	size_t code_size;
	const char *entry_point; /* Função de entrada */
	const char *label;
};

/**
 * Descrição de atributo de vértice
 */
struct ri_gpu_vertex_attr {
	uint32_t location;		    /* Índice do atributo */
	uint32_t binding;		    /* Qual buffer de vértice */
	enum ri_gpu_texture_format format; /* Reusa os formatos */
	uint32_t offset;		    /* Offset no vértice */
};

/**
 * Descrição de binding de vértice
 */
struct ri_gpu_vertex_binding {
	uint32_t binding;
	uint32_t stride;
	bool per_instance; /* false = per vertex */
};

/**
 * Configuração de blending por render target
 */
struct ri_gpu_blend_state {
	bool enabled;
	enum ri_gpu_blend_factor src_color;
	enum ri_gpu_blend_factor dst_color;
	enum ri_gpu_blend_op color_op;
	enum ri_gpu_blend_factor src_alpha;
	enum ri_gpu_blend_factor dst_alpha;
	enum ri_gpu_blend_op alpha_op;
};

/**
 * Configuração do pipeline gráfico
 */
struct ri_gpu_pipeline_config {
	ri_gpu_shader_t vertex_shader;
	ri_gpu_shader_t fragment_shader;

	/* Vertex input */
	const struct ri_gpu_vertex_attr *vertex_attrs;
	uint32_t vertex_attr_count;
	const struct ri_gpu_vertex_binding *vertex_bindings;
	uint32_t vertex_binding_count;

	/* Rasterização */
	enum ri_gpu_primitive primitive;
	enum ri_gpu_cull_mode cull_mode;
	bool front_ccw; /* Counter-clockwise = front */
	bool depth_clip;

	/* Depth/Stencil */
	bool depth_test;
	bool depth_write;
	enum ri_gpu_compare_func depth_compare;

	/* Blending */
	const struct ri_gpu_blend_state *blend_states;
	uint32_t blend_state_count;

	/* Render targets */
	const enum ri_gpu_texture_format *color_formats;
	uint32_t color_format_count;
	enum ri_gpu_texture_format depth_format; /* 0 = sem depth */
	enum ri_gpu_texture_format
		depth_stencil_format; /* DEPRECATED: merged with depth_format logic but
                               keeping for compat */

	const char *label;
};

/**
 * Descrição de um render target
 */
struct ri_gpu_color_attachment {
	ri_gpu_texture_t texture;
	uint32_t mip_level;
	uint32_t array_layer;
	enum ri_gpu_load_action load_action;
	enum ri_gpu_store_action store_action;
	float clear_color[4]; /* RGBA, usado se load_action == CLEAR */
};

struct ri_gpu_depth_attachment {
	ri_gpu_texture_t texture;
	enum ri_gpu_load_action load_action;
	enum ri_gpu_store_action store_action;
	float clear_depth;
	uint8_t clear_stencil;
};

struct ri_gpu_render_pass {
	const struct ri_gpu_color_attachment *color_attachments;
	uint32_t color_attachment_count;
	const struct ri_gpu_depth_attachment
		*depth_attachment; /* Pode ser NULL */
};

/**
 * Configuração do pipeline de computação
 */
struct ri_gpu_compute_pipeline_config {
	ri_gpu_shader_t compute_shader;
	const char *label;
};

struct ri_gpu_swapchain_config {
	void *native_display; /* wl_display (Wayland), NULL para outros */
	void *native_window;  /* ri_window_get_native_handle() */
	void *native_layer;   /* ri_window_get_native_layer() */
	uint32_t width;
	uint32_t height;
	enum ri_gpu_texture_format format;
	uint32_t buffer_count; /* 2 = double buffer, 3 = triple */
	bool vsync;
};

/* ============================================================================
 * API DE DEVICE
 * ============================================================================
 */

/**
 * ri_gpu_device_create - Cria device de renderização
 *
 * @config: Configuração do device
 * @device: Ponteiro para receber o handle
 *
 * Retorna: RI_GPU_OK ou código de erro
 */
int ri_gpu_device_create(const struct ri_gpu_device_config *config,
			  ri_gpu_device_t *device);

void ri_gpu_device_destroy(ri_gpu_device_t device);

/**
 * ri_gpu_device_get_backend - Retorna qual backend está em uso
 */
enum ri_gpu_backend ri_gpu_device_get_backend(ri_gpu_device_t device);

/**
 * ri_gpu_device_get_name - Nome do device (ex: "Apple M1")
 */
const char *ri_gpu_device_get_name(ri_gpu_device_t device);

/* ============================================================================
 * API DE BUFFERS
 * ============================================================================
 */

int ri_gpu_buffer_create(ri_gpu_device_t device,
			  const struct ri_gpu_buffer_config *config,
			  ri_gpu_buffer_t *buffer);

void ri_gpu_buffer_destroy(ri_gpu_buffer_t buffer);

/**
 * ri_gpu_buffer_map - Mapeia buffer para CPU
 *
 * Apenas para buffers com RI_MEMORY_CPU_VISIBLE ou similar.
 * Retorna NULL em erro.
 */
void *ri_gpu_buffer_map(ri_gpu_buffer_t buffer);

void ri_gpu_buffer_unmap(ri_gpu_buffer_t buffer);

/**
 * ri_gpu_buffer_upload - Upload de dados para buffer
 *
 * Conveniência para buffers que não são mapeáveis.
 * Usa staging buffer internamente se necessário.
 */
int ri_gpu_buffer_upload(ri_gpu_buffer_t buffer, uint64_t offset,
			  const void *data, uint64_t size);

/* ============================================================================
 * API DE TEXTURAS
 * ============================================================================
 */

int ri_gpu_texture_create(ri_gpu_device_t device,
			   const struct ri_gpu_texture_config *config,
			   ri_gpu_texture_t *texture);

void ri_gpu_texture_destroy(ri_gpu_texture_t texture);

int ri_gpu_texture_upload(ri_gpu_texture_t texture, uint32_t mip_level,
			   uint32_t array_layer, const void *data, size_t size);

/* ============================================================================
 * API DE SAMPLERS
 * ============================================================================
 */

int ri_gpu_sampler_create(ri_gpu_device_t device,
			   const struct ri_gpu_sampler_config *config,
			   ri_gpu_sampler_t *sampler);

void ri_gpu_sampler_destroy(ri_gpu_sampler_t sampler);

/* ============================================================================
 * API DE SHADERS
 * ============================================================================
 */

int ri_gpu_shader_create(ri_gpu_device_t device,
			  const struct ri_gpu_shader_config *config,
			  ri_gpu_shader_t *shader);

void ri_gpu_shader_destroy(ri_gpu_shader_t shader);

/* ============================================================================
 * API DE PIPELINES
 * ============================================================================
 */

int ri_gpu_pipeline_create(ri_gpu_device_t device,
			    const struct ri_gpu_pipeline_config *config,
			    ri_gpu_pipeline_t *pipeline);

void ri_gpu_pipeline_destroy(ri_gpu_pipeline_t pipeline);

int ri_gpu_pipeline_compute_create(
	ri_gpu_device_t device,
	const struct ri_gpu_compute_pipeline_config *config,
	ri_gpu_pipeline_t *pipeline);

/* ============================================================================
 * API DE SWAPCHAIN
 * ============================================================================
 */

int ri_gpu_swapchain_create(ri_gpu_device_t device,
			     const struct ri_gpu_swapchain_config *config,
			     ri_gpu_swapchain_t *swapchain);

int ri_gpu_swapchain_submit(ri_gpu_swapchain_t swapchain,
			     ri_gpu_cmd_buffer_t cmd, ri_gpu_fence_t fence);

int ri_gpu_swapchain_present(ri_gpu_swapchain_t swapchain);

void ri_gpu_swapchain_destroy(ri_gpu_swapchain_t swapchain);

/**
 * ri_gpu_swapchain_resize - Redimensiona swapchain
 *
 * Chamado após resize da janela.
 */
int ri_gpu_swapchain_resize(ri_gpu_swapchain_t swapchain, uint32_t width,
			     uint32_t height);

/**
 * ri_gpu_swapchain_next_texture - Obtém próxima textura para desenhar
 *
 * @swapchain: Handle do swapchain
 * @texture: Ponteiro para receber textura (NÃO destruir - pertence ao
 * swapchain)
 *
 * Retorna: RI_GPU_OK, ou RI_GPU_ERR_SWAPCHAIN se precisar recriar
 */
int ri_gpu_swapchain_next_texture(ri_gpu_swapchain_t swapchain,
				   ri_gpu_texture_t *texture);

/**
 * ri_gpu_swapchain_present - Apresenta frame atual
 */
int ri_gpu_swapchain_present(ri_gpu_swapchain_t swapchain);

/* ============================================================================
 * API DE COMMAND BUFFERS
 * ============================================================================
 */

/**
 * ri_gpu_cmd_buffer_create - Cria command buffer
 *
 * Command buffers são reutilizáveis após reset.
 */
int ri_gpu_cmd_buffer_create(ri_gpu_device_t device,
			      ri_gpu_cmd_buffer_t *cmd);

void ri_gpu_cmd_buffer_destroy(ri_gpu_cmd_buffer_t cmd);

/**
 * ri_gpu_cmd_begin - Inicia gravação de comandos
 */
void ri_gpu_cmd_begin(ri_gpu_cmd_buffer_t cmd);

/**
 * ri_gpu_cmd_end - Finaliza gravação
 */
void ri_gpu_cmd_begin(ri_gpu_cmd_buffer_t cmd);
void ri_gpu_cmd_end(ri_gpu_cmd_buffer_t cmd);

/**
 * ri_gpu_cmd_reset - Limpa comandos para reutilização
 */
void ri_gpu_cmd_reset(ri_gpu_cmd_buffer_t cmd);

/* Render pass */
void ri_gpu_cmd_begin_render_pass(ri_gpu_cmd_buffer_t cmd,
				   const struct ri_gpu_render_pass *pass);
void ri_gpu_cmd_end_render_pass(ri_gpu_cmd_buffer_t cmd);

/* Estado do pipeline */
void ri_gpu_cmd_set_pipeline(ri_gpu_cmd_buffer_t cmd,
			      ri_gpu_pipeline_t pipeline);

void ri_gpu_cmd_set_viewport(ri_gpu_cmd_buffer_t cmd, float x, float y,
			      float width, float height, float min_depth,
			      float max_depth);

void ri_gpu_cmd_set_scissor(ri_gpu_cmd_buffer_t cmd, int32_t x, int32_t y,
			     uint32_t width, uint32_t height);

/* Bindings */
void ri_gpu_cmd_set_vertex_buffer(ri_gpu_cmd_buffer_t cmd, uint32_t binding,
				   ri_gpu_buffer_t buffer, uint64_t offset);

void ri_gpu_cmd_set_index_buffer(
	ri_gpu_cmd_buffer_t cmd, ri_gpu_buffer_t buffer, uint64_t offset,
	bool is_32bit); /* true = uint32, false = uint16 */

/* Push constants / uniforms (simplificado) */
void ri_gpu_cmd_push_constants(ri_gpu_cmd_buffer_t cmd, uint32_t offset,
				const void *data, uint32_t size);

/**
 * ri_gpu_cmd_bind_texture - Binda textura e sampler num binding point
 *
 * Abstração simplificada de descriptors.
 * @set: Índice do Descriptor Set (geralmente 0 ou 1)
 * @binding: Índice do binding dentro do set
 */
void ri_gpu_cmd_bind_texture(ri_gpu_cmd_buffer_t cmd, uint32_t set,
			      uint32_t binding, ri_gpu_texture_t texture,
			      ri_gpu_sampler_t sampler);

/* Draw calls */
void ri_gpu_cmd_draw(ri_gpu_cmd_buffer_t cmd, uint32_t vertex_count,
		      uint32_t instance_count, uint32_t first_vertex,
		      uint32_t first_instance);

void ri_gpu_cmd_draw_indexed(ri_gpu_cmd_buffer_t cmd, uint32_t index_count,
			      uint32_t instance_count, uint32_t first_index,
			      int32_t vertex_offset, uint32_t first_instance);

/* Compute dispatch */
void ri_gpu_cmd_dispatch(ri_gpu_cmd_buffer_t cmd, uint32_t group_count_x,
			  uint32_t group_count_y, uint32_t group_count_z);

/**
 * ri_gpu_cmd_transition_texture - Transição de layout de imagem
 *
 * Útil para sincronizar escrita de Compute com leitura de Fragment.
 */
void ri_gpu_cmd_transition_texture(ri_gpu_cmd_buffer_t cmd,
				    ri_gpu_texture_t texture);

/**
 * ri_gpu_cmd_bind_compute_storage_texture - Bind de storage image para compute
 */
void ri_gpu_cmd_bind_compute_storage_texture(ri_gpu_cmd_buffer_t cmd,
					      ri_gpu_pipeline_t pipeline,
					      uint32_t set, uint32_t binding,
					      ri_gpu_texture_t texture);

/* ============================================================================
 * API DE SUBMISSÃO E SINCRONIZAÇÃO
 * ============================================================================
 */

int ri_gpu_fence_create(ri_gpu_device_t device, ri_gpu_fence_t *fence);
void ri_gpu_fence_destroy(ri_gpu_fence_t fence);

/**
 * ri_gpu_fence_wait - Aguarda fence ser sinalizada
 *
 * @fence: Handle do fence
 * @timeout_ns: Timeout em nanosegundos (0 = não espera, UINT64_MAX = infinito)
 *
 * Retorna: RI_GPU_OK se sinalizado, RI_GPU_ERR_TIMEOUT se expirou
 */
int ri_gpu_fence_wait(ri_gpu_fence_t fence, uint64_t timeout_ns);

void ri_gpu_fence_reset(ri_gpu_fence_t fence);

/**
 * ri_gpu_submit - Submete command buffer para execução
 *
 * @device: Device
 * @cmd: Command buffer a submeter
 * @signal_fence: Fence a sinalizar quando completar (pode ser NULL)
 */
int ri_gpu_submit(ri_gpu_device_t device, ri_gpu_cmd_buffer_t cmd,
		   ri_gpu_fence_t signal_fence);

/**
 * ri_gpu_wait_idle - Aguarda GPU terminar todo trabalho
 *
 * Bloqueia até que todos os command buffers submetidos completem.
 */
void ri_gpu_wait_idle(ri_gpu_device_t device);

#ifdef __cplusplus
}
#endif

#endif /* RI_UX_RENDERER_LIB_H */
