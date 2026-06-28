#include "simulation.h"
#include "global.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <random>
#include <time.h>

static bool is_solid_cell(int i, int j)
{
    int N = SCR_SIZE;
    if (!obstacles_enabled)
        return false;
    if (i < 1 || i > N || j < 1 || j > N)
        return false;
    return solid[IX(i, j)];
}

static float sample_cell(const float *x, int i, int j)
{
    return x[IX(i, j)];
}

static bool is_mouse_obstacle_cell(int i, int j)
{
    if (!obstacles_enabled || !mouse_obstacle_enabled || !obstacle_mouse_down)
        return false;
    if (i < 1 || i > SCR_SIZE || j < 1 || j > SCR_SIZE)
        return false;

    float dx = (float)i - mouse_obstacle_x;
    float dy = (float)j - mouse_obstacle_y;
    return dx * dx + dy * dy <= mouse_obstacle_radius * mouse_obstacle_radius;
}

static int wrap_index(int i, int N)
{
    if (i < 1)
        return N;
    if (i > N)
        return 1;
    return i;
}

static float wrap_position(float x, int N)
{
    while (x < 0.5f)
        x += N;
    while (x > N + 0.5f)
        x -= N;
    return x;
}

static float screen_to_sim_x(double x, int N)
{
    return std::clamp((float)((x / SCR_WIDTH) * N + 1.0), 1.0f, (float)N);
}

static float screen_to_sim_y(double y, int N)
{
    return std::clamp((float)(((SCR_HEIGHT - y) / SCR_HEIGHT) * N + 1.0), 1.0f, (float)N);
}

static void house_image_point_to_sim(int N, float image_u, float image_v, float &sim_x, float &sim_y)
{
    float ndc_height = 2.0f * house_scene_height;
    float image_aspect = 1.0f;
    float ndc_width = ndc_height * image_aspect * ((float)SCR_HEIGHT / (float)SCR_WIDTH);
    float left = -0.5f * ndc_width;
    float bottom = -1.0f - ndc_height * house_bottom_padding_ratio;
    float top = bottom + ndc_height;

    float ndc_x = left + image_u * ndc_width;
    float ndc_y = top - image_v * ndc_height;

    sim_x = std::clamp((ndc_x + 1.0f) * 0.5f * (float)N + 1.0f, 1.0f, (float)N);
    sim_y = std::clamp((ndc_y + 1.0f) * 0.5f * (float)N + 1.0f, 1.0f, (float)N);
}

void update_obstacles(int N)
{
    N = SCR_SIZE;

    static bool had_mouse_obstacle = false;
    static float prev_mouse_obstacle_x = 0.0f;
    static float prev_mouse_obstacle_y = 0.0f;

    int size = (N + 2) * (N + 2);
    for (int i = 0; i < size; i++)
        solid[i] = false;

    mouse_obstacle_u = 0.0f;
    mouse_obstacle_v = 0.0f;

    if (!obstacles_enabled)
    {
        had_mouse_obstacle = false;
        return;
    }

    if (static_obstacle_enabled)
    {
        int cx = N / 2;
        int cy = N / 2;
        float r2 = obstacle_radius * obstacle_radius;

        for (int j = 1; j <= N; j++)
        {
            for (int i = 1; i <= N; i++)
            {
                float dx = (float)(i - cx);
                float dy = (float)(j - cy);
                if (dx * dx + dy * dy <= r2)
                    solid[IX(i, j)] = true;
            }
        }
    }

    if (!mouse_obstacle_enabled || !obstacle_mouse_down)
    {
        had_mouse_obstacle = false;
        return;
    }

    float next_x = screen_to_sim_x(mouse_x, N);
    float next_y = screen_to_sim_y(mouse_y, N);

    if (had_mouse_obstacle)
    {
        float denom = std::max(0.0001f, dt * (float)N);
        mouse_obstacle_u = std::clamp((next_x - prev_mouse_obstacle_x) / denom, -2.0f, 2.0f);
        mouse_obstacle_v = std::clamp((next_y - prev_mouse_obstacle_y) / denom, -2.0f, 2.0f);
    }

    mouse_obstacle_x = next_x;
    mouse_obstacle_y = next_y;
    prev_mouse_obstacle_x = next_x;
    prev_mouse_obstacle_y = next_y;
    had_mouse_obstacle = true;

    int r = std::max(1, (int)std::ceil(mouse_obstacle_radius));
    int mx = (int)std::round(mouse_obstacle_x);
    int my = (int)std::round(mouse_obstacle_y);

    for (int j = std::max(1, my - r); j <= std::min(N, my + r); j++)
    {
        for (int i = std::max(1, mx - r); i <= std::min(N, mx + r); i++)
        {
            float dx = (float)i - mouse_obstacle_x;
            float dy = (float)j - mouse_obstacle_y;
            if (dx * dx + dy * dy <= mouse_obstacle_radius * mouse_obstacle_radius)
                solid[IX(i, j)] = true;
        }
    }
}

