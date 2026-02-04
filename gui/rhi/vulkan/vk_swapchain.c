/**
 * @file vk_swapchain.c
 * @brief Gerenciamento da Swapchain Vulkan
 */

#include <stdlib.h>
#include "gui/rhi/vulkan/vk_internal.h"

/* Helper interno para limpar recursos (exceto surface/struct) */
static void _cleanup_swapchain_resources(struct bhs_gpu_swapchain_impl *sc)
{
	if (!sc || !sc->device)
		return;

	vkDeviceWaitIdle(sc->device->device);

	for (uint32_t i = 0; i < BHS_VK_MAX_FRAMES_IN_FLIGHT; i++) {
		if (sc->image_available[i]) {
			vkDestroySemaphore(sc->device->device,
					   sc->image_available[i], NULL);
			sc->image_available[i] = VK_NULL_HANDLE;
		}
		if (sc->render_finished[i]) {
			vkDestroySemaphore(sc->device->device,
					   sc->render_finished[i], NULL);
			sc->render_finished[i] = VK_NULL_HANDLE;
		}
	}

	for (uint32_t i = 0; i < sc->image_count; i++) {
		if (sc->framebuffers[i]) {
			vkDestroyFramebuffer(sc->device->device,
					     sc->framebuffers[i], NULL);
			sc->framebuffers[i] = VK_NULL_HANDLE;
		}
		if (sc->views[i]) {
			vkDestroyImageView(sc->device->device, sc->views[i],
					   NULL);
			sc->views[i] = VK_NULL_HANDLE;
		}
	}

	/* Limpa Depth Buffer */
	if (sc->depth_view) {
		vkDestroyImageView(sc->device->device, sc->depth_view, NULL);
		sc->depth_view = VK_NULL_HANDLE;
	}
	if (sc->depth_image) {
		vkDestroyImage(sc->device->device, sc->depth_image, NULL);
		sc->depth_image = VK_NULL_HANDLE;
	}
	if (sc->depth_memory) {
		vkFreeMemory(sc->device->device, sc->depth_memory, NULL);
		sc->depth_memory = VK_NULL_HANDLE;
	}

	if (sc->render_pass) {
		vkDestroyRenderPass(sc->device->device, sc->render_pass, NULL);
		sc->render_pass = VK_NULL_HANDLE;
	}

	if (sc->swapchain) {
		vkDestroySwapchainKHR(sc->device->device, sc->swapchain, NULL);
		sc->swapchain = VK_NULL_HANDLE;
	}
}

