#pragma once

#include "global.h"

void add_source(int N, float *x, float *s, float dt);
void add_density_source(int N, float *s);
void get_from_UI(int N, float *d, float *u, float *v);
void diffuse_bad ( int N, int b, float * x, float * x0, float diff, float dt);
void lin_solve(int N, int b, float *x, float *x0, float a, float c);
void diffuse(int N, int b, float *x, float *x0, float diff, float dt);
void init();
void update_obstacles(int N);
void update_velocity_field();
void decay();
void apply_vorticity_confinement(int N, float *u, float *v, float *u_source, float *v_source);
void advect(int N, int b, float *d, float *d0, float *u, float *v, float dt);
void project(int N, float *u, float *v, float *p, float *div);
void vel_step(int N, float *u, float *v, float *u0, float *v0, float visc, float dt);
void dens_step(int N, float * x, float * x0, float * u, float * v, float diff, float dt);
void temp_step(int N, float *x, float *x0, float *u, float *v, float diff, float dt);
void my_dens_step();
