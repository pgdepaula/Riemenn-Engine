#include <string.h>
#include "gui/rhi/webgpu/wgpu_internal.h"

static void on_adapter_request_ended(WGPURequestAdapterStatus status,
				     WGPUAdapter adapter, char const *message,
				     void *userdata)
{
	struct bhs_gpu_device_impl *d = userdata;
	if (status == WGPURequestAdapterStatus_Success) {
		d->adapter = adapter;
	} else {
		LOG_ERR("Falha ao pedir adapter: %s\n", message);
	}
	d->adapter_request_ended = true;
}

static void on_device_request_ended(WGPURequestDeviceStatus status,
				    WGPUDevice device, char const *message,
				    void *userdata)
{
	struct bhs_gpu_device_impl *d = userdata;
	if (status == WGPURequestDeviceStatus_Success) {
		d->device = device;
		d->queue = wgpuDeviceGetQueue(d->device);
	} else {
		LOG_ERR("Falha ao pedir device: %s\n", message);
	}
	d->device_request_ended = true;
}

int bhs_gpu_device_create(const struct bhs_gpu_device_config *config,
			  bhs_gpu_device_t *device)
{
	struct bhs_gpu_device_impl *d = calloc(1, sizeof(*d));
	if (!d)
		return BHS_GPU_ERR_NOMEM;

	d->instance = wgpuCreateInstance(NULL);
	if (!d->instance) {
		LOG_ERR("wgpuCreateInstance falhou.\n");
		free(d);
		return BHS_GPU_ERR_DEVICE;
	}

	WGPURequestAdapterOptions options = {
		.powerPreference = WGPUPowerPreference_HighPerformance
	};

	d->adapter_request_ended = false;
	wgpuInstanceRequestAdapter(d->instance, &options,
				   on_adapter_request_ended, d);

	int timeout = 100;
	while (!d->adapter_request_ended && timeout-- > 0) {
		emscripten_sleep(10);
	}

	if (!d->adapter) {
		LOG_ERR("Sem adapter. Abortando.\n");
		return BHS_GPU_ERR_DEVICE;
	}

	WGPUDeviceDescriptor deviceDesc = { .label = "BHS Device" };

	d->device_request_ended = false;
	wgpuAdapterRequestDevice(d->adapter, &deviceDesc,
				 on_device_request_ended, d);

	timeout = 100;
	while (!d->device_request_ended && timeout-- > 0) {
		emscripten_sleep(10);
	}

	if (!d->device)
		return BHS_GPU_ERR_DEVICE;

	*device = d;
	LOG_INFO("WebGPU Device criado com sucesso.\n");
	return BHS_GPU_OK;
}

void bhs_gpu_device_destroy(bhs_gpu_device_t device)
{
	if (device)
		free(device);
}

enum bhs_gpu_backend bhs_gpu_device_get_backend(bhs_gpu_device_t device)
{
	return BHS_GPU_BACKEND_OPENGL; // Fallback
}

const char *bhs_gpu_device_get_name(bhs_gpu_device_t device)
{
	return "WebGPU (Emscripten)";
}

WGPUBufferUsage wgpu_map_buffer_usage(uint32_t flags)
{
	WGPUBufferUsage u = WGPUBufferUsage_None;
	if (flags & BHS_BUFFER_VERTEX)
		u |= WGPUBufferUsage_Vertex;
	if (flags & BHS_BUFFER_INDEX)
		u |= WGPUBufferUsage_Index;
	if (flags & BHS_BUFFER_UNIFORM)
		u |= WGPUBufferUsage_Uniform;
	if (flags & BHS_BUFFER_STORAGE)
		u |= WGPUBufferUsage_Storage;
	if (flags & BHS_BUFFER_INDIRECT)
		u |= WGPUBufferUsage_Indirect;
	if (flags & BHS_BUFFER_TRANSFER_SRC)
		u |= WGPUBufferUsage_CopySrc;
	if (flags & BHS_BUFFER_TRANSFER_DST)
		u |= WGPUBufferUsage_CopyDst;
	return u;
}
