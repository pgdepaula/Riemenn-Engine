/**
 * @file presets.h
 * @brief Corpos Celestes Pré-Definidos
 *
 * "Quando você precisa de um Sol, Terra ou Lua de verdade."
 *
 * Este arquivo APENAS declara as funções de preset.
 * Todas as constantes físicas e escalas estão em lib/units.h.
 */

#ifndef RI_ENGINE_PRESETS_H
#define RI_ENGINE_PRESETS_H

#include "engine/scene/scene.h"
#include "engine/math/units.h"

/* ============================================================================
 * FACTORIES DE CORPOS CELESTES
 * ============================================================================
 */

/**
 * ri_preset_sun - Cria o Sol com dados físicos reais
 * @pos: Posição inicial (geralmente origem)
 *
 * Retorna uma struct ri_body completamente inicializada
 * com todos os dados físicos do Sol.
 */
struct ri_body ri_preset_sun(struct ri_vec3 pos);

/**
 * ri_preset_earth - Cria a Terra em órbita
 * @sun_pos: Posição do Sol (centro da órbita)
 *
 * A Terra é criada com velocidade orbital correta para
 * órbita circular estável ao redor do Sol.
 */
struct ri_body ri_preset_earth(struct ri_vec3 sun_pos);

/**
 * ri_preset_moon - Cria a Lua em órbita da Terra
 * @earth_pos: Posição da Terra
 * @earth_vel: Velocidade da Terra (para composição)
 *
 * A Lua recebe velocidade orbital em relação à Terra
 * MAIS a velocidade da Terra.
 */
struct ri_body ri_preset_moon(struct ri_vec3 earth_pos,
				struct ri_vec3 earth_vel);

/**
 * ri_preset_solar_system - Cria Sistema Solar completo
 * @scene: Cena onde adicionar os corpos
 *
 * Cria Sol + todos os planetas com órbitas estáveis e dados físicos reais.
 * Os dados vêm de engine/planets/
 */
void ri_preset_solar_system(ri_scene_t scene);

/**
 * ri_preset_earth_moon_sun - Cria apenas Sol, Terra e Lua
 * @scene: Cena onde adicionar os corpos
 *
 * Para debug: visualização da escala real Terra-Sol-Lua.
 */
void ri_preset_earth_moon_sun(ri_scene_t scene);

/**
 * ri_preset_earth_moon_only - Cria apenas Terra e Lua (Sem Sol)
 * @scene: Cena onde adicionar os corpos
 */
void ri_preset_earth_moon_only(ri_scene_t scene);

/**
 * ri_preset_jupiter_pluto_pull - Cria Júpiter e Plutão próximos
 * @scene: Cena onde adicionar os corpos
 *
 * Posiciona Plutão próximo a Júpiter para demonstrar a atração gravitacional.
 */
void ri_preset_jupiter_pluto_pull(ri_scene_t scene);

/**
 * ri_preset_orbital_velocity - Calcula velocidade orbital
 * @central_mass: Massa do corpo central (unidades de simulação)
 * @orbital_radius: Distância orbital (unidades de simulação)
 *
 * Retorna a velocidade para órbita circular: v = sqrt(G*M/r)
 * Com G = 1 (unidades naturais).
 */
double ri_preset_orbital_velocity(double central_mass, double orbital_radius);

#endif /* RI_ENGINE_PRESETS_H */
