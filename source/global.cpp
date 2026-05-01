#include "global.h"

const unsigned int SCR_WIDTH = SCR_SIZE;
const unsigned int SCR_HEIGHT = SCR_SIZE;

const float g = 9.8;
const float pi = 3.141592;

float t = 0.0f;
float dt = 0.1;
float decay_rate = 0.01f;
float diff = 0.8f;
float radius = 15.0f;

float u[(SCR_SIZE + 2) * (SCR_SIZE + 2)],      v[(SCR_SIZE + 2) * (SCR_SIZE + 2)];
float u_prev[(SCR_SIZE + 2) * (SCR_SIZE + 2)], v_prev[(SCR_SIZE + 2) * (SCR_SIZE + 2)];
float dens[(SCR_SIZE + 2) * (SCR_SIZE + 2)];
float dens_prev[(SCR_SIZE + 2) * (SCR_SIZE + 2)];

bool mouse_down = false;
double mouse_x = 0.0l;
double mouse_y = 0.01;
    