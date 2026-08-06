/**
 * @file vk_sync.c
 * @brief Sincronização Vulkan (Fences, Wait Idle)
 */

#include <stdlib.h>
#include "engine/rhi/vulkan/vk_internal.h"

int ri_gpu_fence_create(ri_gpu_device_t device, ri_gpu_fence_t *fence)
{
	if (!device || !fence)
		return RI_GPU_ERR_INVALID;

	struct ri_gpu_fence_impl *f = calloc(1, sizeof(*f));
	if (!f)
		return RI_GPU_ERR_NOMEM;

	f->device = device;

	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = 0, /* Criado não-sinalizado */
	};

	VkResult result =
		vkCreateFence(device->device, &fence_info, NULL, &f->fence);
	if (result != VK_SUCCESS) {
		free(f);
		return RI_GPU_ERR_DEVICE;
	}

	*fence = f;
	return RI_GPU_OK;
}

void ri_gpu_fence_destroy(ri_gpu_fence_t fence)
{
	if (!fence)
		return;
	vkDestroyFence(fence->device->device, fence->fence, NULL);
	free(fence);
}

int ri_gpu_fence_wait(ri_gpu_fence_t fence, uint64_t timeout_ns)
{
	if (!fence)
		return RI_GPU_ERR_INVALID;

	VkResult result = vkWaitForFences(fence->device->device, 1,
					  &fence->fence, VK_TRUE, timeout_ns);
	if (result == VK_TIMEOUT) {
		return RI_GPU_ERR_TIMEOUT;
	} else if (result != VK_SUCCESS) {
		return RI_GPU_ERR_DEVICE;
	}

	return RI_GPU_OK;
}

void ri_gpu_fence_reset(ri_gpu_fence_t fence)
{
	if (!fence)
		return;
	vkResetFences(fence->device->device, 1, &fence->fence);
}

void ri_gpu_wait_idle(ri_gpu_device_t device)
{
	if (!device)
		return;
	vkDeviceWaitIdle(device->device);
}
