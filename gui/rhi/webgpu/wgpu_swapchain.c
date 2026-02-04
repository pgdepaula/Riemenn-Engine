#include "gui/rhi/webgpu/wgpu_internal.h"

int bhs_gpu_swapchain_create(bhs_gpu_device_t device,
			     const struct bhs_gpu_swapchain_config *config,
			     bhs_gpu_swapchain_t *swapchain)
{
	struct bhs_gpu_device_impl *dev = device;
	struct bhs_gpu_swapchain_impl *sc = calloc(1, sizeof(*sc));
	sc->device = dev->device;
	sc->width = config->width;
	sc->height = config->height;

	WGPUSurfaceDescriptorFromCanvasHTML5Selector canvasDesc = {
		.chain = { .sType =
				   WGPUSType_SurfaceDescriptorFromCanvasHTML5Selector },
		.selector = (const char *)config->native_window
	};
	WGPUSurfaceDescriptor surfaceDesc = {
		.nextInChain = (const WGPUChainedStruct *)&canvasDesc
	};
	sc->surface = wgpuInstanceCreateSurface(dev->instance, &surfaceDesc);

	sc->format = WGPUTextureFormat_BGRA8Unorm;
	if (config->format == BHS_FORMAT_RGBA8_UNORM)
		sc->format = WGPUTextureFormat_RGBA8Unorm;

	WGPUSurfaceConfiguration surfConfig = {
		.device = dev->device,
		.format = sc->format,
		.usage = WGPUTextureUsage_RenderAttachment,
		.width = sc->width,
		.height = sc->height,
		.presentMode = config->vsync ? WGPUPresentMode_Fifo
					     : WGPUPresentMode_Mailbox,
		.alphaMode = WGPUCompositeAlphaMode_Auto
	};
	wgpuSurfaceConfigure(sc->surface, &surfConfig);

	*swapchain = sc;
	return BHS_GPU_OK;
}

int bhs_gpu_swapchain_next_texture(bhs_gpu_swapchain_t swapchain,
				   bhs_gpu_texture_t *texture)
{
	struct bhs_gpu_swapchain_impl *sc = swapchain;
	WGPUSurfaceTexture surfaceTex;
	wgpuSurfaceGetCurrentTexture(sc->surface, &surfaceTex);

	if (surfaceTex.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
		return BHS_GPU_ERR_SWAPCHAIN;
	}

	sc->current_texture_wrapper.texture = surfaceTex.texture;
	sc->current_texture_wrapper.width = sc->width;
	sc->current_texture_wrapper.height = sc->height;
	sc->current_texture_wrapper.format = sc->format;
	sc->current_texture_wrapper.device = sc->device;

	WGPUTextureViewDescriptor viewDesc = {
		.format = sc->format,
		.dimension = WGPUTextureViewDimension_2D,
		.baseMipLevel = 0,
		.mipLevelCount = 1,
		.baseArrayLayer = 0,
		.arrayLayerCount = 1,
		.aspect = WGPUTextureAspect_All
	};
	sc->current_texture_wrapper.default_view =
		wgpuTextureCreateView(surfaceTex.texture, &viewDesc);

	*texture = &sc->current_texture_wrapper;
	return BHS_GPU_OK;
}

int bhs_gpu_swapchain_present(bhs_gpu_swapchain_t swapchain)
{
	return BHS_GPU_OK;
}

void bhs_gpu_swapchain_destroy(bhs_gpu_swapchain_t s)
{
	if (s) {
		wgpuSurfaceRelease(s->surface);
		free(s);
	}
}
int bhs_gpu_swapchain_resize(bhs_gpu_swapchain_t s, uint32_t w, uint32_t h)
{
	WGPUSurfaceConfiguration surfConfig = {
		.device = s->device,
		.format = s->format,
		.usage = WGPUTextureUsage_RenderAttachment,
		.width = w,
		.height = h,
		.alphaMode = WGPUCompositeAlphaMode_Auto
	};
	wgpuSurfaceConfigure(s->surface, &surfConfig);
	s->width = w;
	s->height = h;
	return BHS_GPU_OK;
}
