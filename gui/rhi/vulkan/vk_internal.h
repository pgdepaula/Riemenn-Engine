/**
 * @file vk_internal.h
 * @brief Definições internas do backend Vulkan
 *
 * Este arquivo define as estruturas opacas que são compartilhadas
 * entre os submódulos do backend Vulkan.
 *
 * "Se não tá aqui, não é interno."
 */

#ifndef BHS_UX_RENDERER_VULKAN_INTERNAL_H
#define BHS_UX_RENDERER_VULKAN_INTERNAL_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>
#include "gui/rhi/rhi.h"

/* ============================================================================
 * CONSTANTES GLORAIS
 * ============================================================================
 */
#define BHS_VK_MAX_SWAPCHAIN_IMAGES 4
#define BHS_VK_MAX_FRAMES_IN_FLIGHT 2

/* ============================================================================
 * MACROS DE DEBUG
 * ============================================================================
 */
#include <stdio.h>

#define BHS_VK_LOG(fmt, ...)                                                   \
	fprintf(stderr, "[vulkan] " fmt "\n", ##__VA_ARGS__)

#define BHS_VK_CHECK(result, msg)                                              \
	do {                                                                   \
		if ((result) != VK_SUCCESS) {                                  \
			BHS_VK_LOG("erro: %s (code=%d)", msg, result);         \
			return BHS_GPU_ERR_DEVICE;                             \
		}                                                              \
	} while (0)

/* ============================================================================
 * ESTRUTURAS DE IMPLEMENTAÇÃO
 * ============================================================================
 */

/* Forward declaration */
struct bhs_gpu_swapchain_impl;

struct bhs_gpu_device_impl {
	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDevice device;
	VkQueue graphics_queue;
	VkQueue present_queue;
	uint32_t graphics_family;
	uint32_t present_family;
	VkCommandPool command_pool;
	VkPhysicalDeviceProperties properties;
	VkPhysicalDeviceMemoryProperties memory_properties;
	bool validation_enabled;

	/* Descriptors (Layouts) */
	VkDescriptorSetLayout
		texture_layout; /* Layout padrão para texturas (binding 0) */

	/* Reference to active swapchain (HACK for render pass) */
	struct bhs_gpu_swapchain_impl *swapchain;
};

struct bhs_gpu_buffer_impl {
	bhs_gpu_device_t device;
	VkBuffer buffer;
	VkDeviceMemory memory;
	uint64_t size;
	void *mapped;
	uint32_t usage;
};

struct bhs_gpu_texture_impl {
	bhs_gpu_device_t device;
	VkImage image;
	VkImageView view;
	VkDeviceMemory memory;
	uint32_t width;
	uint32_t height;
	VkFormat format;
	bool owns_image; /* false para imagens do swapchain */
};

struct bhs_gpu_sampler_impl {
	bhs_gpu_device_t device;
	VkSampler sampler;
};

struct bhs_gpu_shader_impl {
	bhs_gpu_device_t device;
	VkShaderModule module;
	enum bhs_gpu_shader_stage stage;
};

struct bhs_gpu_pipeline_impl {
	bhs_gpu_device_t device;
	VkPipeline pipeline;
	VkPipelineLayout layout;
	VkDescriptorSetLayout set_layout; /* Para compute/storage */
	VkRenderPass render_pass; /* Pode ser VK_NULL_HANDLE para compute */
	VkPipelineBindPoint bind_point;
};

struct bhs_gpu_swapchain_impl {
	bhs_gpu_device_t device;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	VkFormat format;
	VkExtent2D extent;
	uint32_t image_count;
	VkImage images[BHS_VK_MAX_SWAPCHAIN_IMAGES];
	VkImageView views[BHS_VK_MAX_SWAPCHAIN_IMAGES];

	/* RenderPass e Framebuffers associados ao Swapchain */
	VkRenderPass render_pass;
	VkFramebuffer framebuffers[BHS_VK_MAX_SWAPCHAIN_IMAGES];

	/* Depth Buffer (compartilhado entre todas as imagens do swapchain) */
	VkImage depth_image;
	VkImageView depth_view;
	VkDeviceMemory depth_memory;
	VkFormat depth_format;

	uint32_t current_image;
	VkSemaphore image_available[BHS_VK_MAX_FRAMES_IN_FLIGHT];
	VkSemaphore render_finished[BHS_VK_MAX_FRAMES_IN_FLIGHT];
	uint32_t current_frame;
	struct bhs_gpu_texture_impl
		texture_wrappers[BHS_VK_MAX_SWAPCHAIN_IMAGES];
};

struct bhs_gpu_cmd_buffer_impl {
	bhs_gpu_device_t device;
	VkCommandBuffer cmd;
	bool recording;

	/* Recursos efêmeros limpos no reset */

	VkDescriptorPool descriptor_pool; /* Pool exclusivo deste cmd buffer */
	VkPipelineLayout current_pipeline_layout; /* Layout do pipeline atual */
};

struct bhs_gpu_fence_impl {
	bhs_gpu_device_t device;
	VkFence fence;
};

/* ============================================================================
 * HELPERS GLOBAIS
 * ============================================================================
 */
uint32_t bhs_vk_find_memory_type(struct bhs_gpu_device_impl *dev,
				 uint32_t type_filter,
				 VkMemoryPropertyFlags properties);

VkFormat bhs_vk_format(enum bhs_gpu_texture_format fmt);

#endif /* BHS_UX_RENDERER_VULKAN_INTERNAL_H */
