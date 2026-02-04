#ifndef SIMULATION_COMPONENTS_H
#define SIMULATION_COMPONENTS_H

#include "engine/components/components.h"
#include "engine/ecs/ecs.h"

/* IDs dos Componentes de Simulação (começando após componentes da Engine) */
enum {
	BHS_COMP_CELESTIAL = BHS_COMP_COUNT,
	BHS_COMP_ORBIT_DESC,
	BHS_COMP_ORBITAL,
	SIM_COMP_COUNT
};

/**
 * Valores para flags orbitais
 */
#define BHS_ORBITAL_FLAG_TIDAL_LOCK (1 << 0)

/**
 * struct bhs_orbital_component
 * Define relacionamentos pai-filho e parâmetros orbitais.
 */
typedef struct {
	bhs_entity_id parent;	/* ID da Entidade do corpo orbitado (Pai) */
	double semi_major_axis; /* a (metros) - Cacheado para acesso rápido */
	double eccentricity;	/* e - Cacheado */
	double period;		/* T (segundos) - Cacheado */
	uint32_t flags;		/* Bitfield (ex: TIDAL_LOCK) */
} bhs_orbital_component;

/* Tipos Celestiais (Estrela, Planeta, Buraco Negro) */
typedef enum {
	BHS_CELESTIAL_PLANET,
	BHS_CELESTIAL_MOON,
	BHS_CELESTIAL_STAR,
	BHS_CELESTIAL_BLACKHOLE,
	BHS_CELESTIAL_ASTEROID
} bhs_celestial_type;

/* Propriedades de Lógica de Jogo */
typedef struct {
	double density;
	double radius;
	double j2; /* [NOVO] Achatamento (Oblateness) */
	bool has_atmosphere;
	char composition[64];
	struct bhs_vec3 color; // Cor Visual
	/* Parâmetros de Rotação */
	struct bhs_vec3 rotation_axis;
	double rotation_speed;	       /* rad/s */
	double current_rotation_angle; /* rad */
} bhs_planet_component;

typedef struct {
	double luminosity;
	double temp_effective;
	char spectral_type[8];
	struct bhs_vec3 color; // Cor Visual da Estrela
} bhs_star_component;

/**
 * Flags Visuais (Bitfield)
 */
typedef enum {
	BHS_VISUAL_FLAG_SHOW_TRAIL = (1 << 0),
	BHS_VISUAL_FLAG_SHOW_MARKERS = (1 << 1),
	BHS_VISUAL_FLAG_SHOW_VECTORS = (1 << 2), // Gravity/Velocity lines
	BHS_VISUAL_FLAG_SHOW_LABEL = (1 << 3)
} bhs_visual_flags;

/**
 * struct bhs_celestial_component
 * Propriedades de lógica de jogo para corpos celestes.
 */
typedef struct {
	bhs_celestial_type type;
	char name[64];
	uint32_t visual_flags; /* [NOVO] Controle de visibilidade per-entity */

	// Inline factories ou referências poderiam ser usadas,
	// mas manter plano (flat) é mais fácil por enquanto.
	union {
		bhs_planet_component planet;
		bhs_star_component star;
	} data;
} bhs_celestial_component;

/**
 * struct bhs_metadata_component
 * Armazena estado global da simulação que deve ser persistido,
 * mas não é um "corpo físico". Deve ser anexado a uma entidade dummy.
 */
typedef struct {
	double accumulated_time;
	int scenario_type;
	double time_scale_snapshot; /* Opcional: restaurar velocidade */
	/* [NOVO] Metadados Estendidos para UI */
	char display_name[64]; /* "Meu Save Legal" */
	char date_string[64];  /* "2026-01-26 10:00:00" */
} bhs_metadata_component;

#endif
