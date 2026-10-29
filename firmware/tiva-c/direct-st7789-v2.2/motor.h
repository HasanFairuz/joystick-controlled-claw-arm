#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

extern volatile uint8_t button_flag;

void Motor_Init(void);

void Delay_ms(uint32_t ms);
uint16_t ADC_Read(uint8_t channel);   /* 7 = PD0/AIN7, 6 = PD1/AIN6 */

void PWM_SetADC_A(uint16_t adc_value);       /* Servo A: PE4/M1PWM2 */
void PWM_SetADC_BPair(uint16_t adc_value);   /* B1: PE5; B2: PF3 reversed */
void PWM_SetDuty_C(uint8_t state);           /* Servo C: PF2/M1PWM6 */

void StatusLED_Set(uint8_t on);
void StatusLED_Toggle(void);

#endif
