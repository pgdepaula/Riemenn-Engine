#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <riengine.h>
#include <string.h>

// Helper macros for struct UserData
#define BHS_LUA_CHECK(L, i, type) (*(type **)luaL_checkudata(L, i, #type))

// --- bhs_vec4 binding ---
static int bhs_vec4_index(lua_State *L)
{
	struct bhs_vec4 *obj = luaL_checkudata(L, 1, "bhs_vec4");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "t") == 0) {
		lua_pushnumber(L, obj->t);
		return 1;
	}
	if (strcmp(key, "x") == 0) {
		lua_pushnumber(L, obj->x);
		return 1;
	}
	if (strcmp(key, "y") == 0) {
		lua_pushnumber(L, obj->y);
		return 1;
	}
	if (strcmp(key, "z") == 0) {
		lua_pushnumber(L, obj->z);
		return 1;
	}
	return 0;
}
static int bhs_vec4_newindex(lua_State *L)
{
	struct bhs_vec4 *obj = luaL_checkudata(L, 1, "bhs_vec4");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "t") == 0) {
		obj->t = (real_t)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "x") == 0) {
		obj->x = (real_t)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "y") == 0) {
		obj->y = (real_t)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "z") == 0) {
		obj->z = (real_t)luaL_checknumber(L, 3);
		return 0;
	}
	return 0;
}
static const struct luaL_Reg bhs_vec4_methods[] = {
	{ "__index", bhs_vec4_index },
	{ "__newindex", bhs_vec4_newindex },
	{ NULL, NULL }
};

// --- bhs_vec3 binding ---
static int bhs_vec3_index(lua_State *L)
{
	struct bhs_vec3 *obj = luaL_checkudata(L, 1, "bhs_vec3");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "x") == 0) {
		lua_pushnumber(L, obj->x);
		return 1;
	}
	if (strcmp(key, "y") == 0) {
		lua_pushnumber(L, obj->y);
		return 1;
	}
	if (strcmp(key, "z") == 0) {
		lua_pushnumber(L, obj->z);
		return 1;
	}
	return 0;
}
static int bhs_vec3_newindex(lua_State *L)
{
	struct bhs_vec3 *obj = luaL_checkudata(L, 1, "bhs_vec3");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "x") == 0) {
		obj->x = (real_t)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "y") == 0) {
		obj->y = (real_t)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "z") == 0) {
		obj->z = (real_t)luaL_checknumber(L, 3);
		return 0;
	}
	return 0;
}
static const struct luaL_Reg bhs_vec3_methods[] = {
	{ "__index", bhs_vec3_index },
	{ "__newindex", bhs_vec3_newindex },
	{ NULL, NULL }
};

// --- bhs_metric binding ---
static int bhs_metric_index(lua_State *L)
{
	struct bhs_metric *obj = luaL_checkudata(L, 1, "bhs_metric");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "g") == 0) {
		// Nested struct g not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	return 0;
}
static int bhs_metric_newindex(lua_State *L)
{
	struct bhs_metric *obj = luaL_checkudata(L, 1, "bhs_metric");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "g") == 0) {
		// Nested struct set not supported
		return 0;
	}
	return 0;
}
static const struct luaL_Reg bhs_metric_methods[] = {
	{ "__index", bhs_metric_index },
	{ "__newindex", bhs_metric_newindex },
	{ NULL, NULL }
};

// --- bhs_christoffel binding ---
static int bhs_christoffel_index(lua_State *L)
{
	struct bhs_christoffel *obj = luaL_checkudata(L, 1, "bhs_christoffel");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "gamma") == 0) {
		// Nested struct gamma not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	return 0;
}
static int bhs_christoffel_newindex(lua_State *L)
{
	struct bhs_christoffel *obj = luaL_checkudata(L, 1, "bhs_christoffel");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "gamma") == 0) {
		// Nested struct set not supported
		return 0;
	}
	return 0;
}
static const struct luaL_Reg bhs_christoffel_methods[] = {
	{ "__index", bhs_christoffel_index },
	{ "__newindex", bhs_christoffel_newindex },
	{ NULL, NULL }
};

// --- bhs_kerr binding ---
static int bhs_kerr_index(lua_State *L)
{
	struct bhs_kerr *obj = luaL_checkudata(L, 1, "bhs_kerr");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "M") == 0) {
		lua_pushnumber(L, obj->M);
		return 1;
	}
	if (strcmp(key, "a") == 0) {
		lua_pushnumber(L, obj->a);
		return 1;
	}
	return 0;
}
static int bhs_kerr_newindex(lua_State *L)
{
	struct bhs_kerr *obj = luaL_checkudata(L, 1, "bhs_kerr");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "M") == 0) {
		obj->M = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "a") == 0) {
		obj->a = (double)luaL_checknumber(L, 3);
		return 0;
	}
	return 0;
}
static const struct luaL_Reg bhs_kerr_methods[] = {
	{ "__index", bhs_kerr_index },
	{ "__newindex", bhs_kerr_newindex },
	{ NULL, NULL }
};

// --- bhs_schwarzschild binding ---
static int bhs_schwarzschild_index(lua_State *L)
{
	struct bhs_schwarzschild *obj =
		luaL_checkudata(L, 1, "bhs_schwarzschild");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "M") == 0) {
		lua_pushnumber(L, obj->M);
		return 1;
	}
	return 0;
}
static int bhs_schwarzschild_newindex(lua_State *L)
{
	struct bhs_schwarzschild *obj =
		luaL_checkudata(L, 1, "bhs_schwarzschild");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "M") == 0) {
		obj->M = (double)luaL_checknumber(L, 3);
		return 0;
	}
	return 0;
}
static const struct luaL_Reg bhs_schwarzschild_methods[] = {
	{ "__index", bhs_schwarzschild_index },
	{ "__newindex", bhs_schwarzschild_newindex },
	{ NULL, NULL }
};

// --- bhs_sun_desc binding ---
static int bhs_sun_desc_index(lua_State *L)
{
	struct bhs_sun_desc *obj = luaL_checkudata(L, 1, "bhs_sun_desc");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "name") == 0) {
		lua_pushstring(L, obj->name);
		return 1;
	}
	if (strcmp(key, "mass") == 0) {
		lua_pushnumber(L, obj->mass);
		return 1;
	}
	if (strcmp(key, "radius") == 0) {
		lua_pushnumber(L, obj->radius);
		return 1;
	}
	if (strcmp(key, "temperature") == 0) {
		lua_pushnumber(L, obj->temperature);
		return 1;
	}
	if (strcmp(key, "luminosity") == 0) {
		lua_pushnumber(L, obj->luminosity);
		return 1;
	}
	if (strcmp(key, "age") == 0) {
		lua_pushnumber(L, obj->age);
		return 1;
	}
	if (strcmp(key, "metallicity") == 0) {
		lua_pushnumber(L, obj->metallicity);
		return 1;
	}
	if (strcmp(key, "stage") == 0) {
		// Nested struct stage not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "spectral_type") == 0) {
		// Nested struct spectral_type not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "rotation_period") == 0) {
		lua_pushnumber(L, obj->rotation_period);
		return 1;
	}
	if (strcmp(key, "axis_tilt") == 0) {
		lua_pushnumber(L, obj->axis_tilt);
		return 1;
	}
	if (strcmp(key, "base_color") == 0) {
		// Nested struct base_color not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "p_local)") == 0) {
		// Nested struct p_local) not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	return 0;
}
static int bhs_sun_desc_newindex(lua_State *L)
{
	struct bhs_sun_desc *obj = luaL_checkudata(L, 1, "bhs_sun_desc");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "name") == 0) {
		obj->name = (const char *)luaL_checkstring(L, 3);
		return 0;
	}
	if (strcmp(key, "mass") == 0) {
		obj->mass = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "radius") == 0) {
		obj->radius = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "temperature") == 0) {
		obj->temperature = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "luminosity") == 0) {
		obj->luminosity = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "age") == 0) {
		obj->age = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "metallicity") == 0) {
		obj->metallicity = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "stage") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "spectral_type") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "rotation_period") == 0) {
		obj->rotation_period = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "axis_tilt") == 0) {
		obj->axis_tilt = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "base_color") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "p_local)") == 0) {
		// Nested struct set not supported
		return 0;
	}
	return 0;
}
static const struct luaL_Reg bhs_sun_desc_methods[] = {
	{ "__index", bhs_sun_desc_index },
	{ "__newindex", bhs_sun_desc_newindex },
	{ NULL, NULL }
};

