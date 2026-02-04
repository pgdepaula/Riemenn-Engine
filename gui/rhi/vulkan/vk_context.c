/**
 * @file vk_context.c
 * @brief Implementação de Device e Instance Vulkan
 */

#include <stdlib.h>
#include <string.h>
#include "gui/rhi/vulkan/vk_internal.h"

/* ============================================================================
 * HELPERS
 * ============================================================================
 */

uint32_t bhs_vk_find_memory_type(struct bhs_gpu_device_impl *dev,
				 uint32_t type_filter,
				 VkMemoryPropertyFlags properties)
{
	for (uint32_t i = 0; i < dev->memory_properties.memoryTypeCount; i++) {
		if ((type_filter & (1 << i)) &&
		    (dev->memory_properties.memoryTypes[i].propertyFlags &
		     properties) == properties) {
			return i;
		}
	}
	return UINT32_MAX;
}

VkFormat bhs_vk_format(enum bhs_gpu_texture_format fmt)
{
	switch (fmt) {
	case BHS_FORMAT_RGBA8_UNORM:
		return VK_FORMAT_R8G8B8A8_UNORM;
	case BHS_FORMAT_RGBA8_SRGB:
		return VK_FORMAT_R8G8B8A8_SRGB;
	case BHS_FORMAT_BGRA8_UNORM:
		return VK_FORMAT_B8G8R8A8_UNORM;
	case BHS_FORMAT_BGRA8_SRGB:
		return VK_FORMAT_B8G8R8A8_SRGB;
	case BHS_FORMAT_R32_FLOAT:
		return VK_FORMAT_R32_SFLOAT;
	case BHS_FORMAT_RG32_FLOAT:
		return VK_FORMAT_R32G32_SFLOAT;
	case BHS_FORMAT_RGB32_FLOAT:
		return VK_FORMAT_R32G32B32_SFLOAT;
	case BHS_FORMAT_RGBA32_FLOAT:
		return VK_FORMAT_R32G32B32A32_SFLOAT;
	case BHS_FORMAT_DEPTH32_FLOAT:
		return VK_FORMAT_D32_SFLOAT;
	case BHS_FORMAT_DEPTH24_STENCIL8:
		return VK_FORMAT_D24_UNORM_S8_UINT;
	default:
		return VK_FORMAT_R8G8B8A8_UNORM;
	}
}

/* ============================================================================
 * API DE DEVICE
 * ============================================================================
 */

