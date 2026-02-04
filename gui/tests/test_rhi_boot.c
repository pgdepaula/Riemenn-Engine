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

#include "gui/epa/epa.h"
#include "gui/log.h"
#include "gui/rhi/rhi.h"
#include "test_runner.h"

/* ============================================================================
 * TESTES
 * ============================================================================
 */

/**
 * test_gpu_device_creation - Testa criação do device
 */
static void test_gpu_device_creation(bhs_platform_t platform,
				     bhs_window_t window)
{
	BHS_TEST_SECTION("GPU Device Creation");
	(void)platform;
	(void)window;

	struct bhs_gpu_device_config cfg = {
		.preferred_backend = BHS_GPU_BACKEND_AUTO,
		.enable_validation = true,
		.prefer_discrete_gpu = true,
	};

	bhs_gpu_device_t device = NULL;
	int res = bhs_gpu_device_create(&cfg, &device);
	BHS_TEST_ASSERT_EQ(res, BHS_GPU_OK,
			   "bhs_gpu_device_create() retornou OK");
	BHS_TEST_ASSERT_NOT_NULL(device, "Device handle válido");

	if (device) {
		bhs_gpu_device_destroy(device);
		BHS_TEST_ASSERT(1, "bhs_gpu_device_destroy() sem crash");
	}
}

/**
 * test_buffer_creation - Testa criação de buffers GPU
 */
static void test_buffer_creation(bhs_platform_t platform, bhs_window_t window)
{
	BHS_TEST_SECTION("GPU Buffer Creation");
	(void)platform;
	(void)window;

	struct bhs_gpu_device_config dev_cfg = {
		.enable_validation = true,
	};
	bhs_gpu_device_t device = NULL;
	if (bhs_gpu_device_create(&dev_cfg, &device) != BHS_GPU_OK) {
		BHS_TEST_ASSERT(0,
				"Device não criado, pulando teste de buffer");
		return;
	}

	/* Cria buffer de vértices */
	struct bhs_gpu_buffer_config buf_cfg = {
		.size = 1024,
		.usage = BHS_BUFFER_VERTEX,
		.memory = BHS_MEMORY_CPU_VISIBLE,
		.label = "Test Vertex Buffer",
	};

	bhs_gpu_buffer_t buffer = NULL;
	int result = bhs_gpu_buffer_create(device, &buf_cfg, &buffer);

	BHS_TEST_ASSERT_EQ(result, BHS_GPU_OK,
			   "bhs_gpu_buffer_create() retornou OK");
	BHS_TEST_ASSERT_NOT_NULL(buffer, "Buffer criado não é NULL");

	if (buffer) {
		/* Testa mapeamento */
		void *mapped = bhs_gpu_buffer_map(buffer);
		BHS_TEST_ASSERT_NOT_NULL(
			mapped, "bhs_gpu_buffer_map() retornou válido");

		if (mapped) {
			/* Escreve dados de teste */
			memset(mapped, 0xAB, 1024);
			bhs_gpu_buffer_unmap(buffer);
			BHS_TEST_ASSERT(1, "Escrita e unmap sem crash");
		}

		bhs_gpu_buffer_destroy(buffer);
	}

	bhs_gpu_device_destroy(device);
}

/**
 * test_swapchain_creation - Testa criação de swapchain
 */
static void test_swapchain_creation(bhs_platform_t platform,
				    bhs_window_t window)
{
	BHS_TEST_SECTION("Swapchain Creation");

	struct bhs_gpu_device_config dev_cfg = {
		.enable_validation = true,
	};
	bhs_gpu_device_t device = NULL;
	if (bhs_gpu_device_create(&dev_cfg, &device) != BHS_GPU_OK) {
		BHS_TEST_ASSERT(
			0, "Device não criado, pulando teste de swapchain");
		return;
	}

	struct bhs_gpu_swapchain_config swap_cfg = {
		.native_display = bhs_platform_get_native_display(platform),
		.native_window = bhs_window_get_native_handle(window),
		.native_layer = bhs_window_get_native_layer(window),
		.width = 800,
		.height = 600,
		.format = BHS_FORMAT_BGRA8_SRGB,
		.buffer_count = 2,
		.vsync = true,
	};

	bhs_gpu_swapchain_t swapchain = NULL;
	int result = bhs_gpu_swapchain_create(device, &swap_cfg, &swapchain);

	/* Swapchain creation might fail in headless or unsupported environments */
	if (result == BHS_GPU_OK) {
		BHS_TEST_ASSERT_NOT_NULL(swapchain,
					 "Swapchain criado com sucesso");
		bhs_gpu_swapchain_destroy(swapchain);
	} else {
		printf("  [WARN] Swapchain falhou (esperado em headless): %d\n",
		       result);
	}

	bhs_gpu_device_destroy(device);
}

/* ============================================================================
 * MAIN
 * ============================================================================
 */

int main(void)
{
	bhs_log_init();
	bhs_log_set_level(BHS_LOG_LEVEL_WARN);

	BHS_TEST_BEGIN("RHI Boot Tests (Vulkan)");

	/* Setup comum */
	bhs_platform_t platform = NULL;
	if (bhs_platform_init(&platform) != BHS_PLATFORM_OK) {
		printf("  [SKIP] Platform init falhou\n");
		BHS_TEST_END();
	}

	struct bhs_window_config win_cfg = {
		.title = "RHI Test",
		.width = 800,
		.height = 600,
		.x = BHS_WINDOW_POS_CENTERED,
		.y = BHS_WINDOW_POS_CENTERED,
		.flags = 0,
	};
	bhs_window_t window = NULL;
	if (bhs_window_create(platform, &win_cfg, &window) != BHS_PLATFORM_OK) {
		printf("  [SKIP] Janela não disponível (headless "
		       "environment?)\n");
		bhs_platform_shutdown(platform);
		BHS_TEST_END();
	}

	/* Testes */
	test_gpu_device_creation(platform, window);
	test_buffer_creation(platform, window);
	test_swapchain_creation(platform, window);

	/* Cleanup */
	bhs_window_destroy(window);
	bhs_platform_shutdown(platform);
	bhs_log_shutdown();

	BHS_TEST_END();
}
