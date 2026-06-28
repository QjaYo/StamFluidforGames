#include "global.h"

const unsigned int SCR_WIDTH = 1000;
const unsigned int SCR_HEIGHT = 666;

const float g = 9.8;
const float pi = 3.141592;

BoundaryMode boundary_mode = BOUNDARY_SOLID;
bool obstacles_enabled = true;
bool static_obstacle_enabled = false;
bool mouse_obstacle_enabled = true;
bool mouse_force_enabled = true;
bool vorticity_enabled = true;
bool buoyancy_enabled = true;
bool breeze_enabled = true;
bool house_scene_enabled = true;
bool chimney_enabled = true;
float obstacle_radius = SCR_SIZE * 0.10f;
float inflow_velocity = 1.0f;
float chimney_source = 14.0f;
float chimney_force = 0.55f;
float chimney_radius = SCR_SIZE * 0.018f;
float chimney_jitter = SCR_SIZE * 0.014f;
float upward_wind_strength = 0.0f;
float upward_wind_height = 0.42f;
float mouse_force = 5.0f;
float mouse_force_radius = SCR_SIZE * 0.05f;
float vorticity_strength = 3.6f;
float temperature_source = 11.0f;
float temperature_diff = 0.00012f;
float temperature_decay = 0.0012f;
float buoyancy_strength = 0.78f;
float smoke_weight = 0.022f;
float breeze_strength = 0.204f;
float breeze_response = 1.10f;
float breeze_frequency = 0.55f;
float house_scene_height = 1.0f / 3.0f;
float house_bottom_padding_ratio = 35.0f / 640.0f;
const int house_chimney_count = 3;
const float house_chimney_u[3] = {210.0f / 640.0f, 282.0f / 640.0f, 448.0f / 640.0f};
const float house_chimney_v[3] = {72.0f / 640.0f, 72.0f / 640.0f, 204.0f / 640.0f};

float t = 0.0f;
float dt = 0.04f;
float decay_rate = 0.05f;
float diff = 0.00035f;
float visc = 0.0f;
float source = 100.0f;
float render_white_density = 8.0f;
float radius = SCR_SIZE * 0.025f;
float mouse_obstacle_radius = SCR_SIZE * 0.075f;
float mouse_obstacle_x = SCR_SIZE * 0.5f;
float mouse_obstacle_y = SCR_SIZE * 0.5f;
float mouse_obstacle_u = 0.0f;
float mouse_obstacle_v = 0.0f;

float u[(SCR_SIZE + 2) * (SCR_SIZE + 2)],      v[(SCR_SIZE + 2) * (SCR_SIZE + 2)];
float u_prev[(SCR_SIZE + 2) * (SCR_SIZE + 2)], v_prev[(SCR_SIZE + 2) * (SCR_SIZE + 2)];
float dens[(SCR_SIZE + 2) * (SCR_SIZE + 2)];
float dens_prev[(SCR_SIZE + 2) * (SCR_SIZE + 2)];
float temp[(SCR_SIZE + 2) * (SCR_SIZE + 2)];
float temp_prev[(SCR_SIZE + 2) * (SCR_SIZE + 2)];
bool solid[(SCR_SIZE + 2) * (SCR_SIZE + 2)];

bool mouse_down = false;
bool obstacle_mouse_down = false;
double mouse_x = 0.0l;
double mouse_y = 0.01;
