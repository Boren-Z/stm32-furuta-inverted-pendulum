#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "LED.h"
#include "Timer.h"
#include "Key.h"
#include "RP.h"
#include "Motor.h"
#include "Encoder.h"
#include "Serial.h"
#include "AD.h"
#include "PID.h"


// Recalibrate all of these for your own hardware.
#define CENTER_ANGLE		0			// ADC reading at the balance point
#define CENTER_RANGE		0			// Half-width of the "catchable" window around the upright angle
#define START_PWM			0			// Swing-up kick strength (PWM)
#define START_TIME			0			// Swing-up kick duration (1 ms ticks)
#define POS_STEP			0			// Position-target nudge per key press
#define POS_LIMIT			0			// Position-target travel limit



/*
	Inverted-pendulum skeleton
	1. Initialise every low-level driver
	2. Once Timer_Init has run, the timer ISR must be provided
*/

/*
	Two requirements:
	1. A key starts / stops the balancing routine manually
	2. While running, if the pendulum falls out of range, stop immediately (safety)
*/

uint8_t KeyNum;

/*
	RunState map (every transition happens inside this ISR)

	   0          stopped, motor forced to 0
	   1          "detecting": watch the free swing and decide which way to kick
	   21..24     one left  kick  (push +, wait, push -, wait), then back to 1
	   31..34     one right kick  (push -, wait, push +, wait), then back to 1
	   4          balancing: the cascaded PID holds the arm upright

	Manual : key 1 toggles 0 <-> 21 (start = begin swinging up).
	Auto   : 1 -> 21/31 -> ... -> 1 -> ...  repeats until the tip enters the
	         centre window, then 1 -> 4.  Leaving the window in state 4 -> 0.
*/

uint8_t RunState;					// Pendulum run-state machine (driven by the TIM1 ISR)

uint16_t Angle;
int16_t Speed, Location;


/* Inner loop: holds the arm at the upright angle, output drives the motor PWM */
PID_t AnglePID = {
	.Target = CENTER_ANGLE,

	.Kp = 0,			// gains redacted -- Tune for your own device
	.Ki = 0,
	.Kd = 0,
	.OutMax = 100,
	.OutMin = -100,
};

/* Outer loop: regulates cart position, output biases AnglePID.Target */
PID_t LocationPID = {
	.Target = 0,

	.Kp = 0,			// gains redacted -- Tune for your own device
	.Ki = 0,
	.Kd = 0,
	.OutMax = 100,
	.OutMin = -100,
};


/**
  * @brief  Bring up all drivers, then poll keys: key 1 arms/disarms the routine,
  *         keys 2/3 shift the target position. All real-time work runs in the TIM1 ISR.
  * @retval Never returns
  */
int main(void)
{
	LED_Init();
	Key_Init();
	RP_Init();
	Motor_Init();
	Encoder_Init();
	Serial_Init();
	AD_Init();

	Timer_Init();

	while (1)
	{
		KeyNum = Key_GetNum();
		if (KeyNum == 1)						// key 1: start / stop
		{
			if (RunState == 0) 	RunState = 21; // arm: swing up first, leave the dead zone
			else 				RunState = 0;  // any other state -> stop
		}

		if (KeyNum == 2)						// key 2: nudge target position right (clamped)
		{
			LocationPID.Target += POS_STEP;
			if (LocationPID.Target > POS_LIMIT)  LocationPID.Target = POS_LIMIT;
		}

		if (KeyNum == 3)						// key 3: nudge target position left (clamped)
		{
			LocationPID.Target -= POS_STEP;
			if (LocationPID.Target < -POS_LIMIT) LocationPID.Target = -POS_LIMIT;
		}

		if (RunState) 	LED_ON();
		else 			LED_OFF();
	}
}



/**
  * @brief  1 ms control tick. Runs the key scan, samples angle/speed/position,
  *         drives the swing-up state machine, and (in the balancing state) the
  *         cascaded positional PID: inner AnglePID every 5 ms, outer LocationPID
  *         every 50 ms.
  * @retval None
  */
