#include "gui/rhi/webgpu/wgpu_internal.h"

int bhs_gpu_cmd_buffer_create(bhs_gpu_device_t device,
			      bhs_gpu_cmd_buffer_t *cmd)
{
	struct bhs_gpu_cmd_buffer_impl *c = calloc(1, sizeof(*c));
	c->device = device->device;
	*cmd = c;
	return BHS_GPU_OK;
}

void bhs_gpu_cmd_begin(bhs_gpu_cmd_buffer_t cmd)
{
	if (cmd->encoder)
		wgpuCommandEncoderRelease(cmd->encoder);

	WGPUCommandEncoderDescriptor desc = {};
	cmd->encoder = wgpuDeviceCreateCommandEncoder(cmd->device, &desc);
	cmd->render_pass = NULL;

	cmd->bind_state.count = 0;
	cmd->bind_state.dirty = false;
}

void bhs_gpu_cmd_end(bhs_gpu_cmd_buffer_t cmd)
{
	if (cmd->render_pass) {
		wgpuRenderPassEncoderEnd(cmd->render_pass);
		wgpuRenderPassEncoderRelease(cmd->render_pass);
		cmd->render_pass = NULL;
	}
	if (cmd->encoder) {
		cmd->pending_cb = wgpuCommandEncoderFinish(cmd->encoder, NULL);
		wgpuCommandEncoderRelease(cmd->encoder);
		cmd->encoder = NULL;
	}
}

void bhs_gpu_cmd_reset(bhs_gpu_cmd_buffer_t cmd)
{
	if (cmd->pending_cb) {
		wgpuCommandBufferRelease(cmd->pending_cb);
		cmd->pending_cb = NULL;
	}
}

void bhs_gpu_cmd_begin_render_pass(bhs_gpu_cmd_buffer_t cmd,
				   const struct bhs_gpu_render_pass *pass)
{
	WGPURenderPassColorAttachment ca = {
		.view = pass->color_attachments[0].texture->default_view,
		.loadOp = WGPULoadOp_Clear,
		.storeOp = WGPUStoreOp_Store,
		.clearValue = { pass->color_attachments[0].clear_color[0],
				pass->color_attachments[0].clear_color[1],
				pass->color_attachments[0].clear_color[2],
				pass->color_attachments[0].clear_color[3] }
	};

	WGPURenderPassDescriptor desc = {
		.colorAttachmentCount = 1,
		.colorAttachments = &ca,
	};

	cmd->render_pass =
		wgpuCommandEncoderBeginRenderPass(cmd->encoder, &desc);
}

void bhs_gpu_cmd_end_render_pass(bhs_gpu_cmd_buffer_t cmd)
{
	if (cmd->render_pass) {
		wgpuRenderPassEncoderEnd(cmd->render_pass);
		wgpuRenderPassEncoderRelease(cmd->render_pass);
		cmd->render_pass = NULL;
	}
}

void bhs_gpu_cmd_set_pipeline(bhs_gpu_cmd_buffer_t cmd,
			      bhs_gpu_pipeline_t pipeline)
{
	cmd->current_pipeline = pipeline;
	if (cmd->render_pass) {
		wgpuRenderPassEncoderSetPipeline(cmd->render_pass,
						 pipeline->render_pipeline);
	}
}

static void flush_bind_groups(bhs_gpu_cmd_buffer_t cmd)
{
	if (!cmd->bind_state.dirty || !cmd->current_pipeline ||
	    cmd->bind_state.count == 0)
		return;

	WGPUBindGroupDescriptor desc = {
		.layout = cmd->current_pipeline->bind_group_layout,
		.entryCount = cmd->bind_state.count,
		.entries = cmd->bind_state.entries,
		.label = "TempBindGroup"
	};

	WGPUBindGroup bg = wgpuDeviceCreateBindGroup(cmd->device, &desc);

	if (cmd->render_pass) {
		wgpuRenderPassEncoderSetBindGroup(cmd->render_pass, 0, bg, 0,
						  NULL);
	}

	wgpuBindGroupRelease(bg);
	cmd->bind_state.dirty = false;
}

void bhs_gpu_cmd_draw(bhs_gpu_cmd_buffer_t cmd, uint32_t v, uint32_t i,
		      uint32_t fv, uint32_t fi)
{
	if (cmd->render_pass) {
		flush_bind_groups(cmd);
		wgpuRenderPassEncoderDraw(cmd->render_pass, v, i, fv, fi);
	}
}