// --- bhs_blackhole_desc binding ---
static int bhs_blackhole_desc_index(lua_State *L)
{
	struct bhs_blackhole_desc *obj =
		luaL_checkudata(L, 1, "bhs_blackhole_desc");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "name") == 0) {
		lua_pushstring(L, obj->name);
		return 1;
	}
	if (strcmp(key, "mass") == 0) {
		lua_pushnumber(L, obj->mass);
		return 1;
	}
	if (strcmp(key, "spin") == 0) {
		lua_pushnumber(L, obj->spin);
		return 1;
	}
	if (strcmp(key, "charge") == 0) {
		lua_pushnumber(L, obj->charge);
		return 1;
	}
	if (strcmp(key, "event_horizon_r") == 0) {
		lua_pushnumber(L, obj->event_horizon_r);
		return 1;
	}
	if (strcmp(key, "accretion_disk_mass") == 0) {
		lua_pushnumber(L, obj->accretion_disk_mass);
		return 1;
	}
	if (strcmp(key, "base_color") == 0) {
		// Nested struct base_color not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "p_local)") == 0) {
		// Nested struct p_local) not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	return 0;
}
static int bhs_blackhole_desc_newindex(lua_State *L)
{
	struct bhs_blackhole_desc *obj =
		luaL_checkudata(L, 1, "bhs_blackhole_desc");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "name") == 0) {
		obj->name = (const char *)luaL_checkstring(L, 3);
		return 0;
	}
	if (strcmp(key, "mass") == 0) {
		obj->mass = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "spin") == 0) {
		obj->spin = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "charge") == 0) {
		obj->charge = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "event_horizon_r") == 0) {
		obj->event_horizon_r = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "accretion_disk_mass") == 0) {
		obj->accretion_disk_mass = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "base_color") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "p_local)") == 0) {
		// Nested struct set not supported
		return 0;
	}
	return 0;
}
static const struct luaL_Reg bhs_blackhole_desc_methods[] = {
	{ "__index", bhs_blackhole_desc_index },
	{ "__newindex", bhs_blackhole_desc_newindex },
	{ NULL, NULL }
};

// --- bhs_planet_data binding ---
static int bhs_planet_data_index(lua_State *L)
{
	struct bhs_planet_data *obj = luaL_checkudata(L, 1, "bhs_planet_data");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "density") == 0) {
		lua_pushnumber(L, obj->density);
		return 1;
	}
	if (strcmp(key, "axis_tilt") == 0) {
		lua_pushnumber(L, obj->axis_tilt);
		return 1;
	}
	if (strcmp(key, "rotation_period") == 0) {
		lua_pushnumber(L, obj->rotation_period);
		return 1;
	}
	if (strcmp(key, "j2") == 0) {
		lua_pushnumber(L, obj->j2);
		return 1;
	}
	if (strcmp(key, "albedo") == 0) {
		lua_pushnumber(L, obj->albedo);
		return 1;
	}
	if (strcmp(key, "has_atmosphere") == 0) {
		lua_pushboolean(L, obj->has_atmosphere);
		return 1;
	}
	if (strcmp(key, "surface_pressure") == 0) {
		lua_pushnumber(L, obj->surface_pressure);
		return 1;
	}
	if (strcmp(key, "atmosphere_mass") == 0) {
		lua_pushnumber(L, obj->atmosphere_mass);
		return 1;
	}
	if (strcmp(key, "composition") == 0) {
		// Nested struct composition not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "temperature") == 0) {
		lua_pushnumber(L, obj->temperature);
		return 1;
	}
	if (strcmp(key, "heat_capacity") == 0) {
		lua_pushnumber(L, obj->heat_capacity);
		return 1;
	}
	if (strcmp(key, "energy_flux") == 0) {
		lua_pushnumber(L, obj->energy_flux);
		return 1;
	}
	if (strcmp(key, "physical_state") == 0) {
		lua_pushinteger(L, obj->physical_state);
		return 1;
	}
	if (strcmp(key, "has_magnetic_field") == 0) {
		lua_pushboolean(L, obj->has_magnetic_field);
		return 1;
	}
	return 0;
}
static int bhs_planet_data_newindex(lua_State *L)
{
	struct bhs_planet_data *obj = luaL_checkudata(L, 1, "bhs_planet_data");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "density") == 0) {
		obj->density = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "axis_tilt") == 0) {
		obj->axis_tilt = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "rotation_period") == 0) {
		obj->rotation_period = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "j2") == 0) {
		obj->j2 = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "albedo") == 0) {
		obj->albedo = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "has_atmosphere") == 0) {
		obj->has_atmosphere = (bool)lua_toboolean(L, 3);
		return 0;
	}
	if (strcmp(key, "surface_pressure") == 0) {
		obj->surface_pressure = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "atmosphere_mass") == 0) {
		obj->atmosphere_mass = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "composition") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "temperature") == 0) {
		obj->temperature = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "heat_capacity") == 0) {
		obj->heat_capacity = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "energy_flux") == 0) {
		obj->energy_flux = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "physical_state") == 0) {
		obj->physical_state = (int)luaL_checkinteger(L, 3);
		return 0;
	}
	if (strcmp(key, "has_magnetic_field") == 0) {
		obj->has_magnetic_field = (bool)lua_toboolean(L, 3);
		return 0;
	}
	return 0;
}
static const struct luaL_Reg bhs_planet_data_methods[] = {
	{ "__index", bhs_planet_data_index },
	{ "__newindex", bhs_planet_data_newindex },
	{ NULL, NULL }
};

// --- bhs_star_data binding ---
static int bhs_star_data_index(lua_State *L)
{
	struct bhs_star_data *obj = luaL_checkudata(L, 1, "bhs_star_data");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "luminosity") == 0) {
		lua_pushnumber(L, obj->luminosity);
		return 1;
	}
	if (strcmp(key, "temp_effective") == 0) {
		lua_pushnumber(L, obj->temp_effective);
		return 1;
	}
	if (strcmp(key, "age") == 0) {
		lua_pushnumber(L, obj->age);
		return 1;
	}
	if (strcmp(key, "density") == 0) {
		lua_pushnumber(L, obj->density);
		return 1;
	}
	if (strcmp(key, "hydrogen_frac") == 0) {
		lua_pushnumber(L, obj->hydrogen_frac);
		return 1;
	}
	if (strcmp(key, "helium_frac") == 0) {
		lua_pushnumber(L, obj->helium_frac);
		return 1;
	}
	if (strcmp(key, "metals_frac") == 0) {
		lua_pushnumber(L, obj->metals_frac);
		return 1;
	}
	if (strcmp(key, "stage") == 0) {
		lua_pushinteger(L, obj->stage);
		return 1;
	}
	if (strcmp(key, "metallicity") == 0) {
		lua_pushnumber(L, obj->metallicity);
		return 1;
	}
	if (strcmp(key, "spectral_type") == 0) {
		// Nested struct spectral_type not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	return 0;
}
static int bhs_star_data_newindex(lua_State *L)
{
	struct bhs_star_data *obj = luaL_checkudata(L, 1, "bhs_star_data");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "luminosity") == 0) {
		obj->luminosity = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "temp_effective") == 0) {
		obj->temp_effective = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "age") == 0) {
		obj->age = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "density") == 0) {
		obj->density = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "hydrogen_frac") == 0) {
		obj->hydrogen_frac = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "helium_frac") == 0) {
		obj->helium_frac = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "metals_frac") == 0) {
		obj->metals_frac = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "stage") == 0) {
		obj->stage = (int)luaL_checkinteger(L, 3);
		return 0;
	}
	if (strcmp(key, "metallicity") == 0) {
		obj->metallicity = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "spectral_type") == 0) {
		// Nested struct set not supported
		return 0;
	}
	return 0;
}
static const struct luaL_Reg bhs_star_data_methods[] = {
	{ "__index", bhs_star_data_index },
	{ "__newindex", bhs_star_data_newindex },
	{ NULL, NULL }
};

