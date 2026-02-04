#define _POSIX_C_SOURCE 200809L
/**
 * @file rhi_vulkan.c
 * @brief Implementação do RHI sobre Vulkan 1.3 (Dynamic Rendering)
 *
 * "No RenderPasses. No Framebuffers. Just raw power."
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "hal/rhi/rhi.h"

/* ============================================================================
 * ESTRUTURAS INTERNAS (Opaque Handles)
 * ============================================================================
 */

struct bhs_rhi_device_t {
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	VkPhysicalDevice phys_dev;
	VkDevice dev;
	VkQueue queue_graphics;
	VkQueue queue_compute;
	VkQueue queue_transfer;
	uint32_t qfam_graphics;
	uint32_t qfam_compute;
	uint32_t qfam_transfer;

	// Allocator VMA ou simples
	// Por enquanto simples offset allocator linear ou direto
};

struct bhs_rhi_buffer_t {
	VkDevice device; // Ref for mapping
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize size;
	void *mapped_ptr;
};

struct bhs_rhi_shader_t {
	VkShaderModule module;
	bhs_rhi_shader_stage stage;
	const char *entry_point;
};

struct bhs_rhi_cmd_list_t {
	VkCommandBuffer cmd;
	VkCommandPool pool; // Um pool por thread/lista idealmente
};

/* ============================================================================
 * HELPER MACROS
 * ============================================================================
 */

#define VK_CHECK(expr)                                                         \
	do {                                                                   \
		VkResult res = (expr);                                         \
		if (res != VK_SUCCESS) {                                       \
			fprintf(stderr,                                        \
				"[RHI-VK] FATAL: %s failed with %d at "        \
				"%s:%d\n",                                     \
				#expr, res, __FILE__, __LINE__);               \
			abort();                                               \
		}                                                              \
	} while (0)

/* ============================================================================
 * DEBUG CALLBACK
 * ============================================================================
 */

static VKAPI_ATTR VkBool32 VKAPI_CALL
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	       VkDebugUtilsMessageTypeFlagsEXT type,
	       const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	       void *pUserData)
{
	if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
		fprintf(stderr, "[RHI-VK] Validation: %s\n",
			pCallbackData->pMessage);
	}
	return VK_FALSE;
}

/* ============================================================================
 * DEVICE CREATION
 * ============================================================================
 */

bhs_rhi_device_handle bhs_rhi_create_device(const bhs_rhi_device_desc *desc)
{
	bhs_rhi_device_handle rhi = calloc(1, sizeof(struct bhs_rhi_device_t));

	// 1. Create Instance
	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Black Hole Simulator (Supreme)",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "Event Horizon Engine",
		.engineVersion = VK_MAKE_VERSION(2, 0, 0),
		.apiVersion = VK_API_VERSION_1_3
	};

	const char *layers[] = { "VK_LAYER_KHRONOS_validation" };
	const char *extensions[] = { VK_EXT_DEBUG_UTILS_EXTENSION_NAME };

	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledLayerCount = desc->enable_validation ? 1 : 0,
		.ppEnabledLayerNames = desc->enable_validation ? layers : NULL,
		.enabledExtensionCount =
			desc->enable_validation
				? 1
				: 0, // Mock: precisa lógica real de extensões platforma
		.ppEnabledExtensionNames =
			desc->enable_validation ? extensions : NULL
	};

	VK_CHECK(vkCreateInstance(&create_info, NULL, &rhi->instance));

	// Setup Debug Messenger
	if (desc->enable_validation) {
		VkDebugUtilsMessengerCreateInfoEXT dbi = {
			.sType =
				VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity =
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType =
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = debug_callback
		};

		PFN_vkCreateDebugUtilsMessengerEXT func =
			(PFN_vkCreateDebugUtilsMessengerEXT)
				vkGetInstanceProcAddr(
					rhi->instance,
					"vkCreateDebugUtilsMessengerEXT");
		if (func)
			func(rhi->instance, &dbi, NULL, &rhi->debug_messenger);
	}

	// 2. Pick Physical Device (Simplificado: Pega o primeiro discreto ou integrado)
	uint32_t count = 0;
	vkEnumeratePhysicalDevices(rhi->instance, &count, NULL);
	VkPhysicalDevice devices[8];
	vkEnumeratePhysicalDevices(rhi->instance, &count, devices);
	rhi->phys_dev = devices[0]; // TODO: Fazer uma lógica de score decente, não pegar o primeiro que aparecer feito preguiçoso.

	// 3. Create Logical Device (Dynamic Rendering required)
	float priority = 1.0f;
	VkDeviceQueueCreateInfo qci = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = 0, // Mock: assumindo 0 é gfx/compute
		.queueCount = 1,
		.pQueuePriorities = &priority
	};

	VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering = {
		.sType =
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
		.dynamicRendering = VK_TRUE
	};

	VkPhysicalDeviceVulkan13Features features13 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &dynamic_rendering,
		.synchronization2 = VK_TRUE // Critical for Barriers!
	};

	VkPhysicalDeviceFeatures2 features2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &features13
	};

	const char *dev_exts[] = { "VK_KHR_swapchain" }; // Para apresentar

	VkDeviceCreateInfo dci = { .sType =
					   VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				   .pNext = &features2,
				   .queueCreateInfoCount = 1,
				   .pQueueCreateInfos = &qci,
				   .enabledExtensionCount = 1,
				   .ppEnabledExtensionNames = dev_exts };

	VK_CHECK(vkCreateDevice(rhi->phys_dev, &dci, NULL, &rhi->dev));

	// Get Queues
	vkGetDeviceQueue(rhi->dev, 0, 0, &rhi->queue_graphics); // Mock

	return rhi;
}

