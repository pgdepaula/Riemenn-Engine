-- Auto-generated LuaJIT bindings for Black Hole Simulator
local ffi = require('ffi')
local libs = { 'libriengine.so', './build/bin/libriengine.so', '../build/bin/libriengine.so' }
local lib = nil
for _, p in ipairs(libs) do
    local status, result = pcall(ffi.load, p)
    if status then lib = result; break end
end
if not lib then error('Could not load libriengine.so') end

ffi.cdef[[
    static const int M_PI = 3.14159265358979323846;
    static const int BHS_ENTITY_INVALID = 0;
    static const int BHS_MAX_ENTITIES = 10000;
    static const int BHS_MAX_TRAIL_POINTS = 65536;
    typedef struct bhs_world_handle bhs_world_handle;
    typedef struct bhs_scene_t bhs_scene_t;
    enum bhs_sun_stage {
        BHS_SUN_MAIN_SEQUENCE = 0,
        BHS_SUN_RED_GIANT = 1,
        BHS_SUN_WHITE_DWARF = 2,
        BHS_SUN_NEUTRON_STAR = 3,
    };
    enum bhs_body_type {
        BHS_BODY_PLANET = 0,
        BHS_BODY_MOON = 1,
        BHS_BODY_STAR = 2,
        BHS_BODY_BLACKHOLE = 3,
        BHS_BODY_ASTEROID = 4,
    };
    enum bhs_matter_state {
        BHS_STATE_SOLID = 0,
        BHS_STATE_LIQUID = 1,
        BHS_STATE_GAS = 2,
        BHS_STATE_PLASMA = 3,
    };
    enum bhs_shape_type {
        BHS_SHAPE_SPHERE = 0,
        BHS_SHAPE_ELLIPSOID = 1,
        BHS_SHAPE_IRREGULAR = 2,
    };
    enum bhs_star_stage {
        BHS_STAR_MAIN_SEQUENCE = 0,
        BHS_STAR_GIANT = 1,
        BHS_STAR_WHITE_DWARF = 2,
        BHS_STAR_NEUTRON = 3,
    };
    typedef struct bhs_vec4 {
        real_t t;
        real_t x;
        real_t y;
        real_t z;
    } bhs_vec4;
    typedef struct bhs_vec3 {
        real_t x;
        real_t y;
        real_t z;
    } bhs_vec3;
    typedef struct bhs_metric {
        BHS_ALIGN(16) real_t[4][4] g;
    } bhs_metric;
    typedef struct bhs_christoffel {
        BHS_ALIGN(16) real_t[4][4][4] gamma;
    } bhs_christoffel;
    typedef struct bhs_kerr {
        double M;
        double a;
    } bhs_kerr;
    typedef struct bhs_schwarzschild {
        double M;
    } bhs_schwarzschild;
    typedef struct bhs_sun_desc {
        const char* name;
        double mass;
        double radius;
        double temperature;
        double luminosity;
        double age;
        double metallicity;
        enum bhs_sun_stage stage;
        char[8] spectral_type;
        double rotation_period;
        double axis_tilt;
        struct bhs_vec3 base_color;
        struct bhs_vec3 (*get_surface_color)(struct bhs_vec3 p_local);
    } bhs_sun_desc;
    typedef struct bhs_blackhole_desc {
        const char* name;
        double mass;
        double spin;
        double charge;
        double event_horizon_r;
        double accretion_disk_mass;
        struct bhs_vec3 base_color;
        struct bhs_vec3 (*get_surface_color)(struct bhs_vec3 p_local);
    } bhs_blackhole_desc;
    typedef struct bhs_planet_data {
        double density;
        double axis_tilt;
        double rotation_period;
        double j2;
        double albedo;
        bool has_atmosphere;
        double surface_pressure;
        double atmosphere_mass;
        char[64] composition;
        double temperature;
        double heat_capacity;
        double energy_flux;
        int physical_state;
        bool has_magnetic_field;
    } bhs_planet_data;
    typedef struct bhs_star_data {
        double luminosity;
        double temp_effective;
        double age;
        double density;
        double hydrogen_frac;
        double helium_frac;
        double metals_frac;
        int stage;
        double metallicity;
        char[8] spectral_type;
    } bhs_star_data;
    typedef struct bhs_blackhole_data {
        double spin_factor;
        double event_horizon_r;
        double ergososphere_r;
        double accretion_disk_mass;
        double accretion_rate;
    } bhs_blackhole_data;
    typedef struct bhs_body_state {
        struct bhs_vec3 pos;
        struct bhs_vec3 vel;
        struct bhs_vec3 acc;
        struct bhs_vec3 rot_axis;
        double rot_speed;
        double moment_inertia;
        double mass;
        double radius;
        double current_rotation_angle;
        int shape;
    } bhs_body_state;
    typedef struct bhs_body {
        struct bhs_body_state state;
        enum bhs_body_type type;
        void* prop;
        struct bhs_vec3 color;
        bool is_fixed;
        bool is_alive;
        char[32] name;
        float[BHS_MAX_TRAIL_POINTS][3] trail_positions;
        int trail_head;
        int trail_count;
    } bhs_body;
    bhs_vec4 bhs_vec4_add(struct bhs_vec4 a, struct bhs_vec4 b);
    bhs_vec4 bhs_vec4_sub(struct bhs_vec4 a, struct bhs_vec4 b);
    bhs_vec4 bhs_vec4_scale(struct bhs_vec4 v, real_t s);
    bhs_vec4 bhs_vec4_neg(struct bhs_vec4 v);
    real_t bhs_vec4_dot_minkowski(struct bhs_vec4 a, struct bhs_vec4 b);
    real_t bhs_vec4_norm2_minkowski(struct bhs_vec4 v);
    bool bhs_vec4_is_null(struct bhs_vec4 v, real_t epsilon);
    bool bhs_vec4_is_timelike(struct bhs_vec4 v);
    bool bhs_vec4_is_spacelike(struct bhs_vec4 v);
    bhs_vec3 bhs_vec3_add(struct bhs_vec3 a, struct bhs_vec3 b);
    bhs_vec3 bhs_vec3_sub(struct bhs_vec3 a, struct bhs_vec3 b);
    bhs_vec3 bhs_vec3_scale(struct bhs_vec3 v, real_t s);
    real_t bhs_vec3_dot(struct bhs_vec3 a, struct bhs_vec3 b);
    bhs_vec3 bhs_vec3_cross(struct bhs_vec3 a, struct bhs_vec3 b);
    real_t bhs_vec3_norm(struct bhs_vec3 v);
    real_t bhs_vec3_norm2(struct bhs_vec3 v);
    bhs_vec3 bhs_vec3_normalize(struct bhs_vec3 v);
    void bhs_vec3_to_spherical(struct bhs_vec3 v, real_t* r, real_t* theta, real_t* phi);
    bhs_vec3 bhs_vec3_from_spherical(real_t r, real_t theta, real_t phi);
    bhs_world_handle bhs_ecs_create_world();
    void bhs_ecs_destroy_world(bhs_world_handle world);
    bhs_entity_id bhs_ecs_create_entity(bhs_world_handle world);
    void bhs_ecs_destroy_entity(bhs_world_handle world, bhs_entity_id entity);
    void * bhs_ecs_add_component(bhs_world_handle world, bhs_entity_id entity, bhs_component_type type, size_t size, const void* data);
    void bhs_ecs_remove_component(bhs_world_handle world, bhs_entity_id entity, bhs_component_type type);
    void * bhs_ecs_get_component(bhs_world_handle world, bhs_entity_id entity, bhs_component_type type);
    void bhs_ecs_query_init(bhs_ecs_query* q, bhs_world_handle world, bhs_component_mask required);
    void bhs_ecs_query_init_cached(bhs_ecs_query* q, bhs_world_handle world, bhs_component_mask required);
    bool bhs_ecs_query_next(bhs_ecs_query* q, bhs_entity_id* out_entity);
    void bhs_ecs_query_reset(bhs_ecs_query* q);
    void bhs_ecs_query_destroy(bhs_ecs_query* q);
    bool bhs_ecs_entity_has_components(bhs_world_handle world, bhs_entity_id entity, bhs_component_mask mask);
    bool bhs_ecs_save_world(bhs_world_handle world, const char* filename);
    bool bhs_ecs_load_world(bhs_world_handle world, const char* filename);
    bool bhs_ecs_peek_metadata(const char* filename, void* out_metadata, size_t metadata_size, uint32_t metadata_type_id);
    bool bhs_ecs_update_metadata(const char* filename, const void* new_metadata, size_t metadata_size, uint32_t metadata_type_id);
    fprint f(stderr , "[ASSERT FALHOU] %s:%d: %s\n", __FILE__ , __LINE__ , \ #cond);
    abor t();
    fprint f(stderr , "[FATAL] Código inalcançável atingido em %s:%d\n", \ __FILE__, __LINE__ );
    abor t();
    bhs_metric bhs_metric_zero();
    bhs_metric bhs_metric_minkowski();
    bhs_metric bhs_metric_diag(real_t g00, real_t g11, real_t g22, real_t g33);
    bool bhs_metric_is_symmetric(const struct bhs_metric* m, real_t tol);
    real_t bhs_metric_det(const struct bhs_metric* m);
    int bhs_metric_invert(const struct bhs_metric* m, struct bhs_metric* inv);
    bhs_vec4 bhs_metric_lower(const struct bhs_metric* m, struct bhs_vec4 v);
    bhs_vec4 bhs_metric_raise(const struct bhs_metric* m_inv, struct bhs_vec4 v);
    real_t bhs_metric_dot(const struct bhs_metric* m, struct bhs_vec4 a, struct bhs_vec4 b);
    int bhs_christoffel_compute(bhs_metric_func metric_fn, struct bhs_vec4 coords, void* userdata, real_t h, struct bhs_christoffel* out);
    bhs_christoffel bhs_christoffel_zero();
    bhs_vec4 bhs_geodesic_accel(const struct bhs_christoffel* chris, struct bhs_vec4 vel);
    co s(theta );
    double bhs_kerr_horizon_outer(const struct bhs_kerr* bh);
    double bhs_kerr_horizon_inner(const struct bhs_kerr* bh);
    double bhs_kerr_ergosphere(const struct bhs_kerr* bh, double theta);
    double bhs_kerr_isco(const struct bhs_kerr* bh, bool prograde);
    double bhs_kerr_omega_frame(const struct bhs_kerr* bh, double r, double theta);
    void bhs_kerr_metric(const struct bhs_kerr* bh, double r, double theta, struct bhs_metric* out);
    void bhs_kerr_metric_inverse(const struct bhs_kerr* bh, double r, double theta, struct bhs_metric* out);
    double bhs_kerr_redshift_zamo(const struct bhs_kerr* bh, double r, double theta);
    void bhs_kerr_metric_func(struct bhs_vec4 coords, void* userdata, struct bhs_metric* out);
    void bhs_schwarzschild_metric(const struct bhs_schwarzschild* bh, double r, double theta, struct bhs_metric* out);
    void bhs_schwarzschild_metric_inverse(const struct bhs_schwarzschild* bh, double r, double theta, struct bhs_metric* out);
    double bhs_schwarzschild_redshift(const struct bhs_schwarzschild* bh, double r);
    double bhs_schwarzschild_escape_velocity(const struct bhs_schwarzschild* bh, double r);
    void bhs_schwarzschild_metric_func(struct bhs_vec4 coords, void* userdata, struct bhs_metric* out);
    void bhs_engine_init();
    void bhs_engine_shutdown();
    void bhs_engine_update(double dt);
    void bhs_scene_load(const char* path);
    bhs_scene_t bhs_scene_create();
    void bhs_scene_destroy(bhs_scene_t scene);
    void bhs_scene_init_default(bhs_scene_t scene);
    void bhs_scene_update(bhs_scene_t scene, double dt);
    bhs_world_handle bhs_scene_get_world(bhs_scene_t scene);
    bhs_body * bhs_scene_get_bodies(bhs_scene_t scene, int* count);
    bhs_entity_id bhs_scene_add_body_struct(bhs_scene_t scene, struct bhs_body b);
    bhs_entity_id bhs_scene_add_body(bhs_scene_t scene, enum bhs_body_type type, struct bhs_vec3 pos, struct bhs_vec3 vel, double mass, double radius, struct bhs_vec3 color);
    bhs_entity_id bhs_scene_add_body_named(bhs_scene_t scene, enum bhs_body_type type, struct bhs_vec3 pos, struct bhs_vec3 vel, double mass, double radius, struct bhs_vec3 color, const char* name);
    void bhs_scene_remove_body(bhs_scene_t scene, int index);
    void bhs_scene_reset_counters();
    bhs_body bhs_body_create_planet_simple(struct bhs_vec3 pos, double mass, double radius, struct bhs_vec3 color);
    bhs_body bhs_body_create_star_simple(struct bhs_vec3 pos, double mass, double radius, struct bhs_vec3 color);
    bhs_body bhs_body_create_blackhole_simple(struct bhs_vec3 pos, double mass, double radius);
    bhs_body bhs_body_create_from_desc(const struct bhs_planet_desc* desc, struct bhs_vec3 pos);
    bhs_body bhs_body_create_from_sun_desc(const struct bhs_sun_desc* desc, struct bhs_vec3 pos);
    bhs_body bhs_body_create_from_bh_desc(const struct bhs_blackhole_desc* desc, struct bhs_vec3 pos);
]]

return lib