// --- bhs_blackhole_data binding ---
static int bhs_blackhole_data_index(lua_State *L)
{
	struct bhs_blackhole_data *obj =
		luaL_checkudata(L, 1, "bhs_blackhole_data");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "spin_factor") == 0) {
		lua_pushnumber(L, obj->spin_factor);
		return 1;
	}
	if (strcmp(key, "event_horizon_r") == 0) {
		lua_pushnumber(L, obj->event_horizon_r);
		return 1;
	}
	if (strcmp(key, "ergososphere_r") == 0) {
		lua_pushnumber(L, obj->ergososphere_r);
		return 1;
	}
	if (strcmp(key, "accretion_disk_mass") == 0) {
		lua_pushnumber(L, obj->accretion_disk_mass);
		return 1;
	}
	if (strcmp(key, "accretion_rate") == 0) {
		lua_pushnumber(L, obj->accretion_rate);
		return 1;
	}
	return 0;
}
static int bhs_blackhole_data_newindex(lua_State *L)
{
	struct bhs_blackhole_data *obj =
		luaL_checkudata(L, 1, "bhs_blackhole_data");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "spin_factor") == 0) {
		obj->spin_factor = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "event_horizon_r") == 0) {
		obj->event_horizon_r = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "ergososphere_r") == 0) {
		obj->ergososphere_r = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "accretion_disk_mass") == 0) {
		obj->accretion_disk_mass = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "accretion_rate") == 0) {
		obj->accretion_rate = (double)luaL_checknumber(L, 3);
		return 0;
	}
	return 0;
}
static const struct luaL_Reg bhs_blackhole_data_methods[] = {
	{ "__index", bhs_blackhole_data_index },
	{ "__newindex", bhs_blackhole_data_newindex },
	{ NULL, NULL }
};

// --- bhs_body_state binding ---
static int bhs_body_state_index(lua_State *L)
{
	struct bhs_body_state *obj = luaL_checkudata(L, 1, "bhs_body_state");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "pos") == 0) {
		// Nested struct pos not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "vel") == 0) {
		// Nested struct vel not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "acc") == 0) {
		// Nested struct acc not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "rot_axis") == 0) {
		// Nested struct rot_axis not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "rot_speed") == 0) {
		lua_pushnumber(L, obj->rot_speed);
		return 1;
	}
	if (strcmp(key, "moment_inertia") == 0) {
		lua_pushnumber(L, obj->moment_inertia);
		return 1;
	}
	if (strcmp(key, "mass") == 0) {
		lua_pushnumber(L, obj->mass);
		return 1;
	}
	if (strcmp(key, "radius") == 0) {
		lua_pushnumber(L, obj->radius);
		return 1;
	}
	if (strcmp(key, "current_rotation_angle") == 0) {
		lua_pushnumber(L, obj->current_rotation_angle);
		return 1;
	}
	if (strcmp(key, "shape") == 0) {
		lua_pushinteger(L, obj->shape);
		return 1;
	}
	return 0;
}
static int bhs_body_state_newindex(lua_State *L)
{
	struct bhs_body_state *obj = luaL_checkudata(L, 1, "bhs_body_state");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "pos") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "vel") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "acc") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "rot_axis") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "rot_speed") == 0) {
		obj->rot_speed = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "moment_inertia") == 0) {
		obj->moment_inertia = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "mass") == 0) {
		obj->mass = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "radius") == 0) {
		obj->radius = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "current_rotation_angle") == 0) {
		obj->current_rotation_angle = (double)luaL_checknumber(L, 3);
		return 0;
	}
	if (strcmp(key, "shape") == 0) {
		obj->shape = (int)luaL_checkinteger(L, 3);
		return 0;
	}
	return 0;
}
static const struct luaL_Reg bhs_body_state_methods[] = {
	{ "__index", bhs_body_state_index },
	{ "__newindex", bhs_body_state_newindex },
	{ NULL, NULL }
};

// --- bhs_body binding ---
static int bhs_body_index(lua_State *L)
{
	struct bhs_body *obj = luaL_checkudata(L, 1, "bhs_body");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "state") == 0) {
		// Nested struct state not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "type") == 0) {
		// Nested struct type not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "prop") == 0) {
		// Nested struct prop not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "color") == 0) {
		// Nested struct color not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "is_fixed") == 0) {
		lua_pushboolean(L, obj->is_fixed);
		return 1;
	}
	if (strcmp(key, "is_alive") == 0) {
		lua_pushboolean(L, obj->is_alive);
		return 1;
	}
	if (strcmp(key, "name") == 0) {
		// Nested struct name not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "trail_positions") == 0) {
		// Nested struct trail_positions not fully supported in auto-gen
		lua_pushnil(L);
		return 1;
	}
	if (strcmp(key, "trail_head") == 0) {
		lua_pushinteger(L, obj->trail_head);
		return 1;
	}
	if (strcmp(key, "trail_count") == 0) {
		lua_pushinteger(L, obj->trail_count);
		return 1;
	}
	return 0;
}
static int bhs_body_newindex(lua_State *L)
{
	struct bhs_body *obj = luaL_checkudata(L, 1, "bhs_body");
	const char *key = luaL_checkstring(L, 2);
	if (strcmp(key, "state") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "type") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "prop") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "color") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "is_fixed") == 0) {
		obj->is_fixed = (bool)lua_toboolean(L, 3);
		return 0;
	}
	if (strcmp(key, "is_alive") == 0) {
		obj->is_alive = (bool)lua_toboolean(L, 3);
		return 0;
	}
	if (strcmp(key, "name") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "trail_positions") == 0) {
		// Nested struct set not supported
		return 0;
	}
	if (strcmp(key, "trail_head") == 0) {
		obj->trail_head = (int)luaL_checkinteger(L, 3);
		return 0;
	}
	if (strcmp(key, "trail_count") == 0) {
		obj->trail_count = (int)luaL_checkinteger(L, 3);
		return 0;
	}
	return 0;
}
static const struct luaL_Reg bhs_body_methods[] = {
	{ "__index", bhs_body_index },
	{ "__newindex", bhs_body_newindex },
	{ NULL, NULL }
};

static int l_bhs_vec4_add(lua_State *L)
{
	struct bhs_vec4 arg0 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 1, "struct bhs_vec4");
	struct bhs_vec4 arg1 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 2, "struct bhs_vec4");
	struct bhs_vec4 ret = bhs_vec4_add(arg0, arg1);
	struct bhs_vec4 *ret_obj = lua_newuserdata(L, sizeof(struct bhs_vec4));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_vec4");
	return 1;
}

static int l_bhs_vec4_sub(lua_State *L)
{
	struct bhs_vec4 arg0 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 1, "struct bhs_vec4");
	struct bhs_vec4 arg1 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 2, "struct bhs_vec4");
	struct bhs_vec4 ret = bhs_vec4_sub(arg0, arg1);
	struct bhs_vec4 *ret_obj = lua_newuserdata(L, sizeof(struct bhs_vec4));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_vec4");
	return 1;
}

static int l_bhs_vec4_scale(lua_State *L)
{
	struct bhs_vec4 arg0 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 1, "struct bhs_vec4");
	real_t arg1 = (real_t)luaL_checknumber(L, 2);
	struct bhs_vec4 ret = bhs_vec4_scale(arg0, arg1);
	struct bhs_vec4 *ret_obj = lua_newuserdata(L, sizeof(struct bhs_vec4));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_vec4");
	return 1;
}

static int l_bhs_vec4_neg(lua_State *L)
{
	struct bhs_vec4 arg0 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 1, "struct bhs_vec4");
	struct bhs_vec4 ret = bhs_vec4_neg(arg0);
	struct bhs_vec4 *ret_obj = lua_newuserdata(L, sizeof(struct bhs_vec4));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_vec4");
	return 1;
}

static int l_bhs_vec4_dot_minkowski(lua_State *L)
{
	struct bhs_vec4 arg0 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 1, "struct bhs_vec4");
	struct bhs_vec4 arg1 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 2, "struct bhs_vec4");
	real_t ret = bhs_vec4_dot_minkowski(arg0, arg1);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_vec4_norm2_minkowski(lua_State *L)
{
	struct bhs_vec4 arg0 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 1, "struct bhs_vec4");
	real_t ret = bhs_vec4_norm2_minkowski(arg0);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_vec4_is_null(lua_State *L)
{
	struct bhs_vec4 arg0 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 1, "struct bhs_vec4");
	real_t arg1 = (real_t)luaL_checknumber(L, 2);
	bool ret = bhs_vec4_is_null(arg0, arg1);
	lua_pushboolean(L, ret);
	return 1;
}

