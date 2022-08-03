/**
 * @file finite_difference_solver.h
 * @brief This file is the header file of Lagrangian hydrocode in finite volume framework.
 * @details This header file declares functions in files 'Godunov_solver_source.c',
 *          and 'GRP_solver_source.c'.
 */

void Godunov_solver_source
(double * config, int m, double * RHO[], double * U[], double * P[],
 double * E[], double * X[],
 double * MASS, double * RHOL, double * UL, double * PL,
 double * RHOR, double * UR, double * PR, double * cpu_time);

void GRP_solver_source
(double * config, int m, double * RHO[], double * U[], double * P[],
 double * E[], double * X[], double * MASS,
 double * RHOL, double * UL, double * PL,
 double * RHOR, double * UR, double * PR,
 double * SRHOL, double * SUL, double * SPL,
 double * SRHOR, double * SUR, double * SPR,
 double * cpu_time);
