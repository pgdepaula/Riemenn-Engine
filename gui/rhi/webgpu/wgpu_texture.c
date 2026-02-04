#include "gui/rhi/webgpu/wgpu_internal.h"

int bhs_gpu_texture_create(bhs_gpu_device_t device,
			   const struct bhs_gpu_texture_config *config,
			   bhs_gpu_texture_t *texture)
{
	struct bhs_gpu_device_impl *dev = device;
	struct bhs_gpu_texture_impl *t = calloc(1, sizeof(*t));

	t->width = config->width;
	t->height = config->height;
	t->format = WGPUTextureFormat_RGBA8Unorm;
	t->device = dev->device;

	WGPUTextureDescriptor desc = { .usage =
					       WGPUTextureUsage_TextureBinding |
					       WGPUTextureUsage_CopyDst,
				       .dimension = WGPUTextureDimension_2D,
				       .size = { .width = config->width,
						 .height = config->height,
						 .depthOrArrayLayers = 1 },
				       .format = t->format,
				       .mipLevelCount = 1,
				       .sampleCount = 1,
				       .label = config->label };

	if (config->usage & BHS_TEXTURE_RENDER_TARGET)
		desc.usage |= WGPUTextureUsage_RenderAttachment;

	t->texture = wgpuDeviceCreateTexture(dev->device, &desc);
	WGPU_CHECK(t->texture);

	WGPUTextureViewDescriptor viewDesc = {
		.format = t->format,
		.dimension = WGPUTextureViewDimension_2D,
		.baseMipLevel = 0,
		.mipLevelCount = 1,
		.baseArrayLayer = 0,
		.arrayLayerCount = 1,
		.aspect = WGPUTextureAspect_All
	};
	t->default_view = wgpuTextureCreateView(t->texture, &viewDesc);

	*texture = t;
	return BHS_GPU_OK;
}

void bhs_gpu_texture_destroy(bhs_gpu_texture_t t)
{
	if (t) {
		wgpuTextureViewRelease(t->default_view);
		wgpuTextureDestroy(t->texture);
		wgpuTextureRelease(t->texture);
		free(t);
	}
}

int bhs_gpu_texture_upload(bhs_gpu_texture_t t, uint32_t mip_level,
			   uint32_t array_layer, const void *data, size_t size)
{
	if (!t || !t->device)
		return BHS_GPU_ERR_INVALID;

	WGPUQueue queue = wgpuDeviceGetQueue(t->device);

	WGPUImageCopyTexture destination = { .texture = t->texture,
					     .mipLevel = mip_level,
					     .origin = { 0, 0, 0 },
					     .aspect = WGPUTextureAspect_All };

	uint32_t bytesPerPixel = 4;
	uint32_t bytesPerRow = t->width * bytesPerPixel;

	WGPUTextureDataLayout layout = { .offset = 0,
					 .bytesPerRow = bytesPerRow,
					 .rowsPerImage = t->height };

	WGPUExtent3D writeSize = { .width = t->width,
				   .height = t->height,
				   .depthOrArrayLayers = 1 };

	wgpuQueueWriteTexture(queue, &destination, data, size, &layout,
			      &writeSize);
	return BHS_GPU_OK;
}

int bhs_gpu_sampler_create(bhs_gpu_device_t device,
			   const struct bhs_gpu_sampler_config *config,
			   bhs_gpu_sampler_t *sampler)
{
	struct bhs_gpu_device_impl *dev = device;
	struct bhs_gpu_sampler_impl *s = calloc(1, sizeof(*s));

	WGPUSamplerDescriptor desc = {
		.addressModeU = WGPUAddressMode_ClampToEdge,
		.addressModeV = WGPUAddressMode_ClampToEdge,
		.addressModeW = WGPUAddressMode_ClampToEdge,
		.magFilter = WGPUFilterMode_Linear,
		.minFilter = WGPUFilterMode_Linear,
		.mipmapFilter = WGPUMipmapFilterMode_Linear,
		.lodMinClamp = 0.0f,
		.lodMaxClamp = 100.0f,
		.maxAnisotropy = 1
	};

	s->sampler = wgpuDeviceCreateSampler(dev->device, &desc);
	WGPU_CHECK(s->sampler);

	*sampler = s;
	return BHS_GPU_OK;
}

void bhs_gpu_sampler_destroy(bhs_gpu_sampler_t s)
{
	if (s) {
		wgpuSamplerRelease(s->sampler);
		free(s);
	}
}