/* Helper interno para criar recursos */
static int _create_swapchain_resources(struct bhs_gpu_swapchain_impl *sc,
				       uint32_t width, uint32_t height,
				       bool vsync)
{
	bhs_gpu_device_t device = sc->device;

	/* Verificações de Superfície */
	VkSurfaceCapabilitiesKHR caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device,
						  sc->surface, &caps);

	VkExtent2D extent = { width, height };
	if (caps.currentExtent.width != UINT32_MAX) {
		extent = caps.currentExtent;
	}

	sc->extent = extent;
	/* Re-check formato se necessário, mas assumindo fixo por enquanto */

	uint32_t image_count = BHS_VK_MAX_SWAPCHAIN_IMAGES;
	if (image_count < caps.minImageCount)
		image_count = caps.minImageCount;
	if (caps.maxImageCount > 0 && image_count > caps.maxImageCount)
		image_count = caps.maxImageCount;

	/* Configuração Swapchain */
	VkSwapchainCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = sc->surface,
		.minImageCount = image_count,
		.imageFormat = sc->format,
		.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = sc->extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.preTransform = caps.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = vsync ? VK_PRESENT_MODE_FIFO_KHR
				     : VK_PRESENT_MODE_IMMEDIATE_KHR,
		.clipped = VK_TRUE,
		.oldSwapchain =
			VK_NULL_HANDLE, /* Poderíamos usar para otimização */
	};

	if (vkCreateSwapchainKHR(device->device, &create_info, NULL,
				 &sc->swapchain) != VK_SUCCESS) {
		return BHS_GPU_ERR_SWAPCHAIN;
	}

	/* Imagens */
	vkGetSwapchainImagesKHR(device->device, sc->swapchain, &sc->image_count,
				NULL);
	if (sc->image_count > BHS_VK_MAX_SWAPCHAIN_IMAGES)
		sc->image_count = BHS_VK_MAX_SWAPCHAIN_IMAGES;
	vkGetSwapchainImagesKHR(device->device, sc->swapchain, &sc->image_count,
				sc->images);

	/* Views */
	for (uint32_t i = 0; i < sc->image_count; i++) {
		VkImageViewCreateInfo view_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = sc->images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = sc->format,
			.subresourceRange = { .aspectMask =
						      VK_IMAGE_ASPECT_COLOR_BIT,
					      .baseMipLevel = 0,
					      .levelCount = 1,
					      .baseArrayLayer = 0,
					      .layerCount = 1 },
		};
		if (vkCreateImageView(device->device, &view_info, NULL,
				      &sc->views[i]) != VK_SUCCESS) {
			return BHS_GPU_ERR_DEVICE;
		}

		/* Update Wrappers */
		sc->texture_wrappers[i].device = device;
		sc->texture_wrappers[i].image = sc->images[i];
		sc->texture_wrappers[i].view = sc->views[i];
		sc->texture_wrappers[i].width = sc->extent.width;
		sc->texture_wrappers[i].height = sc->extent.height;
		sc->texture_wrappers[i].format = sc->format;
		sc->texture_wrappers[i].owns_image = false;
	}

	/* =========================================================================
   * DEPTH BUFFER - Porque profundidade importa, caralho!
   * ========================================================================= */
	sc->depth_format = VK_FORMAT_D32_SFLOAT;

	VkImageCreateInfo depth_image_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = sc->depth_format,
		.extent = { sc->extent.width, sc->extent.height, 1 },
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	if (vkCreateImage(device->device, &depth_image_info, NULL,
			  &sc->depth_image) != VK_SUCCESS) {
		return BHS_GPU_ERR_DEVICE;
	}

	/* Aloca memória pro depth buffer */
	VkMemoryRequirements mem_reqs;
	vkGetImageMemoryRequirements(device->device, sc->depth_image,
				     &mem_reqs);

	VkMemoryAllocateInfo alloc_info = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = mem_reqs.size,
		.memoryTypeIndex = bhs_vk_find_memory_type(
			device, mem_reqs.memoryTypeBits,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
	};

	if (vkAllocateMemory(device->device, &alloc_info, NULL,
			     &sc->depth_memory) != VK_SUCCESS) {
		return BHS_GPU_ERR_NOMEM;
	}

	vkBindImageMemory(device->device, sc->depth_image, sc->depth_memory, 0);

	/* View pro depth buffer */
	VkImageViewCreateInfo depth_view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = sc->depth_image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = sc->depth_format,
      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
  };

	if (vkCreateImageView(device->device, &depth_view_info, NULL,
			      &sc->depth_view) != VK_SUCCESS) {
		return BHS_GPU_ERR_DEVICE;
	}

	/* =========================================================================
   * RENDER PASS - Agora com depth attachment!
   * ========================================================================= */
	VkAttachmentDescription attachments[2] = {
		/* [0] Color Attachment */
		{
			.format = sc->format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		},
		/* [1] Depth Attachment */
		{
			.format = sc->depth_format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout =
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		},
	};

	VkAttachmentReference color_attachment_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference depth_attachment_ref = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref,
		.pDepthStencilAttachment = &depth_attachment_ref,
	};

	/* Dependência para garantir que depth buffer está pronto */
	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
				 VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	};

	VkRenderPassCreateInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 2,
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency,
	};

	if (vkCreateRenderPass(device->device, &render_pass_info, NULL,
			       &sc->render_pass) != VK_SUCCESS) {
		return BHS_GPU_ERR_DEVICE;
	}

	/* =========================================================================
   * FRAMEBUFFERS - Agora com 2 attachments (cor + depth)
   * ========================================================================= */
	for (uint32_t i = 0; i < sc->image_count; i++) {
		VkImageView fb_attachments[] = { sc->views[i], sc->depth_view };
		VkFramebufferCreateInfo framebuffer_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = sc->render_pass,
			.attachmentCount = 2,
			.pAttachments = fb_attachments,
			.width = sc->extent.width,
			.height = sc->extent.height,
			.layers = 1,
		};
		if (vkCreateFramebuffer(device->device, &framebuffer_info, NULL,
					&sc->framebuffers[i]) != VK_SUCCESS) {
			return BHS_GPU_ERR_DEVICE;
		}
	}

	/* Semáforos */
	VkSemaphoreCreateInfo sem_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};
	for (uint32_t i = 0; i < BHS_VK_MAX_FRAMES_IN_FLIGHT; i++) {
		vkCreateSemaphore(device->device, &sem_info, NULL,
				  &sc->image_available[i]);
		vkCreateSemaphore(device->device, &sem_info, NULL,
				  &sc->render_finished[i]);
	}

	sc->current_frame = 0;
	return BHS_GPU_OK;
}

