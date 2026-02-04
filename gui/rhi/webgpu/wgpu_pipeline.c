#include <string.h>
#include "gui/rhi/webgpu/wgpu_internal.h"

int bhs_gpu_shader_create(bhs_gpu_device_t device,
			  const struct bhs_gpu_shader_config *config,
			  bhs_gpu_shader_t *shader)
{
	struct bhs_gpu_device_impl *dev = device;
	struct bhs_gpu_shader_impl *s = calloc(1, sizeof(*s));

	s->entry_point =
		strdup(config->entry_point ? config->entry_point : "main");

	WGPUShaderModuleWGSLDescriptor wgslDesc = {
		.chain = { .sType = WGPUSType_ShaderModuleWGSLDescriptor },
		.code = (const char *)config->code
	};

	WGPUShaderModuleDescriptor desc = {
		.nextInChain = (const WGPUChainedStruct *)&wgslDesc,
		.label = config->label
	};

	s->module = wgpuDeviceCreateShaderModule(dev->device, &desc);
	WGPU_CHECK(s->module);

	*shader = s;
	return BHS_GPU_OK;
}

void bhs_gpu_shader_destroy(bhs_gpu_shader_t s)
{
	if (s) {
		wgpuShaderModuleRelease(s->module);
		free(s->entry_point);
		free(s);
	}
}

int bhs_gpu_pipeline_create(bhs_gpu_device_t device,
			    const struct bhs_gpu_pipeline_config *config,
			    bhs_gpu_pipeline_t *pipeline)
{
	struct bhs_gpu_device_impl *dev = device;
	struct bhs_gpu_pipeline_impl *p = calloc(1, sizeof(*p));

	WGPUPipelineLayoutDescriptor layoutDesc = { .bindGroupLayoutCount = 0 };
	p->layout = wgpuDeviceCreatePipelineLayout(dev->device, &layoutDesc);

	WGPUColorTargetState colorTarget = {
		.format = config->color_formats[0] == BHS_FORMAT_BGRA8_UNORM
				  ? WGPUTextureFormat_BGRA8Unorm
				  : WGPUTextureFormat_RGBA8Unorm,
		.writeMask = WGPUColorWriteMask_All
	};

	WGPUFragmentState fragment = {
		.module = config->fragment_shader->module,
		.entryPoint = config->fragment_shader->entry_point,
		.targetCount = 1,
		.targets = &colorTarget
	};

	WGPURenderPipelineDescriptor desc = {
		.layout = p->layout,
		.vertex = { .module = config->vertex_shader->module,
			    .entryPoint = config->vertex_shader->entry_point,
			    .bufferCount = 0 },
		.fragment = &fragment,
		.primitive = { .topology = WGPUPrimitiveTopology_TriangleList,
			       .cullMode = WGPUCullMode_None },
		.multisample = { .count = 1, .mask = 0xFFFFFFFF },
		.label = config->label
	};

	p->render_pipeline = wgpuDeviceCreateRenderPipeline(dev->device, &desc);
	WGPU_CHECK(p->render_pipeline);

	p->bind_group_layout =
		wgpuRenderPipelineGetBindGroupLayout(p->render_pipeline, 0);

	*pipeline = p;
	return BHS_GPU_OK;
}

void bhs_gpu_pipeline_destroy(bhs_gpu_pipeline_t p)
{
	if (p) {
		wgpuRenderPipelineRelease(p->render_pipeline);
		wgpuPipelineLayoutRelease(p->layout);
		free(p);
	}
}

int bhs_gpu_pipeline_compute_create(
	bhs_gpu_device_t d, const struct bhs_gpu_compute_pipeline_config *c,
	bhs_gpu_pipeline_t *p)
{
	return BHS_GPU_ERR_UNSUPPORTED;
}
