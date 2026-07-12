#ifndef HEAT_SERIAL_H
#define HEAT_SERIAL_H

// Avanza UNA iteración de Jacobi (esquema de 5 puntos) sobre los puntos
// interiores. Lee de u, escribe en u_new (los bordes no se tocan).
// Devuelve el residuo L2 de la iteración:
//   sqrt( suma((u_new - u)^2) / (nx*ny) )
// r = alpha*dt/h^2 (ya validado contra CFL en parse_args).
double heat_step_serial(const double* u, double* u_new, int nx, int ny, double r);

#endif // HEAT_SERIAL_H
