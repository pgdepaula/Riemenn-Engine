#include "config.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#define CONFIG_MAGIC 0xB1ACDEC0 // "Black Hole Config" kinda
#define CONFIG_VERSION 1

void bhs_config_defaults(bhs_user_config_t *cfg)
{
	if (!cfg)
		return;
	memset(cfg, 0, sizeof(*cfg));

	cfg->magic = CONFIG_MAGIC;
	cfg->version = CONFIG_VERSION;

	/* Default Values */
	cfg->vsync_enabled = true;
	cfg->show_fps = false;
	cfg->time_scale_val = 0.5f; /* ~30 dias/min default */

	cfg->visual_mode = 0; /* Scientific */
	cfg->show_gravity_line = false;
	cfg->show_orbit_trail = true;
	cfg->show_satellite_orbits = true;
	cfg->show_planet_markers = true;
	cfg->show_moon_markers = false;
	cfg->top_down_view = false;
}

static void ensure_data_dir(const char *path)
{
	/* Basic attempt to create "data" dir if missing */
	/* Assumes path starts with "data/" */
	if (strncmp(path, "data/", 5) == 0) {
#ifdef _WIN32
		_mkdir("data");
#else
		mkdir("data", 0755);
#endif
	}
}

bool bhs_config_load(bhs_user_config_t *cfg, const char *relative_path)
{
	if (!cfg || !relative_path)
		return false;

	FILE *f = fopen(relative_path, "rb");
	if (!f) {
		/* File doesn't exist, use defaults */
		bhs_config_defaults(cfg);
		return false;
	}

	size_t read = fread(cfg, sizeof(*cfg), 1, f);
	fclose(f);

	if (read != 1) {
		printf("[CONFIG] Read error or short read. Using defaults.\n");
		bhs_config_defaults(cfg);
		return false;
	}

	if (cfg->magic != CONFIG_MAGIC) {
		printf("[CONFIG] Invalid magic number. Ignoring.\n");
		bhs_config_defaults(cfg);
		return false;
	}

	if (cfg->version != CONFIG_VERSION) {
		printf("[CONFIG] Version mismatch (Got %d, Expected %d). Using "
		       "defaults.\n",
		       cfg->version, CONFIG_VERSION);
		bhs_config_defaults(cfg);
		return false;
	}

	printf("[CONFIG] Loaded settings from %s\n", relative_path);
	return true;
}

bool bhs_config_save(const bhs_user_config_t *cfg, const char *relative_path)
{
	if (!cfg || !relative_path)
		return false;

	ensure_data_dir(relative_path);

	FILE *f = fopen(relative_path, "wb");
	if (!f) {
		printf("[CONFIG] Failed to open %s for writing: %s\n",
		       relative_path, strerror(errno));
		return false;
	}

	size_t wrote = fwrite(cfg, sizeof(*cfg), 1, f);
	fclose(f);

	if (wrote == 1) {
		printf("[CONFIG] Saved settings to %s\n", relative_path);
		return true;
	}

	return false;
}