static int l_bhs_vec4_is_timelike(lua_State *L)
{
	struct bhs_vec4 arg0 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 1, "struct bhs_vec4");
	bool ret = bhs_vec4_is_timelike(arg0);
	lua_pushboolean(L, ret);
	return 1;
}

static int l_bhs_vec4_is_spacelike(lua_State *L)
{
	struct bhs_vec4 arg0 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 1, "struct bhs_vec4");
	bool ret = bhs_vec4_is_spacelike(arg0);
	lua_pushboolean(L, ret);
	return 1;
}

static int l_bhs_vec3_add(lua_State *L)
{
	struct bhs_vec3 arg0 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 1, "struct bhs_vec3");
	struct bhs_vec3 arg1 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 2, "struct bhs_vec3");
	struct bhs_vec3 ret = bhs_vec3_add(arg0, arg1);
	struct bhs_vec3 *ret_obj = lua_newuserdata(L, sizeof(struct bhs_vec3));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_vec3");
	return 1;
}

static int l_bhs_vec3_sub(lua_State *L)
{
	struct bhs_vec3 arg0 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 1, "struct bhs_vec3");
	struct bhs_vec3 arg1 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 2, "struct bhs_vec3");
	struct bhs_vec3 ret = bhs_vec3_sub(arg0, arg1);
	struct bhs_vec3 *ret_obj = lua_newuserdata(L, sizeof(struct bhs_vec3));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_vec3");
	return 1;
}

static int l_bhs_vec3_scale(lua_State *L)
{
	struct bhs_vec3 arg0 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 1, "struct bhs_vec3");
	real_t arg1 = (real_t)luaL_checknumber(L, 2);
	struct bhs_vec3 ret = bhs_vec3_scale(arg0, arg1);
	struct bhs_vec3 *ret_obj = lua_newuserdata(L, sizeof(struct bhs_vec3));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_vec3");
	return 1;
}

static int l_bhs_vec3_dot(lua_State *L)
{
	struct bhs_vec3 arg0 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 1, "struct bhs_vec3");
	struct bhs_vec3 arg1 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 2, "struct bhs_vec3");
	real_t ret = bhs_vec3_dot(arg0, arg1);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_vec3_cross(lua_State *L)
{
	struct bhs_vec3 arg0 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 1, "struct bhs_vec3");
	struct bhs_vec3 arg1 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 2, "struct bhs_vec3");
	struct bhs_vec3 ret = bhs_vec3_cross(arg0, arg1);
	struct bhs_vec3 *ret_obj = lua_newuserdata(L, sizeof(struct bhs_vec3));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_vec3");
	return 1;
}

static int l_bhs_vec3_norm(lua_State *L)
{
	struct bhs_vec3 arg0 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 1, "struct bhs_vec3");
	real_t ret = bhs_vec3_norm(arg0);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_vec3_norm2(lua_State *L)
{
	struct bhs_vec3 arg0 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 1, "struct bhs_vec3");
	real_t ret = bhs_vec3_norm2(arg0);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_vec3_normalize(lua_State *L)
{
	struct bhs_vec3 arg0 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 1, "struct bhs_vec3");
	struct bhs_vec3 ret = bhs_vec3_normalize(arg0);
	struct bhs_vec3 *ret_obj = lua_newuserdata(L, sizeof(struct bhs_vec3));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_vec3");
	return 1;
}

static int l_bhs_vec3_to_spherical(lua_State *L)
{
	struct bhs_vec3 arg0 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 1, "struct bhs_vec3");
	real_t *arg1 = luaL_checkudata(L, 2, "real_t");
	real_t *arg2 = luaL_checkudata(L, 3, "real_t");
	real_t *arg3 = luaL_checkudata(L, 4, "real_t");
	bhs_vec3_to_spherical(arg0, arg1, arg2, arg3);
	return 0;
}

static int l_bhs_vec3_from_spherical(lua_State *L)
{
	real_t arg0 = (real_t)luaL_checknumber(L, 1);
	real_t arg1 = (real_t)luaL_checknumber(L, 2);
	real_t arg2 = (real_t)luaL_checknumber(L, 3);
	struct bhs_vec3 ret = bhs_vec3_from_spherical(arg0, arg1, arg2);
	struct bhs_vec3 *ret_obj = lua_newuserdata(L, sizeof(struct bhs_vec3));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_vec3");
	return 1;
}

static int l_bhs_ecs_create_world(lua_State *L)
{
	bhs_world_handle ret = bhs_ecs_create_world();
	bhs_world_handle *ret_obj =
		lua_newuserdata(L, sizeof(bhs_world_handle));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_world_handle");
	return 1;
}

static int l_bhs_ecs_destroy_world(lua_State *L)
{
	bhs_world_handle arg0 = luaL_checkudata(L, 1, "bhs_world_handle");
	bhs_ecs_destroy_world(arg0);
	return 0;
}

static int l_bhs_ecs_create_entity(lua_State *L)
{
	bhs_world_handle arg0 = luaL_checkudata(L, 1, "bhs_world_handle");
	bhs_entity_id ret = bhs_ecs_create_entity(arg0);
	bhs_entity_id *ret_obj = lua_newuserdata(L, sizeof(bhs_entity_id));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_entity_id");
	return 1;
}

static int l_bhs_ecs_destroy_entity(lua_State *L)
{
	bhs_world_handle arg0 = luaL_checkudata(L, 1, "bhs_world_handle");
	bhs_entity_id arg1 = luaL_checkudata(L, 2, "bhs_entity_id");
	bhs_ecs_destroy_entity(arg0, arg1);
	return 0;
}

static int l_bhs_ecs_add_component(lua_State *L)
{
	bhs_world_handle arg0 = luaL_checkudata(L, 1, "bhs_world_handle");
	bhs_entity_id arg1 = luaL_checkudata(L, 2, "bhs_entity_id");
	bhs_component_type arg2 = luaL_checkudata(L, 3, "bhs_component_type");
	size_t arg3 = luaL_checkudata(L, 4, "size_t");
	const void *arg4 = luaL_checkudata(L, 5, "const void");
	void *ret = bhs_ecs_add_component(arg0, arg1, arg2, arg3, arg4);
	void *ret_obj = lua_newuserdata(L, sizeof(void));
	*ret_obj = ret;
	luaL_setmetatable(L, "void");
	return 1;
}

static int l_bhs_ecs_remove_component(lua_State *L)
{
	bhs_world_handle arg0 = luaL_checkudata(L, 1, "bhs_world_handle");
	bhs_entity_id arg1 = luaL_checkudata(L, 2, "bhs_entity_id");
	bhs_component_type arg2 = luaL_checkudata(L, 3, "bhs_component_type");
	bhs_ecs_remove_component(arg0, arg1, arg2);
	return 0;
}

static int l_bhs_ecs_get_component(lua_State *L)
{
	bhs_world_handle arg0 = luaL_checkudata(L, 1, "bhs_world_handle");
	bhs_entity_id arg1 = luaL_checkudata(L, 2, "bhs_entity_id");
	bhs_component_type arg2 = luaL_checkudata(L, 3, "bhs_component_type");
	void *ret = bhs_ecs_get_component(arg0, arg1, arg2);
	void *ret_obj = lua_newuserdata(L, sizeof(void));
	*ret_obj = ret;
	luaL_setmetatable(L, "void");
	return 1;
}

static int l_bhs_ecs_query_init(lua_State *L)
{
	bhs_ecs_query *arg0 = luaL_checkudata(L, 1, "bhs_ecs_query");
	bhs_world_handle arg1 = luaL_checkudata(L, 2, "bhs_world_handle");
	bhs_component_mask arg2 = luaL_checkudata(L, 3, "bhs_component_mask");
	bhs_ecs_query_init(arg0, arg1, arg2);
	return 0;
}

