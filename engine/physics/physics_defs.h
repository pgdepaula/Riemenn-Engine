/**
 * @file physics_defs.h
 * @brief Constantes e definições físicas fundamentais
 */

#ifndef RI_ENGINE_PHYSICS_DEFS_H
#define RI_ENGINE_PHYSICS_DEFS_H

/* Constante Gravitacional (CODATA 2018) */
#define IAU_G 6.67430e-11     /* m³/(kg·s²) */
#define IAU_C 299792458.0     /* m/s */
#define IAU_AU 1.495978707e11 /* m */

/* G para simulação (Normalizado se necessário, ou usar IAU_G) */
/* Se estivermos usando unidades SI reais: */
#define RI_G 1.0

#endif /* RI_ENGINE_PHYSICS_DEFS_H */