void bhs_rhi_destroy_device(bhs_rhi_device_handle dev)
{
	if (!dev)
		return;

	vkDeviceWaitIdle(dev->dev);
	vkDestroyDevice(dev->dev, NULL);

	if (dev->debug_messenger) {
		PFN_vkDestroyDebugUtilsMessengerEXT func =
			(PFN_vkDestroyDebugUtilsMessengerEXT)
				vkGetInstanceProcAddr(
					dev->instance,
					"vkDestroyDebugUtilsMessengerEXT");
		if (func)
			func(dev->instance, dev->debug_messenger, NULL);
	}

	vkDestroyInstance(dev->instance, NULL);
	free(dev);
}

/* ============================================================================
 * MEMORY HELPERS
 * ============================================================================
 */

static uint32_t find_memory_type(VkPhysicalDevice phys_dev,
				 uint32_t type_filter,
				 VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phys_dev, &mem_props);

	for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
		if ((type_filter & (1 << i)) &&
		    (mem_props.memoryTypes[i].propertyFlags & properties) ==
			    properties) {
			return i;
		}
	}

	fprintf(stderr, "[RHI-VK] Failed to find suitable memory type!\n");
	abort();
	return 0;
}

/* ============================================================================
 * BUFFERS
 * ============================================================================
 */

bhs_rhi_buffer_handle bhs_rhi_create_buffer(bhs_rhi_device_handle dev,
					    const bhs_rhi_buffer_desc *desc)
{
	bhs_rhi_buffer_handle buf = calloc(1, sizeof(struct bhs_rhi_buffer_t));
	buf->device = dev->dev;
	buf->size = desc->size;

	VkBufferCreateInfo bci = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = desc->size,
		.usage =
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT |
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // Genérico por enquanto
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VK_CHECK(vkCreateBuffer(dev->dev, &bci, NULL, &buf->buffer));

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(dev->dev, buf->buffer, &mem_reqs);

	VkMemoryPropertyFlags props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	if (desc->cpu_visible) {
		props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	}

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = mem_reqs.size,
		.memoryTypeIndex = find_memory_type(
			dev->phys_dev, mem_reqs.memoryTypeBits, props)
	};

	VK_CHECK(vkAllocateMemory(dev->dev, &alloc_info, NULL, &buf->memory));
	VK_CHECK(vkBindBufferMemory(dev->dev, buf->buffer, buf->memory, 0));

	return buf;
}

void *bhs_rhi_map_buffer(bhs_rhi_buffer_handle buf)
{
	if (buf->mapped_ptr)
		return buf->mapped_ptr;

	VK_CHECK(vkMapMemory(buf->device, buf->memory, 0, buf->size, 0,
			     &buf->mapped_ptr));
	return buf->mapped_ptr;
}

void bhs_rhi_unmap_buffer(bhs_rhi_buffer_handle buf)
{
	if (buf->mapped_ptr) {
		vkUnmapMemory(buf->device, buf->memory);
		buf->mapped_ptr = NULL;
	}
}

/* ============================================================================
 * SHADERS
 * ============================================================================
 */