static int l_bhs_ecs_query_init_cached(lua_State *L)
{
	bhs_ecs_query *arg0 = luaL_checkudata(L, 1, "bhs_ecs_query");
	bhs_world_handle arg1 = luaL_checkudata(L, 2, "bhs_world_handle");
	bhs_component_mask arg2 = luaL_checkudata(L, 3, "bhs_component_mask");
	bhs_ecs_query_init_cached(arg0, arg1, arg2);
	return 0;
}

static int l_bhs_ecs_query_next(lua_State *L)
{
	bhs_ecs_query *arg0 = luaL_checkudata(L, 1, "bhs_ecs_query");
	bhs_entity_id *arg1 = luaL_checkudata(L, 2, "bhs_entity_id");
	bool ret = bhs_ecs_query_next(arg0, arg1);
	lua_pushboolean(L, ret);
	return 1;
}

static int l_bhs_ecs_query_reset(lua_State *L)
{
	bhs_ecs_query *arg0 = luaL_checkudata(L, 1, "bhs_ecs_query");
	bhs_ecs_query_reset(arg0);
	return 0;
}

static int l_bhs_ecs_query_destroy(lua_State *L)
{
	bhs_ecs_query *arg0 = luaL_checkudata(L, 1, "bhs_ecs_query");
	bhs_ecs_query_destroy(arg0);
	return 0;
}

static int l_bhs_ecs_entity_has_components(lua_State *L)
{
	bhs_world_handle arg0 = luaL_checkudata(L, 1, "bhs_world_handle");
	bhs_entity_id arg1 = luaL_checkudata(L, 2, "bhs_entity_id");
	bhs_component_mask arg2 = luaL_checkudata(L, 3, "bhs_component_mask");
	bool ret = bhs_ecs_entity_has_components(arg0, arg1, arg2);
	lua_pushboolean(L, ret);
	return 1;
}

static int l_bhs_ecs_save_world(lua_State *L)
{
	bhs_world_handle arg0 = luaL_checkudata(L, 1, "bhs_world_handle");
	const char *arg1 = (const char *)luaL_checkstring(L, 2);
	bool ret = bhs_ecs_save_world(arg0, arg1);
	lua_pushboolean(L, ret);
	return 1;
}

static int l_bhs_ecs_load_world(lua_State *L)
{
	bhs_world_handle arg0 = luaL_checkudata(L, 1, "bhs_world_handle");
	const char *arg1 = (const char *)luaL_checkstring(L, 2);
	bool ret = bhs_ecs_load_world(arg0, arg1);
	lua_pushboolean(L, ret);
	return 1;
}

static int l_bhs_ecs_peek_metadata(lua_State *L)
{
	const char *arg0 = (const char *)luaL_checkstring(L, 1);
	void *arg1 = luaL_checkudata(L, 2, "void");
	size_t arg2 = luaL_checkudata(L, 3, "size_t");
	uint32_t arg3 = luaL_checkudata(L, 4, "uint32_t");
	bool ret = bhs_ecs_peek_metadata(arg0, arg1, arg2, arg3);
	lua_pushboolean(L, ret);
	return 1;
}

static int l_bhs_ecs_update_metadata(lua_State *L)
{
	const char *arg0 = (const char *)luaL_checkstring(L, 1);
	const void *arg1 = luaL_checkudata(L, 2, "const void");
	size_t arg2 = luaL_checkudata(L, 3, "size_t");
	uint32_t arg3 = luaL_checkudata(L, 4, "uint32_t");
	bool ret = bhs_ecs_update_metadata(arg0, arg1, arg2, arg3);
	lua_pushboolean(L, ret);
	return 1;
}

static int l_f(lua_State *L)
{
	stderr arg0 = luaL_checkudata(L, 1, "stderr");
    "[ASSERT FALHOU] %s:%d: arg1 = luaL_checkudata(L, 2, ""[ASSERT FALHOU] %s:%d:");
    __FILE__ arg2 = luaL_checkudata(L, 3, "__FILE__");
    __LINE__ arg3 = luaL_checkudata(L, 4, "__LINE__");
    \ arg4 = luaL_checkudata(L, 5, "\");
    fprint ret = f(arg0, arg1, arg2, arg3, arg4);
    fprint *ret_obj = lua_newuserdata(L, sizeof(fprint));
    *ret_obj = ret;
    luaL_setmetatable(L, "fprint");
    return 1;
}

static int l_t(lua_State *L)
{
	abor ret = t();
	abor *ret_obj = lua_newuserdata(L, sizeof(abor));
	*ret_obj = ret;
	luaL_setmetatable(L, "abor");
	return 1;
}

static int l_f(lua_State *L)
{
	stderr arg0 = luaL_checkudata(L, 1, "stderr");
    "[FATAL] Código inalcançável atingido em arg1 = luaL_checkudata(L, 2, ""[FATAL] Código inalcançável atingido em");
    \ arg2 = luaL_checkudata(L, 3, "\");
    __LINE__ arg3 = luaL_checkudata(L, 4, "__LINE__");
    fprint ret = f(arg0, arg1, arg2, arg3);
    fprint *ret_obj = lua_newuserdata(L, sizeof(fprint));
    *ret_obj = ret;
    luaL_setmetatable(L, "fprint");
    return 1;
}

static int l_t(lua_State *L)
{
	abor ret = t();
	abor *ret_obj = lua_newuserdata(L, sizeof(abor));
	*ret_obj = ret;
	luaL_setmetatable(L, "abor");
	return 1;
}

static int l_bhs_metric_zero(lua_State *L)
{
	struct bhs_metric ret = bhs_metric_zero();
	struct bhs_metric *ret_obj =
		lua_newuserdata(L, sizeof(struct bhs_metric));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_metric");
	return 1;
}

static int l_bhs_metric_minkowski(lua_State *L)
{
	struct bhs_metric ret = bhs_metric_minkowski();
	struct bhs_metric *ret_obj =
		lua_newuserdata(L, sizeof(struct bhs_metric));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_metric");
	return 1;
}

static int l_bhs_metric_diag(lua_State *L)
{
	real_t arg0 = (real_t)luaL_checknumber(L, 1);
	real_t arg1 = (real_t)luaL_checknumber(L, 2);
	real_t arg2 = (real_t)luaL_checknumber(L, 3);
	real_t arg3 = (real_t)luaL_checknumber(L, 4);
	struct bhs_metric ret = bhs_metric_diag(arg0, arg1, arg2, arg3);
	struct bhs_metric *ret_obj =
		lua_newuserdata(L, sizeof(struct bhs_metric));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_metric");
	return 1;
}

static int l_bhs_metric_is_symmetric(lua_State *L)
{
	const struct bhs_metric *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_metric");
	real_t arg1 = (real_t)luaL_checknumber(L, 2);
	bool ret = bhs_metric_is_symmetric(arg0, arg1);
	lua_pushboolean(L, ret);
	return 1;
}

static int l_bhs_metric_det(lua_State *L)
{
	const struct bhs_metric *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_metric");
	real_t ret = bhs_metric_det(arg0);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_metric_invert(lua_State *L)
{
	const struct bhs_metric *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_metric");
	struct bhs_metric *arg1 = luaL_checkudata(L, 2, "struct bhs_metric");
	int ret = bhs_metric_invert(arg0, arg1);
	lua_pushinteger(L, ret);
	return 1;
}

static int l_bhs_metric_lower(lua_State *L)
{
	const struct bhs_metric *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_metric");
	struct bhs_vec4 arg1 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 2, "struct bhs_vec4");
	struct bhs_vec4 ret = bhs_metric_lower(arg0, arg1);
	struct bhs_vec4 *ret_obj = lua_newuserdata(L, sizeof(struct bhs_vec4));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_vec4");
	return 1;
}

static int l_bhs_metric_raise(lua_State *L)
{
	const struct bhs_metric *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_metric");
	struct bhs_vec4 arg1 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 2, "struct bhs_vec4");
	struct bhs_vec4 ret = bhs_metric_raise(arg0, arg1);
	struct bhs_vec4 *ret_obj = lua_newuserdata(L, sizeof(struct bhs_vec4));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_vec4");
	return 1;
}

