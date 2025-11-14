#include "global.h"

const unsigned int SCR_WIDTH = SCR_SIZE;
const unsigned int SCR_HEIGHT = SCR_SIZE;

const float g = 9.8;
const float pi = 3.141592;

float dt = 0.01;
float decay_rate = 0.98;
float diff = 0.07f;

float u[SCR_SIZE], v[SCR_SIZE], u_prev[SCR_SIZE], v_prev[SCR_SIZE];
float dens[(SCR_SIZE+2)*(SCR_SIZE+2)];
float dens_prev[(SCR_SIZE+2)*(SCR_SIZE+2)];

bool mouse_down = false;
double mouse_x = 0.0l;
double mouse_y = 0.01;
