/**
 * @file  GRP_solver_EUL_source.c
 * @brief This is an Eulerian GRP scheme to solve 1-D Euler equations.
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#include "../include/var_struc.h"
#include "../include/Riemann_solver.h"
#include "../include/tools.h"


/**
 * @brief This function use GRP scheme to solve 1-D Euler
 *        equations of motion on Eulerian coordinate.
 * @param[in]  m:        Number of the grids.
 * @param[in,out] CV:    Structural body of cell variable data.
 * @param[out] cpu_time: Array of the CPU time recording.
 */
void GRP_solver_EUL_source
(const int m, struct cell_var_stru CV, double * cpu_time)
{
    double ** RHO = CV.RHO;
    double ** U   = CV.U;
    double ** P   = CV.P;
    double ** E   = CV.E;
    /* 
     * j is a frequently used index for spatial variables.
     * k is a frequently used index for the time step.
     */
  int j, k;

  clock_t tic, toc;
  double cpu_time_sum = 0.0;

  double const t_all = config[1];        // the total time
  double const eps   = config[4];        // the largest value could be seen as zero
  int    const N     = (int)(config[5]); // the maximum number of time steps
  double const gamma = config[6];        // the constant of the perfect gas
  double const CFL   = config[7];        // the CFL number
  double const h     = config[10];       // the length of the initial spatial grids
  double       tau   = config[16];       // the length of the time step
  int    const bound = (int)(config[17]);// the boundary condition
  double const alpha = config[41];       // the paramater in slope limiters.
  printf("alpha = %g\n",alpha);

  _Bool find_bound = false;
  
  double Mom, Ene;
  double u_L, p_L, rho_L;
  double u_R, p_R, rho_R;
  double c_L, c_R; // the speeds of sound

  double s_u_L, s_p_L, s_rho_L; // spatial derivatives in coordinate x (slopes)
  double s_u_R, s_p_R, s_rho_R;
  /*
   * dire: the temporal derivative of fluid variables.
   *       \frac{\partial [rho, u, p]}{\partial t}
   * mid:  the Riemann solutions.
   *       [rho_star, u_star, p_star]
   */
  double dire[3], mid[3];

  // the slopes of variable values
  double * s_rho = calloc(m, sizeof(double));
  double * s_u   = calloc(m, sizeof(double));
  double * s_p   = calloc(m, sizeof(double));
  // the variable values at (x_{j-1/2}, t_{n+1}).
  double * U_next   = malloc((m+1) * sizeof(double));
  double * P_next   = malloc((m+1) * sizeof(double));
  double * RHO_next = malloc((m+1) * sizeof(double));
  // the temporal derivatives at (x_{j-1/2}, t_{n}).
  double * U_t   = malloc((m+1) * sizeof(double));
  double * P_t   = malloc((m+1) * sizeof(double));
  double * RHO_t = malloc((m+1) * sizeof(double));
  // the numerical flux at (x_{j-1/2}, t_{n}).
  double * F1 = malloc((m+1) * sizeof(double));
  double * F2 = malloc((m+1) * sizeof(double));
  double * F3 = malloc((m+1) * sizeof(double));
  if(s_rho == NULL || s_u == NULL || s_p == NULL)
      {
	  printf("NOT enough memory! Slope\n");
	  goto return_NULL;
      }
  if(U_next == NULL || P_next == NULL || RHO_next == NULL)
      {
	  printf("NOT enough memory! Variables_next\n");
	  goto return_NULL;
      }
  if(U_t == NULL || P_t == NULL || RHO_t == NULL)
      {
	  printf("NOT enough memory! Temproal derivative\n");
	  goto return_NULL;
      }
  if(F1 == NULL || F2 == NULL || F3 == NULL)
      {
	  printf("NOT enough memory! Flux\n");
	  goto return_NULL;
      }

  double nu;  // nu = tau/h
  double h_S_max; // h/S_max, S_max is the maximum wave speed
  double time_c = 0.0; // the current time
  int n = 1; // the number of times storing plotting data

  double UL, PL, RHOL, SUL = 0.0, SPL = 0.0, SRHOL = 0.0; // Left  boundary condition
  double UR, PR, RHOR, SUR = 0.0, SPR = 0.0, SRHOR = 0.0; // Right boundary condition

//-----------------------THE MAIN LOOP--------------------------------
  for(k = 1; k <= N; ++k)
  {
      h_S_max = INFINITY; // h/S_max = INFINITY
      tic = clock();

      switch (bound)
	  {
	  case -1: // initial boudary conditions
	      if(find_bound)
		  break;
	      else
		  printf("Initial boudary conditions.\n");		  
	      find_bound = true;
	      UL   =   U[0][0]; UR   =   U[0][m-1];
	      PL   =   P[0][0]; PR   =   P[0][m-1];
	      RHOL = RHO[0][0]; RHOR = RHO[0][m-1];
	      break;
	  case -2: // reflective boundary conditions
	      if(!find_bound)
		  printf("Reflective boudary conditions.\n");
	      find_bound = true;
	      UL   = - U[n-1][0]; UR   = - U[n-1][m-1];
	      PL   =   P[n-1][0]; PR   =   P[n-1][m-1];
	      RHOL = RHO[n-1][0]; RHOR = RHO[n-1][m-1];
	      break;
	  case -4: // free boundary conditions
	      if(!find_bound)
		  printf("Free boudary conditions.\n");
	      find_bound = true;
	      UL   =   U[n-1][0]; UR   =   U[n-1][m-1];
	      PL   =   P[n-1][0]; PR   =   P[n-1][m-1];
	      RHOL = RHO[n-1][0]; RHOR = RHO[n-1][m-1];
	      break;
	  case -5: // periodic boundary conditions
	      if(!find_bound)
		  printf("Periodic boudary conditions.\n");
	      find_bound = true;
	      UL   =   U[n-1][m-1]; UR   =   U[n-1][0];
	      PL   =   P[n-1][m-1]; PR   =   P[n-1][0];
	      RHOL = RHO[n-1][m-1]; RHOR = RHO[n-1][0];
	      break;
	  case -24: // reflective + free boundary conditions
	      if(!find_bound)
		  printf("Reflective + Free boudary conditions.\n");
	      find_bound = true;
	      UL   = - U[n-1][0]; UR   =   U[n-1][m-1];
	      PL   =   P[n-1][0]; PR   =   P[n-1][m-1];
	      RHOL = RHO[n-1][0]; RHOR = RHO[n-1][m-1];
	      break;
	  default:
	      printf("No suitable boundary coditions!\n");
	      goto return_NULL;
	  }

//=================Initialize slopes=====================
      for(j = 0; j < m; ++j) // Reconstruct slopes
	  { /*
	     *  j-1          j          j+1
	     * j-1/2  j-1  j+1/2   j   j+3/2  j+1
	     *   o-----X-----o-----X-----o-----X--...
	     */
	      if(j)
		  {
		      s_u_L   = (U[n-1][j]   -   U[n-1][j-1]) / h;
		      s_p_L   = (P[n-1][j]   -   P[n-1][j-1]) / h;
		      s_rho_L = (RHO[n-1][j] - RHO[n-1][j-1]) / h;
		  }
	      else
		  {
		      s_u_L   = (U[n-1][j]   -   UL) / h;
		      s_p_L   = (P[n-1][j]   -   PL) / h;
		      s_rho_L = (RHO[n-1][j] - RHOL) / h;
		  }
	      if(j < m-1)
		  {
		      s_u_R   = (U[n-1][j+1]   -   U[n-1][j]) / h;
		      s_p_R   = (P[n-1][j+1]   -   P[n-1][j]) / h;
		      s_rho_R = (RHO[n-1][j+1] - RHO[n-1][j]) / h;
		  }
	      else
		  {
		      s_u_R   = (UR   -   U[n-1][j]) / h;
		      s_p_R   = (PR   -   P[n-1][j]) / h;
		      s_rho_R = (RHOR - RHO[n-1][j]) / h;
		  }
	      if (k == 1)
		  {
		      s_u[j]   = minmod2(s_u_L,   s_u_R);
		      s_p[j]   = minmod2(s_p_L,   s_p_R);
		      s_rho[j] = minmod2(s_rho_L, s_rho_R);
		  }
	      else
		  {
		      s_u[j]   = minmod3(alpha*s_u_L,   alpha*s_u_R,   s_u[j]);
		      s_p[j]   = minmod3(alpha*s_p_L,   alpha*s_p_R,   s_p[j]);
		      s_rho[j] = minmod3(alpha*s_rho_L, alpha*s_rho_R, s_rho[j]);
		  }
	  }
      switch(bound)
	  {
	  case -2: // reflective boundary conditions
	      SUL = - s_u[0]; SUR = - s_u[m-1];
	      break;
	  case -5: // periodic boundary conditions
	      SUL   =   s_u[m-1]; SUR   =   s_u[0];
	      SPL   =   s_p[m-1]; SPR   =   s_p[0];
	      SRHOL = s_rho[m-1]; SRHOR = s_rho[0];
	      break;
	  case -24: // reflective + free boundary conditions
	      SUL = - s_u[0];
	      break;
	  }
      
      for(j = 0; j <= m; ++j)
	  { /*
	     *  j-1          j          j+1
	     * j-1/2  j-1  j+1/2   j   j+3/2  j+1
	     *   o-----X-----o-----X-----o-----X--...
	     */
	      if(j) // Initialize the initial values.
		  {
		      rho_L = RHO[n-1][j-1] + 0.5*h*s_rho[j-1];
		      u_L   =   U[n-1][j-1] + 0.5*h*s_u[j-1];
		      p_L   =   P[n-1][j-1] + 0.5*h*s_p[j-1];
		  }
	      else
		  {
		      rho_L = RHOL + 0.5*h*SRHOL;
		      u_L   =   UL + 0.5*h*SUL;
		      p_L   =   PL + 0.5*h*SPL;
		  }
	      if(j < m)
		  {
		      rho_R = RHO[n-1][j] - 0.5*h*s_rho[j];
		      u_R   =   U[n-1][j] - 0.5*h*s_u[j];
		      p_R   =   P[n-1][j] - 0.5*h*s_p[j];
		  }
	      else
		  {
		      rho_R = RHOR + 0.5*h*SRHOR;
		      u_R   =   UR + 0.5*h*SUR;
		      p_R   =   PR + 0.5*h*SPR;
		  }
	      if(p_L < eps || p_R < eps || rho_L < eps || rho_R < eps)
		  {
		      printf("<0.0 error on [%d, %d] (t_n, x) - Reconstruction\n", k, j);
		      goto return_NULL;
		  }
	      if(!isfinite(p_L)|| !isfinite(p_R)|| !isfinite(u_L)|| !isfinite(u_R)|| !isfinite(rho_L)|| !isfinite(rho_R))
		  {
		      printf("NAN or INFinite error on [%d, %d] (t_n, x) - Reconstruction\n", k, j); 
		      goto return_NULL;
		  }

	      c_L = sqrt(gamma * p_L / rho_L);
	      c_R = sqrt(gamma * p_R / rho_R);
	      h_S_max = fmin(h_S_max, h/(fabs(u_L)+fabs(c_L)));
	      h_S_max = fmin(h_S_max, h/(fabs(u_R)+fabs(c_R)));

	      if(j) //calculate the material derivatives
		  {
		      s_u_L   =   s_u[j-1];
		      s_p_L   =   s_p[j-1];
		      s_rho_L = s_rho[j-1];
		  }
	      else
		  {
		      s_rho_L = SRHOL;
		      s_u_L   =   SUL;
		      s_p_L   =   SPL;
		  }
	      if(j < m)
		  {
		      s_u_R   =   s_u[j];
		      s_p_R   =   s_p[j];
		      s_rho_R = s_rho[j];
		  }
	      else
		  {
		      s_rho_R = SRHOR;
		      s_u_R   =   SUR;
		      s_p_R   =   SPR;
		  }

//========================Solve GRP========================
	      linear_GRP_solver_Edir(dire, mid, rho_L, rho_R, s_rho_L, s_rho_R, u_L, u_R, s_u_L, s_u_R, p_L, p_R, s_p_L, s_p_R, gamma, eps);

	      if(mid[2] < eps)
		  {
		      printf("<0.0 error on [%d, %d] (t_n, x) - STAR\n", k, j);
		      time_c = t_all;
		  }
	      if(!isfinite(mid[1])|| !isfinite(mid[2]))
		  {
		      printf("NAN or INFinite error on [%d, %d] (t_n, x) - STAR\n", k, j); 
		      time_c = t_all;
		  }

	      RHO_next[j] = mid[0];
	      U_next[j]   = mid[1];
	      P_next[j]   = mid[2];
	      RHO_t[j] = dire[0];
	      U_t[j]   = dire[1];
	      P_t[j]   = dire[2];
	  }

//====================Time step and grid fixed======================
    // If no total time, use fixed tau and time step N.
    if (isfinite(t_all) || !isfinite(config[16]) || config[16] <= 0.0)
	{
	    tau = CFL * h_S_max;
	    if ((time_c + tau) > (t_all - eps))
		tau = t_all - time_c;
	}
    nu = tau / h;
    
    for(j = 0; j <= m; ++j)
	{
	    RHO_next[j] += 0.5 * tau * RHO_t[j];;
	    U_next[j]   += 0.5 * tau * U_t[j];
	    P_next[j]   += 0.5 * tau * P_t[j];

	    F1[j] = RHO_next[j]*U_next[j];
	    F2[j] = F1[j]*U_next[j] + P_next[j];
	    F3[j] = (gamma/(gamma-1.0))*P_next[j] + 0.5*F1[j]*U_next[j];
	    F3[j] = F3[j]*U_next[j];

	    RHO_next[j] += 0.5 * tau * RHO_t[j];;
	    U_next[j]   += 0.5 * tau * U_t[j];
	    P_next[j]   += 0.5 * tau * P_t[j];
	}

//======================THE CORE ITERATION=========================(On Eulerian Coordinate)
    for(j = 0; j < m; ++j) // forward Euler
	{ /*
	   *  j-1          j          j+1
	   * j-1/2  j-1  j+1/2   j   j+3/2  j+1
	   *   o-----X-----o-----X-----o-----X--...
	   */
	    RHO[n][j] = RHO[n-1][j] - nu*(F1[j+1]-F1[j]);
	    Mom = RHO[n-1][j]*U[n-1][j] - nu*(F2[j+1]-F2[j]);
	    Ene = RHO[n-1][j]*E[n-1][j] - nu*(F3[j+1]-F3[j]);

	    U[n][j] = Mom / RHO[n][j];
	    E[n][j] = Ene / RHO[n][j];
	    P[n][j] = (Ene - 0.5*Mom*U[n][j])*(gamma-1.0);

	    if(P[n][j] < eps || RHO[n][j] < eps)
		{
		    printf("<0.0 error on [%d, %d] (t_n, x) - Update\n", k, j);
		    time_c = t_all;
		}
	    if(!isfinite(P[n][j])|| !isfinite(U[n][j])|| !isfinite(RHO[n][j]))
		{
		    printf("NAN or INFinite error on [%d, %d] (t_n, x) - Update\n", k, j); 
		    time_c = t_all;
		}
	    
//============================compute the slopes============================
	    s_u[j]   = (  U_next[j+1] -   U_next[j])/h;
	    s_p[j]   = (  P_next[j+1] -   P_next[j])/h;
	    s_rho[j] = (RHO_next[j+1] - RHO_next[j])/h;
	}

//============================Time update=======================

    toc = clock();
    cpu_time[n] = ((double)toc - (double)tic) / (double)CLOCKS_PER_SEC;;
    cpu_time_sum += cpu_time[n];

    time_c += tau;
    if (isfinite(t_all))
        DispPro(time_c*100.0/t_all, k);
    else
        DispPro(k*100.0/N, k);
    if(time_c > (t_all - eps) || isinf(time_c))
	{
	    config[5] = (double)k;
	    break;
	}

//===========================Fixed variable location=======================	
    for(j = 0; j < m; ++j)
	{
	    RHO[n-1][j] = RHO[n][j];
	    U[n-1][j]   =   U[n][j];
	    E[n-1][j]   =   E[n][j];  
	    P[n-1][j]   =   P[n][j];
	}
  }

  printf("\nTime is up at time step %d.\n", k);
  printf("The cost of CPU time for 1D-GRP Eulerian scheme for this problem is %g seconds.\n", cpu_time_sum);
//---------------------END OF THE MAIN LOOP----------------------

return_NULL:
  free(s_u);
  free(s_p);
  free(s_rho);
  s_u   = NULL;
  s_p   = NULL;
  s_rho = NULL;
  free(U_next);
  free(P_next);
  free(RHO_next);
  U_next   = NULL;
  P_next   = NULL;
  RHO_next = NULL;
  free(U_t);
  free(P_t);
  free(RHO_t);
  U_t   = NULL;
  P_t   = NULL;
  RHO_t = NULL;
  free(F1);
  free(F2);
  free(F3);
  F1 = NULL;
  F2 = NULL;
  F3 = NULL;
}
