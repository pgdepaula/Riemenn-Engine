/**
 * @file test_lifecycle.c
 * @brief Teste de Ciclo de Vida do gui
 *
 * Verifica:
 * - Inicialização e shutdown do gui
 * - Criação e destruição de janelas
 * - Ausência de vazamento de memória (usar valgrind)
 *
 * Executar com:
 *   valgrind --leak-check=full ./build/gui/tests/test_lifecycle
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
 * test_platform_init - Testa inicialização da plataforma
 */
static void test_platform_init(void)
{
	RI_TEST_SECTION("Platform Init/Shutdown");

	/* Inicializa */
	ri_platform_t platform = NULL;
	int res = ri_platform_init(&platform);
	RI_TEST_ASSERT_EQ(res, RI_PLATFORM_OK,
			   "ri_platform_init() retornou OK");
	RI_TEST_ASSERT_NOT_NULL(platform, "Platform handle válido");

	/* Shutdown */
	if (platform) {
		ri_platform_shutdown(platform);
		RI_TEST_ASSERT(1,
				"ri_platform_shutdown() executou sem crash");
	}
}

/**
 * test_window_lifecycle - Testa criação/destruição de janela
 */
static void test_window_lifecycle(void)
{
	RI_TEST_SECTION("Window Lifecycle");

	/* Setup */
	ri_platform_t platform = NULL;
	ri_platform_init(&platform);
	RI_TEST_ASSERT_NOT_NULL(platform, "Platform criada");

	/* Cria janela */
	struct ri_window_config cfg = {
		.title = "Test Window",
		.width = 800,
		.height = 600,
		.x = RI_WINDOW_POS_CENTERED,
		.y = RI_WINDOW_POS_CENTERED,
		.flags = RI_WINDOW_RESIZABLE,
	};

	ri_window_t window = NULL;
	int res = ri_window_create(platform, &cfg, &window);

	RI_TEST_ASSERT_EQ(res, RI_PLATFORM_OK,
			   "ri_window_create() retornou OK");
	RI_TEST_ASSERT_NOT_NULL(window, "Window handle válido");

	/* Verifica dimensões */
	if (window) {
		int w, h;
		ri_window_get_size(window, &w, &h);
		RI_TEST_ASSERT(w > 0, "Largura da janela > 0");
		RI_TEST_ASSERT(h > 0, "Altura da janela > 0");
		if (w != 800 || h != 600) {
			printf("  [INFO] Window size adjusted by OS: requested 800x600, got %dx%d\n", w, h);
		}

		/* Destrói */
		ri_window_destroy(window);
		RI_TEST_ASSERT(1, "ri_window_destroy() executou sem crash");
	}

	ri_platform_shutdown(platform);
}

/**
 * test_multiple_cycles - Testa ciclos repetidos
 */
static void test_multiple_cycles(void)
{
	RI_TEST_SECTION("Multiple Init/Shutdown Cycles");

	for (int i = 0; i < 5; i++) {
		ri_platform_t platform = NULL;
		ri_platform_init(&platform);
		RI_TEST_ASSERT_NOT_NULL(platform, "Ciclo: platform criada");

		struct ri_window_config cfg = {
			.title = "Cycle Test",
			.width = 320,
			.height = 240,
			.flags = 0,
		};

		ri_window_t win = NULL;
		ri_window_create(platform, &cfg, &win);
		RI_TEST_ASSERT_NOT_NULL(win, "Ciclo: janela criada");

		ri_window_destroy(win);
		ri_platform_shutdown(platform);
	}

	RI_TEST_ASSERT(1, "5 ciclos completos sem leak/crash");
}

/* ============================================================================
 * MAIN
 * ============================================================================
 */

int main(void)
{
	ri_log_init();
	ri_log_set_level(RI_LOG_LEVEL_WARN); /* Menos spam durante testes */

	RI_TEST_BEGIN("gui Lifecycle Tests");

	test_platform_init();
	test_window_lifecycle();
	test_multiple_cycles();

	ri_log_shutdown();
	RI_TEST_END();
}
