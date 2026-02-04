/**
 * @file vk_texture.c
 * @brief Gerenciamento de Texturas Vulkan
 */

#include <stdlib.h>
#include "gui/rhi/vulkan/vk_internal.h"

/* ============================================================================
 * TEXTURAS
 * ============================================================================
 */

int bhs_gpu_texture_create(bhs_gpu_device_t device,
			   const struct bhs_gpu_texture_config *config,
			   bhs_gpu_texture_t *texture)
{
	if (!device || !config || !texture)
		return BHS_GPU_ERR_INVALID;

	struct bhs_gpu_texture_impl *tex = calloc(1, sizeof(*tex));
	if (!tex)
		return BHS_GPU_ERR_NOMEM;

	tex->device = device;
	tex->width = config->width;
	tex->height = config->height;
	tex->format = bhs_vk_format(config->format);
	tex->owns_image = true;

	VkImageUsageFlags usage = 0;
	if (config->usage & BHS_TEXTURE_SAMPLED)
		usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (config->usage & BHS_TEXTURE_STORAGE)
		usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	if (config->usage & BHS_TEXTURE_RENDER_TARGET)
		usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (config->usage & BHS_TEXTURE_DEPTH_STENCIL)
		usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

	VkImageCreateInfo image_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = tex->format,
		.extent = { config->width, config->height, 1 },
		.mipLevels = config->mip_levels > 0 ? config->mip_levels : 1,
		.arrayLayers =
			config->array_layers > 0 ? config->array_layers : 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VkResult result =
		vkCreateImage(device->device, &image_info, NULL, &tex->image);
	if (result != VK_SUCCESS) {
		free(tex);
		return BHS_GPU_ERR_NOMEM;
	}

	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(device->device, tex->image, &mem_reqs);

	uint32_t mem_type =
		bhs_vk_find_memory_type(device, mem_reqs.memoryTypeBits,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = mem_reqs.size,
		.memoryTypeIndex = mem_type,
	};

	result = vkAllocateMemory(device->device, &alloc_info, NULL,
				  &tex->memory);
	if (result != VK_SUCCESS) {
		vkDestroyImage(device->device, tex->image, NULL);
		free(tex);
		return BHS_GPU_ERR_NOMEM;
	}

	vkBindImageMemory(device->device, tex->image, tex->memory, 0);

	/* Cria ImageView */
	VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	if (config->usage & BHS_TEXTURE_DEPTH_STENCIL)
		aspect = VK_IMAGE_ASPECT_DEPTH_BIT;

	VkImageViewCreateInfo view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = tex->image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = tex->format,
      .subresourceRange =
          {
              .aspectMask = aspect,
              .baseMipLevel = 0,
              .levelCount = image_info.mipLevels,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };

	result =
		vkCreateImageView(device->device, &view_info, NULL, &tex->view);
	if (result != VK_SUCCESS) {
		vkFreeMemory(device->device, tex->memory, NULL);
		vkDestroyImage(device->device, tex->image, NULL);
		free(tex);
		return BHS_GPU_ERR_DEVICE;
	}

	*texture = tex;
	return BHS_GPU_OK;
}

void bhs_gpu_texture_destroy(bhs_gpu_texture_t texture)
{
	if (!texture)
		return;

	if (texture->view)
		vkDestroyImageView(texture->device->device, texture->view,
				   NULL);

	if (texture->owns_image) {
		if (texture->image)
			vkDestroyImage(texture->device->device, texture->image,
				       NULL);
		if (texture->memory)
			vkFreeMemory(texture->device->device, texture->memory,
				     NULL);
	}

	free(texture);
}

/* Helper para transição de layout de imagem */
static void bhs_vk_transition_layout(VkCommandBuffer cmd, VkImage image,
				     VkFormat format, VkImageLayout old_layout,
				     VkImageLayout new_layout)
{
	(void)format; /* Podemos precisar no futuro para depth/stencil */

	VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };

	VkPipelineStageFlags source_stage;
	VkPipelineStageFlags dest_stage;

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
	    new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dest_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
		   new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dest_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else {
		/* Layout transition não suportada pelo helper simplificado */
		return;
	}

	vkCmdPipelineBarrier(cmd, source_stage, dest_stage, 0, 0, NULL, 0, NULL,
			     1, &barrier);
}

