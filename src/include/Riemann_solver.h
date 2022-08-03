/**
 * @file Riemann_solver.h
 * @brief This file is the header file of several Riemann solvers and GRP solvers.
 * @details This header file declares functions in files 'Riemann_solver_exact.c',
 *          'Riemann_solver_exact_Toro.c' and 'linear_GRP_solver_LAG.c'.
 */

double Riemann_solver_exact(double * U_star, double * P_star, const double gamma,
			    const double u_L, const double u_R, const double p_L, const double p_R,
			    const double c_L, const double c_R, int * CRW,
			    const double eps, const double tol, const int N);

double Riemann_solver_exact_Toro(double * U_star, double * P_star, const double gamma,
				 const double U_l, const double U_r, const double P_l, const double P_r,
				 const double c_l, const double c_r, int * CRW,
				 const double eps, const double tol, const int N);


void linear_GRP_solver_LAG
(double * direvative, double * mid,
 double rho_L, double rho_R, double s_rho_L, double s_rho_R,
 double   u_L, double   u_R, double   s_u_L, double   s_u_R,
 double   p_L, double   p_R, double   s_p_L, double   s_p_R,
 double gamma, double eps);