static void apply_obstacle_bnd(int N, int b, float *x)
{
    N = SCR_SIZE;

    if (!obstacles_enabled)
        return;

    for (int j = 1; j <= N; j++)
    {
        for (int i = 1; i <= N; i++)
        {
            if (!solid[IX(i, j)])
                continue;

            bool moving_obstacle = is_mouse_obstacle_cell(i, j);
            float body_velocity = 0.0f;
            auto is_fluid_neighbor = [&](int ni, int nj)
            {
                return ni >= 1 && ni <= N && nj >= 1 && nj <= N && !solid[IX(ni, nj)];
            };
            if (moving_obstacle && b == 1)
                body_velocity = mouse_obstacle_u;
            if (moving_obstacle && b == 2)
                body_velocity = mouse_obstacle_v;

            auto boundary_value = [&](int ni, int nj)
            {
                float neighbor = x[IX(ni, nj)];
                if (b == 0)
                    return neighbor;
                if (moving_obstacle)
                    return 2.0f * body_velocity - neighbor;
                return -neighbor;
            };

            float sum = 0.0f;
            int count = 0;

            if (is_fluid_neighbor(i - 1, j))
            {
                sum += boundary_value(i - 1, j);
                count++;
            }
            if (is_fluid_neighbor(i + 1, j))
            {
                sum += boundary_value(i + 1, j);
                count++;
            }
            if (is_fluid_neighbor(i, j - 1))
            {
                sum += boundary_value(i, j - 1);
                count++;
            }
            if (is_fluid_neighbor(i, j + 1))
            {
                sum += boundary_value(i, j + 1);
                count++;
            }

            x[IX(i, j)] = count > 0 ? sum / count : body_velocity;        }
    }
}

void init()
{
    int N = SCR_SIZE;
    update_obstacles(N);

    int init_mode = 1;

    switch (init_mode)
    {
    case 0:
        {
            srand((unsigned int)time(NULL));
            for (int i = 0; i < SCR_SIZE + 2; i++)
            {
                for (int j = 0; j < SCR_SIZE + 2; j++)
                {
                    float r = (float)rand() / (float)RAND_MAX;
                    dens_prev[IX(i, j)] = r;
                    dens[IX(i, j)] = r;
                }
            }
            break;
        }
    case 1:
        {
            break;
        }
    default:
        break;
    }
}

void update_velocity_field()
{
    int N = SCR_SIZE;
    for (int i = 0; i <= N + 1; i++)
    {
        for (int j = 0; j <= N + 1; j++)
        {
            u[IX(i, j)] = 0.3f * (sinf(1.0f * t));
            v[IX(i, j)] = 0.0f;
        }
    }
}