int bhs_gpu_texture_upload(bhs_gpu_texture_t texture, uint32_t mip_level,
			   uint32_t array_layer, const void *data, size_t size)
{
	if (!texture || !data)
		return BHS_GPU_ERR_INVALID;

	struct bhs_gpu_device_impl *dev = texture->device;
	VkDevice device = dev->device;

	/* 1. Criar Staging Buffer */
	bhs_gpu_buffer_t staging_buf;
	struct bhs_gpu_buffer_config buf_config = {
		.size = size,
		.usage = BHS_BUFFER_TRANSFER_SRC,
		.memory = BHS_MEMORY_CPU_TO_GPU,
		.label = "Staging Texture Upload",
	};

	if (bhs_gpu_buffer_create(dev, &buf_config, &staging_buf) !=
	    BHS_GPU_OK) {
		return BHS_GPU_ERR_NOMEM;
	}

	/* 2. Copiar dados para o buffer */
	if (bhs_gpu_buffer_upload(staging_buf, 0, data, size) != BHS_GPU_OK) {
		bhs_gpu_buffer_destroy(staging_buf);
		return BHS_GPU_ERR_DEVICE;
	}

	/* 3. Alocar Command Buffer temporário */
	VkCommandBufferAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = dev->command_pool,
		.commandBufferCount = 1,
	};

	VkCommandBuffer cmd;
	vkAllocateCommandBuffers(device, &alloc_info, &cmd);

	VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkBeginCommandBuffer(cmd, &begin_info);

	/* 4. Transição Undefined -> TransferDst */
	bhs_vk_transition_layout(cmd, texture->image, texture->format,
				 VK_IMAGE_LAYOUT_UNDEFINED,
				 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	/* 5. Copy Buffer to Image */
	VkBufferImageCopy region = {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .mipLevel = mip_level,
              .baseArrayLayer = array_layer,
              .layerCount = 1,
          },
      .imageOffset = {0, 0, 0},
      .imageExtent = {texture->width, texture->height, 1},
  };

	vkCmdCopyBufferToImage(cmd, staging_buf->buffer, texture->image,
			       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
			       &region);

	/* 6. Transição TransferDst -> ShaderReadOnly */
	bhs_vk_transition_layout(cmd, texture->image, texture->format,
				 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkEndCommandBuffer(cmd);

	/* 7. Submit e Wait */
	VkSubmitInfo submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd,
	};

	vkQueueSubmit(dev->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(dev->graphics_queue);

	/* 8. Cleanup */
	vkFreeCommandBuffers(device, dev->command_pool, 1, &cmd);
	bhs_gpu_buffer_destroy(staging_buf);

	return BHS_GPU_OK;
}

/* ============================================================================
 * SAMPLERS
 * ============================================================================
 */

int bhs_gpu_sampler_create(bhs_gpu_device_t device,
			   const struct bhs_gpu_sampler_config *config,
			   bhs_gpu_sampler_t *sampler)
{
	if (!device || !config || !sampler)
		return BHS_GPU_ERR_INVALID;

	struct bhs_gpu_sampler_impl *s = calloc(1, sizeof(*s));
	if (!s)
		return BHS_GPU_ERR_NOMEM;

	s->device = device;

	VkFilter filter_map[] = { VK_FILTER_NEAREST, VK_FILTER_LINEAR };
	VkSamplerAddressMode addr_map[] = {
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
	};

	VkSamplerCreateInfo sampler_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = filter_map[config->mag_filter],
		.minFilter = filter_map[config->min_filter],
		.mipmapMode = config->mip_filter == BHS_FILTER_LINEAR
				      ? VK_SAMPLER_MIPMAP_MODE_LINEAR
				      : VK_SAMPLER_MIPMAP_MODE_NEAREST,
		.addressModeU = addr_map[config->address_u],
		.addressModeV = addr_map[config->address_v],
		.addressModeW = addr_map[config->address_w],
		.maxAnisotropy = config->max_anisotropy,
		.anisotropyEnable = config->max_anisotropy > 0,
		.maxLod = VK_LOD_CLAMP_NONE,
	};

	VkResult result = vkCreateSampler(device->device, &sampler_info, NULL,
					  &s->sampler);
	if (result != VK_SUCCESS) {
		free(s);
		return BHS_GPU_ERR_DEVICE;
	}

	*sampler = s;
	return BHS_GPU_OK;
}

void bhs_gpu_sampler_destroy(bhs_gpu_sampler_t sampler)
{
	if (!sampler)
		return;
	vkDestroySampler(sampler->device->device, sampler->sampler, NULL);
	free(sampler);
}

// foram 3 semanas pra terminar isso, nem foi por preguiça pura, no meio disso
// da até dor de cabeça pura, espero q vala a pena
