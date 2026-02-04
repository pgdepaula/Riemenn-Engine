#include "gui/rhi/webgpu/wgpu_internal.h"

int bhs_gpu_buffer_create(bhs_gpu_device_t device,
			  const struct bhs_gpu_buffer_config *config,
			  bhs_gpu_buffer_t *buffer)
{
	struct bhs_gpu_device_impl *dev = device;
	if (!dev->device)
		return BHS_GPU_ERR_DEVICE;

	struct bhs_gpu_buffer_impl *b = calloc(1, sizeof(*b));
	b->size = config->size;
	b->device = dev->device;

	WGPUBufferDescriptor desc = { .usage = wgpu_map_buffer_usage(
						       config->usage) |
					       WGPUBufferUsage_CopyDst,
				      .size = config->size,
				      .mappedAtCreation = false,
				      .label = config->label };

	b->buffer = wgpuDeviceCreateBuffer(dev->device, &desc);
	WGPU_CHECK(b->buffer);

	*buffer = b;
	return BHS_GPU_OK;
}

void bhs_gpu_buffer_destroy(bhs_gpu_buffer_t buffer)
{
	if (buffer) {
		wgpuBufferDestroy(buffer->buffer);
		wgpuBufferRelease(buffer->buffer);
		free(buffer);
	}
}

int bhs_gpu_buffer_upload(bhs_gpu_buffer_t buffer, uint64_t offset,
			  const void *data, uint64_t size)
{
	if (!buffer || !buffer->device)
		return BHS_GPU_ERR_INVALID;
	WGPUQueue queue = wgpuDeviceGetQueue(buffer->device);
	wgpuQueueWriteBuffer(queue, buffer->buffer, offset, data, size);
	return BHS_GPU_OK;
}

void *bhs_gpu_buffer_map(bhs_gpu_buffer_t buffer)
{
	return NULL;
}
void bhs_gpu_buffer_unmap(bhs_gpu_buffer_t buffer)
{
}
