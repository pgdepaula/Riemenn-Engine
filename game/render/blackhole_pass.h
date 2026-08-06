/**
 * @file blackhole_pass.h
 * @brief Gerenciador do Compute Pass do Buraco Negro
 *
 * Responsável por:
 * 1. Inicializar o pipeline de compute (carregar shader SPV)
 * 2. Gerenciar texturas de output (Storage Images)
 * 3. Rotina de Dispatch (atualizar uniforms/push constants e rodar na GPU)
 */

#ifndef RI_UI_RENDER_BLACKHOLE_PASS_H
#define RI_UI_RENDER_BLACKHOLE_PASS_H

#include "engine/scene/scene.h"
#include "engine/rhi/rhi.h"
#include "engine/render/camera.h"

/* Configuração do Pass */
typedef struct ri_blackhole_pass_config {
	int width;
	int height;
} ri_blackhole_pass_config_t;

/* Estado opaco do Pass */
typedef struct ri_blackhole_pass *ri_blackhole_pass_t;

/**
 * Cria o pass de renderização do buraco negro
 */
ri_blackhole_pass_t
ri_blackhole_pass_create(ri_gpu_device_t device,
			  const ri_blackhole_pass_config_t *config);

/**
 * Destrói o pass e libera recursos
 */
void ri_blackhole_pass_destroy(ri_blackhole_pass_t pass);

/**
 * Redimensiona as texturas internas (chamar no resize da janela)
 */
void ri_blackhole_pass_resize(ri_blackhole_pass_t pass, int width,
			       int height);

/**
 * Executa o compute shader
 * 
 * @param pass Handle do pass
 * @param cmd Command buffer (deve estar em estado de gravação)
 * @param scene Cena contendo o blackhole (para pegar massa/spin)
 * @param cam Câmera atual
 */
void ri_blackhole_pass_dispatch(ri_blackhole_pass_t pass,
				 ri_gpu_cmd_buffer_t cmd, ri_scene_t scene,
				 const ri_camera_t *cam);

/**
 * Obtém a textura de resultado para desenhar na tela
 */
ri_gpu_texture_t ri_blackhole_pass_get_output(ri_blackhole_pass_t pass);

#endif /* RI_UI_RENDER_BLACKHOLE_PASS_H */