void TIM1_UP_IRQHandler(void)
{
	static uint16_t Count0, Count1, Count2, CountTime;
	static uint16_t Angle0, Angle1, Angle2;		// 3-sample shift register: Angle0 newest, Angle2 oldest

	if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
	{
		Key_Tick();

		Angle = AD_GetValue();			// pendulum tip angle (ADC)
		Speed = Encoder_Get();			// arm speed 
		Location += Speed;				// integrate speed = arm position

		/*
			Swing-up strategy (no model needed): Give energy in like pushing a swing.
			
		*/
		if (RunState == 0)		// stopped
		{
			Motor_SetPWM(0);	// no PID running
		}
		else if (RunState == 1)	
		{
			Count0++;
			if (Count0 >= 40)			// sample the swing every 40 ms (40 * 1 ms tick)
			{
				Count0 = 0;
				// Keeps the last three samples.
				Angle2 = Angle1;		// push the newest sample in, drop the oldest
				Angle1 = Angle0;
				Angle0 = Angle;

				if (   Angle0 > CENTER_ANGLE+CENTER_RANGE
					&& Angle1 > CENTER_ANGLE+CENTER_RANGE
					&& Angle2 > CENTER_ANGLE+CENTER_RANGE
					&& Angle1 < Angle0
					&& Angle1 < Angle2)// turning point on the right (middle sample is the extreme) -> kick left
				{
					RunState = 21;
				}

				if (   Angle0 < CENTER_ANGLE+CENTER_RANGE
					&& Angle1 < CENTER_ANGLE+CENTER_RANGE
					&& Angle2 < CENTER_ANGLE+CENTER_RANGE
					&& Angle1 > Angle0
					&& Angle1 > Angle2)// turning point on the left (middle sample is the extreme) -> kick right
				{
					RunState = 31;
				}
				if (   Angle0 > CENTER_ANGLE-CENTER_RANGE
					&& Angle0 < CENTER_ANGLE+CENTER_RANGE
					&& Angle1 > CENTER_ANGLE-CENTER_RANGE
					&& Angle1 < CENTER_ANGLE+CENTER_RANGE
					&& Angle2 > CENTER_ANGLE-CENTER_RANGE
					&& Angle2 < CENTER_ANGLE+CENTER_RANGE)// three samples inside the window
				{
					AnglePID.ErrorInt = 0;		// clear both integrators
					LocationPID.ErrorInt = 0;
					Location = 0;				// and make the position origin
					RunState = 4;
				}
			}
		}
		// The best instant to push toward the centre
		// One left kick: push +, hold START_TIME ms, push - to brake, hold, then re-detect
		else if (RunState == 21)		// left kick, step 1: drive +
		{
			Motor_SetPWM(START_PWM);
			CountTime = START_TIME;		// load the hold-time counter
			RunState = 22;
		}
		else if (RunState == 22)		// left, step 2: hold
		{
			CountTime--;
			if (CountTime == 0)
			{
				RunState = 23;
			}
		}
		else if (RunState == 23)		// left, step 3: reverse drive to brake
		{
			Motor_SetPWM(-START_PWM);
			CountTime = START_TIME;
			RunState = 24;
		}
		else if (RunState == 24)		// left, step 4: hold, then back to detecting
		{
			CountTime--;
			if (CountTime == 0)
			{
				Motor_SetPWM(0);		// motor twitches, so zero it
				RunState = 1;
			}
		}
		// The best instant to push toward the centre
		// One right kick: mirror of the left sequence (push -, hold, push +, hold)
		else if (RunState == 31)// right kick, step 1: drive -
		{
			Motor_SetPWM(-START_PWM);
			CountTime = START_TIME;
			RunState = 32;
		}
		else if (RunState == 32)		// right, step 2: hold
		{
			CountTime--;
			if (CountTime == 0)
			{
				RunState = 33;
			}
		}
		else if (RunState == 33)		// right, step 3: reverse drive to brake
		{
			Motor_SetPWM(START_PWM);
			CountTime = START_TIME;
			RunState = 34;
		}
		else if (RunState == 34)// right kick, step 4: hold, then back to detecting
		{
			CountTime--;
			if (CountTime == 0)
			{
				Motor_SetPWM(0);// motor twitches, zero
				RunState = 1;
			}
		}


		/* Successive kicks add a little amplitude every swing until the tip passes over the top,
		   where control is handed to the PID.
		*/
		else if (RunState == 4)// balancing: cascaded PID 
		{
			if (!(Angle > CENTER_ANGLE - CENTER_RANGE
				&& Angle < CENTER_ANGLE + CENTER_RANGE))// safety: tip fell out of the window
			{
				RunState = 0;
			}

			Count1 ++;
			if (Count1 >= 5)// inner loop: angle PID every 5 ms
			{
				Count1 = 0;
				AnglePID.Actual = Angle;			// feedback = tip angle
				PID_Update(&AnglePID);
				Motor_SetPWM(AnglePID.Out);			// PID output -> motor
			}

			// cascade: outer position PID trims the angle setpoint so the arm drifts back to Target
			Count2 ++;
			if (Count2 >= 50)// outer loop: position PID every 50 ms (10x slower than the inner loop)
			{
				Count2 = 0;
				LocationPID.Actual = Location;						// feedback = arm position
				PID_Update(&LocationPID);
				AnglePID.Target = CENTER_ANGLE - LocationPID.Out;	// lean slightly to chase the position
			}
		}
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	}
}
