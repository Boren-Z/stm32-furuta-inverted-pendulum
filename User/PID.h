#ifndef __PID_H
#define __PID_H


typedef struct {
	float Target;		// Setpoint
	float Actual;		// Feedback
	float Out;			

	float Kp;			
	float Ki;			
	float Kd;			

	float Error0;		// Current error (Target - Actual)
	float Error1;		// Previous error, for the derivative term
	float ErrorInt;		// Accumulated error (integral)

	float OutMax;		// Upper output limit
	float OutMin;		// Lower output limit
} PID_t;



/* Positional PID; call at a fixed period so dt is constant */
void PID_Update(PID_t *p);
	



#endif