static int l_bhs_metric_dot(lua_State *L)
{
	const struct bhs_metric *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_metric");
	struct bhs_vec4 arg1 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 2, "struct bhs_vec4");
	struct bhs_vec4 arg2 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 3, "struct bhs_vec4");
	real_t ret = bhs_metric_dot(arg0, arg1, arg2);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_christoffel_compute(lua_State *L)
{
	bhs_metric_func arg0 = luaL_checkudata(L, 1, "bhs_metric_func");
	struct bhs_vec4 arg1 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 2, "struct bhs_vec4");
	void *arg2 = luaL_checkudata(L, 3, "void");
	real_t arg3 = (real_t)luaL_checknumber(L, 4);
	struct bhs_christoffel *arg4 =
		luaL_checkudata(L, 5, "struct bhs_christoffel");
	int ret = bhs_christoffel_compute(arg0, arg1, arg2, arg3, arg4);
	lua_pushinteger(L, ret);
	return 1;
}

static int l_bhs_christoffel_zero(lua_State *L)
{
	struct bhs_christoffel ret = bhs_christoffel_zero();
	struct bhs_christoffel *ret_obj =
		lua_newuserdata(L, sizeof(struct bhs_christoffel));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_christoffel");
	return 1;
}

static int l_bhs_geodesic_accel(lua_State *L)
{
	const struct bhs_christoffel *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_christoffel");
	struct bhs_vec4 arg1 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 2, "struct bhs_vec4");
	struct bhs_vec4 ret = bhs_geodesic_accel(arg0, arg1);
	struct bhs_vec4 *ret_obj = lua_newuserdata(L, sizeof(struct bhs_vec4));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_vec4");
	return 1;
}

static int l_s(lua_State *L)
{
	theta arg0 = luaL_checkudata(L, 1, "theta");
	co ret = s(arg0);
	co *ret_obj = lua_newuserdata(L, sizeof(co));
	*ret_obj = ret;
	luaL_setmetatable(L, "co");
	return 1;
}

static int l_bhs_kerr_horizon_outer(lua_State *L)
{
	const struct bhs_kerr *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_kerr");
	double ret = bhs_kerr_horizon_outer(arg0);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_kerr_horizon_inner(lua_State *L)
{
	const struct bhs_kerr *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_kerr");
	double ret = bhs_kerr_horizon_inner(arg0);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_kerr_ergosphere(lua_State *L)
{
	const struct bhs_kerr *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_kerr");
	double arg1 = (double)luaL_checknumber(L, 2);
	double ret = bhs_kerr_ergosphere(arg0, arg1);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_kerr_isco(lua_State *L)
{
	const struct bhs_kerr *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_kerr");
	bool arg1 = (bool)lua_toboolean(L, 2);
	double ret = bhs_kerr_isco(arg0, arg1);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_kerr_omega_frame(lua_State *L)
{
	const struct bhs_kerr *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_kerr");
	double arg1 = (double)luaL_checknumber(L, 2);
	double arg2 = (double)luaL_checknumber(L, 3);
	double ret = bhs_kerr_omega_frame(arg0, arg1, arg2);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_kerr_metric(lua_State *L)
{
	const struct bhs_kerr *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_kerr");
	double arg1 = (double)luaL_checknumber(L, 2);
	double arg2 = (double)luaL_checknumber(L, 3);
	struct bhs_metric *arg3 = luaL_checkudata(L, 4, "struct bhs_metric");
	bhs_kerr_metric(arg0, arg1, arg2, arg3);
	return 0;
}

static int l_bhs_kerr_metric_inverse(lua_State *L)
{
	const struct bhs_kerr *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_kerr");
	double arg1 = (double)luaL_checknumber(L, 2);
	double arg2 = (double)luaL_checknumber(L, 3);
	struct bhs_metric *arg3 = luaL_checkudata(L, 4, "struct bhs_metric");
	bhs_kerr_metric_inverse(arg0, arg1, arg2, arg3);
	return 0;
}

static int l_bhs_kerr_redshift_zamo(lua_State *L)
{
	const struct bhs_kerr *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_kerr");
	double arg1 = (double)luaL_checknumber(L, 2);
	double arg2 = (double)luaL_checknumber(L, 3);
	double ret = bhs_kerr_redshift_zamo(arg0, arg1, arg2);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_kerr_metric_func(lua_State *L)
{
	struct bhs_vec4 arg0 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 1, "struct bhs_vec4");
	void *arg1 = luaL_checkudata(L, 2, "void");
	struct bhs_metric *arg2 = luaL_checkudata(L, 3, "struct bhs_metric");
	bhs_kerr_metric_func(arg0, arg1, arg2);
	return 0;
}

static int l_bhs_schwarzschild_metric(lua_State *L)
{
	const struct bhs_schwarzschild *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_schwarzschild");
	double arg1 = (double)luaL_checknumber(L, 2);
	double arg2 = (double)luaL_checknumber(L, 3);
	struct bhs_metric *arg3 = luaL_checkudata(L, 4, "struct bhs_metric");
	bhs_schwarzschild_metric(arg0, arg1, arg2, arg3);
	return 0;
}

static int l_bhs_schwarzschild_metric_inverse(lua_State *L)
{
	const struct bhs_schwarzschild *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_schwarzschild");
	double arg1 = (double)luaL_checknumber(L, 2);
	double arg2 = (double)luaL_checknumber(L, 3);
	struct bhs_metric *arg3 = luaL_checkudata(L, 4, "struct bhs_metric");
	bhs_schwarzschild_metric_inverse(arg0, arg1, arg2, arg3);
	return 0;
}

static int l_bhs_schwarzschild_redshift(lua_State *L)
{
	const struct bhs_schwarzschild *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_schwarzschild");
	double arg1 = (double)luaL_checknumber(L, 2);
	double ret = bhs_schwarzschild_redshift(arg0, arg1);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_schwarzschild_escape_velocity(lua_State *L)
{
	const struct bhs_schwarzschild *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_schwarzschild");
	double arg1 = (double)luaL_checknumber(L, 2);
	double ret = bhs_schwarzschild_escape_velocity(arg0, arg1);
	lua_pushnumber(L, ret);
	return 1;
}

static int l_bhs_schwarzschild_metric_func(lua_State *L)
{
	struct bhs_vec4 arg0 =
		*(struct bhs_vec4 *)luaL_checkudata(L, 1, "struct bhs_vec4");
	void *arg1 = luaL_checkudata(L, 2, "void");
	struct bhs_metric *arg2 = luaL_checkudata(L, 3, "struct bhs_metric");
	bhs_schwarzschild_metric_func(arg0, arg1, arg2);
	return 0;
}

static int l_bhs_engine_init(lua_State *L)
{
	bhs_engine_init();
	return 0;
}

static int l_bhs_engine_shutdown(lua_State *L)
{
	bhs_engine_shutdown();
	return 0;
}

static int l_bhs_engine_update(lua_State *L)
{
	double arg0 = (double)luaL_checknumber(L, 1);
	bhs_engine_update(arg0);
	return 0;
}

static int l_bhs_scene_load(lua_State *L)
{
	const char *arg0 = (const char *)luaL_checkstring(L, 1);
	bhs_scene_load(arg0);
	return 0;
}

static int l_bhs_scene_create(lua_State *L)
{
	bhs_scene_t ret = bhs_scene_create();
	bhs_scene_t *ret_obj = lua_newuserdata(L, sizeof(bhs_scene_t));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_scene_t");
	return 1;
}

static int l_bhs_scene_destroy(lua_State *L)
{
	bhs_scene_t arg0 = luaL_checkudata(L, 1, "bhs_scene_t");
	bhs_scene_destroy(arg0);
	return 0;
}

static int l_bhs_scene_init_default(lua_State *L)
{
	bhs_scene_t arg0 = luaL_checkudata(L, 1, "bhs_scene_t");
	bhs_scene_init_default(arg0);
	return 0;
}

static int l_bhs_scene_update(lua_State *L)
{
	bhs_scene_t arg0 = luaL_checkudata(L, 1, "bhs_scene_t");
	double arg1 = (double)luaL_checknumber(L, 2);
	bhs_scene_update(arg0, arg1);
	return 0;
}

static int l_bhs_scene_get_world(lua_State *L)
{
	bhs_scene_t arg0 = luaL_checkudata(L, 1, "bhs_scene_t");
	bhs_world_handle ret = bhs_scene_get_world(arg0);
	bhs_world_handle *ret_obj =
		lua_newuserdata(L, sizeof(bhs_world_handle));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_world_handle");
	return 1;
}

static int l_bhs_scene_get_bodies(lua_State *L)
{
	bhs_scene_t arg0 = luaL_checkudata(L, 1, "bhs_scene_t");
	int *arg1 = luaL_checkudata(L, 2, "int");
	struct bhs_body *ret = bhs_scene_get_bodies(arg0, arg1);
	struct bhs_body *ret_obj = lua_newuserdata(L, sizeof(struct bhs_body));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_body");
	return 1;
}

static int l_bhs_scene_add_body_struct(lua_State *L)
{
	bhs_scene_t arg0 = luaL_checkudata(L, 1, "bhs_scene_t");
	struct bhs_body arg1 =
		*(struct bhs_body *)luaL_checkudata(L, 2, "struct bhs_body");
	bhs_entity_id ret = bhs_scene_add_body_struct(arg0, arg1);
	bhs_entity_id *ret_obj = lua_newuserdata(L, sizeof(bhs_entity_id));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_entity_id");
	return 1;
}

static int l_bhs_scene_add_body(lua_State *L)
{
	bhs_scene_t arg0 = luaL_checkudata(L, 1, "bhs_scene_t");
	enum bhs_body_type arg1 = luaL_checkudata(L, 2, "enum bhs_body_type");
	struct bhs_vec3 arg2 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 3, "struct bhs_vec3");
	struct bhs_vec3 arg3 = luaL_checkudata(L, 4, "struct bhs_vec3");
	double arg4 = (double)luaL_checknumber(L, 5);
	double arg5 = (double)luaL_checknumber(L, 6);
	struct bhs_vec3 arg6 = luaL_checkudata(L, 7, "struct bhs_vec3");
	bhs_entity_id ret =
		bhs_scene_add_body(arg0, arg1, arg2, arg3, arg4, arg5, arg6);
	bhs_entity_id *ret_obj = lua_newuserdata(L, sizeof(bhs_entity_id));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_entity_id");
	return 1;
}

static int l_bhs_scene_add_body_named(lua_State *L)
{
	bhs_scene_t arg0 = luaL_checkudata(L, 1, "bhs_scene_t");
	enum bhs_body_type arg1 = luaL_checkudata(L, 2, "enum bhs_body_type");
	struct bhs_vec3 arg2 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 3, "struct bhs_vec3");
	struct bhs_vec3 arg3 = luaL_checkudata(L, 4, "struct bhs_vec3");
	double arg4 = (double)luaL_checknumber(L, 5);
	double arg5 = (double)luaL_checknumber(L, 6);
	struct bhs_vec3 arg6 = luaL_checkudata(L, 7, "struct bhs_vec3");
	const char *arg7 = (const char *)luaL_checkstring(L, 8);
	bhs_entity_id ret = bhs_scene_add_body_named(arg0, arg1, arg2, arg3,
						     arg4, arg5, arg6, arg7);
	bhs_entity_id *ret_obj = lua_newuserdata(L, sizeof(bhs_entity_id));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_entity_id");
	return 1;
}

static int l_bhs_scene_remove_body(lua_State *L)
{
	bhs_scene_t arg0 = luaL_checkudata(L, 1, "bhs_scene_t");
	int arg1 = (int)luaL_checkinteger(L, 2);
	bhs_scene_remove_body(arg0, arg1);
	return 0;
}

static int l_bhs_scene_reset_counters(lua_State *L)
{
	bhs_scene_reset_counters();
	return 0;
}

static int l_bhs_body_create_planet_simple(lua_State *L)
{
	struct bhs_vec3 arg0 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 1, "struct bhs_vec3");
	double arg1 = (double)luaL_checknumber(L, 2);
	double arg2 = (double)luaL_checknumber(L, 3);
	struct bhs_vec3 arg3 = luaL_checkudata(L, 4, "struct bhs_vec3");
	struct bhs_body ret =
		bhs_body_create_planet_simple(arg0, arg1, arg2, arg3);
	struct bhs_body *ret_obj = lua_newuserdata(L, sizeof(struct bhs_body));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_body");
	return 1;
}

static int l_bhs_body_create_star_simple(lua_State *L)
{
	struct bhs_vec3 arg0 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 1, "struct bhs_vec3");
	double arg1 = (double)luaL_checknumber(L, 2);
	double arg2 = (double)luaL_checknumber(L, 3);
	struct bhs_vec3 arg3 = luaL_checkudata(L, 4, "struct bhs_vec3");
	struct bhs_body ret =
		bhs_body_create_star_simple(arg0, arg1, arg2, arg3);
	struct bhs_body *ret_obj = lua_newuserdata(L, sizeof(struct bhs_body));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_body");
	return 1;
}

static int l_bhs_body_create_blackhole_simple(lua_State *L)
{
	struct bhs_vec3 arg0 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 1, "struct bhs_vec3");
	double arg1 = (double)luaL_checknumber(L, 2);
	double arg2 = (double)luaL_checknumber(L, 3);
	struct bhs_body ret =
		bhs_body_create_blackhole_simple(arg0, arg1, arg2);
	struct bhs_body *ret_obj = lua_newuserdata(L, sizeof(struct bhs_body));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_body");
	return 1;
}

static int l_bhs_body_create_from_desc(lua_State *L)
{
	const struct bhs_planet_desc *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_planet_desc");
	struct bhs_vec3 arg1 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 2, "struct bhs_vec3");
	struct bhs_body ret = bhs_body_create_from_desc(arg0, arg1);
	struct bhs_body *ret_obj = lua_newuserdata(L, sizeof(struct bhs_body));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_body");
	return 1;
}

