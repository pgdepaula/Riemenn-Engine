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

#include "gui/epa/epa.h"
#include "gui/log.h"
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
	BHS_TEST_SECTION("Event Polling Smoke Test");

	/* Setup */
	bhs_platform_t platform = NULL;
	if (bhs_platform_init(&platform) != BHS_PLATFORM_OK) {
		printf("  [SKIP] Platform init falhou\n");
		return;
	}

	struct bhs_window_config cfg = {
		.title = "Input Test",
		.width = 320,
		.height = 240,
		.flags = 0,
	};
	bhs_window_t window = NULL;
	if (bhs_window_create(platform, &cfg, &window) != BHS_PLATFORM_OK) {
		printf("  [SKIP] Window create falhou\n");
		bhs_platform_shutdown(platform);
		return;
	}

	/* Poll loop */
	for (int i = 0; i < 50; i++) {
		bhs_platform_poll_events(platform);

		/* Verifica se há eventos na fila (provavelmente não, em ambiente headless) */
		struct bhs_event evt;
		while (bhs_window_next_event(window, &evt)) {
			/* Consome eventos se houver (resize, focus, etc) */
		}
	}

	BHS_TEST_ASSERT(1, "50 frames de polling + next_event sem crash");

	/* Cleanup */
	bhs_window_destroy(window);
	bhs_platform_shutdown(platform);
}

/* ============================================================================
 * MAIN
 * ============================================================================
 */

int main(void)
{
	bhs_log_init();
	bhs_log_set_level(BHS_LOG_LEVEL_WARN);

	BHS_TEST_BEGIN("Platform Input Infrastructure");

	test_event_polling();

	bhs_log_shutdown();
	BHS_TEST_END();
}
