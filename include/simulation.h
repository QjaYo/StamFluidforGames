#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "global.h"

void add_source(int N, float *x, float *s, float dt);
void diffuse_bad ( int N, int b, float * x, float * x0, float diff, float dt);
void diffuse(int N, int b, float *x, float *x0, float diff, float dt);
void init();
void decay();
void advect(int N, int b, float *d, float *d0, float *u, float *v, float dt);
void dens_step(int N, float * x, float * x0, float * u, float * v, float diff, float dt);
void my_dens_step();