void set_bnd(int N, int b, float *x)
{
    N = SCR_SIZE;

    if (boundary_mode == BOUNDARY_WRAP)
    {
        for (int i = 1; i <= N; i++)
        {
            x[IX(0, i)] = x[IX(N, i)];
            x[IX(N + 1, i)] = x[IX(1, i)];
            x[IX(i, 0)] = x[IX(i, N)];
            x[IX(i, N + 1)] = x[IX(i, 1)];
        }
        x[IX(0, 0)] = x[IX(N, N)];
        x[IX(0, N + 1)] = x[IX(N, 1)];
        x[IX(N + 1, 0)] = x[IX(1, N)];
        x[IX(N + 1, N + 1)] = x[IX(1, 1)];
        apply_obstacle_bnd(N, b, x);
        return;
    }

    if (boundary_mode == BOUNDARY_INFLOW)
    {
        for (int i = 1; i <= N; i++)
        {
            if (b == 1)
            {
                x[IX(0, i)] = inflow_velocity;
                x[IX(1, i)] = inflow_velocity;
            }
            else
            {
                x[IX(0, i)] = x[IX(1, i)];
            }

            x[IX(N + 1, i)] = x[IX(N, i)];
            x[IX(i, 0)] = b == 2 ? -x[IX(i, 1)] : x[IX(i, 1)];
            x[IX(i, N + 1)] = b == 2 ? -x[IX(i, N)] : x[IX(i, N)];
        }
        x[IX(0, 0)] = 0.5f * (x[IX(1, 0)] + x[IX(0, 1)]);
        x[IX(0, N + 1)] = 0.5f * (x[IX(1, N + 1)] + x[IX(0, N)]);
        x[IX(N + 1, 0)] = 0.5f * (x[IX(N, 0)] + x[IX(N + 1, 1)]);
        x[IX(N + 1, N + 1)] = 0.5f * (x[IX(N, N + 1)] + x[IX(N + 1, N)]);
        apply_obstacle_bnd(N, b, x);
        return;
    }

    for (int i = 1; i <= N; i++)
    {
        x[IX(0, i)] = b == 1 ? -x[IX(1, i)] : x[IX(1, i)];
        x[IX(N + 1, i)] = b == 1 ? -x[IX(N, i)] : x[IX(N, i)];
        x[IX(i, 0)] = b == 2 ? -x[IX(i, 1)] : x[IX(i, 1)];
        x[IX(i, N + 1)] = b == 2 ? -x[IX(i, N)] : x[IX(i, N)];
    }
    x[IX(0, 0)] = 0.5f * (x[IX(1, 0)] + x[IX(0, 1)]);
    x[IX(0, N + 1)] = 0.5f * (x[IX(1, N + 1)] + x[IX(0, N)]);
    x[IX(N + 1, 0)] = 0.5f * (x[IX(N, 0)] + x[IX(N + 1, 1)]);
    x[IX(N + 1, N + 1)] = 0.5f * (x[IX(N, N + 1)] + x[IX(N + 1, N)]);
    apply_obstacle_bnd(N, b, x);
}

static void splat_density(int N, float *x, float scale)
{
    N = SCR_SIZE;

    int mx = (int)((mouse_x / SCR_WIDTH) * N + 1);
    if (mx < 1 || mx > N)
        return;
    int my = (int)(((SCR_HEIGHT - mouse_y) / SCR_HEIGHT) * N + 1);
    if (my < 1 || my > N)
        return;

    for (int i = 1; i <= N; i++)
    {
        for (int j = 1; j <= N; j++)
        {
            if (is_solid_cell(i, j))
                continue;

            double dist = sqrt((i - mx) * (i - mx) + (j - my) * (j - my));
            if (dist < radius)
            {
                float density = 1.0f - (float)(dist / radius);
                x[IX(i, j)] += density * scale;
            }
        }
    }
}

static void splat_force(int N, float *u_source, float *v_source, float cx, float cy, float fx, float fy, float brush_radius)
{
    N = SCR_SIZE;

    if (brush_radius <= 0.0f)
        return;

    int r = std::max(1, (int)std::ceil(brush_radius));
    int center_i = (int)std::round(cx);
    int center_j = (int)std::round(cy);

    for (int j = std::max(1, center_j - r); j <= std::min(N, center_j + r); j++)
    {
        for (int i = std::max(1, center_i - r); i <= std::min(N, center_i + r); i++)
        {
            if (is_solid_cell(i, j))
                continue;

            float dx = (float)i - cx;
            float dy = (float)j - cy;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist >= brush_radius)
                continue;

            float weight = 1.0f - dist / brush_radius;
            weight *= weight;
            u_source[IX(i, j)] += fx * weight;
            v_source[IX(i, j)] += fy * weight;
        }
    }
}

void add_source(int N, float *x, float *s, float dt)
{
    N = SCR_SIZE;

    int size = (N + 2) * (N + 2);
    for (int i = 0; i < size; i++)
    {
        x[i] += dt * s[i];
    }
}

