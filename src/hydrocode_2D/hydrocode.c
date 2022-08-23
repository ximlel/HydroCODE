/**
 * @file  hydrocode.c
 * @brief This is a C file of the main function.
 */

/** 
 * @mainpage 2D Godunov/GRP scheme for Eulerian hydrodynamics
 * @brief This is an implementation of fully explict forward Euler scheme
 *        for 2-D Euler equations of motion on Eulerian coordinate.
 * @version 0.2
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
 * <tr><th> finite_volume/             <td> Finite volume scheme programs
 * <tr><th> Riemann_solver/            <td> Riemann solver programs
 * <tr><th> tools/                     <td> Tool functions
 * <tr><th> hydrocode_2D/hydrocode.c   <td> Main program
 * <tr><th> hydrocode_2D/hydrocode.sh  <td> Bash script compiles and runs programs
 * </table>
 *
 * @section Exit_status Program exit status code
 * <table>
 * <tr><th> exit(0)  <td> EXIT_SUCCESS
 * <tr><th> exit(1)  <td> File directory error
 * <tr><th> exit(2)  <td> Data reading error
 * <tr><th> exit(3)  <td> Calculation error
 * <tr><th> exit(4)  <td> Arguments error
 * <tr><th> exit(5)  <td> Memory error
 * </table>
 * 
 * @section Compile_environment Compile environment
 *          - Linux/Unix: gcc, glibc, MATLAB/Octave
 *            - Compile in 'src/hydrocode': Run './make.sh' command on the terminal.
 *          - Winodws: Visual Studio, MATLAB/Octave
 *            - Create a C++ Project from Existing Code in 'src/hydrocode_2D/' with ProjectName 'hydrocode'.
 *            - Compile in 'x64/Debug' using shortcut key 'Ctrl+B' with Visual Studio.
 *
 * @section Usage_description Usage description
 *          - Input files are stored in folder '/data_in/two-dim/name_of_test_example'.
 *          - Input files may be produced by MATLAB/Octave script 'value_start.m'.
 *          - Description of configuration file 'config.txt' refers to 'doc/config.csv'.
 *          - Run program:
 *            - Linux/Unix: Run 'hydrocode.sh' command on the terminal. \n
 *                          The details are as follows: \n
 *                          Run 'hydrocode.out name_of_test_example name_of_numeric_result dimension order[_scheme]
 *                               coordinate config[n]=(double)C' command on the terminal. \n
 *                          e.g. 'hydrocode.out GRP_Book/6_1 GRP_Book/6_1 1 2[_GRP] EUL 5=100' (second-order Eulerian GRP scheme).
 *                          - dim: Dimension of test example (= 2).
 *                          - order: Order of numerical scheme (= 1 or 2).
 *                          - scheme: Scheme name (= Riemann_exact/Godunov, GRP or …)
 *                          - coordinate: Eulerian coordinate framework (= EUL).
 *            - Windows: Run 'hydrocode.bat' command on the terminal. \n
 *                       The details are as follows: \n
 *                       Run 'hydrocode.exe name_of_test_example name_of_numeric_result 2 order[_scheme] 
 *                            coordinate n=C' command on the terminal. \n
 *                       [Debug] Project -> Properties -> Configuration Properties -> Debugging \n
 *             <table>
 *             <tr><th> Command Arguments <td> name_of_test_example name_of_numeric_result 2 order[_scheme] coordinate n=C
 *             <tr><th> Working Directory <td> hydrocode_2D
 *             </table>
 *                       [Run] Project -> Properties -> Configuration Properties -> Linker -> System \n
 *             <table>
 *             <tr><th> Subsystem <td> (/SUBSYSTEM:CONSOLE)
 *             </table>
 * 
 *          - Output files can be found in folder '/data_out/two-dim/'.
 *          - Output files may be visualized by MATLAB/Octave script 'value_plot.m'.
 */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../include/var_struc.h"
#include "../include/file_io.h"
#include "../include/finite_volume.h"


double config[N_CONF]; //!< Initial configuration data array.

/**
 * @brief N memory allocations to the initial fluid variable 'v' in the structural body cell_var_stru.
 */
