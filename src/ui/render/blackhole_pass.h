/**
 * @file blackhole_pass.h
 * @brief Gerenciador do Compute Pass do Buraco Negro
 *
 * Responsável por:
 * 1. Inicializar o pipeline de compute (carregar shader SPV)
 * 2. Gerenciar texturas de output (Storage Images)
 * 3. Rotina de Dispatch (atualizar uniforms/push constants e rodar na GPU)
 */

#ifndef BHS_UI_RENDER_BLACKHOLE_PASS_H
#define BHS_UI_RENDER_BLACKHOLE_PASS_H

#include "engine/scene/scene.h"
#include "gui/rhi/rhi.h"
#include "src/ui/camera/camera.h"

/* Configuração do Pass */
typedef struct bhs_blackhole_pass_config {
	int width;
	int height;
} bhs_blackhole_pass_config_t;

/* Estado opaco do Pass */
typedef struct bhs_blackhole_pass *bhs_blackhole_pass_t;

/**
 * Cria o pass de renderização do buraco negro
 */
bhs_blackhole_pass_t
bhs_blackhole_pass_create(bhs_gpu_device_t device,
			  const bhs_blackhole_pass_config_t *config);

/**
 * Destrói o pass e libera recursos
 */
void bhs_blackhole_pass_destroy(bhs_blackhole_pass_t pass);

/**
 * Redimensiona as texturas internas (chamar no resize da janela)
 */
void bhs_blackhole_pass_resize(bhs_blackhole_pass_t pass, int width,
			       int height);

/**
 * Executa o compute shader
 * 
 * @param pass Handle do pass
 * @param cmd Command buffer (deve estar em estado de gravação)
 * @param scene Cena contendo o blackhole (para pegar massa/spin)
 * @param cam Câmera atual
 */
void bhs_blackhole_pass_dispatch(bhs_blackhole_pass_t pass,
				 bhs_gpu_cmd_buffer_t cmd, bhs_scene_t scene,
				 const bhs_camera_t *cam);

/**
 * Obtém a textura de resultado para desenhar na tela
 */
bhs_gpu_texture_t bhs_blackhole_pass_get_output(bhs_blackhole_pass_t pass);

#endif /* BHS_UI_RENDER_BLACKHOLE_PASS_H */
