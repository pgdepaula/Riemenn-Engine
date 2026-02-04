/**
 * @file test_gravity_logic.c
 * @brief Unit test for validating major gravitational force calculation
 */

#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

/* Mock structures to simulate the needed data */
typedef struct {
	double x, y, z;
} vec3_t;

typedef struct {
	int id;
	double mass;
	vec3_t pos;
	const char *name;
} body_t;

/* 
 * The logic we want to test:
 * Find the body 'j' that exerts the maximum Force ~ Mass_j / Distance_sq_ij 
 * on body 'i'.
 */
int find_major_force_index(int current_idx, body_t *bodies, int n_bodies)
{
	int best_idx = -1;
	double max_force_score = -1.0;

	for (int j = 0; j < n_bodies; j++) {
		if (current_idx == j)
			continue;

		double dx = bodies[j].pos.x - bodies[current_idx].pos.x;
		double dy = bodies[j].pos.y - bodies[current_idx].pos.y;
		double dz = bodies[j].pos.z - bodies[current_idx].pos.z;

		double dist_sq = dx * dx + dy * dy + dz * dz;

		/* Avoid division by zero for identical positions */
		if (dist_sq < 1e-9)
			continue;

		/* Force ~ M / r^2 */
		double score = bodies[j].mass / dist_sq;

		if (score > max_force_score) {
			max_force_score = score;
			best_idx = j;
		}
	}
	return best_idx;
}

int main()
{
	printf("🧪 Testing Gravity Logic...\n");

	/* Setup Scenario: Mars close to Jupiter, but Sun is huge */
	/* 
       Sun: (0,0,0), Mass = 1000
       Jupiter: (100,0,0), Mass = 100
       Mars: (105,0,0), Mass = 1
       
       Dist Mars-Sun = 105. Force ~ 1000 / 105^2 = 1000 / 11025 ~= 0.09
       Dist Mars-Jupiter = 5. Force ~ 100 / 5^2 = 100 / 25 = 4.0
       
       Result: Mars should point to Jupiter.
    */

	body_t bodies[] = { { 0, 1000.0, { 0, 0, 0 }, "Sun" },
			    { 1, 100.0, { 100, 0, 0 }, "Jupiter" },
			    { 2, 1.0, { 105, 0, 0 }, "Mars" } };
	int n = 3;

	/* Check Mars (Index 2) */
	int major_idx = find_major_force_index(2, bodies, n);

	if (major_idx == 1) {
		printf("✅ SUCCESS: Mars points to Jupiter (Index 1). Force "
		       "Score comparison correct.\n");
		return 0;
	} else {
		printf("❌ FAILED: Mars points to %s (Index %d). Expected "
		       "Jupiter.\n",
		       (major_idx >= 0 ? bodies[major_idx].name : "None"),
		       major_idx);
		return 1;
	}
}
