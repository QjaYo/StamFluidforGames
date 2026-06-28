#pragma once

#define SIM_SIZE 450
#define SCR_SIZE SIM_SIZE
#define IX(i,j) ((i) + (SCR_SIZE+2) * (j))
#define SWAP(x0, x) {float *tmp=x0; x0=x; x=tmp;}

enum BoundaryMode
{
    BOUNDARY_SOLID,
    BOUNDARY_WRAP,
    BOUNDARY_INFLOW
};

extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;

extern const float g;
extern const float pi;

extern BoundaryMode boundary_mode;
extern bool obstacles_enabled;
extern bool static_obstacle_enabled;
extern bool mouse_obstacle_enabled;
extern bool mouse_force_enabled;
extern bool vorticity_enabled;
extern bool buoyancy_enabled;
extern bool breeze_enabled;
extern bool house_scene_enabled;
extern bool chimney_enabled;
extern float obstacle_radius;
extern float inflow_velocity;
extern float chimney_source;
extern float chimney_force;
extern float chimney_radius;
extern float chimney_jitter;
extern float upward_wind_strength;
extern float upward_wind_height;
extern float mouse_force;
extern float mouse_force_radius;
extern float vorticity_strength;
extern float temperature_source;
extern float temperature_diff;
extern float temperature_decay;
extern float buoyancy_strength;
extern float smoke_weight;
extern float breeze_strength;
extern float breeze_response;
extern float breeze_frequency;
extern float house_scene_height;
extern float house_bottom_padding_ratio;
extern const int house_chimney_count;
extern const float house_chimney_u[3];
extern const float house_chimney_v[3];

extern float diff;
extern float visc;
extern float source;
extern float decay_rate;
extern float t;
extern float dt;
extern float render_white_density;
extern float radius;
extern float mouse_obstacle_radius;
extern float mouse_obstacle_x;
extern float mouse_obstacle_y;
extern float mouse_obstacle_u;
extern float mouse_obstacle_v;

extern float u[(SCR_SIZE+2)*(SCR_SIZE+2)],      v[(SCR_SIZE+2)*(SCR_SIZE+2)];
extern float u_prev[(SCR_SIZE+2)*(SCR_SIZE+2)], v_prev[(SCR_SIZE+2)*(SCR_SIZE+2)];
extern float dens[(SCR_SIZE+2)*(SCR_SIZE+2)];
extern float dens_prev[(SCR_SIZE+2)*(SCR_SIZE+2)];
extern float temp[(SCR_SIZE+2)*(SCR_SIZE+2)];
extern float temp_prev[(SCR_SIZE+2)*(SCR_SIZE+2)];
extern bool solid[(SCR_SIZE+2)*(SCR_SIZE+2)];

extern bool mouse_down;
extern bool obstacle_mouse_down;
extern double mouse_x;
extern double mouse_y;