int bhs_gpu_swapchain_create(bhs_gpu_device_t device,
			     const struct bhs_gpu_swapchain_config *config,
			     bhs_gpu_swapchain_t *swapchain)
{
	if (!device || !config || !swapchain || !config->native_window)
		return BHS_GPU_ERR_INVALID;

	struct bhs_gpu_swapchain_impl *sc = calloc(1, sizeof(*sc));
	if (!sc)
		return BHS_GPU_ERR_NOMEM;

	sc->device = device;

	/* 1. Cria Surface (Única para a vida do swapchain obj) */
	VkWaylandSurfaceCreateInfoKHR surface_info = {
		.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
		.display = config->native_display,
		.surface = config->native_window,
	};

	if (vkCreateWaylandSurfaceKHR(device->instance, &surface_info, NULL,
				      &sc->surface) != VK_SUCCESS) {
		free(sc);
		return BHS_GPU_ERR_SWAPCHAIN;
	}

	/* Verifica suporte básico */
	VkBool32 supported;
	vkGetPhysicalDeviceSurfaceSupportKHR(device->physical_device,
					     device->present_family,
					     sc->surface, &supported);
	if (!supported) {
		vkDestroySurfaceKHR(device->instance, sc->surface, NULL);
		free(sc);
		return BHS_GPU_ERR_SWAPCHAIN;
	}

	/* Define formato fixo por enquanto (SRGB) */
	sc->format = bhs_vk_format(config->format);

	/* 2. Cria Swapchain e Recursos */
	if (_create_swapchain_resources(sc, config->width, config->height,
					config->vsync) != BHS_GPU_OK) {
		_cleanup_swapchain_resources(sc);
		vkDestroySurfaceKHR(device->instance, sc->surface, NULL);
		free(sc);
		return BHS_GPU_ERR_DEVICE;
	}

	*swapchain = sc;
	device->swapchain =
		sc; /* HACK mantido (necessário para render pass atual) */
	return BHS_GPU_OK;
}

void bhs_gpu_swapchain_destroy(bhs_gpu_swapchain_t swapchain)
{
	if (!swapchain)
		return;

	if (swapchain->device->swapchain == swapchain) {
		swapchain->device->swapchain = NULL;
	}

	_cleanup_swapchain_resources(swapchain);

	if (swapchain->surface)
		vkDestroySurfaceKHR(swapchain->device->instance,
				    swapchain->surface, NULL);

	free(swapchain);
}

int bhs_gpu_swapchain_resize(bhs_gpu_swapchain_t swapchain, uint32_t width,
			     uint32_t height)
{
	if (!swapchain)
		return BHS_GPU_ERR_INVALID;

	if (width == 0 || height == 0)
		return BHS_GPU_OK; /* Minimizado */

	vkDeviceWaitIdle(swapchain->device->device);

	/* Limpa recursos antigos mas MANTÉM surface e struct */
	_cleanup_swapchain_resources(swapchain);

	/* Recria */
	/* Nota: Assumindo VSync ativado por padrão no resize por simplificação,
     idealmente deveria guardar config */
	return _create_swapchain_resources(swapchain, width, height, true);
}

int bhs_gpu_swapchain_next_texture(bhs_gpu_swapchain_t swapchain,
				   bhs_gpu_texture_t *texture)
{
	if (!swapchain || !texture)
		return BHS_GPU_ERR_INVALID;

	uint32_t frame = swapchain->current_frame;
	VkResult result = vkAcquireNextImageKHR(
		swapchain->device->device, swapchain->swapchain, 100000000,
		swapchain->image_available[frame], VK_NULL_HANDLE,
		&swapchain->current_image);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		/* Caller deve tratar chamando resize */
		return BHS_GPU_ERR_SWAPCHAIN_RESIZE;
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		return BHS_GPU_ERR_DEVICE;
	}

	*texture = &swapchain->texture_wrappers[swapchain->current_image];
	return BHS_GPU_OK;
}

int bhs_gpu_swapchain_submit(bhs_gpu_swapchain_t swapchain,
			     bhs_gpu_cmd_buffer_t cmd, bhs_gpu_fence_t fence)
{
	if (!swapchain || !cmd)
		return BHS_GPU_ERR_INVALID;

	VkSemaphore waitSemaphores[] = {
		swapchain->image_available[swapchain->current_frame]
	};
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	VkSemaphore signalSemaphores[] = {
		swapchain->render_finished[swapchain->current_frame]
	};

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd->cmd,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores,
	};

	VkFence vk_fence = VK_NULL_HANDLE;
	if (fence) {
		vk_fence = ((struct bhs_gpu_fence_impl *)fence)->fence;
	}

	if (vkQueueSubmit(swapchain->device->graphics_queue, 1, &submitInfo,
			  vk_fence) != VK_SUCCESS) {
		return BHS_GPU_ERR_DEVICE;
	}
	return BHS_GPU_OK;
}

int bhs_gpu_swapchain_present(bhs_gpu_swapchain_t swapchain)
{
	if (!swapchain)
		return BHS_GPU_ERR_INVALID;

	VkSemaphore signalSemaphores[] = {
		swapchain->render_finished[swapchain->current_frame]
	};
	VkSwapchainKHR swapchains[] = { swapchain->swapchain };

	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = swapchains,
		.pImageIndices = &swapchain->current_image,
	};

	VkResult result = vkQueuePresentKHR(swapchain->device->present_queue,
					    &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		return BHS_GPU_ERR_SWAPCHAIN_RESIZE;
	} else if (result != VK_SUCCESS) {
		return BHS_GPU_ERR_DEVICE;
	}

	swapchain->current_frame =
		(swapchain->current_frame + 1) % BHS_VK_MAX_FRAMES_IN_FLIGHT;

	return BHS_GPU_OK;
}
