#include "stm32f10x.h"
#include "PID.h"

/*
	Design note: positional PID vs incremental PID

	Positional (Current):
	    Out = Kp*e + Ki*(sum e) + Kd*(e - e_prev)
		Pros:
	    - keeps an explicit integral accumulator (ErrorInt)
	    - output is the absolute actuator command, easy to hard-limit
	    - D term is a first difference -> less noise gain
	    Cons: 
		- integral must be guarded against wind-up / reset on mode change

	Incremental:
	    dOut = Kp*(e - e_prev) + Ki*e + Kd*(e - 2*e_prev + e_prev2);  
		Out += dOut
		Pros:
	    - no accumulator; only the last 3 errors are stored
	    - bumpless when gains change or on manual<->auto switch
	    - a single bad sample perturbs one step, not the absolute level
	    Cons:
		- output limiting/feedforward act on an integrated value, D term is a
	      second difference (noisier), suits actuators that integrate by themselves
	      (stepper rate, valve, heater duty)

	Choice for positional PID: the angle inner loop runs every few ms, needs a directly
	bounded PWM value, and relies on the D term acting on angular rate. 
	Incremental has no benefit here.
*/

/**
  * @brief  Run one positional-PID step. Call at a fixed period (the TIM1 1ms ISR),
  *         so the D term uses a constant sample interval and needs no explicit dt.
  * @param  p PID instance; reads Target/Actual/gains/limits, updates Error_star/ErrorInt/Out
  * @retval None
  */
void PID_Update(PID_t *p)
{
	p->Error1 = p->Error0;					// Keep last error for the derivative term
	p->Error0 = p->Target - p->Actual;		// Current error

	if (p->Ki != 0)
	{
		p->ErrorInt += p->Error0;			// Accumulate integral only when Ki is active
	}
	else
	{
		p->ErrorInt = 0;					// Ki disabled: reset so it can't wind up
	}

	p->Out = p->Kp * p->Error0 +
			 p->Ki * p->ErrorInt +
			 p->Kd * (p->Error0 - p->Error1);
/* 
	Incremental form of the same controller (needs Error2 = error two steps back):
	   dOut  = p->Kp * (p->Error0 - p->Error1)
	         + p->Ki *  p->Error0
	         + p->Kd * (p->Error0 - 2*p->Error1 + p->Error2);
	   p->Out += dOut;   // no ErrorInt term; only the last three errors are kept 
*/

	if (p->Out > p->OutMax){p->Out =  p->OutMax;}	// Clamp output to the configured range
	if (p->Out < p->OutMin){p->Out =  p->OutMin;}
}