static int l_bhs_body_create_from_sun_desc(lua_State *L)
{
	const struct bhs_sun_desc *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_sun_desc");
	struct bhs_vec3 arg1 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 2, "struct bhs_vec3");
	struct bhs_body ret = bhs_body_create_from_sun_desc(arg0, arg1);
	struct bhs_body *ret_obj = lua_newuserdata(L, sizeof(struct bhs_body));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_body");
	return 1;
}

static int l_bhs_body_create_from_bh_desc(lua_State *L)
{
	const struct bhs_blackhole_desc *arg0 =
		luaL_checkudata(L, 1, "const struct bhs_blackhole_desc");
	struct bhs_vec3 arg1 =
		*(struct bhs_vec3 *)luaL_checkudata(L, 2, "struct bhs_vec3");
	struct bhs_body ret = bhs_body_create_from_bh_desc(arg0, arg1);
	struct bhs_body *ret_obj = lua_newuserdata(L, sizeof(struct bhs_body));
	*ret_obj = ret;
	luaL_setmetatable(L, "bhs_body");
	return 1;
}

static const struct luaL_Reg riengine_funcs[] = {
	{ "bhs_vec4_add", l_bhs_vec4_add },
	{ "bhs_vec4_sub", l_bhs_vec4_sub },
	{ "bhs_vec4_scale", l_bhs_vec4_scale },
	{ "bhs_vec4_neg", l_bhs_vec4_neg },
	{ "bhs_vec4_dot_minkowski", l_bhs_vec4_dot_minkowski },
	{ "bhs_vec4_norm2_minkowski", l_bhs_vec4_norm2_minkowski },
	{ "bhs_vec4_is_null", l_bhs_vec4_is_null },
	{ "bhs_vec4_is_timelike", l_bhs_vec4_is_timelike },
	{ "bhs_vec4_is_spacelike", l_bhs_vec4_is_spacelike },
	{ "bhs_vec3_add", l_bhs_vec3_add },
	{ "bhs_vec3_sub", l_bhs_vec3_sub },
	{ "bhs_vec3_scale", l_bhs_vec3_scale },
	{ "bhs_vec3_dot", l_bhs_vec3_dot },
	{ "bhs_vec3_cross", l_bhs_vec3_cross },
	{ "bhs_vec3_norm", l_bhs_vec3_norm },
	{ "bhs_vec3_norm2", l_bhs_vec3_norm2 },
	{ "bhs_vec3_normalize", l_bhs_vec3_normalize },
	{ "bhs_vec3_to_spherical", l_bhs_vec3_to_spherical },
	{ "bhs_vec3_from_spherical", l_bhs_vec3_from_spherical },
	{ "bhs_ecs_create_world", l_bhs_ecs_create_world },
	{ "bhs_ecs_destroy_world", l_bhs_ecs_destroy_world },
	{ "bhs_ecs_create_entity", l_bhs_ecs_create_entity },
	{ "bhs_ecs_destroy_entity", l_bhs_ecs_destroy_entity },
	{ "bhs_ecs_add_component", l_bhs_ecs_add_component },
	{ "bhs_ecs_remove_component", l_bhs_ecs_remove_component },
	{ "bhs_ecs_get_component", l_bhs_ecs_get_component },
	{ "bhs_ecs_query_init", l_bhs_ecs_query_init },
	{ "bhs_ecs_query_init_cached", l_bhs_ecs_query_init_cached },
	{ "bhs_ecs_query_next", l_bhs_ecs_query_next },
	{ "bhs_ecs_query_reset", l_bhs_ecs_query_reset },
	{ "bhs_ecs_query_destroy", l_bhs_ecs_query_destroy },
	{ "bhs_ecs_entity_has_components", l_bhs_ecs_entity_has_components },
	{ "bhs_ecs_save_world", l_bhs_ecs_save_world },
	{ "bhs_ecs_load_world", l_bhs_ecs_load_world },
	{ "bhs_ecs_peek_metadata", l_bhs_ecs_peek_metadata },
	{ "bhs_ecs_update_metadata", l_bhs_ecs_update_metadata },
	{ "f", l_f },
	{ "t", l_t },
	{ "f", l_f },
	{ "t", l_t },
	{ "bhs_metric_zero", l_bhs_metric_zero },
	{ "bhs_metric_minkowski", l_bhs_metric_minkowski },
	{ "bhs_metric_diag", l_bhs_metric_diag },
	{ "bhs_metric_is_symmetric", l_bhs_metric_is_symmetric },
	{ "bhs_metric_det", l_bhs_metric_det },
	{ "bhs_metric_invert", l_bhs_metric_invert },
	{ "bhs_metric_lower", l_bhs_metric_lower },
	{ "bhs_metric_raise", l_bhs_metric_raise },
	{ "bhs_metric_dot", l_bhs_metric_dot },
	{ "bhs_christoffel_compute", l_bhs_christoffel_compute },
	{ "bhs_christoffel_zero", l_bhs_christoffel_zero },
	{ "bhs_geodesic_accel", l_bhs_geodesic_accel },
	{ "s", l_s },
	{ "bhs_kerr_horizon_outer", l_bhs_kerr_horizon_outer },
	{ "bhs_kerr_horizon_inner", l_bhs_kerr_horizon_inner },
	{ "bhs_kerr_ergosphere", l_bhs_kerr_ergosphere },
	{ "bhs_kerr_isco", l_bhs_kerr_isco },
	{ "bhs_kerr_omega_frame", l_bhs_kerr_omega_frame },
	{ "bhs_kerr_metric", l_bhs_kerr_metric },
	{ "bhs_kerr_metric_inverse", l_bhs_kerr_metric_inverse },
	{ "bhs_kerr_redshift_zamo", l_bhs_kerr_redshift_zamo },
	{ "bhs_kerr_metric_func", l_bhs_kerr_metric_func },
	{ "bhs_schwarzschild_metric", l_bhs_schwarzschild_metric },
	{ "bhs_schwarzschild_metric_inverse",
	  l_bhs_schwarzschild_metric_inverse },
	{ "bhs_schwarzschild_redshift", l_bhs_schwarzschild_redshift },
	{ "bhs_schwarzschild_escape_velocity",
	  l_bhs_schwarzschild_escape_velocity },
	{ "bhs_schwarzschild_metric_func", l_bhs_schwarzschild_metric_func },
	{ "bhs_engine_init", l_bhs_engine_init },
	{ "bhs_engine_shutdown", l_bhs_engine_shutdown },
	{ "bhs_engine_update", l_bhs_engine_update },
	{ "bhs_scene_load", l_bhs_scene_load },
	{ "bhs_scene_create", l_bhs_scene_create },
	{ "bhs_scene_destroy", l_bhs_scene_destroy },
	{ "bhs_scene_init_default", l_bhs_scene_init_default },
	{ "bhs_scene_update", l_bhs_scene_update },
	{ "bhs_scene_get_world", l_bhs_scene_get_world },
	{ "bhs_scene_get_bodies", l_bhs_scene_get_bodies },
	{ "bhs_scene_add_body_struct", l_bhs_scene_add_body_struct },
	{ "bhs_scene_add_body", l_bhs_scene_add_body },
	{ "bhs_scene_add_body_named", l_bhs_scene_add_body_named },
	{ "bhs_scene_remove_body", l_bhs_scene_remove_body },
	{ "bhs_scene_reset_counters", l_bhs_scene_reset_counters },
	{ "bhs_body_create_planet_simple", l_bhs_body_create_planet_simple },
	{ "bhs_body_create_star_simple", l_bhs_body_create_star_simple },
	{ "bhs_body_create_blackhole_simple",
	  l_bhs_body_create_blackhole_simple },
	{ "bhs_body_create_from_desc", l_bhs_body_create_from_desc },
	{ "bhs_body_create_from_sun_desc", l_bhs_body_create_from_sun_desc },
	{ "bhs_body_create_from_bh_desc", l_bhs_body_create_from_bh_desc },
	{ NULL, NULL }
};

