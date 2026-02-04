#ifndef WGPU_INTERNAL_H
#define WGPU_INTERNAL_H

#include <emscripten/emscripten.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <webgpu/webgpu.h>
#include "gui/rhi/rhi.h"

#define MAX_BIND_ENTRIES 8

#define WGPU_CHECK(obj)                                                        \
	do {                                                                   \
		if (!(obj)) {                                                  \
			fprintf(stderr,                                        \
				"[RHI-WGPU] Objeto WebGPU nulo em %s:%d\n",    \
				__FILE__, __LINE__);                           \
			abort();                                               \
		}                                                              \
	} while (0)
#define LOG_ERR(...) fprintf(stderr, "[RHI-WGPU] ERROR: " __VA_ARGS__)
#define LOG_INFO(...) printf("[RHI-WGPU] " __VA_ARGS__)

struct bhs_gpu_device_impl {
	WGPUInstance instance;
	WGPUAdapter adapter;
	WGPUDevice device;
	WGPUQueue queue;

	bool adapter_request_ended;
	bool device_request_ended;
};

struct bhs_gpu_buffer_impl {
	WGPUBuffer buffer;
	WGPUDevice device;
	uint64_t size;
};

struct bhs_gpu_texture_impl {
	WGPUTexture texture;
	WGPUTextureView default_view;
	WGPUTextureFormat format;
	WGPUDevice device;
	uint32_t width;
	uint32_t height;
};

struct bhs_gpu_sampler_impl {
	WGPUSampler sampler;
};

struct bhs_gpu_shader_impl {
	WGPUShaderModule module;
	char *entry_point;
};

struct bhs_gpu_pipeline_impl {
	WGPURenderPipeline render_pipeline;
	WGPUPipelineLayout layout;
	WGPUBindGroupLayout bind_group_layout;
};

struct bhs_gpu_cmd_buffer_impl {
	WGPUDevice device;
	WGPUCommandEncoder encoder;
	WGPURenderPassEncoder render_pass;
	WGPUCommandBuffer pending_cb;
	bhs_gpu_pipeline_t current_pipeline;

	struct {
		bool dirty;
		WGPUBindGroupEntry entries[MAX_BIND_ENTRIES];
		uint32_t count;
	} bind_state;
};

struct bhs_gpu_fence_impl {
	WGPUQueue queue;
};

struct bhs_gpu_swapchain_impl {
	WGPUSurface surface;
	WGPUDevice device;
	WGPUTextureFormat format;
	uint32_t width;
	uint32_t height;
	struct bhs_gpu_texture_impl current_texture_wrapper;
};

// Utils
WGPUBufferUsage wgpu_map_buffer_usage(uint32_t flags);

#endif
#ifndef WGPU_INTERNAL_H
#define WGPU_INTERNAL_H

#include <emscripten/emscripten.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <webgpu/webgpu.h>
#include "gui/rhi/rhi.h"

#define MAX_BIND_ENTRIES 8

#define WGPU_CHECK(obj)                                                        \
	do {                                                                   \
		if (!(obj)) {                                                  \
			fprintf(stderr,                                        \
				"[RHI-WGPU] Objeto WebGPU nulo em %s:%d\n",    \
				__FILE__, __LINE__);                           \
			abort();                                               \
		}                                                              \
	} while (0)
#define LOG_ERR(...) fprintf(stderr, "[RHI-WGPU] ERROR: " __VA_ARGS__)
#define LOG_INFO(...) printf("[RHI-WGPU] " __VA_ARGS__)

struct bhs_gpu_device_impl {
	WGPUInstance instance;
	WGPUAdapter adapter;
	WGPUDevice device;
	WGPUQueue queue;

	bool adapter_request_ended;
	bool device_request_ended;
};

struct bhs_gpu_buffer_impl {
	WGPUBuffer buffer;
	WGPUDevice device;
	uint64_t size;
};

struct bhs_gpu_texture_impl {
	WGPUTexture texture;
	WGPUTextureView default_view;
	WGPUTextureFormat format;
	WGPUDevice device;
	uint32_t width;
	uint32_t height;
};

struct bhs_gpu_sampler_impl {
	WGPUSampler sampler;
};

struct bhs_gpu_shader_impl {
	WGPUShaderModule module;
	char *entry_point;
};

struct bhs_gpu_pipeline_impl {
	WGPURenderPipeline render_pipeline;
	WGPUPipelineLayout layout;
	WGPUBindGroupLayout bind_group_layout;
};

struct bhs_gpu_cmd_buffer_impl {
	WGPUDevice device;
	WGPUCommandEncoder encoder;
	WGPURenderPassEncoder render_pass;
	WGPUCommandBuffer pending_cb;
	bhs_gpu_pipeline_t current_pipeline;

	struct {
		bool dirty;
		WGPUBindGroupEntry entries[MAX_BIND_ENTRIES];
		uint32_t count;
	} bind_state;
};

struct bhs_gpu_fence_impl {
	WGPUQueue queue;
};

struct bhs_gpu_swapchain_impl {
	WGPUSurface surface;
	WGPUDevice device;
	WGPUTextureFormat format;
	uint32_t width;
	uint32_t height;
	struct bhs_gpu_texture_impl current_texture_wrapper;
};

// Utils
WGPUBufferUsage wgpu_map_buffer_usage(uint32_t flags);

#endif
