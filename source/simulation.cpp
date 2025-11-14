#include "simulation.h"
#include "global.h"

#include <iostream>
#include <cstdlib>
#include <random>
#include <time.h>

void init()
{
    int N = SCR_SIZE;
    
    int init_mode = 1;
    bool box_exist = false;
    bool vel_field = false;
    
    switch (init_mode)
    {
        case 0:
        {
            srand((unsigned int)time(NULL));
            for (int i=0; i<SCR_SIZE+2; i++)
            {
                for (int j=0; j<SCR_SIZE+2; j++)
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
    
    if (box_exist)
    {
        for (int i=SCR_SIZE/2; i<SCR_SIZE/2 + 100; i++)
        {
            for (int j=SCR_SIZE/2; j<SCR_SIZE/2 + 100; j++)
            {
                dens_prev[IX(i, j)] = 1.0f;
                dens[IX(i, j)] = 1.0f;
            }
        }
    }
    
    if (vel_field)
    {
        for (int i=0;i<=N+1;i++)
        {
            for (int j=0; j<=N+1; j++)
            {
                u[IX(i,j)] = 100.0f;
                v[IX(i,j)] = 5.0f;
                u_prev[IX(i,j)] = 100.0f;
                v_prev[IX(i,j)] = 5.0f;
            }
        }
    }
};

void set_bnd ( int N, int b, float * x )
{
    int i;
    for ( i=1 ; i<=N ; i++ )
    {
        x[IX(0 ,i)] = b==1 ? (-1) * x[IX(1,i)] : x[IX(1,i)];
        x[IX(N+1,i)] = b==1 ? (-1) * x[IX(N,i)] : x[IX(N,i)];
        x[IX(i,0 )] = b==2 ? (-1) * x[IX(i,1)] : x[IX(i,1)];
        x[IX(i,N+1)] = b==2 ? (-1) * x[IX(i,N)] : x[IX(i,N)];
    }
    x[IX(0 ,0 )] = 0.5*(x[IX(1,0 )]+x[IX(0 ,1)]);
    x[IX(0 ,N+1)] = 0.5*(x[IX(1,N+1)]+x[IX(0 ,N)]);
    x[IX(N+1,0 )] = 0.5*(x[IX(N,0 )]+x[IX(N+1,1)]);
    x[IX(N+1,N+1)] = 0.5*(x[IX(N,N+1)]+x[IX(N+1,N)]);
}

void add_source(int N, float *x, float *s, float dt)
{
    int mx = (int)((mouse_x / SCR_WIDTH)  * N + 1);
    if (mx < 1 || mx > N)
        return;
    int my = (int)(((SCR_HEIGHT - mouse_y) / SCR_HEIGHT) * N + 1);
    if (my < 1 || my > N)
        return;
    
    float radius = 30.0f;
    for (int i=1;i<=N;i++)
    {
        for (int j=1; j<=N; j++)
        {
            double dist = sqrt((i-mx)*(i-mx) + (j-my)*(j-my));
            if (dist < radius)
            {
                float density = 1.0f - dist/radius;
                dens[IX(i,j)] = std::min(1.0f, dens[IX(i,j)] + density);
            }
        }
    }
}

void decay()
{
    int size = (SCR_SIZE+2)*(SCR_SIZE+2);
    for (int i=0;i<size;i++)
    {
        dens[i] *= decay_rate;
    }
}

//explicit euler method
void diffuse_bad (int N, int b, float * x, float * x0, float diff, float dt)
{
    float a=dt * diff * N*N;
    for (int i=1; i<=N; i++)
    {
        for (int j=1; j<=N; j++)
        {
            x[IX(i,j)] = x0[IX(i,j)] + a*(x0[IX(i-1,j)]+x0[IX(i+1,j)]+x0[IX(i,j-1)]+x0[IX(i,j+1)]-4*x0[IX(i,j)]);
        }
    }
    set_bnd ( N, b, x );
}

//implicit euler
void diffuse(int N, int b, float *x, float *x0, float diff, float dt)
{
    float a = dt * diff * N*N;
    
    for (int k=0; k<50; k++)
    {
        for (int j=1; j<=N; j++)
        {
            for (int i=1; i<=N; i++)
            {
                //Gauss-Seidel Iteration Method
                x[IX(i,j)] = (x0[IX(i,j)] + a*(x[IX(i-1,j)] + x[IX(i+1,j)] + x[IX(i,j-1)] + x[IX(i,j+1)])) / (1 + 4*a);
                
                //Jacobi Iteration Method
                //x[IX(i,j)] = (x0[IX(i,j)] + a*(x0[IX(i-1,j)] + x0[IX(i+1,j)] + x0[IX(i,j-1)] + x0[IX(i,j+1)])) / (1 + 4*a);
            }
        }
        set_bnd(N, b, x);
    }
}

void advect(int N, int b, float *d, float *d0, float *u, float *v, float dt)
{
    int i,j,i0,j0,i1,j1;
    float x,y,s0,t0,s1,t1,dt0;
    
    dt0 = dt*N;
    for (i=1;i<=N;i++)
    {
        for (j=1;j<=N;j++)
        {
            //reverse
            x = i - dt0 * u[IX(i,j)];
            y = j - dt0 * v[IX(i,j)];
            
            //border
            if (x < 0.5)
                x = 0.5;
            if (x > N + 0.5)
                x = N + 0.5;
            
            if (y < 0.5)
                y = 0.5;
            if (y > N + 0.5)
                y = N + 0.5;
            
            //interpolation
            i0 = (int)x;
            i1 = i0 + 1;
            j0 = (int)y;
            j1 = j0 + 1;
            
            s1 = x - i0;
            s0 = 1 - s1;
            t1 = y - j0;
            t0 = 1 - t1;
            
            d[IX(i,j)] = s0*(t0*d0[IX(i0,j0)] + s1*d0[IX(i0,j1)])
                        + s1*(t0*d0[IX(i1,j0)] + t1*d0[IX(i1,j1)]);
        }
    }
    set_bnd(N, b, d);
}

void dens_step(int N, float * x, float * x0, float * u, float * v, float diff, float dt)
{
    if (mouse_down)
        add_source(N, x, x0, dt);
    SWAP(x0, x);
    diffuse(N, 0, x, x0, diff, dt);
    SWAP(x0,x);
    //advect(N, 0, x, x0, u, v, dt);
    SWAP(x0,x);
    decay();
}

void my_dens_step()
{
    if (mouse_down)
        add_source(SCR_SIZE, dens, nullptr, dt);
    std::memcpy(dens_prev, dens, sizeof(dens));
    //diffuse_bad(SCR_SIZE, 0, dens, dens_prev, diff, dt);
    diffuse(SCR_SIZE, 0, dens, dens_prev, diff, dt);
    advect(SCR_SIZE, 0, dens, dens_prev, u, v, dt);
    decay();
}
