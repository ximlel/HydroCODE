/**
 * @file  LAG_source.c
 * @brief This is a C file of the main function.
 */

/** 
 * @mainpage 1D Godunov/GRP scheme for Lagrangian hydrodynamics
 * @brief This is an implementation of fully explict forward Euler scheme
 *        for 1-D Euler equations of motion on Lagrange coordinate.
 * @version 0.1
 *
 * @section File_directory File directory
 * <table>
 * <tr><th> data_in/  <td> Folder to store input files RHO/U/P/config.txt
 * <tr><th> data_out/ <td> Folder to store output files RHO/U/P/E/X/log.txt
 * <tr><th> doc/      <td> Code documentation generated by doxygen
 * <tr><th> src/      <td> Folder to store C source code
 * </table>
 * 
 * @section Program_structure Program structure
 * <table>
 * <tr><th> include/                   <td> Header files
 * <tr><th> file_io/                   <td> Program reads and writes files
 * <tr><th> finite_difference_solver/  <td> Lagrangian finite volume scheme program
 * <tr><th> Riemann_solver/            <td> Riemann solver programs
 * <tr><th> LAG_source.c               <td> Main program
 * <tr><th> make.sh                    <td> Bash script compiles and runs programs
 * </table>
 * 
 * @section Compile_environment Compile environment
 *          - Linux/Unix: gcc, glibc, MATLAB
 *            - Compile in src/: Run './make.sh' command on the terminal.
 *          - Winodws: Visual Studio, MATLAB
 *
 * @section Usage_description Usage description
 *          - Input files are stored in folder '/data_in/one-dim/name_of_test_example'.
 *          - Input files may be produced by MATLAB script 'value_start.m'.
 *          - Description of configuration file 'config.txt' refers to '_1D_configurate()'.
 *          - Run program:
 *            - Linux/Unix: Run 'LAG_source.out name_of_test_example order' command on the terminal. \n
 *                          e.g. 'LAG_source.out 6_1 2' (second-order GRP scheme)
 *            - Windows:
 *          - Output files can be found in folder '/data_out/one-dim/'.
 *          - Output files may be visualized by MATLAB script 'value_plot.m'.
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "include/file_io.h"
#include "include/finite_difference_solver.h"
#include "include/Riemann_solver.h"

double * U0;   //!< Initial velocity data array pointer.
double * P0;   //!< Initial pressure data array pointer.
double * RHO0; //!< Initial density  data array pointer.

/**
 * @brief This is the main function which constructs the
 *        main structure of the Lagrangian hydrocode.
 * @param[in] argv[1]: Name of test example.
 * @param[in] argv[2]: Order of numerical scheme (= 1 or 2).
 * @return ARGuments state of the program.
 *     @retval 0: ARGuments counter is 3.
 *     @retval 1: ARGuments counter is not 3.
 */

int main(int argc, char *argv[])
{
    if (argc!=3)
	{
	    printf("ARGuments counter is %d not equal to 3!\n", argc);
	    return 1;
	}
    char add[FILENAME_MAX];
    // Get the address of initial data files.
    example_io(argv[1], add, 1);
    /* 
     * We read the initial data files.
     * The function initialize return a point pointing to the position
     * of a block of memory consisting (m+1) variables of type double.
     * The value of first array element of these variables is m.
     * The following m variables are the initial value.
     */
    _1D_initialize(argv[1], add); 
    /* 
     * m is the number of initial value as well as the number of grids.
     * As m is frequently use to represent the number of grids,
     * we do not use the name such as num_grid here to correspond to
     * notation in the math theory.
     */
    int m = (int)U0[0];
    
  double config[N_CONF];
  /* 
   * Read the configuration data.
   * The detail could be seen in the definition of array config
   * referring to file '_1D_f_io.c'.
   */
  _1D_configurate(config, argv[1], add); 

  int k;
  // The number of times steps of the fluid data stored for plotting.
  int N = 2; // (int)(config[4]) + 1;
  double h = config[2], gamma = config[0];
  
  // Initialize arrays of fluid variables.
  double ** RHO, ** U, ** P, ** E, ** X;
  double * MASS, * cpu_time;
  RHO = (double **)malloc(N * sizeof(double *));
  RHO[0] = RHO0 + 1;
  for(k = 1; k < N; ++k)
  {
    RHO[k] = (double *)malloc(m * sizeof(double));
    if(RHO[k] == NULL)
    {
      printf("NOT enough memory! RHO[%d]\n", k);
      goto _END_;
    }
  }
  U = (double**)malloc(N * sizeof(double*));
  U[0] = U0 + 1;
  for(k = 1; k < N; ++k)
  {
    U[k] = (double *)malloc(m * sizeof(double));
    if(U[k] == NULL)
    {
      printf("NOT enough memory! U[%d]\n", k);
      goto _END_;
    }
  } 
  P = (double**)malloc(N * sizeof(double*));
  P[0] = P0 + 1;
  for(k = 1; k < N; ++k)
  {
    P[k] = (double *)malloc(m * sizeof(double));
    if(P[k] == NULL)
    {
      printf("NOT enough memory! P[%d]\n", k);
      goto _END_;
    }
  }
  E = (double**)malloc(N * sizeof(double*));
  for(k = 0; k < N; ++k)
  {
    E[k] = (double *)malloc(m * sizeof(double));
    if(E[k] == NULL)
    {
      printf("NOT enough memory! E[%d]\n", k);
      goto _END_;
    }
  }
  X = (double**)malloc(N * sizeof(double*));
  for(k = 0; k < N; ++k)
  {
    X[k] = (double *)malloc((m+1) * sizeof(double));
    if(X[k] == NULL)
    {
      printf("NOT enough memory! X[%d]\n", k);
      goto _END_;
    }
  }
  MASS=malloc(m * sizeof(double));
  if(MASS == NULL)
      {
	  printf("NOT enough memory! MASS\n");
	  goto _END_;
      }
  // Initialize the values of mass, energy in computational cells and x-coordinate of the cell interfaces.
  for(k = 0; k < m; ++k)
		  MASS[k] = h * RHO[0][k];
  for(k = 0; k < m; ++k)
		  E[0][k] = 0.5*U[0][k]*U[0][k] + P[0][k]/(gamma - 1.0)/RHO[0][k]; 
  for(k = 0; k <= m; ++k)
		  X[0][k] = h * k;

  cpu_time = malloc(N * sizeof(double));
  if(cpu_time == NULL)
      {
	  printf("NOT enough memory! CPU_time\n");
	  goto _END_;
      }
      
  int order; // 1-Godunov, 2-GRP
  order = atoi(argv[2]);
  // Use GRP/Godunov scheme to solve it on Lagrange coordinate.
  if (order == 1)
      Godunov_solver_source(config, m, RHO, U, P, E, X, MASS, cpu_time);
  else if (order == 2)
      GRP_solver_source(config, m, RHO, U, P, E, X, MASS, cpu_time);
  else
      {
	  printf("NOT appropriate order of the scheme! The order is %d\n", order);
	  goto _END_;
      }
  
  char name_out[200];
  strcpy(name_out, argv[1]);  
  strcat(name_out, "_");
  strcat(name_out, argv[2]);
  strcat(name_out, "Order");
  example_io(name_out, add, 0);
  // Write the final data down.
  _1D_file_write(m, N, RHO, U, P, E, X, cpu_time, config, argv[1], add);

 _END_:
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
  free(X[0]);
  E[0] = NULL;
  X[0] = NULL;
  free(MASS);
  MASS = NULL;
  free(cpu_time);
  cpu_time = NULL;
  
  return 0;
}