bhs_rhi_shader_handle
bhs_rhi_create_shader_from_bytecode(bhs_rhi_device_handle dev,
				    const bhs_rhi_shader_desc *desc)
{
	bhs_rhi_shader_handle shader =
		calloc(1, sizeof(struct bhs_rhi_shader_t));
	shader->stage = desc->stage;
	shader->entry_point = strdup(desc->entry_point);

	VkShaderModuleCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = desc->bytecode_size,
		.pCode = (const uint32_t *)desc->bytecode
	};

	VK_CHECK(vkCreateShaderModule(dev->dev, &create_info, NULL,
				      &shader->module));
	return shader;
}

bhs_rhi_shader_handle bhs_rhi_create_shader_from_c(bhs_rhi_device_handle dev,
						   const char *source_code,
						   bhs_rhi_shader_stage stage)
{
	// Fase 2 Integration:
	// 1. Salvar source em tmp
	// 2. Invocar clang (system call popen)
	// 3. Ler .spv
	// 4. Create from Bytecode

	// Por enquanto, retorna NULL (TODO: Implementar essa budega depois)
	return NULL;
}

/* ============================================================================
 * COMMAND LISTS
 * ============================================================================
 */

bhs_rhi_cmd_list_handle bhs_rhi_allocate_cmd_list(bhs_rhi_device_handle dev)
{
	bhs_rhi_cmd_list_handle cmd =
		calloc(1, sizeof(struct bhs_rhi_cmd_list_t));

	// Criar pool se não existir (one per thread usually, but here simplified)
	VkCommandPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = dev->qfam_graphics // Mock
	};

	VK_CHECK(vkCreateCommandPool(dev->dev, &pool_info, NULL, &cmd->pool));

	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = cmd->pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	VK_CHECK(vkAllocateCommandBuffers(dev->dev, &alloc_info, &cmd->cmd));
	return cmd;
}

void bhs_rhi_cmd_begin(bhs_rhi_cmd_list_handle cmd)
{
	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
	};
	VK_CHECK(vkBeginCommandBuffer(cmd->cmd, &begin_info));
}

void bhs_rhi_cmd_end(bhs_rhi_cmd_list_handle cmd)
{
	VK_CHECK(vkEndCommandBuffer(cmd->cmd));
}

void bhs_rhi_cmd_set_pipeline_compute(bhs_rhi_cmd_list_handle cmd,
				      bhs_rhi_pipeline_handle pipeline)
{
	// Pipeline handle not fully defined yet in C file, assuming opaque ptr cast for now or placeholder
	// struct bhs_rhi_pipeline_t { VkPipeline p; VkPipelineLayout l; };
	// VK_CHECK(vkCmdBindPipeline(cmd->cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->p));
}

void bhs_rhi_cmd_bind_buffer(bhs_rhi_cmd_list_handle cmd, uint32_t slot,
			     bhs_rhi_buffer_handle buffer)
{
	// Vulkan uses descriptor sets. "Bindless" logic implies we might use push descriptors or a global descriptor set.
	// For simplicity of this "Phase 3" step:
	// TODO: Implementar Gerenciamento de Descritores (DescriptorBuffer ou Sets) pq sem isso nada renderiza, gênio.
	// Placeholder: This would bind the buffer to a descriptor set.
}

void bhs_rhi_cmd_dispatch(bhs_rhi_cmd_list_handle cmd, uint32_t x, uint32_t y,
			  uint32_t z)
{
	vkCmdDispatch(cmd->cmd, x, y, z);
}

void bhs_rhi_cmd_barrier(bhs_rhi_cmd_list_handle cmd)
{
	// Global Memory Barrier (Nuclear Option for now)
	VkMemoryBarrier2 mem_bar = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.srcAccessMask = VK_ACCESS_2_MEMORY_READ_BIT |
				 VK_ACCESS_2_MEMORY_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT |
				 VK_ACCESS_2_MEMORY_WRITE_BIT
	};

	VkDependencyInfo dep_info = { .sType =
					      VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
				      .memoryBarrierCount = 1,
				      .pMemoryBarriers = &mem_bar };

	vkCmdPipelineBarrier2(cmd->cmd, &dep_info);
}

void bhs_rhi_submit(bhs_rhi_device_handle dev, bhs_rhi_cmd_list_handle cmd)
{
	VkSubmitInfo submit_info = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				     .commandBufferCount = 1,
				     .pCommandBuffers = &cmd->cmd };

	// Fence null por enquanto (blocking submit via wait idle)
	VK_CHECK(vkQueueSubmit(dev->queue_graphics, 1, &submit_info,
			       VK_NULL_HANDLE));
}

void bhs_rhi_wait_idle(bhs_rhi_device_handle dev)
{
	vkDeviceWaitIdle(dev->dev);
}