#define CV_INIT_MEM(v, N)						\
    do {								\
    for(k = 0; k < N; ++k)						\
	{								\
	    CV[k].v = (double **)malloc(n_x * sizeof(double *));	\
	    if(CV[k].v == NULL)						\
		{							\
		    printf("NOT enough memory! CV[%d].%s\n", k, #v);	\
		    retval = 5;						\
		    goto return_NULL;					\
		}							\
	    for(j = 0; j < n_x; ++j)					\
		{							\
		    CV[k].v[j] = (double *)malloc(n_y * sizeof(double)); \
		    if(CV[k].v[j] == NULL)				\
			{						\
			    printf("NOT enough memory! CV[%d].%s[%d]\n", k, #v, j); \
			    retval = 5;					\
			    goto return_NULL;				\
			}						\
		}							\
	}								\
    } while (0)

/**
 * @brief This is the main function which constructs the
 *        main structure of the Eulerian hydrocode.
 * @param[in] argc: ARGument counter.
 * @param[in] argv: ARGument values.
 *            - argv[1]: Folder name of test example (input path).
 *            - argv[2]: Folder name of numerical results (output path).
 *            - argv[3]: Dimensionality (= 2).
 *            - argv[4]: Order of numerical scheme[_scheme name] (= 1[_Riemann_exact] or 2[_GRP]).
 *            - argv[5]: Eulerian coordinate framework (= EUL).
 *            - argv[6,7,…]: Configuration supplement config[n]=(double)C (= n=C).
 * @return Program exit status code.
 */
int main(int argc, char *argv[])
{
    printf("\n");
    int k, j, retval = 0;
    for (k = 0; k < argc; k++)
	printf("%s ", argv[k]);
    printf("\n");
    printf("TEST:\n  %s\n", argv[1]);
    printf("Test Beginning: ARGuments Counter = %d.\n", argc);
    
    int k, i, j, retval = 0;
    // Initialize configuration data array
    for(k = 1; k < N_CONF; k++)
        config[k] = INFINITY;

    // Set dimension.
    int dim;
    dim = atoi(argv[3]);
    if (dim != 2)
	{
	    printf("No appropriate dimension was entered!\n");
	    return 4;
	}
    config[0] = (double)dim;

    printf("Configurating:\n");
    char * endptr;
    double conf_tmp;
    for (k = 6; k < argc; k++)
	{
	    errno = 0;
	    j = strtoul(argv[k], &endptr, 10);
	    if (errno != ERANGE && *endptr == '=')
		{							
		    endptr++;
		    errno = 0;
		    conf_tmp = strtod(endptr, &endptr);
		    if (errno != ERANGE && *endptr == '\0')
			{
			    config[j] = conf_tmp;
			    printf("%3d-th configuration: %g (ARGument)\n", j, conf_tmp);
			}
		    else
			{
			    printf("Configuration error in ARGument variable %d! ERROR after '='!\n", k);
			    return 4;
			}
		}
	    else
		{
		    printf("Configuration error in ARGument variable %d! ERROR before '='!\n", k);
		    return 4;
		}
	}
	
  // Set order and scheme.
  int order; // 1, 2
  char * scheme; // Riemann_exact(Godunov), GRP
  printf("Order[_Scheme]: %s\n",argv[4]);
  errno = 0;
  order = strtoul(argv[4], &scheme, 10);
  if (*scheme == '_')
      scheme++;
  else if (*scheme != '\0' || errno == ERANGE)
      {
	  printf("No order or Wrog scheme!\n");
	  return 4;
      }
  config[9] = (double)order;

    /* 
     * We read the initial data files.
     * The function initialize return a point pointing to the position
     * of a block of memory consisting (m+1) variables of type double.
     * The value of first array element of these variables is m.
     * The following m variables are the initial value.
     */
    struct flu_var FV0 = _2D_initialize(argv[1]); // Structural body of initial data array pointer.
    /* 
     * m is the number of initial value as well as the number of grids.
     * As m is frequently use to represent the number of grids,
     * we do not use the name such as num_grid here to correspond to
     * notation in the math theory.
     */
  int n_x = (int)FV0.RHO[1], n_y = (int)FV0.RHO[0];
  double h_x = config[10], h_y = config[11], gamma = config[6];
  // The number of times steps of the fluid data stored for plotting.
  int N = 2; // (int)(config[5]) + 1;
  
  // Structural body of fluid variables in computational cells array pointer.
  struct cell_var_stru * CV = malloc(N * sizeof(struct cell_var_stru));
  if(CV == NULL)
      {
	  printf("NOT enough memory! Cell Variables\n");
	  retval = 5;
	  goto return_NULL;
      }
  double ** X = NULL, ** Y = NULL;
  double * cpu_time = malloc(N * sizeof(double));
  if(cpu_time == NULL)
      {
	  printf("NOT enough memory! CPU_time\n");
	  retval = 5;
	  goto return_NULL;
      }
  // Initialize arrays of fluid variables in cells.
  CV_INIT_MEM(RHO, N);
  CV_INIT_MEM(U, N);
  CV_INIT_MEM(V, N);
  CV_INIT_MEM(P, N);
  CV_INIT_MEM(E, N);
  X = (double **)malloc((n_x+1) * sizeof(double *));
  Y = (double **)malloc((n_x+1) * sizeof(double *));
  if(X == NULL || Y == NULL)
      {
	  printf("NOT enough memory! X or Y\n");
	  retval = 5;
	  goto return_NULL;
      }
  for(j = 0; j <= n_x; ++j)
  {
    X[j] = (double *)malloc((n_y+1) * sizeof(double));
    Y[j] = (double *)malloc((n_y+1) * sizeof(double));
    if(X[j] == NULL || Y[j] == NULL)
    {
      printf("NOT enough memory! X[%d] or Y[%d]\n", j, j);
      retval = 5;
      goto return_NULL;
    }
  }
  // Initialize the values of energy in computational cells and (x,y)-coordinate of the cell interfaces.
  for(j = 0; j <= n_x; ++j)
      for(i = 0; i <= n_y; ++i)	
	  {
	      X[j][i] = i * h_y;
	      Y[j][i] = j * h_x;
	  }
  for(j = 0; j < n_x; ++j)
      for(i = 0; i < n_y; ++i)	
	  {
	      CV[0].RHO[j][i] = FV0.RHO[i*n_x + j + 2];
	      CV[0].U[j][i]   = FV0.U[i*n_x + j + 2];
	      CV[0].V[j][i]   = FV0.V[i*n_x + j + 2];
	      CV[0].P[j][i]   = FV0.P[i*n_x + j + 2];
	      CV[0].E[j][i]   = 0.5*CV[0].U[j][i]*CV[0].U[j][i] + CV[0].P[j][i]/(gamma - 1.0)/CV[0].RHO[j][i];
	      CV[0].E[j][i]  += 0.5*CV[0].V[j][i]*CV[0].V[j][i];
	  }
  free(FV0.RHO);
  free(FV0.U);
  free(FV0.V);
  free(FV0.P);
  FV0.RHO = NULL;
  FV0.U = NULL;
  FV0.V = NULL;
  FV0.P = NULL;

  if (strcmp(argv[5],"EUL") == 0) // Use GRP/Godunov scheme to solve it on Eulerian coordinate.
      {
	  config[8] = (double)0;
	  switch(order)
	      {
	      case 1:
		  // Godunov_solver_2D_EUL_source(n_x, n_y, CV, cpu_time);
		  config[41] = 0.0; // alpha = 0.0
		  GRP_solver_2D_EUL_source(n_x, n_y, CV, cpu_time);
		  break;
	      case 2:
		  GRP_solver_2D_EUL_source(n_x, n_y, CV, cpu_time);
		  break;
	      default:
		  printf("NOT appropriate order of the scheme! The order is %d.\n", order);
		  retval = 4;
		  goto return_NULL;
	      }
      }
  else
      {
	  printf("NOT appropriate coordinate framework! The framework is %s.\n", argv[5]);
	  retval = 4;
	  goto return_NULL;
      }

  // Write the final data down.
  _2D_file_write(n_x, n_y, N, CV, X, Y, cpu_time, argv[2]);

 return_NULL:
  for(k = 0; k < N; ++k)
  {
    for(j = 0; j < n_x; ++j)
	{
            free(CV[k].RHO[j]);
            free(CV[k].U[j]);
            free(CV[k].V[j]);
            free(CV[k].P[j]);
            free(CV[k].E[j]);
            CV[k].RHO[j] = NULL;
            CV[k].U[j]   = NULL;
            CV[k].V[j]   = NULL;
            CV[k].P[j]   = NULL;
            CV[k].E[j]   = NULL;
	}
    free(CV[k].RHO);
    free(CV[k].U);
    free(CV[k].V);
    free(CV[k].P);
    free(CV[k].E);
    CV[k].RHO = NULL;
    CV[k].U   = NULL;
    CV[k].V   = NULL;
    CV[k].P   = NULL;
    CV[k].E   = NULL;
  }
  free(CV);
  CV = NULL;
  for(j = 0; j <= n_x; ++j)
  {
      free(X[j]);
      free(Y[j]);
      X[j] = NULL;
      Y[j] = NULL;
  }
  free(X);
  free(Y);
  X = NULL;
  Y = NULL; 
  free(cpu_time);
  cpu_time = NULL;
  
  return retval;
}
