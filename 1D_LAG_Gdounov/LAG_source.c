/* 
 * This is a implementation of fully explict forward Euler scheme for 1-D equations of motion on Lagrange coordinate
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "file_io.h"
#include "finite_difference_solver.h"
#include "Riemann_solver.h"

#ifndef N_CONF
#define N_CONF 5
#endif /* N_CONF */

double * U0 = NULL;
double * P0 = NULL;
double * RHO0 = NULL;

int main(int argc, char *argv[])
{
  _1D_initialize(argv[0], argv[1], argv[2], argv[3]);  /* Firstly we read the initial
							* data file. The function 
							* initialize return a point
							* pointing to the position
							* of a block of memory
							* consisting (m+1) variables
							* of type double.
							* The value of first of these
							* variables is m. The
							* following m variables
							* are the initial value.
							*/
  int m = (int)U0[0];  /* m is the number of initial value
			* as well as the number of grids.
			* As m is frequently use to
			* represent the number of grids,
			* we do not use the name such as
			* num_grid here to correspond to
			* notation in the math theory.
			*/
  double config[N_CONF];  /* config[0] is the constant of the perfect gas
                           * config[1] is the length of the time step
			   * config[2] is the spatial grid size
			   * config[3] is the largest value can be
			   *           seen as zero
			   * config[4] is the number of time steps
			   */
  _1D_configurate(config, argv[0], argv[4]); /* Read the config-
					      * uration data.
					      * The detail could
					      * be seen in the
					      * definition of
					      * array config.
					      */

  int j = 0, k = 0, N = (int)(config[4]) + 1, i = 0;
  
  double h = config[2], gamma = config[0];

  double * RHO[N];
  RHO[0] = RHO0 + 1;
  for(k = 1; k < N; ++k)
  {
    RHO[k] = (double *)malloc(m * sizeof(double));
    if(RHO[k] == NULL)
    {
      for(i = 1; i < k; ++i)
      {
	free(RHO[i]);
	RHO[i] = NULL;
      }
      free(RHO0);
      free(U0);
      free(P0);
      RHO[0] = NULL;
      RHO0 = NULL;
      U0 = NULL;
      P0 = NULL;
      printf("NOT enough memory! RHO[%d]\n", k);
      exit(5);
    }
  }
  double * U[N];
  U[0] = U0 + 1;
  for(k = 1; k < N; ++k)
  {
    U[k] = (double *)malloc(m * sizeof(double));
    if(U[k] == NULL)
    {
      for(i = 1; i < k; ++i)
      {
	free(U[i]);
	U[i] = NULL;
      }
      for(i = 1; i < N; ++i)
      {
	free(RHO[i]);
	RHO[i] = NULL;
      }
      free(RHO0);
      free(U0);
      free(P0);
      RHO[0] = NULL;
      U[0] = NULL;
      RHO0 = NULL;
      U0 = NULL;
      P0 = NULL;
      printf("NOT enough memory! U[%d]\n", k);
      exit(5);
    }
  }
  
  double * P[N];
  P[0] = P0 + 1;
  for(k = 1; k < N; ++k)
  {
    P[k] = (double *)malloc(m * sizeof(double));
    if(P[k] == NULL)
    {
      for(i = 1; i < k; ++i)
      {
	free(P[i]);
	P[i] = NULL;
      }
      for(i = 1; i < N; ++i)
      {
	free(RHO[i]);
	RHO[i] = NULL;
	free(U[i]);
	P[i] = NULL;
      }
      free(RHO0);
      free(U0);
      free(P0);
      RHO[0] = NULL;
      U[0] = NULL;
      P[0] = NULL;
      RHO0 = NULL;
      U0 = NULL;
      P0 = NULL;
      printf("NOT enough memory! P[%d]\n", k);
      exit(5);
    }
  }


  double UL[N], PL[N], RHOL[N];
  double UR[N], PR[N], RHOR[N];
  double cpu_time[N];

  for(k = 0; k < N; ++k)
  {
    UL[k] = U[0][0];
    UR[k] = U[0][m-1];
    PL[k] = P[0][0];
    PR[k] = P[0][m-1];
    RHOL[k] = RHO[0][0];
    RHOR[k] = RHO[0][m-1];
  }


  double * E[N];
  for(k = 0; k < N; ++k)
  {
    E[k] = (double *)malloc(m * sizeof(double));
    if(E[k] == NULL)
    {
      for(i = 0; i < k; ++i)
      {
	free(E[i]);
	E[i] = NULL;
      }
      for(i = 1; i < N; ++i)
      {
	free(RHO[i]);
	RHO[i] = NULL;
	free(U[i]);
	U[i] = NULL;
	free(P[i]);
	P[i] = NULL;
      }
      free(RHO0);
      free(U0);
      free(P0);
      RHO[0] = NULL;
      U[0] = NULL;
      P[0] = NULL;
      RHO0 = NULL;
      U0 = NULL;
      P0 = NULL;
      printf("NOT enough memory! E[%d]\n", k);
      exit(5);
    }
  }

  double * X[N];
  for(k = 0; k < N; ++k)
  {
    X[k] = (double *)malloc((m+1) * sizeof(double));
    if(X[k] == NULL)
    {
      for(i = 0; i < k; ++i)
      {
	free(X[i]);
	X[i] = NULL;
      }
      for(i = 1; i < N; ++i)
      {
	free(RHO[i]);
	RHO[i] = NULL;
	free(U[i]);
	U[i] = NULL;
	free(P[i]);
	P[i] = NULL;
	free(E[i]);
	E[i] = NULL;
      }
      free(RHO0);
      free(U0);
      free(P0);
      RHO[0] = NULL;
      U[0] = NULL;
      P[0] = NULL;
      RHO0 = NULL;
      U0 = NULL;
      P0 = NULL;
      printf("NOT enough memory! X[%d]\n", k);
      exit(5);
    }
  }


  double MASS[m];

  for(j = 0; j < m; ++j)
		  MASS[j]=config[2]*RHO[0][j];                                               
  for(j = 0; j <= m; ++j)
		  X[0][j] = config[2]*j;
  for(j = 0; j < m; ++j)
		  E[0][j] = 0.5*U[0][j]*U[0][j] + P[0][j]/(config[0] - 1.0)/RHO[0][j]; /* initialize the values of mass,coordinate and energy.
											*/

  Godunov_solver_source(config, m, RHO, U, P, E, X, MASS, RHOL, UL, PL, RHOR, UR, PR, cpu_time);
  /* use Godunov scheme to solve it on Lagrange coordinate. */

  _1D_file_write(m, N-1, RHO, U, P, E, X, cpu_time, config, argv[5]); /*write the final data down.
 								      */ 																								

  for(k = 1; k < N; ++k)
  {
    free(RHO[k]);
    free(U[k]);
    free(P[k]);
    free(E[k]);
    free(X[k]);
    RHO[k] = NULL;
    U[k] = NULL;
    P[k] = NULL;
    E[k] = NULL;
    X[k] = NULL;
  }
  free(RHO0);
  free(U0);
  free(P0);
  RHO0 = NULL;
  U0 = NULL;
  P0 = NULL;
  RHO[0] = NULL;
  U[0] = NULL;
  P[0] = NULL;
  free(E[0]);
  E[0] = NULL;
  free(X[0]);
  X[0] = NULL;


  printf("\n");
  return 0;
}
