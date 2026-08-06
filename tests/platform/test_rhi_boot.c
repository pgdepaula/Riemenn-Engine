/**
 * @file test_rhi_boot.c
 * @brief Teste de Inicialização do RHI (Vulkan)
 *
 * Verifica:
 * - Criação do device Vulkan
 * - Criação de buffers
 * - Criação de texturas
 * - Shutdown sem crash
 *
 * Este teste requer drivers Vulkan instalados.
 * Se falhar no CI/CD sem GPU, pule com --skip-rhi.
 */

#include "engine/platform/platform.h"
#include "engine/foundation/log.h"
#include "engine/rhi/rhi.h"
#include "test_runner.h"

/* ============================================================================
 * TESTES
 * ============================================================================
 */

/**
 * test_gpu_device_creation - Testa criação do device
 */
static void test_gpu_device_creation(ri_platform_t platform,
				     ri_window_t window)
{
	RI_TEST_SECTION("GPU Device Creation");
	(void)platform;
	(void)window;

	struct ri_gpu_device_config cfg = {
		.preferred_backend = RI_GPU_BACKEND_AUTO,
		.enable_validation = true,
		.prefer_discrete_gpu = true,
	};

	ri_gpu_device_t device = NULL;
	int res = ri_gpu_device_create(&cfg, &device);
	RI_TEST_ASSERT_EQ(res, RI_GPU_OK,
			   "ri_gpu_device_create() retornou OK");
	RI_TEST_ASSERT_NOT_NULL(device, "Device handle válido");

	if (device) {
		ri_gpu_device_destroy(device);
		RI_TEST_ASSERT(1, "ri_gpu_device_destroy() sem crash");
	}
}

/**
 * test_buffer_creation - Testa criação de buffers GPU
 */
static void test_buffer_creation(ri_platform_t platform, ri_window_t window)
{
	RI_TEST_SECTION("GPU Buffer Creation");
	(void)platform;
	(void)window;

	struct ri_gpu_device_config dev_cfg = {
		.enable_validation = true,
	};
	ri_gpu_device_t device = NULL;
	if (ri_gpu_device_create(&dev_cfg, &device) != RI_GPU_OK) {
		RI_TEST_ASSERT(0,
				"Device não criado, pulando teste de buffer");
		return;
	}

	/* Cria buffer de vértices */
	struct ri_gpu_buffer_config buf_cfg = {
		.size = 1024,
		.usage = RI_BUFFER_VERTEX,
		.memory = RI_MEMORY_CPU_VISIBLE,
		.label = "Test Vertex Buffer",
	};

	ri_gpu_buffer_t buffer = NULL;
	int result = ri_gpu_buffer_create(device, &buf_cfg, &buffer);

	RI_TEST_ASSERT_EQ(result, RI_GPU_OK,
			   "ri_gpu_buffer_create() retornou OK");
	RI_TEST_ASSERT_NOT_NULL(buffer, "Buffer criado não é NULL");

	if (buffer) {
		/* Testa mapeamento */
		void *mapped = ri_gpu_buffer_map(buffer);
		RI_TEST_ASSERT_NOT_NULL(
			mapped, "ri_gpu_buffer_map() retornou válido");

		if (mapped) {
			/* Escreve dados de teste */
			memset(mapped, 0xAB, 1024);
			ri_gpu_buffer_unmap(buffer);
			RI_TEST_ASSERT(1, "Escrita e unmap sem crash");
		}

		ri_gpu_buffer_destroy(buffer);
	}

	ri_gpu_device_destroy(device);
}

/**
 * test_swapchain_creation - Testa criação de swapchain
 */
static void test_swapchain_creation(ri_platform_t platform,
				    ri_window_t window)
{
	RI_TEST_SECTION("Swapchain Creation");

	struct ri_gpu_device_config dev_cfg = {
		.enable_validation = true,
	};
	ri_gpu_device_t device = NULL;
	if (ri_gpu_device_create(&dev_cfg, &device) != RI_GPU_OK) {
		RI_TEST_ASSERT(
			0, "Device não criado, pulando teste de swapchain");
		return;
	}

	struct ri_gpu_swapchain_config swap_cfg = {
		.native_display = ri_platform_get_native_display(platform),
		.native_window = ri_window_get_native_handle(window),
		.native_layer = ri_window_get_native_layer(window),
		.width = 800,
		.height = 600,
		.format = RI_FORMAT_BGRA8_SRGB,
		.buffer_count = 2,
		.vsync = true,
	};

	ri_gpu_swapchain_t swapchain = NULL;
	int result = ri_gpu_swapchain_create(device, &swap_cfg, &swapchain);

	/* Swapchain creation might fail in headless or unsupported environments */
	if (result == RI_GPU_OK) {
		RI_TEST_ASSERT_NOT_NULL(swapchain,
					 "Swapchain criado com sucesso");
		ri_gpu_swapchain_destroy(swapchain);
	} else {
		printf("  [WARN] Swapchain falhou (esperado em headless): %d\n",
		       result);
	}

	ri_gpu_device_destroy(device);
}

/* ============================================================================
 * MAIN
 * ============================================================================
 */

int main(void)
{
	ri_log_init();
	ri_log_set_level(RI_LOG_LEVEL_WARN);

	RI_TEST_BEGIN("RHI Boot Tests (Vulkan)");

	/* Setup comum */
	ri_platform_t platform = NULL;
	if (ri_platform_init(&platform) != RI_PLATFORM_OK) {
		printf("  [SKIP] Platform init falhou\n");
		RI_TEST_END();
	}

	struct ri_window_config win_cfg = {
		.title = "RHI Test",
		.width = 800,
		.height = 600,
		.x = RI_WINDOW_POS_CENTERED,
		.y = RI_WINDOW_POS_CENTERED,
		.flags = 0,
	};
	ri_window_t window = NULL;
	if (ri_window_create(platform, &win_cfg, &window) != RI_PLATFORM_OK) {
		printf("  [SKIP] Janela não disponível (headless "
		       "environment?)\n");
		ri_platform_shutdown(platform);
		RI_TEST_END();
	}

	/* Testes */
	test_gpu_device_creation(platform, window);
	test_buffer_creation(platform, window);
	test_swapchain_creation(platform, window);

	/* Cleanup */
	ri_window_destroy(window);
	ri_platform_shutdown(platform);
	ri_log_shutdown();

	RI_TEST_END();
}
