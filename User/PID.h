#ifndef __PID_H
#define __PID_H


typedef struct {
	float Target;		// Setpoint
	float Actual;		// Measured feedback
	float Out;			// Controller output (clamped to OutMin..OutMax)

	float Kp;			// Proportional gain
	float Ki;			// Integral gain (0 disables and clears the integrator)
	float Kd;			// Derivative gain

	float Error0;		// Current error (Target - Actual)
	float Error1;		// Previous error, for the derivative term
	float ErrorInt;		// Accumulated error (integral)

	float OutMax;		// Upper output limit
	float OutMin;		// Lower output limit
} PID_t;



/* Positional PID; call at a fixed period so dt is constant */
void PID_Update(PID_t *p);
	



#endif