void add_density_source(int N, float *s)
{
    N = SCR_SIZE;

    splat_density(N, s, source);
}

static void add_mouse_force_source(int N, float *u_source, float *v_source)
{
    N = SCR_SIZE;

    static bool had_mouse = false;
    static float prev_x = 0.0f;
    static float prev_y = 0.0f;

    if (!mouse_force_enabled || !mouse_down)
    {
        had_mouse = false;
        return;
    }

    float cur_x = screen_to_sim_x(mouse_x, N);
    float cur_y = screen_to_sim_y(mouse_y, N);

    if (had_mouse)
    {
        float denom = std::max(0.0001f, dt * (float)N);
        float vx = std::clamp((cur_x - prev_x) / denom, -4.0f, 4.0f);
        float vy = std::clamp((cur_y - prev_y) / denom, -4.0f, 4.0f);
        splat_force(N, u_source, v_source, cur_x, cur_y, vx * mouse_force, vy * mouse_force, mouse_force_radius);
    }

    prev_x = cur_x;
    prev_y = cur_y;
    had_mouse = true;
}

struct ChimneyPuff
{
    bool active;
    float x;
    float y;
    float age;
    float lifetime;
    float radius;
    float density;
    float heat;
    float lift_speed;
    float drift_speed;
    float side_force;
    float lift_force;
    float swirl_force;
    float phase;
};

static ChimneyPuff chimney_puffs[256];

static ChimneyPuff *reserve_chimney_puff()
{
    int oldest = 0;
    float oldest_age = -1.0f;

    for (int i = 0; i < 256; i++)
    {
        if (!chimney_puffs[i].active)
            return &chimney_puffs[i];
        if (chimney_puffs[i].age > oldest_age)
        {
            oldest_age = chimney_puffs[i].age;
            oldest = i;
        }
    }

    return &chimney_puffs[oldest];
}

static void emit_chimney_puff(int N, float center_x, float base_y, std::mt19937 &rng)
{
    N = SCR_SIZE;

    std::uniform_real_distribution<float> offset_dist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> radius_dist(0.16f, 0.30f);
    std::uniform_real_distribution<float> density_dist(0.32f, 0.62f);
    std::uniform_real_distribution<float> heat_dist(0.28f, 0.52f);
    std::uniform_real_distribution<float> life_dist(0.18f, 0.34f);
    std::uniform_real_distribution<float> lift_dist(0.08f, 0.18f);
    std::uniform_real_distribution<float> force_dist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> phase_dist(0.0f, 6.283185f);

    ChimneyPuff *puff = reserve_chimney_puff();
    puff->active = true;
    puff->age = 0.0f;
    puff->lifetime = life_dist(rng);
    puff->radius = chimney_radius * radius_dist(rng);
    puff->x = std::clamp(center_x + offset_dist(rng) * chimney_radius * 0.30f, 1.0f, (float)N);
    puff->y = std::clamp(base_y + 1.0f + std::abs(offset_dist(rng)) * chimney_radius * 0.08f, 1.0f, (float)N);
    puff->density = density_dist(rng);
    puff->heat = heat_dist(rng);
    puff->lift_speed = chimney_radius * lift_dist(rng);
    puff->drift_speed = chimney_radius * 0.24f * offset_dist(rng);
    puff->side_force = chimney_force * 0.11f * force_dist(rng);
    puff->lift_force = chimney_force * 0.038f * (0.70f + 0.30f * heat_dist(rng));
    puff->swirl_force = 0.22f * force_dist(rng);
    puff->phase = phase_dist(rng);
}