int luaopen_riengine(lua_State *L)
{
	luaL_newlib(L, riengine_funcs);

	// Register Metatables
	luaL_newmetatable(L, "bhs_vec4");
	luaL_setfuncs(L, bhs_vec4_methods, 0);
	lua_pop(L, 1);
	luaL_newmetatable(L, "bhs_vec3");
	luaL_setfuncs(L, bhs_vec3_methods, 0);
	lua_pop(L, 1);
	luaL_newmetatable(L, "bhs_metric");
	luaL_setfuncs(L, bhs_metric_methods, 0);
	lua_pop(L, 1);
	luaL_newmetatable(L, "bhs_christoffel");
	luaL_setfuncs(L, bhs_christoffel_methods, 0);
	lua_pop(L, 1);
	luaL_newmetatable(L, "bhs_kerr");
	luaL_setfuncs(L, bhs_kerr_methods, 0);
	lua_pop(L, 1);
	luaL_newmetatable(L, "bhs_schwarzschild");
	luaL_setfuncs(L, bhs_schwarzschild_methods, 0);
	lua_pop(L, 1);
	luaL_newmetatable(L, "bhs_sun_desc");
	luaL_setfuncs(L, bhs_sun_desc_methods, 0);
	lua_pop(L, 1);
	luaL_newmetatable(L, "bhs_blackhole_desc");
	luaL_setfuncs(L, bhs_blackhole_desc_methods, 0);
	lua_pop(L, 1);
	luaL_newmetatable(L, "bhs_planet_data");
	luaL_setfuncs(L, bhs_planet_data_methods, 0);
	lua_pop(L, 1);
	luaL_newmetatable(L, "bhs_star_data");
	luaL_setfuncs(L, bhs_star_data_methods, 0);
	lua_pop(L, 1);
	luaL_newmetatable(L, "bhs_blackhole_data");
	luaL_setfuncs(L, bhs_blackhole_data_methods, 0);
	lua_pop(L, 1);
	luaL_newmetatable(L, "bhs_body_state");
	luaL_setfuncs(L, bhs_body_state_methods, 0);
	lua_pop(L, 1);
	luaL_newmetatable(L, "bhs_body");
	luaL_setfuncs(L, bhs_body_methods, 0);
	lua_pop(L, 1);

	// Constants
	lua_pushnumber(L, 3.14159265358979323846);
	lua_setfield(L, -2, "M_PI");
	lua_pushnumber(L, 0);
	lua_setfield(L, -2, "BHS_ENTITY_INVALID");
	lua_pushnumber(L, 10000);
	lua_setfield(L, -2, "BHS_MAX_ENTITIES");
	lua_pushnumber(L, 65536);
	lua_setfield(L, -2, "BHS_MAX_TRAIL_POINTS");
	return 1;
}