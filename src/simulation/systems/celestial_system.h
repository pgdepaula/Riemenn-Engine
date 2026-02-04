/**
 * @file celestial_system.h
 * @brief Sistema de Gameplay Celestial
 *
 * "Física? O motor cuida. Gameplay? Eu cuido."
 *
 * Este sistema escuta eventos de colisão e reage de acordo
 * com o tipo de corpo celestial envolvido:
 * - Estrela + Estrela = Fusão / Supernova
 * - Qualquer coisa + Buraco Negro = Tchau, foi bom te conhecer
 * - Planeta + Planeta = Catástrofe planetária
 */

#ifndef BHS_ENGINE_SYSTEMS_CELESTIAL_H
#define BHS_ENGINE_SYSTEMS_CELESTIAL_H

#include "engine/ecs/ecs.h"
#include "engine/scene/scene.h"

/**
 * bhs_celestial_system_init - Inicializa o sistema e registra listeners
 * @world: Mundo ECS
 *
 * Chame isso UMA VEZ durante a inicialização do jogo.
 * Registra callbacks para eventos de colisão.
 */
void bhs_celestial_system_init(bhs_world_handle world);

/**
 * bhs_celestial_system_shutdown - Remove listeners e limpa recursos
 * @world: Mundo ECS
 */
void bhs_celestial_system_shutdown(bhs_world_handle world);

/**
 * bhs_celestial_system_update - Atualiza estado celestial (rotação, orbitas, etc)
 * @scene: Cena contendo os corpos
 * @dt: Delta time em segundos
 */
void bhs_celestial_system_update(bhs_scene_t scene, double dt);

#endif /* BHS_ENGINE_SYSTEMS_CELESTIAL_H */
