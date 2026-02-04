/**
 * @file vk_sync.c
 * @brief Sincronização Vulkan (Fences, Wait Idle)
 */

#include <stdlib.h>
#include "gui/rhi/vulkan/vk_internal.h"

int bhs_gpu_fence_create(bhs_gpu_device_t device, bhs_gpu_fence_t *fence)
{
	if (!device || !fence)
		return BHS_GPU_ERR_INVALID;

	struct bhs_gpu_fence_impl *f = calloc(1, sizeof(*f));
	if (!f)
		return BHS_GPU_ERR_NOMEM;

	f->device = device;

	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = 0, /* Criado não-sinalizado */
	};

	VkResult result =
		vkCreateFence(device->device, &fence_info, NULL, &f->fence);
	if (result != VK_SUCCESS) {
		free(f);
		return BHS_GPU_ERR_DEVICE;
	}

	*fence = f;
	return BHS_GPU_OK;
}

void bhs_gpu_fence_destroy(bhs_gpu_fence_t fence)
{
	if (!fence)
		return;
	vkDestroyFence(fence->device->device, fence->fence, NULL);
	free(fence);
}

int bhs_gpu_fence_wait(bhs_gpu_fence_t fence, uint64_t timeout_ns)
{
	if (!fence)
		return BHS_GPU_ERR_INVALID;

	VkResult result = vkWaitForFences(fence->device->device, 1,
					  &fence->fence, VK_TRUE, timeout_ns);
	if (result == VK_TIMEOUT) {
		return BHS_GPU_ERR_TIMEOUT;
	} else if (result != VK_SUCCESS) {
		return BHS_GPU_ERR_DEVICE;
	}

	return BHS_GPU_OK;
}

void bhs_gpu_fence_reset(bhs_gpu_fence_t fence)
{
	if (!fence)
		return;
	vkResetFences(fence->device->device, 1, &fence->fence);
}

void bhs_gpu_wait_idle(bhs_gpu_device_t device)
{
	if (!device)
		return;
	vkDeviceWaitIdle(device->device);
}