int bhs_gpu_device_create(const struct bhs_gpu_device_config *config,
			  bhs_gpu_device_t *device)
{
	if (!config || !device)
		return BHS_GPU_ERR_INVALID;

	struct bhs_gpu_device_impl *dev = calloc(1, sizeof(*dev));
	if (!dev)
		return BHS_GPU_ERR_NOMEM;

	VkResult result;

	/* Verifica extensões disponíveis */
	uint32_t ext_count_available = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &ext_count_available,
					       NULL);
	VkExtensionProperties *available_exts =
		malloc(ext_count_available * sizeof(VkExtensionProperties));
	vkEnumerateInstanceExtensionProperties(NULL, &ext_count_available,
					       available_exts);

	const char *required_extensions[] = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
	};
	uint32_t required_count = 2;
	const char *enabled_extensions[16];
	uint32_t enabled_count = 0;

	for (uint32_t i = 0; i < required_count; i++) {
		bool found = false;
		for (uint32_t j = 0; j < ext_count_available; j++) {
			if (strcmp(required_extensions[i],
				   available_exts[j].extensionName) == 0) {
				found = true;
				enabled_extensions[enabled_count++] =
					required_extensions[i];
				break;
			}
		}
		if (!found) {
			BHS_VK_LOG("aviso: extensão %s não suportada pelo "
				   "driver Vulkan",
				   required_extensions[i]);
		}
	}
	free(available_exts);

	/* Verifica layers disponíveis */
	uint32_t layer_count_available = 0;
	vkEnumerateInstanceLayerProperties(&layer_count_available, NULL);
	VkLayerProperties *available_layers =
		malloc(layer_count_available * sizeof(VkLayerProperties));
	vkEnumerateInstanceLayerProperties(&layer_count_available,
					   available_layers);

	const char *enabled_layers[4];
	uint32_t enabled_layer_count = 0;

	if (config->enable_validation) {
		const char *validation = "VK_LAYER_KHRONOS_validation";
		bool found = false;
		for (uint32_t i = 0; i < layer_count_available; i++) {
			if (strcmp(validation, available_layers[i].layerName) ==
			    0) {
				found = true;
				enabled_layers[enabled_layer_count++] =
					validation;
				break;
			}
		}
		if (!found) {
			BHS_VK_LOG("aviso: validation layer solicitada mas não "
				   "encontrada");
		}
	}
	free(available_layers);

	/* App Info */
	VkApplicationInfo app_info = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Black Hole Simulator",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "BHS Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_2,
	};

	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &app_info,
		.enabledExtensionCount = enabled_count,
		.ppEnabledExtensionNames = enabled_extensions,
		.enabledLayerCount = enabled_layer_count,
		.ppEnabledLayerNames = enabled_layers,
	};

	result = vkCreateInstance(&create_info, NULL, &dev->instance);
	BHS_VK_CHECK(result, "falha ao criar VkInstance");

	dev->validation_enabled = config->enable_validation;

	/* Enumera physical devices */
	uint32_t gpu_count = 0;
	vkEnumeratePhysicalDevices(dev->instance, &gpu_count, NULL);
	if (gpu_count == 0) {
		BHS_VK_LOG("erro: nenhuma GPU com Vulkan encontrada");
		vkDestroyInstance(dev->instance, NULL);
		free(dev);
		return BHS_GPU_ERR_DEVICE;
	}

	VkPhysicalDevice *gpus = malloc(gpu_count * sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(dev->instance, &gpu_count, gpus);

	/* Seleciona GPU (preferir discreta se configurado) */
	dev->physical_device = gpus[0];
	for (uint32_t i = 0; i < gpu_count; i++) {
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(gpus[i], &props);
		if (config->prefer_discrete_gpu &&
		    props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			dev->physical_device = gpus[i];
			break;
		}
	}
	free(gpus);

	vkGetPhysicalDeviceProperties(dev->physical_device, &dev->properties);
	vkGetPhysicalDeviceMemoryProperties(dev->physical_device,
					    &dev->memory_properties);

	BHS_VK_LOG("usando GPU: %s", dev->properties.deviceName);

	/* Encontra queue families */
	uint32_t queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(dev->physical_device,
						 &queue_family_count, NULL);
	VkQueueFamilyProperties *queue_families =
		malloc(queue_family_count * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(
		dev->physical_device, &queue_family_count, queue_families);

	dev->graphics_family = UINT32_MAX;
	for (uint32_t i = 0; i < queue_family_count; i++) {
		if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			dev->graphics_family = i;
			dev->present_family = i; /* Assume mesmo para present */
			break;
		}
	}
	free(queue_families);

	if (dev->graphics_family == UINT32_MAX) {
		BHS_VK_LOG("erro: GPU não tem queue de gráficos");
		vkDestroyInstance(dev->instance, NULL);
		free(dev);
		return BHS_GPU_ERR_DEVICE;
	}

	/* Cria logical device */
	float queue_priority = 1.0f;
	VkDeviceQueueCreateInfo queue_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = dev->graphics_family,
		.queueCount = 1,
		.pQueuePriorities = &queue_priority,
	};

	const char *device_extensions[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	VkPhysicalDeviceFeatures features = { 0 };

	VkDeviceCreateInfo device_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queue_create_info,
		.enabledExtensionCount = 1,
		.ppEnabledExtensionNames = device_extensions,
		.pEnabledFeatures = &features,
	};

	result = vkCreateDevice(dev->physical_device, &device_create_info, NULL,
				&dev->device);
	BHS_VK_CHECK(result, "falha ao criar VkDevice");

	vkGetDeviceQueue(dev->device, dev->graphics_family, 0,
			 &dev->graphics_queue);
	dev->present_queue = dev->graphics_queue;

	/* Cria command pool */
	VkCommandPoolCreateInfo pool_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = dev->graphics_family,
	};

	result = vkCreateCommandPool(dev->device, &pool_info, NULL,
				     &dev->command_pool);
	BHS_VK_CHECK(result, "falha ao criar command pool");

	/* Texture Descriptor Set Layout (Binding 0: Sampler) */
	VkDescriptorSetLayoutBinding sampler_layout_binding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.pImmutableSamplers = NULL,
	};

	VkDescriptorSetLayoutCreateInfo layout_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &sampler_layout_binding,
	};

	if (vkCreateDescriptorSetLayout(dev->device, &layout_info, NULL,
					&dev->texture_layout) != VK_SUCCESS) {
		vkDestroyCommandPool(dev->device, dev->command_pool, NULL);
		vkDestroyDevice(dev->device, NULL);
		vkDestroyInstance(dev->instance, NULL);
		free(dev);
		return BHS_GPU_ERR_DEVICE;
	}

	*device = dev;
	return BHS_GPU_OK;
}

void bhs_gpu_device_destroy(bhs_gpu_device_t device)
{
	if (!device)
		return;

	if (device->texture_layout)
		vkDestroyDescriptorSetLayout(device->device,
					     device->texture_layout, NULL);

	if (device->command_pool)
		vkDestroyCommandPool(device->device, device->command_pool,
				     NULL);
	vkDestroyDevice(device->device, NULL);
	vkDestroyInstance(device->instance, NULL);
	free(device);
}

enum bhs_gpu_backend bhs_gpu_device_get_backend(bhs_gpu_device_t device)
{
	(void)device;
	return BHS_GPU_BACKEND_VULKAN;
}

const char *bhs_gpu_device_get_name(bhs_gpu_device_t device)
{
	if (!device)
		return "Unknown";
	return device->properties.deviceName;
}
