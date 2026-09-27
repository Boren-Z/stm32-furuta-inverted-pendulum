# STM32F103 Furuta Inverted Pendulum
<p align="center">
<img width="575" height="689" alt="pendulum demp" src="https://github.com/user-attachments/assets/eb8f5764-f26e-46e5-99f7-885743f10300" />
</p>

A rotary (Furuta-style) inverted pendulum built on the STM32F103C8T6 using the Standard Peripheral Library. A motor drives a horizontal arm, with a pendulum hanging perpendicular from the arm's far end (potentiometer feedback for the angle signal).

This was an earlier PID investigation project, built before the [STM32 self-balancing robot](https://github.com/Boren-Z/stm32-balance-car-ola), where the motor control and cascade-PID fundamentals used in that later project were first worked out.

---

## Hardware

- **MCU**: STM32F103C8T6
- **Arm angle feedback**: quadrature encoder, TIM3 hardware encoder interface
- **Pendulum tip angle feedback**: potentiometer, ADC1 single channel, software-triggered polling
- **Motor driver**: TB6612FNG, GPIO direction pins + TIM2 PWM

---

## Control

Two PID loops in cascade:

- **Inner loop** (5 ms): holds the pendulum tip at the upright angle; output drives the motor PWM directly
- **Outer loop** (50 ms): regulates the arm's position; its output biases the inner loop's target angle so the arm slowly drifts back toward the target position

Both loops run inside a single 1 ms hardware timer interrupt (TIM1), with the 5 ms / 50 ms periods derived by software counters inside the ISR rather than separate timers. Driving PID off a fixed interrupt period instead of polling in the main loop.

### Swing-up

The arm has no gearbox or feedback stiff enough to snap the pendulum upright from rest, so it must be swung up first, like pumping a swing. A detection state samples the tip angle every 40 ms and keeps the last three samples; three samples on the same side, with the middle one at the extreme, means the pendulum just hit a turning point and is momentarily still, and that is the best instant to kick it toward the centre. Each kick is a short, fixed-duration PWM burst in one direction followed by a reverse burst to brake. Kicks repeat, gaining a little amplitude each swing, until the tip passes over the top and settles inside a window around the upright angle, at which point control hands off to the cascade PID.

---

## Build

Built with **Keil MDK5**, using the STM32 Standard Peripheral Library (not HAL).

1. Open `Project.uvprojx` in Keil5
2. Build (F7)
3. Flash via ST-Link (SWD)

> Tuning constants (`CENTER_ANGLE`, `START_PWM`, PID gains, etc.) in `main.c` are hardware-specific and have been zeroed out.

---

## Repository Structure

```
├── Hardware/    LED, key, motor, PWM, encoder, ADC (angle + tuning-pot) drivers
├── System/      Delay (SysTick), 1ms scheduling timer
├── User/        main.c, PID
├── Start/       Startup files, CMSIS core
├── Library/     ST Standard Peripheral Library
└── Project.uvprojx
```