static void splat_chimney_puff(int N, float *d, float *temp_source_field, float *u_source, float *v_source, ChimneyPuff &puff)
{
    N = SCR_SIZE;

    float progress = std::clamp(puff.age / std::max(0.001f, puff.lifetime), 0.0f, 1.0f);
    float envelope = std::sin(pi * progress);
    if (envelope <= 0.0f)
        return;

    float radius = puff.radius * (1.0f + 0.38f * progress);
    float cx = puff.x + puff.drift_speed * puff.age + 0.10f * puff.radius * std::sin(puff.phase + t * 1.7f);
    float cy = puff.y + puff.lift_speed * puff.age;
    float cutoff = radius * 1.45f;
    int reach = std::max(2, (int)std::ceil(cutoff));
    int center_i = (int)std::round(cx);
    int center_j = (int)std::round(cy);

    for (int j = std::max(1, center_j - reach); j <= std::min(N, center_j + reach); j++)
    {
        for (int i = std::max(1, center_i - reach); i <= std::min(N, center_i + reach); i++)
        {
            if (is_solid_cell(i, j))
                continue;

            float dx = (float)i - cx;
            float dy = (float)j - cy;
            float dist2 = dx * dx + dy * dy;
            if (dist2 > cutoff * cutoff)
                continue;

            float inv_radius = 1.0f / std::max(1.0f, radius);
            float q = dist2 * inv_radius * inv_radius;
            float gaussian = std::exp(-0.5f * q);
            float dist = std::sqrt(dist2);
            float edge_fade = std::clamp(1.0f - dist / cutoff, 0.0f, 1.0f);
            float grain = 0.82f + 0.18f * std::sin(puff.phase + (float)i * 0.37f + (float)j * 0.23f + t * 1.1f);
            float source_weight = envelope * gaussian * grain;
            float force_weight = envelope * gaussian * edge_fade;

            d[IX(i, j)] += chimney_source * puff.density * source_weight;
            temp_source_field[IX(i, j)] += temperature_source * puff.heat * source_weight;

            float tangent_x = -dy * inv_radius;
            float tangent_y = dx * inv_radius;
            u_source[IX(i, j)] += (puff.side_force + tangent_x * puff.swirl_force) * force_weight;
            v_source[IX(i, j)] += (puff.lift_force + tangent_y * puff.swirl_force * 0.35f) * force_weight;
        }
    }
}

