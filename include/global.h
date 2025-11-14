#pragma once

#define SCR_SIZE 600
#define IX(i,j) ((i) + (SCR_SIZE+2) * (j))
#define SWAP(x0, x) {float *tmp=x0; x0=x; x=tmp;}

extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;

extern const float g;
extern const float pi;

extern float diff;
extern float decay_rate;
extern float dt;

extern float u[SCR_SIZE], v[SCR_SIZE], u_prev[SCR_SIZE], v_prev[SCR_SIZE];
extern float dens[(SCR_SIZE+2)*(SCR_SIZE+2)];
extern float dens_prev[(SCR_SIZE+2)*(SCR_SIZE+2)];

extern bool mouse_down;
extern double mouse_x;
extern double mouse_y;
