/**
 * @file assert.h
 * @brief Assertion macro para o gui
 *
 * gui tem seu próprio assert porque não pode depender de math/.
 * Se math/ mudar o assert, gui não é afetado. Desacoplamento total.
 */

#ifndef RI_GUI_FRAMEWORK_ASSERT_H
#define RI_GUI_FRAMEWORK_ASSERT_H

#include <assert.h>

/*
 * RI_ASSERT - Assertion para gui
 *
 * Usa o assert padrão do C, mas encapsulado pra
 * eventual customização (log, recovery, etc).
 */
#define RI_ASSERT(expr) assert(expr)

#endif /* RI_gui_ASSERT_H */
