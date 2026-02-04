/**
 * @file assert.h
 * @brief Assertion macro para o gui
 *
 * gui tem seu próprio assert porque não pode depender de math/.
 * Se math/ mudar o assert, gui não é afetado. Desacoplamento total.
 */

#ifndef BHS_GUI_FRAMEWORK_ASSERT_H
#define BHS_GUI_FRAMEWORK_ASSERT_H

#include <assert.h>

/*
 * BHS_ASSERT - Assertion para gui
 *
 * Usa o assert padrão do C, mas encapsulado pra
 * eventual customização (log, recovery, etc).
 */
#define BHS_ASSERT(expr) assert(expr)

#endif /* BHS_gui_ASSERT_H */
