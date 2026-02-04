/**
 * @file vk_buffer.c
 * @brief Gerenciamento de Buffers Vulkan
 */

#include <stdlib.h>
#include <string.h>
#include "gui/rhi/vulkan/vk_internal.h"

int bhs_gpu_buffer_create(bhs_gpu_device_t device,
			  const struct bhs_gpu_buffer_config *config,
			  bhs_gpu_buffer_t *buffer)
{
	if (!device || !config || !buffer)
		return BHS_GPU_ERR_INVALID;

	struct bhs_gpu_buffer_impl *buf = calloc(1, sizeof(*buf));
	if (!buf)
		return BHS_GPU_ERR_NOMEM;

	buf->device = device;
	buf->size = config->size;
	buf->usage = config->usage;

	VkBufferUsageFlags usage = 0;
	if (config->usage & BHS_BUFFER_VERTEX)
		usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (config->usage & BHS_BUFFER_INDEX)
		usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if (config->usage & BHS_BUFFER_UNIFORM)
		usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if (config->usage & BHS_BUFFER_STORAGE)
		usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	if (config->usage & BHS_BUFFER_TRANSFER_SRC)
		usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	if (config->usage & BHS_BUFFER_TRANSFER_DST)
		usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = config->size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VkResult result = vkCreateBuffer(device->device, &buffer_info, NULL,
					 &buf->buffer);
	if (result != VK_SUCCESS) {
		free(buf);
		return BHS_GPU_ERR_NOMEM;
	}

	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(device->device, buf->buffer, &mem_reqs);

	VkMemoryPropertyFlags mem_props = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	if (config->memory == BHS_MEMORY_CPU_VISIBLE ||
	    config->memory == BHS_MEMORY_CPU_TO_GPU) {
		mem_props = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	}

	uint32_t mem_type = bhs_vk_find_memory_type(
		device, mem_reqs.memoryTypeBits, mem_props);

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = mem_reqs.size,
		.memoryTypeIndex = mem_type,
	};

	result = vkAllocateMemory(device->device, &alloc_info, NULL,
				  &buf->memory);
	if (result != VK_SUCCESS) {
		vkDestroyBuffer(device->device, buf->buffer, NULL);
		free(buf);
		return BHS_GPU_ERR_NOMEM;
	}

	vkBindBufferMemory(device->device, buf->buffer, buf->memory, 0);

	*buffer = buf;
	return BHS_GPU_OK;
}

void bhs_gpu_buffer_destroy(bhs_gpu_buffer_t buffer)
{
	if (!buffer)
		return;

	if (buffer->mapped)
		vkUnmapMemory(buffer->device->device, buffer->memory);

	vkDestroyBuffer(buffer->device->device, buffer->buffer, NULL);
	vkFreeMemory(buffer->device->device, buffer->memory, NULL);
	free(buffer);
}

void *bhs_gpu_buffer_map(bhs_gpu_buffer_t buffer)
{
	if (!buffer || buffer->mapped)
		return buffer ? buffer->mapped : NULL;

	VkResult result = vkMapMemory(buffer->device->device, buffer->memory, 0,
				      buffer->size, 0, &buffer->mapped);
	if (result != VK_SUCCESS)
		return NULL;

	return buffer->mapped;
}

void bhs_gpu_buffer_unmap(bhs_gpu_buffer_t buffer)
{
	if (!buffer || !buffer->mapped)
		return;

	vkUnmapMemory(buffer->device->device, buffer->memory);
	buffer->mapped = NULL;
}

int bhs_gpu_buffer_upload(bhs_gpu_buffer_t buffer, uint64_t offset,
			  const void *data, uint64_t size)
{
	if (!buffer || !data)
		return BHS_GPU_ERR_INVALID;

	void *mapped = bhs_gpu_buffer_map(buffer);
	if (!mapped)
		return BHS_GPU_ERR_DEVICE;

	memcpy((char *)mapped + offset, data, size);
	bhs_gpu_buffer_unmap(buffer);

	return BHS_GPU_OK;
}
