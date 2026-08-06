/**
 * @file test_input.c
 * @brief Teste de Sistema de Input (Platform Layer)
 *
 * Verifica:
 * - Polling de eventos (smoke test)
 * - Processamento da fila de eventos
 *
 * Nota: Testar input específico requer injeção de eventos ou UI layer.
 *       Aqui testamos apenas a infraestrutura de platform.
 */

#include "engine/platform/platform.h"
#include "engine/foundation/log.h"
#include "test_runner.h"

/* ============================================================================
 * TESTES
 * ============================================================================
 */

/**
 * test_event_polling - Verifica que poll não crasheia
 */
static void test_event_polling(void)
{
	RI_TEST_SECTION("Event Polling Smoke Test");

	/* Setup */
	ri_platform_t platform = NULL;
	if (ri_platform_init(&platform) != RI_PLATFORM_OK) {
		printf("  [SKIP] Platform init falhou\n");
		return;
	}

	struct ri_window_config cfg = {
		.title = "Input Test",
		.width = 320,
		.height = 240,
		.flags = 0,
	};
	ri_window_t window = NULL;
	if (ri_window_create(platform, &cfg, &window) != RI_PLATFORM_OK) {
		printf("  [SKIP] Window create falhou\n");
		ri_platform_shutdown(platform);
		return;
	}

	/* Poll loop */
	for (int i = 0; i < 50; i++) {
		ri_platform_poll_events(platform);

		/* Verifica se há eventos na fila (provavelmente não, em ambiente headless) */
		struct ri_event evt;
		while (ri_window_next_event(window, &evt)) {
			/* Consome eventos se houver (resize, focus, etc) */
		}
	}

	RI_TEST_ASSERT(1, "50 frames de polling + next_event sem crash");

	/* Cleanup */
	ri_window_destroy(window);
	ri_platform_shutdown(platform);
}

/* ============================================================================
 * MAIN
 * ============================================================================
 */

int main(void)
{
	ri_log_init();
	ri_log_set_level(RI_LOG_LEVEL_WARN);

	RI_TEST_BEGIN("Platform Input Infrastructure");

	test_event_polling();

	ri_log_shutdown();
	RI_TEST_END();
}
