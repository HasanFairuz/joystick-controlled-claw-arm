/*
 * Tiva Claw Arm Controller v2.2
 *
 * Diagnostic revision:
 *   - Motor/ADC initialization occurs before the TFT initialization.
 *   - No floating-point calculations are used in the control loop.
 *   - PF1, the LaunchPad red LED, blinks every 500 ms while main() runs.
 *   - The TFT shows red, green and blue startup test screens before the arm UI.
 *
 * Controls:
 *   PD0 / joystick X -> Servo A on PE4
 *   PD1 / joystick Y -> Servo B1 on PE5 and Servo B2 on PF3
 *   PF4 button       -> Servo C claw on PF2
 */
#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/sysctl.h"
#include "display.h"
#include "motor.h"

#define THETA_MIN_DEG   0
#define THETA_MAX_DEG   90
#define PHI_MIN_DEG     0
#define PHI_MAX_DEG     180

int main(void)
{
    uint16_t joystick_x;
    uint16_t joystick_y;
    int32_t theta_deg;
    int32_t phi_deg;
    uint8_t heartbeat_count = 0U;

    SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL |
                   SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

    /* Initialize motion first so it can be tested independently of the TFT. */
    Motor_Init();

    /* Solid red during TFT initialization. */
    StatusLED_Set(1U);
    Display_Init();
    StatusLED_Set(0U);

    while (1) {
        joystick_x = ADC_Read(7U);   /* PD0 / AIN7 */
        joystick_y = ADC_Read(6U);   /* PD1 / AIN6 */

        theta_deg = THETA_MIN_DEG +
            (int32_t)(((uint32_t)joystick_x *
            (THETA_MAX_DEG - THETA_MIN_DEG)) / 4095U);

        phi_deg = PHI_MIN_DEG +
            (int32_t)(((uint32_t)joystick_y *
            (PHI_MAX_DEG - PHI_MIN_DEG)) / 4095U);

        PWM_SetADC_A(joystick_x);
        PWM_SetADC_BPair(joystick_y);
        PWM_SetDuty_C(button_flag);

        ArmDrawFrame(theta_deg, phi_deg, button_flag);

        /* Red LED heartbeat: one toggle every 500 ms. */
        heartbeat_count++;
        if (heartbeat_count >= 10U) {
            StatusLED_Toggle();
            heartbeat_count = 0U;
        }

        Delay_ms(50U);
    }
}
