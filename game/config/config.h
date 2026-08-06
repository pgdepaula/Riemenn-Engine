#ifndef RI_SYSTEM_CONFIG_H
#define RI_SYSTEM_CONFIG_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	/* Magic & Version */
	uint32_t magic;
	uint32_t version;

	/* Settings */
	bool vsync_enabled;
	bool show_fps;
	float time_scale_val;

	/* Visual Options */
	bool top_down_view;
	bool show_gravity_line;
	bool show_orbit_trail;
	bool show_satellite_orbits;
	bool show_planet_markers;
	bool show_moon_markers;
	int visual_mode; /* enum ri_visual_mode_t */

	/* [Padding for future proofing] */
	uint8_t reserved[64];
} ri_user_config_t;

/* Inicializa com defaults */
void ri_config_defaults(ri_user_config_t *cfg);

/* Carrega do disco (retorna sucesso) */
bool ri_config_load(ri_user_config_t *cfg, const char *relative_path);

/* Salva no disco */
bool ri_config_save(const ri_user_config_t *cfg, const char *relative_path);

#endif
