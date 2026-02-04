/*
 * wrapper.h - O Ponto de Encontro do C e do Rust.
 *
 * Se uma função ou tipo não está incluído aqui, o Rust não vai enxergar.
 * Simples assim.
 */

#include <bhs_types.h>
#include <bhs_math.h>

// Engine Core
#include <engine.h>
#include <ecs/ecs.h>
#include <well.h>

// GUI - Só o que for realmente necessário.
#include <rhi/rhi.h>
#include <ui/lib.h>
#include <log.h>