void bhs_gpu_cmd_draw_indexed(bhs_gpu_cmd_buffer_t cmd, uint32_t ic, uint32_t i,
			      uint32_t fi, int32_t vo, uint32_t fis)
{
	if (cmd->render_pass) {
		flush_bind_groups(cmd);
		wgpuRenderPassEncoderDrawIndexed(cmd->render_pass, ic, i, fi,
						 vo, fis);
	}
}

void bhs_gpu_cmd_set_viewport(bhs_gpu_cmd_buffer_t c, float x, float y, float w,
			      float h, float min, float max)
{
	if (c->render_pass)
		wgpuRenderPassEncoderSetViewport(c->render_pass, x, y, w, h,
						 min, max);
}

void bhs_gpu_cmd_set_scissor(bhs_gpu_cmd_buffer_t c, int32_t x, int32_t y,
			     uint32_t w, uint32_t h)
{
	if (c->render_pass)
		wgpuRenderPassEncoderSetScissorRect(c->render_pass, x, y, w, h);
}

void bhs_gpu_cmd_bind_texture(bhs_gpu_cmd_buffer_t c, uint32_t s, uint32_t b,
			      bhs_gpu_texture_t t, bhs_gpu_sampler_t sm)
{
	if (c->bind_state.count >= MAX_BIND_ENTRIES - 1)
		return;

	WGPUBindGroupEntry *et = &c->bind_state.entries[c->bind_state.count++];
	et->binding = b;
	et->textureView = t->default_view;
	et->sampler = NULL;

	WGPUBindGroupEntry *es = &c->bind_state.entries[c->bind_state.count++];
	es->binding = b + 1;
	es->textureView = NULL;
	es->sampler = sm->sampler;

	c->bind_state.dirty = true;
}

int bhs_gpu_submit(bhs_gpu_device_t device, bhs_gpu_cmd_buffer_t cmd,
		   bhs_gpu_fence_t fence)
{
	WGPUQueue queue = wgpuDeviceGetQueue(device->device);

	if (cmd->pending_cb) {
		wgpuQueueSubmit(queue, 1, &cmd->pending_cb);
		wgpuCommandBufferRelease(cmd->pending_cb);
		cmd->pending_cb = NULL;
	}
	return BHS_GPU_OK;
}

void bhs_gpu_cmd_buffer_destroy(bhs_gpu_cmd_buffer_t c)
{
	bhs_gpu_cmd_reset(c);
	free(c);
}
void bhs_gpu_cmd_set_vertex_buffer(bhs_gpu_cmd_buffer_t c, uint32_t b,
				   bhs_gpu_buffer_t buf, uint64_t o)
{
	if (c->render_pass)
		wgpuRenderPassEncoderSetVertexBuffer(
			c->render_pass, b, buf->buffer, o, buf->size - o);
}
void bhs_gpu_cmd_set_index_buffer(bhs_gpu_cmd_buffer_t c, bhs_gpu_buffer_t buf,
				  uint64_t o, bool i32)
{
	if (c->render_pass)
		wgpuRenderPassEncoderSetIndexBuffer(
			c->render_pass, buf->buffer,
			i32 ? WGPUIndexFormat_Uint32 : WGPUIndexFormat_Uint16,
			o, buf->size - o);
}
void bhs_gpu_cmd_push_constants(bhs_gpu_cmd_buffer_t c, uint32_t o,
				const void *d, uint32_t s)
{
}
void bhs_gpu_cmd_dispatch(bhs_gpu_cmd_buffer_t c, uint32_t x, uint32_t y,
			  uint32_t z)
{
}
void bhs_gpu_cmd_transition_texture(bhs_gpu_cmd_buffer_t c, bhs_gpu_texture_t t)
{
}
void bhs_gpu_cmd_bind_compute_storage_texture(bhs_gpu_cmd_buffer_t c,
					      bhs_gpu_pipeline_t p, uint32_t s,
					      uint32_t b, bhs_gpu_texture_t t)
{
}
int bhs_gpu_fence_create(bhs_gpu_device_t d, bhs_gpu_fence_t *f)
{
	return BHS_GPU_OK;
}
void bhs_gpu_fence_destroy(bhs_gpu_fence_t f)
{
}
int bhs_gpu_fence_wait(bhs_gpu_fence_t f, uint64_t t)
{
	return BHS_GPU_OK;
}
void bhs_gpu_fence_reset(bhs_gpu_fence_t f)
{
}
void bhs_gpu_wait_idle(bhs_gpu_device_t d)
{
}