static void add_chimney_source(int N, float *d, float *temp_source_field, float *u_source, float *v_source)
{
    N = SCR_SIZE;

    static std::mt19937 rng(std::random_device{}());
    static bool initialized = false;
    static float emitter_timer[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    std::uniform_real_distribution<float> interval_dist(0.045f, 0.085f);
    std::uniform_real_distribution<float> offset_dist(-1.0f, 1.0f);
    std::uniform_int_distribution<int> burst_count_dist(3, 6);

    int emitter_count = house_scene_enabled ? house_chimney_count : 1;
    emitter_count = std::clamp(emitter_count, 1, 3);

    if (!initialized)
    {
        for (int c = 0; c < emitter_count; c++)
            emitter_timer[c] = -0.04f + 0.075f * (float)c;
        initialized = true;
    }

    for (int c = 0; c < emitter_count; c++)
    {
        float center_x = N / 2.0f;
        float base_y = 1.0f;
        if (house_scene_enabled)
            house_image_point_to_sim(N, house_chimney_u[c], house_chimney_v[c], center_x, base_y);

        emitter_timer[c] -= dt;
        if (emitter_timer[c] <= 0.0f)
        {
            int burst_count = burst_count_dist(rng);
            for (int p = 0; p < burst_count; p++)
            {
                float puff_x = center_x + offset_dist(rng) * chimney_radius * 0.42f;
                emit_chimney_puff(N, puff_x, base_y, rng);
            }
            emitter_timer[c] += interval_dist(rng);
        }
    }

    for (int i = 0; i < 256; i++)
    {
        if (!chimney_puffs[i].active)
            continue;

        chimney_puffs[i].age += dt;
        if (chimney_puffs[i].age >= chimney_puffs[i].lifetime)
        {
            chimney_puffs[i].active = false;
            continue;
        }

        splat_chimney_puff(N, d, temp_source_field, u_source, v_source, chimney_puffs[i]);
    }
}

static void add_upward_wind(int N, float *v_source)
{
    N = SCR_SIZE;

    if (upward_wind_strength <= 0.0f)
        return;

    int height = std::max(1, (int)((float)N * upward_wind_height));
    for (int j = 1; j <= height; j++)
    {
        float y = height > 1 ? (float)(j - 1) / (float)(height - 1) : 0.0f;
        float fade = 1.0f - y;
        fade = fade * fade * (3.0f - 2.0f * fade);

        for (int i = 1; i <= N; i++)
        {
            if (is_solid_cell(i, j))
                continue;

            v_source[IX(i, j)] += upward_wind_strength * fade;
        }
    }
}

static void add_buoyancy_force(int N, float *v_source)
{
    N = SCR_SIZE;

    if (!buoyancy_enabled)
        return;

    for (int j = 1; j <= N; j++)
    {
        for (int i = 1; i <= N; i++)
        {
            if (is_solid_cell(i, j))
                continue;

            int idx = IX(i, j);
            float heat = std::max(0.0f, temp[idx]);
            float smoke = std::max(0.0f, dens[idx]);
            float force = buoyancy_strength * heat - smoke_weight * smoke;
            v_source[idx] += std::clamp(force, -0.75f, 2.2f);
        }
    }
}

static void add_breeze_force(int N, float *u_source, float *v_source)
{
    N = SCR_SIZE;

    if (!breeze_enabled || breeze_strength <= 0.0f || breeze_response <= 0.0f)
        return;

    float slow = std::sin(t * breeze_frequency + 0.65f);
    float slower = 0.45f * std::sin(t * breeze_frequency * 0.37f + 2.10f);
    float direction = std::clamp(slow + slower, -1.0f, 1.0f);

    for (int j = 1; j <= N; j++)
    {
        float y = (float)(j - 1) / (float)std::max(1, N - 1);
        float height_profile = 0.20f + 0.80f * y;
        float vertical_wave = std::sin(t * 0.83f + (float)j * 0.045f);

        for (int i = 1; i <= N; i++)
        {
            if (is_solid_cell(i, j))
                continue;

            int idx = IX(i, j);
            float local_wave = 0.18f * std::sin(t * 0.71f + (float)i * 0.031f + (float)j * 0.017f);
            float target_u = breeze_strength * height_profile * (direction + local_wave);
            float target_v = breeze_strength * 0.08f * height_profile * vertical_wave;

            u_source[idx] += (target_u - u[idx]) * breeze_response;
            v_source[idx] += (target_v - v[idx]) * breeze_response * 0.35f;
        }
    }
}

void apply_vorticity_confinement(int N, float *u, float *v, float *u_source, float *v_source)
{
    N = SCR_SIZE;

    if (vorticity_strength <= 0.0f)
        return;

    static float curl[(SCR_SIZE + 2) * (SCR_SIZE + 2)];
    int size = (N + 2) * (N + 2);
    for (int idx = 0; idx < size; idx++)
        curl[idx] = 0.0f;

    for (int j = 1; j <= N; j++)
    {
        for (int i = 1; i <= N; i++)
        {
            if (is_solid_cell(i, j))
                continue;

            curl[IX(i, j)] = 0.5f * ((sample_cell(v, i + 1, j) - sample_cell(v, i - 1, j))
                - (sample_cell(u, i, j + 1) - sample_cell(u, i, j - 1)));
        }
    }

    for (int j = 2; j < N; j++)
    {
        for (int i = 2; i < N; i++)
        {
            if (is_solid_cell(i, j))
                continue;

            float grad_x = 0.5f * (std::abs(curl[IX(i + 1, j)]) - std::abs(curl[IX(i - 1, j)]));
            float grad_y = 0.5f * (std::abs(curl[IX(i, j + 1)]) - std::abs(curl[IX(i, j - 1)]));
            float len = std::sqrt(grad_x * grad_x + grad_y * grad_y) + 1.0e-6f;
            float nx = grad_x / len;
            float ny = grad_y / len;
            float w = curl[IX(i, j)];

            u_source[IX(i, j)] += vorticity_strength * ny * w;
            v_source[IX(i, j)] -= vorticity_strength * nx * w;
        }
    }
}

static void clear_field(int N, float *x)
{
    N = SCR_SIZE;

    int size = (N + 2) * (N + 2);
    for (int i = 0; i < size; i++)
    {
        x[i] = 0.0f;
    }
}

void get_from_UI(int N, float *d, float *u_source, float *v_source)
{
    N = SCR_SIZE;

    clear_field(N, d);
    clear_field(N, temp_prev);
    clear_field(N, u_source);
    clear_field(N, v_source);

    add_upward_wind(N, v_source);

    if (chimney_enabled)
        add_chimney_source(N, d, temp_prev, u_source, v_source);

    add_buoyancy_force(N, v_source);
    add_breeze_force(N, u_source, v_source);

    if (mouse_down)
        add_density_source(N, d);

    add_mouse_force_source(N, u_source, v_source);

    if (vorticity_enabled)
        apply_vorticity_confinement(N, u, v, u_source, v_source);
}

void decay()
{
    int size = (SCR_SIZE + 2) * (SCR_SIZE + 2);
    for (int i = 0; i < size; i++)
    {
        dens[i] *= (1.0f - decay_rate);
    }
}

void diffuse_bad(int N, int b, float *x, float *x0, float diff, float dt)
{
    N = SCR_SIZE;

    float a = dt * diff * N * N;
    for (int i = 1; i <= N; i++)
    {
        for (int j = 1; j <= N; j++)
        {
            if (is_solid_cell(i, j))
            {
                x[IX(i, j)] = 0.0f;
                continue;
            }

            x[IX(i, j)] = x0[IX(i, j)] + a * (sample_cell(x0, i - 1, j) + sample_cell(x0, i + 1, j) + sample_cell(x0, i, j - 1) + sample_cell(x0, i, j + 1) - 4.0f * x0[IX(i, j)]);
        }
    }
    set_bnd(N, b, x);
}

void lin_solve(int N, int b, float *x, float *x0, float a, float c)
{
    N = SCR_SIZE;

    for (int k = 0; k < 20; k++)
    {
        for (int j = 1; j <= N; j++)
        {
            for (int i = 1; i <= N; i++)
            {
                if (is_solid_cell(i, j))
                {
                    x[IX(i, j)] = 0.0f;
                    continue;
                }

                x[IX(i, j)] = (x0[IX(i, j)] + a * (sample_cell(x, i - 1, j) + sample_cell(x, i + 1, j)
                    + sample_cell(x, i, j - 1) + sample_cell(x, i, j + 1))) / c;
            }
        }
        set_bnd(N, b, x);
    }
}

void diffuse(int N, int b, float *x, float *x0, float diff, float dt)
{
    N = SCR_SIZE;

    float a = dt * diff * N * N;
    lin_solve(N, b, x, x0, a, 1.0f + 4.0f * a);
}

void advect(int N, int b, float *d, float *d0, float *u, float *v, float dt)
{
    N = SCR_SIZE;

    int i, j, i0, j0, i1, j1;
    float x, y, s0, t0, s1, t1, dt0;

    dt0 = dt * N;
    for (i = 1; i <= N; i++)
    {
        for (j = 1; j <= N; j++)
        {
            if (is_solid_cell(i, j))
            {
                d[IX(i, j)] = 0.0f;
                continue;
            }

            x = i - dt0 * u[IX(i, j)];
            y = j - dt0 * v[IX(i, j)];

            if (boundary_mode == BOUNDARY_WRAP)
            {
                x = wrap_position(x, N);
                y = wrap_position(y, N);
                i0 = wrap_index((int)x, N);
                i1 = wrap_index((int)x + 1, N);
                j0 = wrap_index((int)y, N);
                j1 = wrap_index((int)y + 1, N);
            }
            else
            {
                if (x < 0.5f)
                    x = 0.5f;
                if (x > N + 0.5f)
                    x = N + 0.5f;
                if (y < 0.5f)
                    y = 0.5f;
                if (y > N + 0.5f)
                    y = N + 0.5f;

                i0 = (int)x;
                i1 = i0 + 1;
                j0 = (int)y;
                j1 = j0 + 1;
            }

            s1 = x - (int)x;
            s0 = 1.0f - s1;
            t1 = y - (int)y;
            t0 = 1.0f - t1;

            float fallback = d0[IX(i, j)];
            auto advect_sample = [&](int si, int sj)
            {
                return is_solid_cell(si, sj) ? fallback : d0[IX(si, sj)];
            };

            d[IX(i, j)] = s0 * (t0 * advect_sample(i0, j0) + t1 * advect_sample(i0, j1))
                + s1 * (t0 * advect_sample(i1, j0) + t1 * advect_sample(i1, j1));
        }
    }
    set_bnd(N, b, d);
}

void project(int N, float *u, float *v, float *p, float *div)
{
    N = SCR_SIZE;

    int i, j, k;
    float h = 1.0f / N;

    for (i = 1; i <= N; i++)
    {
        for (j = 1; j <= N; j++)
        {
            if (is_solid_cell(i, j))
            {
                div[IX(i, j)] = 0.0f;
                p[IX(i, j)] = 0.0f;
                continue;
            }

            div[IX(i, j)] = -0.5f * h * (sample_cell(u, i + 1, j) - sample_cell(u, i - 1, j)
                + sample_cell(v, i, j + 1) - sample_cell(v, i, j - 1));
            p[IX(i, j)] = 0.0f;
        }
    }
    set_bnd(N, 0, div);
    set_bnd(N, 0, p);

    lin_solve(N, 0, p, div, 1.0f, 4.0f);

    for (i = 1; i <= N; i++)
    {
        for (j = 1; j <= N; j++)
        {
            if (is_solid_cell(i, j))
            {
                u[IX(i, j)] = 0.0f;
                v[IX(i, j)] = 0.0f;
                continue;
            }

            u[IX(i, j)] -= 0.5f * (sample_cell(p, i + 1, j) - sample_cell(p, i - 1, j)) / h;
            v[IX(i, j)] -= 0.5f * (sample_cell(p, i, j + 1) - sample_cell(p, i, j - 1)) / h;
        }
    }
    set_bnd(N, 1, u);
    set_bnd(N, 2, v);
}

void vel_step(int N, float *u, float *v, float *u0, float *v0, float visc, float dt)
{
    N = SCR_SIZE;

    add_source(N, u, u0, dt);
    add_source(N, v, v0, dt);
    SWAP(u0, u);
    diffuse(N, 1, u, u0, visc, dt);
    SWAP(v0, v);
    diffuse(N, 2, v, v0, visc, dt);
    project(N, u, v, u0, v0);
    SWAP(u0, u);
    SWAP(v0, v);
    advect(N, 1, u, u0, u0, v0, dt);
    advect(N, 2, v, v0, u0, v0, dt);
    project(N, u, v, u0, v0);
}

void dens_step(int N, float *x, float *x0, float *u, float *v, float diff, float dt)
{
    N = SCR_SIZE;

    add_source(N, x, x0, dt);
    SWAP(x0, x);
    diffuse(N, 0, x, x0, diff, dt);
    SWAP(x0, x);
    advect(N, 0, x, x0, u, v, dt);
}

static void dissipate_temperature(int N, float *x)
{
    N = SCR_SIZE;

    float keep = std::clamp(1.0f - temperature_decay, 0.0f, 1.0f);
    int size = (N + 2) * (N + 2);
    for (int i = 0; i < size; i++)
        x[i] *= keep;
}

void temp_step(int N, float *x, float *x0, float *u, float *v, float diff, float dt)
{
    N = SCR_SIZE;

    add_source(N, x, x0, dt);
    SWAP(x0, x);
    diffuse(N, 0, x, x0, diff, dt);
    SWAP(x0, x);
    advect(N, 0, x, x0, u, v, dt);
    dissipate_temperature(N, x);
    set_bnd(N, 0, x);
}

void my_dens_step()
{
    if (mouse_down)
        splat_density(SCR_SIZE, dens, 1.0f);

    std::memcpy(dens_prev, dens, sizeof(dens));
    diffuse(SCR_SIZE, 0, dens, dens_prev, diff, dt);

    std::memcpy(dens_prev, dens, sizeof(dens));
    advect(SCR_SIZE, 0, dens, dens_prev, u, v, dt);

    decay();

    static int frame = 0;
    if (++frame % 30 == 0)
    {
        int sz = (SCR_SIZE + 2) * (SCR_SIZE + 2);
        float max_u = 0.0f, max_v = 0.0f, sum_dens = 0.0f;
        for (int i = 0; i < sz; i++)
        {
            if (std::abs(u[i]) > max_u) max_u = std::abs(u[i]);
            if (std::abs(v[i]) > max_v) max_v = std::abs(v[i]);
            sum_dens += dens[i];
        }
        std::cout << "[f" << frame << "] max|u|=" << max_u
            << " max|v|=" << max_v
            << " sum(dens)=" << sum_dens << std::endl;
    }
}
