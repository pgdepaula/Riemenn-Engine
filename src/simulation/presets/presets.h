/**
 * @file presets.h
 * @brief Corpos Celestes Pré-Definidos
 *
 * "Quando você precisa de um Sol, Terra ou Lua de verdade."
 *
 * Este arquivo APENAS declara as funções de preset.
 * Todas as constantes físicas e escalas estão em lib/units.h.
 */

#ifndef BHS_ENGINE_PRESETS_H
#define BHS_ENGINE_PRESETS_H

#include "engine/scene/scene.h"
#include "math/units.h"

/* ============================================================================
 * FACTORIES DE CORPOS CELESTES
 * ============================================================================
 */

/**
 * bhs_preset_sun - Cria o Sol com dados físicos reais
 * @pos: Posição inicial (geralmente origem)
 *
 * Retorna uma struct bhs_body completamente inicializada
 * com todos os dados físicos do Sol.
 */
struct bhs_body bhs_preset_sun(struct bhs_vec3 pos);

/**
 * bhs_preset_earth - Cria a Terra em órbita
 * @sun_pos: Posição do Sol (centro da órbita)
 *
 * A Terra é criada com velocidade orbital correta para
 * órbita circular estável ao redor do Sol.
 */
struct bhs_body bhs_preset_earth(struct bhs_vec3 sun_pos);

/**
 * bhs_preset_moon - Cria a Lua em órbita da Terra
 * @earth_pos: Posição da Terra
 * @earth_vel: Velocidade da Terra (para composição)
 *
 * A Lua recebe velocidade orbital em relação à Terra
 * MAIS a velocidade da Terra.
 */
struct bhs_body bhs_preset_moon(struct bhs_vec3 earth_pos,
				struct bhs_vec3 earth_vel);

/**
 * bhs_preset_solar_system - Cria Sistema Solar completo
 * @scene: Cena onde adicionar os corpos
 *
 * Cria Sol + todos os planetas com órbitas estáveis e dados físicos reais.
 * Os dados vêm de engine/planets/
 */
void bhs_preset_solar_system(bhs_scene_t scene);

/**
 * bhs_preset_earth_moon_sun - Cria apenas Sol, Terra e Lua
 * @scene: Cena onde adicionar os corpos
 *
 * Para debug: visualização da escala real Terra-Sol-Lua.
 */
void bhs_preset_earth_moon_sun(bhs_scene_t scene);

/**
 * bhs_preset_earth_moon_only - Cria apenas Terra e Lua (Sem Sol)
 * @scene: Cena onde adicionar os corpos
 */
void bhs_preset_earth_moon_only(bhs_scene_t scene);

/**
 * bhs_preset_jupiter_pluto_pull - Cria Júpiter e Plutão próximos
 * @scene: Cena onde adicionar os corpos
 *
 * Posiciona Plutão próximo a Júpiter para demonstrar a atração gravitacional.
 */
void bhs_preset_jupiter_pluto_pull(bhs_scene_t scene);

/**
 * bhs_preset_orbital_velocity - Calcula velocidade orbital
 * @central_mass: Massa do corpo central (unidades de simulação)
 * @orbital_radius: Distância orbital (unidades de simulação)
 *
 * Retorna a velocidade para órbita circular: v = sqrt(G*M/r)
 * Com G = 1 (unidades naturais).
 */
double bhs_preset_orbital_velocity(double central_mass, double orbital_radius);

#endif /* BHS_ENGINE_PRESETS_H */
